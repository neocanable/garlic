#ifdef _WIN32
#include <windows.h>
#else
#include <sys/wait.h>
#endif

#include "jd_mcp.h"
#include "str_tools.h"
#include <dirent.h>
#include <errno.h>
#include <stdarg.h>

static const char* garlic_bin(void)
{
    return mcp_self_path ? mcp_self_path : "garlic";
}

#define SCHEMA_DECOMPILE  \
    "{\"type\":\"object\",\"properties\":{"  \
    "\"path\":{\"type\":\"string\",\"description\":\"Path to .class/.jar/.dex/.apk file\"},"  \
    "\"output_dir\":{\"type\":\"string\",\"description\":\"Output directory for decompiled source\"}"  \
    "},\"required\":[\"path\"]}"

#define SCHEMA_DUMP_INFO  \
    "{\"type\":\"object\",\"properties\":{"  \
    "\"path\":{\"type\":\"string\",\"description\":\"Path to .class or .dex file\"}"  \
    "},\"required\":[\"path\"]}"

#define SCHEMA_CALL_GRAPH  \
    "{\"type\":\"object\",\"properties\":{"  \
    "\"path\":{\"type\":\"string\",\"description\":\"Path to .dex or .apk file\"},"  \
    "\"output_dir\":{\"type\":\"string\",\"description\":\"Output directory for call graph CSV files\"}"  \
    "},\"required\":[\"path\"]}"

#define SCHEMA_CG_IMPORT  \
    "{\"type\":\"object\",\"properties\":{"  \
    "\"cg_dir\":{\"type\":\"string\",\"description\":\"Directory containing call_graph_node.csv and call_graph_edge.csv\"},"  \
    "\"db_path\":{\"type\":\"string\",\"description\":\"Output path for .duckdb database file\"}"  \
    "},\"required\":[\"cg_dir\",\"db_path\"]}"

#define SCHEMA_CG_QUERY  \
    "{\"type\":\"object\",\"properties\":{"  \
    "\"db_path\":{\"type\":\"string\",\"description\":\"Path to .duckdb database file\"},"  \
    "\"sql\":{\"type\":\"string\",\"description\":\"SQL query to execute\"}"  \
    "},\"required\":[\"db_path\",\"sql\"]}"

#define SCHEMA_ANALYZE  \
    "{\"type\":\"object\",\"properties\":{"  \
    "\"path\":{\"type\":\"string\",\"description\":\"Path to .dex or .apk file\"},"  \
    "\"output_dir\":{\"type\":\"string\",\"description\":\"Working directory for all outputs\"}"  \
    "},\"required\":[\"path\",\"output_dir\"]}"

#define SCHEMA_ANDROID_MANIFEST  \
    "{\"type\":\"object\",\"properties\":{"  \
    "\"output_dir\":{\"type\":\"string\",\"description\":\"Output directory from analyze or decompile tool\"}"  \
    "},\"required\":[\"output_dir\"]}"


const jd_mcp_tool MCP_TOOLS[] = {
    {
        .name         = "decompile",
        .description  = "Decompile a Java class, JAR, DEX or APK to Java source code via `garlic -o`",
        .input_schema = SCHEMA_DECOMPILE,
    },
    {
        .name         = "dump_info",
        .description  = "Display class/dex file structure (like javap / dexdump) via `garlic -p`",
        .input_schema = SCHEMA_DUMP_INFO,
    },
    {
        .name         = "call_graph",
        .description  = "Generate call graph for a DEX or APK file via `garlic -g`",
        .input_schema = SCHEMA_CALL_GRAPH,
    },
    {
        .name         = "cg_import",
        .description  = "Import call graph CSV files into a DuckDB database for SQL analysis",
        .input_schema = SCHEMA_CG_IMPORT,
    },
    {
        .name         = "cg_query",
        .description  = "Run a SQL query against a call graph DuckDB database",
        .input_schema = SCHEMA_CG_QUERY,
    },
    {
        .name         = "analyze",
        .description  = "One-shot: decompile + call graph + import DuckDB for an APK/DEX file",
        .input_schema = SCHEMA_ANALYZE,
    },
    {
        .name         = "android_manifest",
        .description  = "Read AndroidManifest.xml from a previous decompile/analyze output directory",
        .input_schema = SCHEMA_ANDROID_MANIFEST,
    },
};

const int MCP_TOOL_COUNT = (int)(sizeof(MCP_TOOLS) / sizeof(MCP_TOOLS[0]));

