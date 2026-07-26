#ifdef _WIN32
#include <windows.h>
#else
#include <sys/wait.h>
#endif

#include "jd_mcp.h"
#include "str_tools.h"
#include <stdarg.h>
#include <dirent.h>

static const char* garlic_bin(void)
{
    return mcp_self_path ? mcp_self_path : "garlic";
}

/* ------------------------------------------------------------------ *
 *  JSON Schema definitions for each tool's input parameters.
 *  These property descriptions are surfaced in the MCP tools/list
 *  response alongside the tool description, so every field must be
 *  self-explanatory — the LLM reads them to decide what to fill in.
 * ------------------------------------------------------------------ */

#define SCHEMA_DECOMPILE  \
    "{\"type\":\"object\",\"properties\":{"  \
    "\"path\":{\"type\":\"string\",\"description\":\"Path to the target file. Supports .class (Java bytecode), .jar (Java archive), .dex (Dalvik bytecode), and .apk (Android package). Required.\"},"  \
    "\"output_dir\":{\"type\":\"string\",\"description\":\"Directory to write decompiled Java source files into. Directory structure mirrors the Java package hierarchy. When omitted, sources are returned inline as text and the temp directory is cleaned up automatically.\"}"  \
    "},\"required\":[\"path\"]}"

#define SCHEMA_DUMP_INFO  \
    "{\"type\":\"object\",\"properties\":{"  \
    "\"path\":{\"type\":\"string\",\"description\":\"Path to a .class (Java bytecode) or .dex (Dalvik bytecode) file. Shows the internal structure — methods, fields, constants, annotations, and signatures — without performing a full decompilation.\"}"  \
    "},\"required\":[\"path\"]}"

#define SCHEMA_CALL_GRAPH  \
    "{\"type\":\"object\",\"properties\":{"  \
    "\"path\":{\"type\":\"string\",\"description\":\"Path to a .dex (Dalvik bytecode) or .apk (Android package) file from which to generate the call graph.\"},"  \
    "\"output_dir\":{\"type\":\"string\",\"description\":\"Directory to write CSV files into. Produces call_graph_node.csv (method metadata with type/api_type classification) and call_graph_edge.csv (caller-callee edges). When omitted, uses a temp directory. These CSVs are designed for import into DuckDB via the cg_import tool.\"}"  \
    "},\"required\":[\"path\"]}"

#define SCHEMA_CG_IMPORT  \
    "{\"type\":\"object\",\"properties\":{"  \
    "\"cg_dir\":{\"type\":\"string\",\"description\":\"Directory containing the CSV output from a previous call_graph run. Must include call_graph_node.csv and call_graph_edge.csv. Optionally also imports string_node.csv and string_edge.csv if present.\"},"  \
    "\"db_path\":{\"type\":\"string\",\"description\":\"File path where the DuckDB database (.duckdb) will be created. This database is then queried via the cg_query tool. Example: ./analysis.duckdb\"}"  \
    "},\"required\":[\"cg_dir\",\"db_path\"]}"

#define SCHEMA_CG_QUERY  \
    "{\"type\":\"object\",\"properties\":{"  \
    "\"db_path\":{\"type\":\"string\",\"description\":\"Path to a .duckdb database file previously created by cg_import. Contains java_cg_nodes (method metadata) and java_cg_edges (call relationships) tables, plus optionally string_nodes (string constants) and string_edges (string-to-method references).\"},"  \
    "\"sql\":{\"type\":\"string\",\"description\":\"SQL query to run. Supports SELECT, JOIN, aggregation, subqueries — any valid DuckDB SQL. Example: SELECT * FROM java_cg_nodes WHERE node_type = 1 LIMIT 20\"}"  \
    "},\"required\":[\"db_path\",\"sql\"]}"

#define SCHEMA_ANALYZE  \
    "{\"type\":\"object\",\"properties\":{"  \
    "\"path\":{\"type\":\"string\",\"description\":\"Path to a .dex (Dalvik bytecode) or .apk (Android package) file for comprehensive one-shot analysis.\"},"  \
    "\"output_dir\":{\"type\":\"string\",\"description\":\"Working directory for all outputs. Creates: decompiled/ (Java sources), cg/ (call graph CSVs), and analysis.duckdb (imported DuckDB database). Example: ./my_app_analysis\"}"  \
    "},\"required\":[\"path\",\"output_dir\"]}"

