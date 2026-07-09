#include "jd_mcp.h"
#include "str_tools.h"

#ifdef _WIN32
#include <windows.h>
#include <time.h>

/* Windows implementation of mkdtemp — creates a unique temp directory */
static char* win_mkdtemp(const char *prefix)
{
    char tmp_path[MAX_PATH];
    char dir_name[MAX_PATH];

    if (!GetTempPathA(sizeof(tmp_path), tmp_path))
        return NULL;

    /* Try up to 100 times with different suffixes */
    srand((unsigned)time(NULL));
    for (int i = 0; i < 100; i++) {
        snprintf(dir_name, sizeof(dir_name), "%s%s%04x",
                 tmp_path, prefix, (unsigned)rand() & 0xFFFF);
        /* Remove trailing X's in the template if any, already handled */
        if (CreateDirectoryA(dir_name, NULL))
            return strdup(dir_name);
        if (GetLastError() != ERROR_ALREADY_EXISTS)
            break;
    }
    return NULL;
}
#endif

const char *mcp_self_path = NULL;

void jd_mcp_set_self_path(const char *path)
{
    mcp_self_path = path;
}

static char* read_line(void)
{
    size_t size = 1024;
    size_t len  = 0;
    char  *buf  = malloc(size);
    if (!buf) return NULL;

    int c;
    while ((c = getchar()) != EOF && c != '\n') {
        if (len + 1 >= size) {
            size *= 2;
            if (size > JD_MCP_MAX_LINE_SIZE) {
                free(buf);
                return NULL;
            }
            char *nb = realloc(buf, size);
            if (!nb) { free(buf); return NULL; }
            buf = nb;
        }
        buf[len++] = (char)c;
    }
    if (c == EOF && len == 0) {
        free(buf);
        return NULL;
    }
    buf[len] = '\0';
    return buf;
}

void jd_mcp_send_message(const cJSON *json)
{
    char *str = cJSON_PrintUnformatted(json);
    if (str) {
        printf("%s\n", str);
        fflush(stdout);
        free(str);
    }
}

void jd_mcp_send_error(unsigned id, int code, const char *msg)
{
    cJSON *resp = cJSON_CreateObject();
    cJSON_AddStringToObject(resp, "jsonrpc", "2.0");
    cJSON_AddNumberToObject(resp, "id", (double)id);

    cJSON *err = cJSON_CreateObject();
    cJSON_AddNumberToObject(err, "code", (double)code);
    cJSON_AddStringToObject(err, "message", msg);
    cJSON_AddItemToObject(resp, "error", err);

    jd_mcp_send_message(resp);
    cJSON_Delete(resp);
}

void jd_mcp_send_tool_result(unsigned id, const char *text)
{
    cJSON *result  = cJSON_CreateObject();
    cJSON *content = cJSON_CreateArray();
    cJSON *item    = cJSON_CreateObject();

    cJSON_AddStringToObject(item, "type", "text");
    cJSON_AddStringToObject(item, "text", text ? text : "");
    cJSON_AddItemToArray(content, item);
    cJSON_AddItemToObject(result, "content", content);

    cJSON *resp = cJSON_CreateObject();
    cJSON_AddStringToObject(resp, "jsonrpc", "2.0");
    cJSON_AddNumberToObject(resp, "id", (double)id);
    cJSON_AddItemToObject(resp, "result", result);

    jd_mcp_send_message(resp);
    cJSON_Delete(resp);
}

char* jd_mcp_read_file(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    if (sz < 0) { fclose(f); return NULL; }
    rewind(f);

    char *buf = malloc((size_t)sz + 1);
    if (!buf) { fclose(f); return NULL; }

    size_t n = fread(buf, 1, (size_t)sz, f);
    fclose(f);
    buf[n] = '\0';
    return buf;
}

char* jd_mcp_create_temp_dir(void)
{
#ifdef _WIN32
    return win_mkdtemp(JD_MCP_TEMP_DIR_PREFIX);
#else
    char *dir = strdup(JD_MCP_TEMP_PATH);
    if (!dir) return NULL;
    if (mkdtemp(dir) == NULL) {
        free(dir);
        return NULL;
    }
    return dir;
#endif
}

static void remove_dir_recursive(const char *path)
{
    DIR *d = opendir(path);
    if (!d) return;

    struct dirent *e;
    char  full[4096];
    while ((e = readdir(d)) != NULL) {
        if (STR_EQL(e->d_name, ".") || STR_EQL(e->d_name, ".."))
            continue;
        snprintf(full, sizeof(full), "%s/%s", path, e->d_name);
        struct stat st;
        if (stat(full, &st) == 0) {
            if (S_ISDIR(st.st_mode))
                remove_dir_recursive(full);
            else
                unlink(full);
        }
    }
    closedir(d);
    rmdir(path);
}

void jd_mcp_remove_temp_dir(const char *path)
{
    if (path) remove_dir_recursive(path);
}

