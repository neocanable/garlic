#include "ai/http_server.h"

#ifdef _WIN32
  #include <winsock2.h>
  #include <windows.h>
  #pragma comment(lib, "ws2_32.lib")
  #define close_socket closesocket
  #define socklen_t int
  typedef SOCKET sock_t;
#else
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <unistd.h>
  #include <arpa/inet.h>
  #define close_socket close
  typedef int sock_t;
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <dirent.h>

#define MAX_PATH_LEN 4096
#define RESPONSE_BUF  65536

static const char *mime_type(const char *path) {
    const char *ext = strrchr(path, '.');
    if (!ext) return "application/octet-stream";
    if (strcmp(ext, ".html") == 0) return "text/html; charset=utf-8";
    if (strcmp(ext, ".css")  == 0) return "text/css";
    if (strcmp(ext, ".js")   == 0) return "application/javascript";
    if (strcmp(ext, ".md")   == 0) return "text/markdown; charset=utf-8";
    if (strcmp(ext, ".json") == 0) return "application/json";
    if (strcmp(ext, ".png")  == 0) return "image/png";
    if (strcmp(ext, ".jpg"  ) == 0 || strcmp(ext, ".jpeg") == 0) return "image/jpeg";
    if (strcmp(ext, ".svg")  == 0) return "image/svg+xml";
    if (strcmp(ext, ".txt")  == 0) return "text/plain; charset=utf-8";
    if (strcmp(ext, ".csv")  == 0) return "text/csv; charset=utf-8";
    if (strcmp(ext, ".xml")  == 0) return "application/xml";
    return "application/octet-stream";
}

static void url_decode(const char *src, char *dst, size_t dst_sz) {
    size_t i = 0, j = 0;
    while (src[i] && j < dst_sz - 1) {
        if (src[i] == '%' && src[i+1] && src[i+2]) {
            char hex[3] = {src[i+1], src[i+2], '\0'};
            dst[j++] = (char)strtol(hex, NULL, 16);
            i += 3;
        } else if (src[i] == '+') {
            dst[j++] = ' ';
            i++;
        } else {
            dst[j++] = src[i++];
        }
    }
    dst[j] = '\0';
}

static int is_safe_path(const char *path) {
    /* Simple check: reject paths containing ".." */
    return strstr(path, "..") == NULL;
}

static void serve_file(sock_t client, const char *full_path) {
    FILE *fp = fopen(full_path, "rb");
    if (!fp) {
        const char *err = "HTTP/1.0 404 Not Found\r\nContent-Length: 0\r\n\r\n";
        send(client, err, (int)strlen(err), 0);
        return;
    }

    fseek(fp, 0, SEEK_END);
    long sz = ftell(fp);
    rewind(fp);

    char header[1024];
    snprintf(header, sizeof(header),
        "HTTP/1.0 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %ld\r\n"
        "Connection: close\r\n\r\n",
        mime_type(full_path), sz);
    send(client, header, (int)strlen(header), 0);

    char buf[8192];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), fp)) > 0)
        send(client, buf, (int)n, 0);
    fclose(fp);
}

static void serve_dir_listing(sock_t client, const char *dir_path, const char *req_path) {
    DIR *d = opendir(dir_path);
    if (!d) {
        const char *err = "HTTP/1.0 404 Not Found\r\nContent-Length: 0\r\n\r\n";
        send(client, err, (int)strlen(err), 0);
        return;
    }

    char body[RESPONSE_BUF];
    int n = snprintf(body, sizeof(body),
        "<!DOCTYPE html>\n<html><head><meta charset=\"UTF-8\">"
        "<title>%s</title>"
        "<style>body{font-family:monospace;max-width:800px;margin:2em auto}"
        "a{text-decoration:none;color:#0366d6}"
        "a:hover{text-decoration:underline}"
        "tr:hover{background:#f5f5f5}"
        "td{padding:4px 8px}</style></head>\n<body>\n"
        "<h1>Index of %s</h1>\n<table>\n",
        req_path, req_path);

    struct dirent *e;
    while ((e = readdir(d)) != NULL && n < (int)sizeof(body) - 256) {
        if (e->d_name[0] == '.' && (e->d_name[1] == '\0' ||
            (e->d_name[1] == '.' && e->d_name[2] == '\0')))
            continue;

        char full[MAX_PATH_LEN];
        snprintf(full, sizeof(full), "%s/%s", dir_path, e->d_name);
        struct stat st;
        if (stat(full, &st) != 0) continue;

        const char *icon = S_ISDIR(st.st_mode) ? "📁" : "📄";
        char size_str[32] = "-";
        if (!S_ISDIR(st.st_mode)) {
            if (st.st_size < 1024) snprintf(size_str, sizeof(size_str), "%lld B", (long long)st.st_size);
            else if (st.st_size < 1024*1024) snprintf(size_str, sizeof(size_str), "%.1f KB", st.st_size/1024.0);
            else snprintf(size_str, sizeof(size_str), "%.1f MB", st.st_size/(1024.0*1024.0));
        }

        n += snprintf(body + n, sizeof(body) - (size_t)n,
            "<tr><td>%s</td><td><a href=\"%s%s\">%s</a></td>"
            "<td style=\"text-align:right\">%s</td></tr>\n",
            icon, e->d_name, S_ISDIR(st.st_mode) ? "/" : "", e->d_name, size_str);
    }
    closedir(d);

    n += snprintf(body + n, sizeof(body) - (size_t)n,
        "</table>\n</body>\n</html>\n");

    char header[256];
    snprintf(header, sizeof(header),
        "HTTP/1.0 200 OK\r\n"
        "Content-Type: text/html; charset=utf-8\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n\r\n", n);
    send(client, header, (int)strlen(header), 0);
    send(client, body, n, 0);
}