#define SCHEMA_ANDROID_MANIFEST  \
    "{\"type\":\"object\",\"properties\":{"  \
    "\"output_dir\":{\"type\":\"string\",\"description\":\"The output directory from a previous decompile or analyze run. Must contain AndroidManifest.xml which was extracted from the APK during decompilation. Returns the full XML content including permissions, activities, services, receivers, and intent filters.\"}"  \
    "},\"required\":[\"output_dir\"]}"


#ifdef _WIN32
#define DUCKDB_CMD_CHECK "where duckdb >nul 2>&1"
#else
#define DUCKDB_CMD_CHECK "command -v duckdb >/dev/null 2>&1"
#endif

/* ------------------------------------------------------------------ *
 *  Tool declarations for the MCP protocol.
 *
 *  The `description` field is what every MCP client (and therefore
 *  every LLM that decides whether to call the tool) sees in the
 *  tools/list response.  These descriptions must be self-contained,
 *  covering: what the tool does, when to use it, what it depends on,
 *  what output to expect, and how it fits into a multi-tool workflow.
 *
 *  ORDERING NOTE:  tools are listed in typical workflow order so
 *  that an LLM scanning the list sees the "entry point" tools first.
 * ------------------------------------------------------------------ */

const jd_mcp_tool MCP_TOOLS[] = {
    {
        .name         = "analyze",
        .description  = "RECOMMENDED ENTRY POINT. One-shot comprehensive reverse-engineering of an APK or DEX: (1) decompile to Java source, (2) generate a full call graph, (3) import everything into DuckDB for SQL analysis. Creates decompiled/ (Java sources), cg/ (call graph CSVs), and analysis.duckdb. Run this first for most workflows, then use cg_query to explore the call graph with SQL or android_manifest to read app permissions/entry points. Requires the 'duckdb' CLI on PATH.",
        .input_schema = SCHEMA_ANALYZE,
    },
    {
        .name         = "decompile",
        .description  = "Decompile a Java class (.class), JAR archive (.jar), DEX bytecode (.dex), or Android APK (.apk) to readable Java source code. Preserves package directory structure and extracts AndroidManifest.xml alongside the sources. When no output_dir is given, results are returned inline and temp files are cleaned. Use this when you only need Java source recovery without call graph or SQL analysis.",
        .input_schema = SCHEMA_DECOMPILE,
    },
    {
        .name         = "dump_info",
        .description  = "Quick-inspect the internal structure of a .class or .dex file — methods, fields, constants, annotations, and signatures — similar to javap / dexdump. Use this for fast validation or API-surface inspection without generating full decompiled source.",
        .input_schema = SCHEMA_DUMP_INFO,
    },
    {
        .name         = "call_graph",
        .description  = "Generate a call graph for a .dex or .apk file, writing CSV files for downstream analysis. Produces call_graph_node.csv (each row = one method with node_type/api_type classification) and call_graph_edge.csv (each row = one caller→callee edge). When no output_dir is given, uses a temp directory. Use this when you want to understand method dependencies, find entry points, trace data flow, or detect unused code. The CSV output can be imported into DuckDB via cg_import for SQL queries.",
        .input_schema = SCHEMA_CALL_GRAPH,
    },
    {
        .name         = "cg_import",
        .description  = "Import call graph CSV files (produced by call_graph) into a DuckDB database with indexed tables for fast SQL querying. Creates java_cg_nodes (method metadata including type/api_type) and java_cg_edges (caller→callee relationships) tables with indexes. Also imports string_node.csv / string_edge.csv if present. Requires the 'duckdb' CLI. Run this after call_graph, then use cg_query to analyse the database.",
        .input_schema = SCHEMA_CG_IMPORT,
    },
    {
        .name         = "cg_query",
        .description  = "Run a SQL query against a call graph DuckDB database (created by cg_import). Supports full DuckDB SQL — SELECT, JOIN, aggregation, subqueries, window functions. Tables: java_cg_nodes(node_id, method_raw, node_type, api_type) for method metadata; java_cg_edges(src_id, dst_id) for call relationships; plus optional string_nodes/string_edges for string-reference analysis. Use cases: find all callers/callees of a method, discover app entry points, measure API usage frequency, trace data-flow paths through the call chain, find unused (zero-callee) methods. Requires the 'duckdb' CLI.",
        .input_schema = SCHEMA_CG_QUERY,
    },
    {
        .name         = "android_manifest",
        .description  = "Read the AndroidManifest.xml from a previous decompile or analyze output directory. Returns the full XML content including app permissions, activity/service/receiver declarations, and intent filters. Use after decompile or analyze on an APK to understand the app's attack surface, entry points, and declared capabilities.",
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


static char* exec_and_capture(const char *cmd_fmt, ...)
{
    char cmd[4096];
    va_list ap;
    va_start(ap, cmd_fmt);
    vsnprintf(cmd, sizeof(cmd), cmd_fmt, ap);
    va_end(ap);

    jd_mcp_log("exec: %s", cmd);

    FILE *fp = popen(cmd, "r");
    if (!fp) return NULL;

    size_t cap = 4096, len = 0;
    char *buf = malloc(cap);
    if (!buf) { pclose(fp); return NULL; }
    buf[0] = '\0';

    char line[4096];
    while (fgets(line, sizeof(line), fp)) {
        size_t slen = strlen(line);
        if (len + slen + 1 > cap) {
            cap = (len + slen + 1) * 2;
            char *nb = realloc(buf, cap);
            if (!nb) { free(buf); pclose(fp); return NULL; }
            buf = nb;
        }
        memcpy(buf + len, line, slen);
        len += slen;
        buf[len] = '\0';
    }

    int rc = pclose(fp);
    if (rc != 0 && len == 0) {
        free(buf);
        return NULL;
    }
    return buf;
}

static int exec_silent(const char *cmd_fmt, ...)
{
    char cmd[4096];
    va_list ap;
    va_start(ap, cmd_fmt);
    vsnprintf(cmd, sizeof(cmd), cmd_fmt, ap);
    va_end(ap);

    jd_mcp_log("exec: %s", cmd);
    int rc = system(cmd);
    if (rc == -1) return -1;
#ifndef _WIN32
    if (WIFEXITED(rc)) return WEXITSTATUS(rc);
#else
    return rc;
#endif
    return -1;
}

static string tool_decompile(const char *path, const char *output_dir)
{
    char escaped_path[2048];
    char escaped_out[2048];
    snprintf(escaped_path, sizeof(escaped_path), "'%s'", path);

    const char *save_dir = output_dir;
    char tmp_path[2048];
    if (!output_dir || output_dir[0] == '\0') {
        char *d = jd_mcp_create_temp_dir();
        if (!d) return strdup("Error: cannot create temp directory");
        snprintf(tmp_path, sizeof(tmp_path), "%s", d);
        free(d);
        save_dir = tmp_path;
    }
    snprintf(escaped_out, sizeof(escaped_out), "'%s'", save_dir);

    char cmd[4096];
    snprintf(cmd, sizeof(cmd), "%s %s -o %s 2>/dev/null",
             garlic_bin(), escaped_path, escaped_out);

    int rc = exec_silent(cmd);

    if (rc != 0 && !output_dir) {
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
    char cmd[4096];
    snprintf(cmd, sizeof(cmd), "%s '%s' -p 2>/dev/null", garlic_bin(), path);

    char *out = exec_and_capture(cmd);
    if (!out) return strdup("Error: dump_info failed (file not found or unsupported)");
    return out;
}

static string tool_call_graph(const char *path, const char *output_dir)
{
    char cmd[4096];
    const char *save_dir = output_dir;
    char tmp_path[2048];

    if (!output_dir || output_dir[0] == '\0') {
        char *d = jd_mcp_create_temp_dir();
        if (!d) return strdup("Error: cannot create temp directory");
        snprintf(tmp_path, sizeof(tmp_path), "%s", d);
        free(d);
        save_dir = tmp_path;
    }

    snprintf(cmd, sizeof(cmd), "%s '%s' -g -o '%s'",
             garlic_bin(), path, save_dir);

    int rc = exec_silent(cmd);

    if (rc != 0) {
        if (!output_dir || output_dir[0] == '\0')
            jd_mcp_remove_temp_dir(save_dir);
        char err[256];
        snprintf(err, sizeof(err), "Error: call_graph failed (exit=%d)", rc);
        return strdup(err);
    }

    char *result = malloc(1024);
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

    if (system(DUCKDB_CMD_CHECK) != 0)
        return strdup("err: duckdb not found on PATH — install it and ensure "
                      "it is on PATH (e.g. Homebrew's /opt/homebrew/bin)");

    char cmd[16384];
    int n = snprintf(cmd, sizeof(cmd),
        "duckdb '%s' 2>&1 << 'DUCKEOF'\n"
        "PRAGMA threads=4;\n"
        "PRAGMA memory_limit='4GB';\n"
        "PRAGMA preserve_insertion_order=false;\n"
        "BEGIN TRANSACTION;\n"
        "CREATE TABLE IF NOT EXISTS java_cg_nodes(node_id BIGINT, method_raw VARCHAR, node_type BIGINT, api_type BIGINT);\n"
        "INSERT INTO java_cg_nodes SELECT CAST(id AS BIGINT), CAST(method AS VARCHAR), CAST(COALESCE(type,0) AS BIGINT), CAST(COALESCE(api_type,0) AS BIGINT) FROM read_csv_auto('%s', HEADER=TRUE, SAMPLE_SIZE=-1, ignore_errors=true);\n"
        "CREATE TABLE IF NOT EXISTS java_cg_edges(src_id BIGINT, dst_id BIGINT);\n"
        "INSERT INTO java_cg_edges SELECT DISTINCT CAST(src_id AS BIGINT), CAST(dst_id AS BIGINT) FROM read_csv_auto('%s', HEADER=TRUE, SAMPLE_SIZE=-1, ignore_errors=true);\n",
        db_path, node_csv, edge_csv);

    if (access(str_node_csv, F_OK) == 0) {
        n += snprintf(cmd + n, sizeof(cmd) - n,
            "CREATE TABLE IF NOT EXISTS string_nodes(id BIGINT, pc BIGINT, str VARCHAR, is_class_desc BOOLEAN, is_field_name BOOLEAN, is_method_name BOOLEAN, is_return_type BOOLEAN, is_method_param_type BOOLEAN, is_internal_class_desc BOOLEAN, is_url BOOLEAN, is_enc_dec BOOLEAN, is_uuid BOOLEAN, is_pem_key BOOLEAN, is_so_name BOOLEAN, is_ipv4 BOOLEAN);\n"
            "INSERT INTO string_nodes SELECT * FROM read_csv_auto('%s', HEADER=TRUE, SAMPLE_SIZE=-1, ignore_errors=true);\n",
            str_node_csv);
    }
    if (access(str_edge_csv, F_OK) == 0) {
        n += snprintf(cmd + n, sizeof(cmd) - n,
            "CREATE TABLE IF NOT EXISTS string_edges(string_id BIGINT, method_id BIGINT);\n"
            "INSERT INTO string_edges SELECT src_id AS string_id, dst_id AS method_id FROM read_csv_auto('%s', HEADER=TRUE, SAMPLE_SIZE=-1, ignore_errors=true);\n",
            str_edge_csv);
    }

    snprintf(cmd + n, sizeof(cmd) - n,
        "CREATE INDEX IF NOT EXISTS idx_nodes_id ON java_cg_nodes(node_id);\n"
        "CREATE INDEX IF NOT EXISTS idx_edges_src ON java_cg_edges(src_id);\n"
        "CREATE INDEX IF NOT EXISTS idx_edges_dst ON java_cg_edges(dst_id);\n"
        "COMMIT;\n"
        "SELECT 'nodes' AS tbl, COUNT(*) AS cnt FROM java_cg_nodes "
        "UNION ALL SELECT 'edges', COUNT(*) FROM java_cg_edges;\n"
        "DUCKEOF");

    char *out = exec_and_capture("%s", cmd);
    if (!out) return strdup("err: duckdb import failed");
    return out;
}

static char* tool_cg_query(const char *db_path, const char *sql)
{
    if (access(db_path, F_OK) != 0)
        return strdup("err: database file not found");

    /* Pass SQL via a quoted heredoc (as tool_cg_import does) so the shell
     * performs no expansion or quote processing on it — avoids the
     * single-quote-in-double-quote collapse entirely. */
    char cmd[16512];
    snprintf(cmd, sizeof(cmd),
        "duckdb -readonly -csv '%s' 2>/dev/null << 'DUCKEOF'\n"
        "%s\n"
        "DUCKEOF",
        db_path, sql);

    char *out = exec_and_capture("%s", cmd);
    if (!out) return strdup("(no results)");
    return out;
}

static char* tool_analyze(const char *path, const char *out_dir)
{
    char decompile_dir[4096], cg_dir[4096], db_path[4096];
    snprintf(decompile_dir, sizeof(decompile_dir), "%s/decompiled", out_dir);
    snprintf(cg_dir, sizeof(cg_dir), "%s/cg", out_dir);
    snprintf(db_path, sizeof(db_path), "%s/analysis.duckdb", out_dir);

    char cmd[8192];
    snprintf(cmd, sizeof(cmd), "%s '%s' -o '%s' >/dev/null 2>&1", garlic_bin(), path, decompile_dir);
    int rc = exec_silent(cmd);
    if (rc != 0)
        return strdup("err: decompilation failed");

    snprintf(cmd, sizeof(cmd), "%s '%s' -g -o '%s' >/dev/null 2>&1", garlic_bin(), path, cg_dir);
    rc = exec_silent(cmd);
    if (rc != 0)
        return strdup("err: call graph created failed");

    char node_csv[4096], edge_csv[4096];
    snprintf(node_csv, sizeof(node_csv), "%s/call_graph_node.csv", cg_dir);
    snprintf(edge_csv, sizeof(edge_csv), "%s/call_graph_edge.csv", cg_dir);
    if (access(node_csv, F_OK) == 0 && access(edge_csv, F_OK) == 0) {
        char *import_result = tool_cg_import(cg_dir, db_path);
        if (import_result && strncmp(import_result, "err:", 4) == 0)
            return import_result;   /* propagate the real reason (e.g. "not on PATH") */
        free(import_result);
        if (access(db_path, F_OK) != 0)
            return strdup("err: duckdb import produced no database file");
    }

    char result[16384];
    int n = snprintf(result, sizeof(result),
        "Analysis complete for: %s\n\n"
        "  Decompiled:  %s/\n"
        "  Call graph:  %s/\n"
        "  DuckDB:      %s\n\n",
        path, decompile_dir, cg_dir, db_path);

    snprintf(cmd, sizeof(cmd), "find '%s' -name '*.java' 2>/dev/null | wc -l", decompile_dir);
    FILE *fp = popen(cmd, "r");
    if (fp) {
        char buf[64];
        if (fgets(buf, sizeof(buf), fp))
            n += snprintf(result + n, sizeof(result) - n, "  Java files:  %s", buf);
        pclose(fp);
    }

    snprintf(cmd, sizeof(cmd), "wc -l < '%s' 2>/dev/null", node_csv);
    fp = popen(cmd, "r");
    if (fp) {
        char buf[64];
        if (fgets(buf, sizeof(buf), fp))
            n += snprintf(result + n, sizeof(result) - n, "  CG nodes:    %d\n", atoi(buf) - 1);
        pclose(fp);
    }

    snprintf(cmd, sizeof(cmd), "wc -l < '%s' 2>/dev/null", edge_csv);
    fp = popen(cmd, "r");
    if (fp) {
        char buf[64];
        if (fgets(buf, sizeof(buf), fp))
            n += snprintf(result + n, sizeof(result) - n, "  CG edges:    %d\n", atoi(buf) - 1);
        pclose(fp);
    }

    n += snprintf(result + n, sizeof(result) - n,
        "\nReady: cg_query(db_path=\"%s\", sql=\"...\")\n", db_path);
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