static char* mcp_read_dir_java(const char *dir)
{
    DIR *d = opendir(dir);
    if (!d) return NULL;

    size_t cap = 4096, len = 0;
    char *out = malloc(cap);
    if (!out) { closedir(d); return NULL; }
    out[0] = '\0';

    struct dirent *e;
    char full[4096];
    while ((e = readdir(d)) != NULL) {
        if (STR_EQL(e->d_name, ".") || STR_EQL(e->d_name, ".."))
            continue;
        snprintf(full, sizeof(full), "%s/%s", dir, e->d_name);

        struct stat st;
        if (stat(full, &st) != 0) continue;

        if (S_ISDIR(st.st_mode)) {
            char *sub = mcp_read_dir_java(full);
            if (sub) {
                size_t slen = strlen(sub);
                if (len + slen + 2 > cap) {
                    cap = (len + slen + 2) * 2;
                    char *nb = realloc(out, cap);
                    if (!nb) { free(out); free(sub); closedir(d); return NULL; }
                    out = nb;
                }
                memcpy(out + len, sub, slen);
                len += slen; out[len] = '\0';
                free(sub);
            }
            continue;
        }

        if (!strstr(e->d_name, ".java")) continue;

        char *content = jd_mcp_read_file(full);
        if (!content) continue;

        size_t ctn_len = strlen(content);
        size_t need = len + strlen(e->d_name) + 16 + ctn_len + 2;
        if (need > cap) {
            cap = need * 2;
            char *nb = realloc(out, cap);
            if (!nb) { free(out); free(content); closedir(d); return NULL; }
            out = nb;
        }
        int written = snprintf(out + len, cap - len,
                               "// --- %s ---\n%s\n", e->d_name, content);
        if (written > 0) len += (size_t)written;
        out[len] = '\0';
        free(content);
    }
    closedir(d);
    return out;
}


static bool append_output(char **out, size_t *cap, size_t *len,
                          const char *data, size_t data_len)
{
    if (*len + data_len + 1 > *cap) {
        size_t new_cap = (*len + data_len + 1) * 2;
        char *new_out = realloc(*out, new_cap);
        if (!new_out) return false;
        *out = new_out;
        *cap = new_cap;
    }
    memcpy(*out + *len, data, data_len);
    *len += data_len;
    (*out)[*len] = '\0';
    return true;
}

#ifdef _WIN32
static wchar_t* utf8_to_wide(const char *str)
{
    UINT code_page = CP_UTF8;
    int len = MultiByteToWideChar(code_page, MB_ERR_INVALID_CHARS,
                                  str, -1, NULL, 0);
    if (len == 0) {
        code_page = CP_ACP;
        len = MultiByteToWideChar(code_page, 0, str, -1, NULL, 0);
    }
    if (len == 0) return NULL;

    wchar_t *wide = malloc((size_t)len * sizeof(wchar_t));
    if (!wide) return NULL;
    if (MultiByteToWideChar(code_page, 0, str, -1, wide, len) == 0) {
        free(wide);
        return NULL;
    }
    return wide;
}

static bool append_wchars(wchar_t *dst, size_t cap, size_t *len,
                          wchar_t value, size_t count)
{
    if (*len + count + 1 > cap) return false;
    for (size_t i = 0; i < count; ++i)
        dst[(*len)++] = value;
    dst[*len] = L'\0';
    return true;
}

static bool append_windows_arg(wchar_t *cmd, size_t cap, size_t *len,
                               const char *arg)
{
    wchar_t *wide = utf8_to_wide(arg);
    if (!wide) return false;

    bool ok = true;
    if (*len > 0)
        ok = append_wchars(cmd, cap, len, L' ', 1);
    if (ok)
        ok = append_wchars(cmd, cap, len, L'"', 1);

    size_t backslashes = 0;
    for (const wchar_t *p = wide; ok && *p; ++p) {
        if (*p == L'\\') {
            backslashes++;
            continue;
        }
        if (*p == L'"') {
            ok = append_wchars(cmd, cap, len, L'\\', backslashes * 2 + 1) &&
                 append_wchars(cmd, cap, len, L'"', 1);
        } else {
            ok = append_wchars(cmd, cap, len, L'\\', backslashes) &&
                 append_wchars(cmd, cap, len, *p, 1);
        }
        backslashes = 0;
    }
    if (ok)
        ok = append_wchars(cmd, cap, len, L'\\', backslashes * 2) &&
             append_wchars(cmd, cap, len, L'"', 1);

    free(wide);
    return ok;
}

