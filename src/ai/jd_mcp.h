#ifndef GARLIC_JD_MCP_H
#define GARLIC_JD_MCP_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include "cJSON.h"
#include "types.h"

#define JD_MCP_PROTOCOL_VERSION         "2024-11-05"
#define JD_MCP_SERVER_NAME              "garlic-mcp"
#define JD_MCP_SERVER_VERSION           "1.0.0"

#define JD_MCP_MAX_LINE_SIZE            (1024U * 1024U)   // 1mb message
#define JD_MCP_TEMP_PATH                "/tmp/garlic_mcp_XXXXXX"

#define JD_MCP_ERROR_PARSE              -32700
#define JD_MCP_ERROR_INVALID_REQ        -32600
#define JD_MCP_ERROR_METHOD_NOT_FOUND   -32601
#define JD_MCP_ERROR_INVALID_PARAMS     -32602
#define JD_MCP_ERROR_INTERNAL           -32603
#define JD_MCP_ERROR_TOOL_EXEC          -32003


#ifdef NDEBUG
  #define jd_mcp_log(fmt, ...)  ((void)0)
#else
  #define jd_mcp_log(fmt, ...)  fprintf(stderr, "[mcp] " fmt "\n", ##__VA_ARGS__)
#endif

#define JD_MCP_FILE_UNKNOWN  0
#define JD_MCP_FILE_CLASS    1
#define JD_MCP_FILE_JAR      2
#define JD_MCP_FILE_DEX      3
#define JD_MCP_FILE_APK      4

typedef struct {
    string name;
    string description;
    string input_schema;
} jd_mcp_tool;

typedef struct {
    bool initialized;
    bool shutdown;
    jd_mcp_tool *tools;
    int tool_count;
} jd_mcp_server;

void  jd_mcp_server_init(jd_mcp_server *server);
void  jd_mcp_server_run(jd_mcp_server *server);
void  jd_mcp_server_cleanup(jd_mcp_server *server);

extern const char *mcp_self_path;
void  jd_mcp_set_self_path(const char *path);


void  jd_mcp_send_message(const cJSON *json);
void  jd_mcp_send_error(unsigned id, int code, const char *msg);
void  jd_mcp_send_tool_result(unsigned id, const char *text);
string jd_mcp_read_file(const char *path);
string jd_mcp_create_temp_dir(void);
void  jd_mcp_remove_temp_dir(const char *path);
int jd_mcp_detect_file_type(const char *path);

#endif /* GARLIC_JD_MCP_H */