int jd_mcp_detect_file_type(const char *path)
{
    FILE *fp = fopen(path, "rb");
    if (!fp) return JD_MCP_FILE_UNKNOWN;

    u4 magic = 0;
    if (fread(&magic, 1, sizeof(magic), fp) != sizeof(magic)) {
        fclose(fp);
        return JD_MCP_FILE_UNKNOWN;
    }
    fclose(fp);

    u4 be = ((magic & 0xFF) << 24) |
            (((magic >> 8) & 0xFF) << 16) |
            (((magic >> 16) & 0xFF) << 8) |
            ((magic >> 24) & 0xFF);

    switch (be) {
        case JAVA_CLASS_MAGIC:
            return JD_MCP_FILE_CLASS;
        case DEX_FILE_MAGIC:
            return JD_MCP_FILE_DEX;
        case ZIP_FILE_MAGIC: {
            if (strstr(path, ".apk") || strstr(path, ".APK"))
                return JD_MCP_FILE_APK;
            return JD_MCP_FILE_JAR;
        }
        default: return JD_MCP_FILE_UNKNOWN;
    }
}

void mcp_handle_tools_call(jd_mcp_server *server, unsigned id,
                           const char *tool_name, cJSON *args);

static void handle_initialize(jd_mcp_server *server, unsigned id)
{
    server->initialized = true;

    cJSON *result = cJSON_CreateObject();
    cJSON_AddStringToObject(result, "protocolVersion", JD_MCP_PROTOCOL_VERSION);

    cJSON *caps = cJSON_CreateObject();
    cJSON_AddObjectToObject(caps, "tools");
    cJSON_AddItemToObject(result, "capabilities", caps);

    cJSON *info = cJSON_CreateObject();
    cJSON_AddStringToObject(info, "name", JD_MCP_SERVER_NAME);
    cJSON_AddStringToObject(info, "version", JD_MCP_SERVER_VERSION);
    cJSON_AddItemToObject(result, "serverInfo", info);

    cJSON *resp = cJSON_CreateObject();
    cJSON_AddStringToObject(resp, "jsonrpc", "2.0");
    cJSON_AddNumberToObject(resp, "id", (double)id);
    cJSON_AddItemToObject(resp, "result", result);

    jd_mcp_send_message(resp);
    cJSON_Delete(resp);
}

static void handle_tools_list(jd_mcp_server *server, unsigned id)
{
    cJSON *result = cJSON_CreateObject();
    cJSON *tools  = cJSON_CreateArray();

    for (int i = 0; i < server->tool_count; i++) {
        cJSON *t = cJSON_CreateObject();
        cJSON_AddStringToObject(t, "name",        server->tools[i].name);
        cJSON_AddStringToObject(t, "description", server->tools[i].description);

        cJSON *schema = cJSON_Parse(server->tools[i].input_schema);
        if (schema)
            cJSON_AddItemToObject(t, "inputSchema", schema);
        else
            cJSON_AddObjectToObject(t, "inputSchema");

        cJSON_AddItemToArray(tools, t);
    }

    cJSON_AddItemToObject(result, "tools", tools);

    cJSON *resp = cJSON_CreateObject();
    cJSON_AddStringToObject(resp, "jsonrpc", "2.0");
    cJSON_AddNumberToObject(resp, "id", (double)id);
    cJSON_AddItemToObject(resp, "result", result);

    jd_mcp_send_message(resp);
    cJSON_Delete(resp);
}


static void dispatch(jd_mcp_server *server, cJSON *msg)
{
    cJSON *id_json = cJSON_GetObjectItem(msg, "id");
    cJSON *method_json = cJSON_GetObjectItem(msg, "method");

    if (!method_json || !cJSON_IsString(method_json)) {
        if (id_json)
            jd_mcp_send_error((unsigned) cJSON_GetNumberValue(id_json),
                              JD_MCP_ERROR_INVALID_REQ,
                              "Invalid Request: missing method");
        return;
    }

    const char *method = method_json->valuestring;
    unsigned id = id_json && cJSON_IsNumber(id_json)
                      ? (unsigned)cJSON_GetNumberValue(id_json) : 0;

    jd_mcp_log("-> %s (id=%u)", method, id);

    if (STR_EQL(method, "initialize")) {
        handle_initialize(server, id);
    }
    else if (STR_EQL(method, "notifications/initialized")) {
        jd_mcp_log("client initialized");
    }
    else if (STR_EQL(method, "tools/list")) {
        handle_tools_list(server, id);
    }
    else if (STR_EQL(method, "tools/call")) {
        cJSON *par  = cJSON_GetObjectItem(msg, "params");
        cJSON *name = par ? cJSON_GetObjectItem(par, "name") : NULL;
        cJSON *args = par ? cJSON_GetObjectItem(par, "arguments") : NULL;

        if (!name || !cJSON_IsString(name)) {
            jd_mcp_send_error(id, JD_MCP_ERROR_INVALID_PARAMS,
                              "tools/call requires 'name' (string)");
            return;
        }
        mcp_handle_tools_call(server, id,
                              name->valuestring,
                              args ? args : cJSON_CreateObject());
    }
    else {
        jd_mcp_send_error(id, JD_MCP_ERROR_METHOD_NOT_FOUND, "Method not found");
    }
}

void jd_mcp_server_init(jd_mcp_server *server)
{
    server->initialized = false;
    server->shutdown    = false;
}

void jd_mcp_server_run(jd_mcp_server *server)
{
    jd_mcp_log("MCP server starting");

    char *line;
    while (!server->shutdown && (line = read_line()) != NULL) {
        cJSON *msg = cJSON_Parse(line);
        free(line);

        if (!msg) {
            jd_mcp_log("parse error (skipping)");
            continue;
        }

        dispatch(server, msg);
        cJSON_Delete(msg);
    }

    jd_mcp_log("MCP server exiting");
}

void jd_mcp_server_cleanup(jd_mcp_server *server)
{
    (void)server;
    jd_mcp_log("cleanup");
}