static void handle_client(sock_t client, const char *root_dir) {
    char req_buf[8192];
    int n = recv(client, req_buf, sizeof(req_buf) - 1, 0);
    if (n <= 0) { close_socket(client); return; }
    req_buf[n] = '\0';

    /* Parse GET /path HTTP/1.x */
    char method[8], path[MAX_PATH_LEN];
    if (sscanf(req_buf, "%7s %4095s", method, path) != 2 ||
        strcmp(method, "GET") != 0) {
        const char *err = "HTTP/1.0 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
        send(client, err, (int)strlen(err), 0);
        close_socket(client);
        return;
    }

    /* Decode URL and build filesystem path */
    char decoded[MAX_PATH_LEN];
    url_decode(path, decoded, sizeof(decoded));

    char full_path[MAX_PATH_LEN];
    snprintf(full_path, sizeof(full_path), "%s%s", root_dir, decoded);

    /* Strip trailing slash for stat() */
    size_t plen = strlen(full_path);
    while (plen > 1 && full_path[plen-1] == '/') full_path[--plen] = '\0';

    /* Path traversal check */
    if (!is_safe_path(decoded)) {
        const char *err = "HTTP/1.0 403 Forbidden\r\nContent-Length: 0\r\n\r\n";
        send(client, err, (int)strlen(err), 0);
        close_socket(client);
        return;
    }

    struct stat st;
    if (stat(full_path, &st) != 0) {
        const char *err = "HTTP/1.0 404 Not Found\r\nContent-Length: 0\r\n\r\n";
        send(client, err, (int)strlen(err), 0);
        close_socket(client);
        return;
    }

    if (S_ISDIR(st.st_mode)) {
        /* Check for index.html */
        char index_path[MAX_PATH_LEN];
        snprintf(index_path, sizeof(index_path), "%s/index.html", full_path);
        if (access(index_path, F_OK) == 0)
            serve_file(client, index_path);
        else
            serve_dir_listing(client, full_path, decoded);
    } else {
        serve_file(client, full_path);
    }

    close_socket(client);
}

int http_serve(const char *root_dir, int port) {
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) return -1;
#endif

    sock_t server = socket(AF_INET, SOCK_STREAM, 0);
    if (server < 0) { perror("[garlic] socket"); return -1; }

    int opt = 1;
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt));
#ifdef SO_REUSEPORT
    setsockopt(server, SOL_SOCKET, SO_REUSEPORT, (const char *)&opt, sizeof(opt));
#endif

    struct sockaddr_in addr = {0};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons((unsigned short)port);

    if (bind(server, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("[garlic] bind");
        close_socket(server);
        return -1;
    }

    if (listen(server, 10) < 0) {
        perror("[garlic] listen");
        close_socket(server);
        return -1;
    }

    /* Resolve root_dir to absolute path for safe path checking */
    char abs_root[MAX_PATH_LEN];
#ifdef _WIN32
    if (!_fullpath(abs_root, root_dir, sizeof(abs_root)))
        strncpy(abs_root, root_dir, sizeof(abs_root) - 1);
#else
    if (!realpath(root_dir, abs_root))
        strncpy(abs_root, root_dir, sizeof(abs_root) - 1);
#endif

    fprintf(stderr, "[garlic] HTTP server: http://localhost:%d/  (serving %s)\n",
            port, abs_root);
    fprintf(stderr, "[garlic] Press Ctrl+C to stop.\n");

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        sock_t client = accept(server, (struct sockaddr *)&client_addr, &client_len);
        if (client < 0) continue;

        handle_client(client, abs_root);
    }

    close_socket(server);
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