static int exec_process(const char *const argv[], const char *stdin_path,
                        bool capture, char **output)
{
    wchar_t command_line[32768] = {0};
    size_t command_len = 0;
    for (int i = 0; argv[i]; ++i) {
        if (!append_windows_arg(command_line,
                                sizeof(command_line) / sizeof(command_line[0]),
                                &command_len, argv[i]))
            return -1;
    }

    SECURITY_ATTRIBUTES sa = {
        .nLength = sizeof(SECURITY_ATTRIBUTES),
        .lpSecurityDescriptor = NULL,
        .bInheritHandle = TRUE,
    };
    HANDLE output_read = NULL;
    HANDLE output_write = NULL;
    HANDLE null_output = NULL;
    if (capture) {
        if (!CreatePipe(&output_read, &output_write, &sa, 0))
            return -1;
        SetHandleInformation(output_read, HANDLE_FLAG_INHERIT, 0);
    } else {
        null_output = CreateFileW(L"NUL", GENERIC_WRITE,
                                  FILE_SHARE_READ | FILE_SHARE_WRITE,
                                  &sa, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                                  NULL);
        if (null_output == INVALID_HANDLE_VALUE)
            return -1;
        output_write = null_output;
    }

    HANDLE input_handle = NULL;
    wchar_t *wide_stdin = NULL;
    if (stdin_path) {
        wide_stdin = utf8_to_wide(stdin_path);
        if (wide_stdin)
            input_handle = CreateFileW(wide_stdin, GENERIC_READ, FILE_SHARE_READ,
                                       &sa, OPEN_EXISTING,
                                       FILE_ATTRIBUTE_NORMAL, NULL);
    } else {
        input_handle = CreateFileW(L"NUL", GENERIC_READ,
                                   FILE_SHARE_READ | FILE_SHARE_WRITE,
                                   &sa, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                                   NULL);
    }
    free(wide_stdin);
    if (!input_handle || input_handle == INVALID_HANDLE_VALUE) {
        if (capture) {
            CloseHandle(output_read);
            CloseHandle(output_write);
        } else {
            CloseHandle(null_output);
        }
        return -1;
    }

    STARTUPINFOW si = {0};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = input_handle;
    si.hStdOutput = output_write;
    si.hStdError = output_write;

    PROCESS_INFORMATION pi = {0};
    BOOL created = CreateProcessW(NULL, command_line, NULL, NULL, TRUE,
                                  CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
    CloseHandle(input_handle);
    CloseHandle(output_write);
    if (!created) {
        if (capture) CloseHandle(output_read);
        return -1;
    }

    char *captured = NULL;
    size_t cap = 0;
    size_t len = 0;
    bool read_ok = true;
    if (capture) {
        cap = 4096;
        captured = malloc(cap);
        if (!captured) {
            read_ok = false;
        } else {
            captured[0] = '\0';
            char buf[4096];
            DWORD read_len;
            while (ReadFile(output_read, buf, sizeof(buf), &read_len, NULL) &&
                   read_len > 0) {
                if (!append_output(&captured, &cap, &len, buf, read_len)) {
                    read_ok = false;
                    break;
                }
            }
        }
        CloseHandle(output_read);
    }

    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exit_code = 1;
    GetExitCodeProcess(pi.hProcess, &exit_code);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    if (!read_ok) {
        free(captured);
        captured = NULL;
    }
    if (output) *output = read_ok ? captured : NULL;
    else free(captured);
    return (int)exit_code;
}
#else
static int exec_process(const char *const argv[], const char *stdin_path,
                        bool capture, char **output)
{
    int pipe_fd[2] = {-1, -1};
    if (capture && pipe(pipe_fd) != 0)
        return -1;

    pid_t pid = fork();
    if (pid < 0) {
        if (capture) {
            close(pipe_fd[0]);
            close(pipe_fd[1]);
        }
        return -1;
    }

    if (pid == 0) {
        int input_fd = open(stdin_path ? stdin_path : "/dev/null", O_RDONLY);
        int output_fd = capture ? pipe_fd[1] : open("/dev/null", O_WRONLY);
        if (input_fd < 0 || output_fd < 0)
            _exit(126);

        dup2(input_fd, STDIN_FILENO);
        dup2(output_fd, STDOUT_FILENO);
        dup2(output_fd, STDERR_FILENO);
        close(input_fd);
        close(output_fd);
        if (capture) close(pipe_fd[0]);

        execvp(argv[0], (char *const *)argv);
        _exit(127);
    }

    char *captured = NULL;
    size_t cap = 0;
    size_t len = 0;
    bool read_ok = true;
    if (capture) {
        close(pipe_fd[1]);
        cap = 4096;
        captured = malloc(cap);
        if (!captured) {
            read_ok = false;
        } else {
            captured[0] = '\0';
            char buf[4096];
            ssize_t read_len;
            while ((read_len = read(pipe_fd[0], buf, sizeof(buf))) > 0) {
                if (!append_output(&captured, &cap, &len, buf,
                                   (size_t)read_len)) {
                    read_ok = false;
                    break;
                }
            }
        }
        close(pipe_fd[0]);
    }

    int status;
    while (waitpid(pid, &status, 0) < 0) {
        if (errno != EINTR) {
            free(captured);
            return -1;
        }
    }

    if (!read_ok) {
        free(captured);
        captured = NULL;
    }
    if (output) *output = read_ok ? captured : NULL;
    else free(captured);

    if (WIFEXITED(status)) return WEXITSTATUS(status);
    return -1;
}
#endif

static bool ensure_directory(const char *path)
{
    if (!path || path[0] == '\0') return false;

#ifdef _WIN32
    wchar_t *wide = utf8_to_wide(path);
    if (!wide) return false;

    for (wchar_t *p = wide; *p; ++p) {
        if (*p != L'/' && *p != L'\\')
            continue;
        if (p == wide || (p == wide + 2 && wide[1] == L':'))
            continue;

        wchar_t separator = *p;
        *p = L'\0';
        if (!CreateDirectoryW(wide, NULL) &&
            GetLastError() != ERROR_ALREADY_EXISTS) {
            *p = separator;
            free(wide);
            return false;
        }
        *p = separator;
    }

    bool ok = CreateDirectoryW(wide, NULL) ||
              GetLastError() == ERROR_ALREADY_EXISTS;
    if (ok) {
        DWORD attrs = GetFileAttributesW(wide);
        ok = attrs != INVALID_FILE_ATTRIBUTES &&
             (attrs & FILE_ATTRIBUTE_DIRECTORY) != 0;
    }
    free(wide);
    return ok;
#else
    char *copy = strdup(path);
    if (!copy) return false;

    for (char *p = copy + 1; *p; ++p) {
        if (*p != '/') continue;
        *p = '\0';
        if (mkdir(copy, 0777) != 0 && errno != EEXIST) {
            free(copy);
            return false;
        }
        *p = '/';
    }

    bool ok = mkdir(copy, 0777) == 0 || errno == EEXIST;
    if (ok) {
        struct stat st;
        ok = stat(copy, &st) == 0 && S_ISDIR(st.st_mode);
    }
    free(copy);
    return ok;
#endif
}

static bool ensure_parent_directory(const char *path)
{
    char *copy = strdup(path);
    if (!copy) return false;

    char *slash = strrchr(copy, '/');
    char *backslash = strrchr(copy, '\\');
    char *separator = slash;
    if (!separator || (backslash && backslash > separator))
        separator = backslash;
    if (!separator) {
        free(copy);
        return true;
    }
    *separator = '\0';
    bool ok = copy[0] == '\0' || ensure_directory(copy);
    free(copy);
    return ok;
}

static bool duckdb_available(void)
{
    const char *argv[] = {"duckdb", "--version", NULL};
    return exec_process(argv, NULL, false, NULL) == 0;
}

static bool append_format(char *buf, size_t cap, size_t *len,
                          const char *fmt, ...)
{
    if (*len >= cap) return false;
    va_list ap;
    va_start(ap, fmt);
    int written = vsnprintf(buf + *len, cap - *len, fmt, ap);
    va_end(ap);
    if (written < 0 || (size_t)written >= cap - *len)
        return false;
    *len += (size_t)written;
    return true;
}

static char* duckdb_escape_path(const char *path)
{
    size_t len = strlen(path);
    char *escaped = malloc(len * 2 + 1);
    if (!escaped) return NULL;

    size_t out = 0;
    for (size_t i = 0; i < len; ++i) {
        char c = path[i];
#ifdef _WIN32
        if (c == '\\') c = '/';
#endif
        if (c == '\'') escaped[out++] = '\'';
        escaped[out++] = c;
    }
    escaped[out] = '\0';
    return escaped;
}

static char* run_duckdb_script(const char *db_path, const char *sql,
                               bool readonly_csv, int *exit_code)
{
    char *temp_dir = jd_mcp_create_temp_dir();
    if (!temp_dir) {
        if (exit_code) *exit_code = -1;
        return NULL;
    }

    char script_path[4096];
    snprintf(script_path, sizeof(script_path), "%s/query.sql", temp_dir);
    FILE *script = fopen(script_path, "wb");
    if (!script) {
        jd_mcp_remove_temp_dir(temp_dir);
        free(temp_dir);
        if (exit_code) *exit_code = -1;
        return NULL;
    }
    fwrite(sql, 1, strlen(sql), script);
    fputc('\n', script);
    fclose(script);

    const char *write_argv[] = {"duckdb", db_path, NULL};
    const char *read_argv[] = {
        "duckdb", "-readonly", "-csv", db_path, NULL
    };
    char *output = NULL;
    int rc = exec_process(readonly_csv ? read_argv : write_argv,
                          script_path, true, &output);

    jd_mcp_remove_temp_dir(temp_dir);
    free(temp_dir);
    if (exit_code) *exit_code = rc;
    return output;
}

static char* process_error(const char *operation, int rc, const char *detail)
{
    size_t detail_len = detail ? strlen(detail) : 0;
    char *result = malloc(strlen(operation) + detail_len + 64);
    if (!result) return NULL;
    snprintf(result, strlen(operation) + detail_len + 64,
             "err: %s failed (exit=%d)%s%s",
             operation, rc, detail_len ? ": " : "", detail_len ? detail : "");
    return result;
}

static string tool_decompile(const char *path, const char *output_dir)
{
    if (jd_mcp_detect_file_type(path) == JD_MCP_FILE_CLASS) {
        const char *argv[] = {garlic_bin(), path, NULL};
        char *source = NULL;
        int rc = exec_process(argv, NULL, true, &source);
        if (rc != 0) {
            free(source);
            char err[256];
            snprintf(err, sizeof(err),
                     "Error: decompilation failed (exit=%d)", rc);
            return strdup(err);
        }
        if (!source || source[0] == '\0') {
            free(source);
            return strdup("(decompilation produced no output)");
        }
        return source;
    }

    const char *save_dir = output_dir;
    char tmp_path[2048];
    if (!output_dir || output_dir[0] == '\0') {
        char *d = jd_mcp_create_temp_dir();
        if (!d) return strdup("Error: cannot create temp directory");
        snprintf(tmp_path, sizeof(tmp_path), "%s", d);
        free(d);
        save_dir = tmp_path;
    }

    if (!ensure_directory(save_dir)) {
        if (!output_dir || output_dir[0] == '\0')
            jd_mcp_remove_temp_dir(save_dir);
        return strdup("Error: cannot create output directory");
    }

    const char *argv[] = {garlic_bin(), path, "-o", save_dir, NULL};
    int rc = exec_process(argv, NULL, false, NULL);
    if (rc != 0) {
        if (!output_dir || output_dir[0] == '\0')
            jd_mcp_remove_temp_dir(save_dir);
        char err[256];
        snprintf(err, sizeof(err), "Error: decompilation failed (exit=%d)", rc);
        return strdup(err);
    }

    if (!output_dir || output_dir[0] == '\0') {
        char *result = mcp_read_dir_java(save_dir);
        jd_mcp_remove_temp_dir(save_dir);
        if (!result) result = strdup("(decompilation produced no output)");
        return result;
    }

    char *result = malloc(1024);
    if (!result)
        return strdup("Error: out of memory while formatting result");
#ifdef _WIN32
    char abs[4096];
    char *p = _fullpath(abs, save_dir, sizeof(abs));
    if (p) {
        snprintf(result, 1024, "Decompiled to: %s", abs);
    } else {
        snprintf(result, 1024, "Decompiled to: %s", save_dir);
    }
#else
    char *abs = realpath(save_dir, NULL);
    if (abs) {
        snprintf(result, 1024, "Decompiled to: %s", abs);
        free(abs);
    } else {
        snprintf(result, 1024, "Decompiled to: %s", save_dir);
    }
#endif
    return result;
}

static string tool_dump_info(const char *path)
{
    const char *argv[] = {garlic_bin(), path, "-p", NULL};
    char *out = NULL;
    int rc = exec_process(argv, NULL, true, &out);
    if (rc != 0) {
        free(out);
        char err[256];
        snprintf(err, sizeof(err),
                 "Error: dump_info failed (exit=%d; file may be unsupported)",
                 rc);
        return strdup(err);
    }
    if (!out) return strdup("(no output)");
    return out;
}

static string tool_call_graph(const char *path, const char *output_dir)
{
    const char *save_dir = output_dir;
    char tmp_path[2048];

    if (!output_dir || output_dir[0] == '\0') {
        char *d = jd_mcp_create_temp_dir();
        if (!d) return strdup("Error: cannot create temp directory");
        snprintf(tmp_path, sizeof(tmp_path), "%s", d);
        free(d);
        save_dir = tmp_path;
    }

    if (!ensure_directory(save_dir)) {
        if (!output_dir || output_dir[0] == '\0')
            jd_mcp_remove_temp_dir(save_dir);
        return strdup("Error: cannot create output directory");
    }

    const char *argv[] = {garlic_bin(), path, "-g", "-o", save_dir, NULL};
    int rc = exec_process(argv, NULL, false, NULL);

    if (rc != 0) {
        if (!output_dir || output_dir[0] == '\0')
            jd_mcp_remove_temp_dir(save_dir);
        char err[256];
        snprintf(err, sizeof(err), "Error: call_graph failed (exit=%d)", rc);
        return strdup(err);
    }

    char *result = malloc(1024);
    if (!result)
        return strdup("Error: out of memory while formatting result");
    snprintf(result, 1024, "Call graph generated in: %s", save_dir);
    return result;
}


static char* tool_cg_import(const char *cg_dir, const char *db_path)
{
    char node_csv[4096], edge_csv[4096], str_node_csv[4096], str_edge_csv[4096];
    snprintf(node_csv, sizeof(node_csv), "%s/call_graph_node.csv", cg_dir);
    snprintf(edge_csv, sizeof(edge_csv), "%s/call_graph_edge.csv", cg_dir);
    snprintf(str_node_csv, sizeof(str_node_csv), "%s/string_node.csv", cg_dir);
    snprintf(str_edge_csv, sizeof(str_edge_csv), "%s/string_edge.csv", cg_dir);

    if (access(node_csv, F_OK) != 0 || access(edge_csv, F_OK) != 0) {
        return strdup("err: call graph code not found");
    }

    if (!duckdb_available())
        return strdup("err: duckdb not found on PATH");

    if (!ensure_parent_directory(db_path))
        return strdup("err: cannot create database output directory");

    char *node_path = duckdb_escape_path(node_csv);
    char *edge_path = duckdb_escape_path(edge_csv);
    char *str_node_path = duckdb_escape_path(str_node_csv);
    char *str_edge_path = duckdb_escape_path(str_edge_csv);
    if (!node_path || !edge_path || !str_node_path || !str_edge_path) {
        free(node_path);
        free(edge_path);
        free(str_node_path);
        free(str_edge_path);
        return strdup("err: out of memory while preparing DuckDB import");
    }

    size_t sql_cap = 65536 + strlen(node_path) + strlen(edge_path) +
                     strlen(str_node_path) + strlen(str_edge_path);
    char *sql = malloc(sql_cap);
    if (!sql) {
        free(node_path);
        free(edge_path);
        free(str_node_path);
        free(str_edge_path);
        return strdup("err: out of memory while preparing DuckDB import");
    }

    size_t n = 0;
    bool ok = append_format(sql, sql_cap, &n,
        "PRAGMA threads=4;\n"
        "PRAGMA memory_limit='4GB';\n"
        "PRAGMA preserve_insertion_order=false;\n"
        "BEGIN TRANSACTION;\n"
        "CREATE TABLE IF NOT EXISTS java_cg_nodes(node_id BIGINT, method_raw VARCHAR, node_type BIGINT, api_type BIGINT);\n"
        "INSERT INTO java_cg_nodes SELECT CAST(id AS BIGINT), CAST(method AS VARCHAR), CAST(COALESCE(type,0) AS BIGINT), CAST(COALESCE(api_type,0) AS BIGINT) FROM read_csv_auto('%s', HEADER=TRUE, SAMPLE_SIZE=-1, ignore_errors=true);\n"
        "CREATE TABLE IF NOT EXISTS java_cg_edges(src_id BIGINT, dst_id BIGINT);\n"
        "INSERT INTO java_cg_edges SELECT DISTINCT CAST(src_id AS BIGINT), CAST(dst_id AS BIGINT) FROM read_csv_auto('%s', HEADER=TRUE, SAMPLE_SIZE=-1, ignore_errors=true);\n",
        node_path, edge_path);

    if (ok && access(str_node_csv, F_OK) == 0) {
        ok = append_format(sql, sql_cap, &n,
            "CREATE TABLE IF NOT EXISTS string_nodes(id BIGINT, pc BIGINT, str VARCHAR, is_class_desc BOOLEAN, is_field_name BOOLEAN, is_method_name BOOLEAN, is_return_type BOOLEAN, is_method_param_type BOOLEAN, is_internal_class_desc BOOLEAN, is_url BOOLEAN, is_enc_dec BOOLEAN, is_uuid BOOLEAN, is_pem_key BOOLEAN, is_so_name BOOLEAN, is_ipv4 BOOLEAN);\n"
            "INSERT INTO string_nodes SELECT * FROM read_csv_auto('%s', HEADER=TRUE, SAMPLE_SIZE=-1, ignore_errors=true);\n",
            str_node_path);
    }
    if (ok && access(str_edge_csv, F_OK) == 0) {
        ok = append_format(sql, sql_cap, &n,
            "CREATE TABLE IF NOT EXISTS string_edges(string_id BIGINT, method_id BIGINT);\n"
            "INSERT INTO string_edges SELECT src_id AS string_id, dst_id AS method_id FROM read_csv_auto('%s', HEADER=TRUE, SAMPLE_SIZE=-1, ignore_errors=true);\n",
            str_edge_path);
    }

    if (ok) {
        ok = append_format(sql, sql_cap, &n,
        "CREATE INDEX IF NOT EXISTS idx_nodes_id ON java_cg_nodes(node_id);\n"
        "CREATE INDEX IF NOT EXISTS idx_edges_src ON java_cg_edges(src_id);\n"
        "CREATE INDEX IF NOT EXISTS idx_edges_dst ON java_cg_edges(dst_id);\n"
        "COMMIT;\n"
        "SELECT 'nodes' AS tbl, COUNT(*) AS cnt FROM java_cg_nodes "
        "UNION ALL SELECT 'edges', COUNT(*) FROM java_cg_edges;\n");
    }

    free(node_path);
    free(edge_path);
    free(str_node_path);
    free(str_edge_path);
    if (!ok) {
        free(sql);
        return strdup("err: DuckDB import SQL is too large");
    }

    int rc = -1;
    char *out = run_duckdb_script(db_path, sql, false, &rc);
    free(sql);
    if (rc != 0) {
        char *error = process_error("duckdb import", rc, out);
        free(out);
        return error ? error : strdup("err: duckdb import failed");
    }
    if (!out) return strdup("err: duckdb import produced no output");
    return out;
}

static char* tool_cg_query(const char *db_path, const char *sql)
{
    if (access(db_path, F_OK) != 0)
        return strdup("err: database file not found");

    if (!duckdb_available())
        return strdup("err: duckdb not found on PATH");

    int rc = -1;
    char *out = run_duckdb_script(db_path, sql, true, &rc);
    if (rc != 0) {
        char *error = process_error("duckdb query", rc, out);
        free(out);
        return error ? error : strdup("err: duckdb query failed");
    }
    if (!out || out[0] == '\0') {
        free(out);
        return strdup("(no results)");
    }
    return out;
}

static size_t count_files_with_suffix(const char *dir, const char *suffix)
{
    DIR *d = opendir(dir);
    if (!d) return 0;

    size_t count = 0;
    struct dirent *entry;
    char path[4096];
    while ((entry = readdir(d)) != NULL) {
        if (STR_EQL(entry->d_name, ".") || STR_EQL(entry->d_name, ".."))
            continue;

        snprintf(path, sizeof(path), "%s/%s", dir, entry->d_name);
        struct stat st;
        if (stat(path, &st) != 0)
            continue;
        if (S_ISDIR(st.st_mode))
            count += count_files_with_suffix(path, suffix);
        else if (str_end_with(entry->d_name, suffix))
            count++;
    }
    closedir(d);
    return count;
}

static size_t count_file_lines(const char *path)
{
    FILE *file = fopen(path, "rb");
    if (!file) return 0;

    size_t lines = 0;
    int c;
    int last = '\n';
    while ((c = fgetc(file)) != EOF) {
        if (c == '\n') lines++;
        last = c;
    }
    fclose(file);
    if (last != '\n') lines++;
    return lines;
}

static char* tool_analyze(const char *path, const char *out_dir)
{
    if (!ensure_directory(out_dir))
        return strdup("err: cannot create analysis output directory");

    char decompile_dir[4096], cg_dir[4096], db_path[4096];
    snprintf(decompile_dir, sizeof(decompile_dir), "%s/decompiled", out_dir);
    snprintf(cg_dir, sizeof(cg_dir), "%s/cg", out_dir);
    snprintf(db_path, sizeof(db_path), "%s/analysis.duckdb", out_dir);

    if (!ensure_directory(decompile_dir) || !ensure_directory(cg_dir))
        return strdup("err: cannot create analysis subdirectories");

    const char *decompile_argv[] = {
        garlic_bin(), path, "-o", decompile_dir, NULL
    };
    int rc = exec_process(decompile_argv, NULL, false, NULL);
    if (rc != 0) {
        char err[128];
        snprintf(err, sizeof(err),
                 "err: decompilation failed (exit=%d)", rc);
        return strdup(err);
    }

    const char *call_graph_argv[] = {
        garlic_bin(), path, "-g", "-o", cg_dir, NULL
    };
    rc = exec_process(call_graph_argv, NULL, false, NULL);
    if (rc != 0) {
        char err[128];
        snprintf(err, sizeof(err),
                 "err: call graph creation failed (exit=%d)", rc);
        return strdup(err);
    }

    char node_csv[4096], edge_csv[4096];
    snprintf(node_csv, sizeof(node_csv), "%s/call_graph_node.csv", cg_dir);
    snprintf(edge_csv, sizeof(edge_csv), "%s/call_graph_edge.csv", cg_dir);
    if (access(node_csv, F_OK) != 0 || access(edge_csv, F_OK) != 0)
        return strdup("err: call graph CSV files were not created");

    char *import_result = tool_cg_import(cg_dir, db_path);
    if (!import_result)
        return strdup("err: duckdb import produced no result");
    if (import_result && strncmp(import_result, "err:", 4) == 0)
        return import_result;
    free(import_result);
    if (access(db_path, F_OK) != 0)
        return strdup("err: duckdb import produced no database file");

    char result[16384];
    size_t n = 0;
    bool ok = append_format(result, sizeof(result), &n,
        "Analysis complete for: %s\n\n"
        "  Decompiled:  %s/\n"
        "  Call graph:  %s/\n"
        "  DuckDB:      %s\n\n",
        path, decompile_dir, cg_dir, db_path);

    size_t java_files = count_files_with_suffix(decompile_dir, ".java");
    size_t node_lines = count_file_lines(node_csv);
    size_t edge_lines = count_file_lines(edge_csv);
    size_t node_count = node_lines > 0 ? node_lines - 1 : 0;
    size_t edge_count = edge_lines > 0 ? edge_lines - 1 : 0;

    ok = ok && append_format(result, sizeof(result), &n,
        "  Java files:  %zu\n"
        "  CG nodes:    %zu\n"
        "  CG edges:    %zu\n"
        "\nReady: cg_query(db_path=\"%s\", sql=\"...\")\n",
        java_files, node_count, edge_count, db_path);
    if (!ok)
        return strdup("err: analysis result is too large");
    return strdup(result);
}

static char* tool_android_manifest(const char *output_dir)
{
    char manifest_path[4096];
    snprintf(manifest_path, sizeof(manifest_path), "%s/AndroidManifest.xml", output_dir);

    if (access(manifest_path, F_OK) != 0) {
        return strdup("Error: AndroidManifest.xml not found in the specified output directory. "
                       "Make sure you have decompiled an APK first.");
    }

    return jd_mcp_read_file(manifest_path);
}

void mcp_handle_tools_call(jd_mcp_server *server, unsigned id,
                           const char *tool_name, cJSON *args)
{
    (void)server;

    if (!args || !cJSON_IsObject(args)) {
        jd_mcp_send_error(id, JD_MCP_ERROR_INVALID_PARAMS,
                          "tools/call requires 'arguments' object");
        return;
    }

    char *output = NULL;

    if (STR_EQL(tool_name, "decompile") ||
        STR_EQL(tool_name, "dump_info") ||
        STR_EQL(tool_name, "call_graph") ||
        STR_EQL(tool_name, "analyze"))
    {
        cJSON *path_json = cJSON_GetObjectItem(args, "path");
        if (!path_json || !cJSON_IsString(path_json) ||
            strlen(path_json->valuestring) == 0) {
            jd_mcp_send_error(id, JD_MCP_ERROR_INVALID_PARAMS,
                              "Missing required argument: 'path' (string)");
            return;
        }
        const char *file_path = path_json->valuestring;
        if (access(file_path, F_OK) != 0) {
            char errmsg[1024];
            snprintf(errmsg, sizeof(errmsg), "File not found: %s", file_path);
            jd_mcp_send_error(id, JD_MCP_ERROR_INVALID_PARAMS, errmsg);
            return;
        }
        jd_mcp_log("executing %s on %s", tool_name, file_path);

        cJSON *outdir_json = cJSON_GetObjectItem(args, "output_dir");
        const char *output_dir = (outdir_json && cJSON_IsString(outdir_json))
                                     ? outdir_json->valuestring : NULL;

        if (STR_EQL(tool_name, "analyze") &&
            (!output_dir || output_dir[0] == '\0')) {
            jd_mcp_send_error(id, JD_MCP_ERROR_INVALID_PARAMS,
                              "analyze requires 'output_dir' (string)");
            return;
        }

        if (STR_EQL(tool_name, "decompile")) {
            output = tool_decompile(file_path, output_dir);
        }
        else if (STR_EQL(tool_name, "dump_info")) {
            output = tool_dump_info(file_path);
        }
        else if (STR_EQL(tool_name, "call_graph")) {
            output = tool_call_graph(file_path, output_dir);
        }
        else {
            output = tool_analyze(file_path, output_dir);
        }
    }

    else if (STR_EQL(tool_name, "cg_import")) {
        cJSON *cg_dir_json = cJSON_GetObjectItem(args, "cg_dir");
        cJSON *db_json = cJSON_GetObjectItem(args, "db_path");
        if (!cg_dir_json || !cJSON_IsString(cg_dir_json) ||
            !db_json || !cJSON_IsString(db_json)) {
            jd_mcp_send_error(id, JD_MCP_ERROR_INVALID_PARAMS,
                              "cg_import requires 'cg_dir' and 'db_path' (strings)");
            return;
        }
        output = tool_cg_import(cg_dir_json->valuestring, db_json->valuestring);
    } else if (STR_EQL(tool_name, "cg_query")) {
        cJSON *db_json = cJSON_GetObjectItem(args, "db_path");
        cJSON *sql_json = cJSON_GetObjectItem(args, "sql");
        if (!db_json || !cJSON_IsString(db_json) ||
            !sql_json || !cJSON_IsString(sql_json)) {
            jd_mcp_send_error(id, JD_MCP_ERROR_INVALID_PARAMS,
                              "cg_query requires 'db_path' and 'sql' (strings)");
            return;
        }
        output = tool_cg_query(db_json->valuestring, sql_json->valuestring);
    } else if (STR_EQL(tool_name, "android_manifest")) {
        cJSON *outdir_json = cJSON_GetObjectItem(args, "output_dir");
        if (!outdir_json || !cJSON_IsString(outdir_json) ||
            strlen(outdir_json->valuestring) == 0) {
            jd_mcp_send_error(id, JD_MCP_ERROR_INVALID_PARAMS,
                              "android_manifest requires 'output_dir' (string)");
            return;
        }
        output = tool_android_manifest(outdir_json->valuestring);
    } else {
        jd_mcp_send_error(id, JD_MCP_ERROR_METHOD_NOT_FOUND,
                          "Unknown tool");
        return;
    }

    jd_mcp_send_tool_result(id, output ? output : "(no output)");
    free(output);
}
