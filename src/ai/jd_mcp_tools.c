#ifdef _WIN32
#include <windows.h>
#else
#include <sys/wait.h>
#endif

#include "jd_mcp.h"
#include "jd_mcp_sql.h"
#include "str_tools.h"
#include "rosemary/rosemary_embed.h"
#include "report/report.h"
#include <dirent.h>
#include <errno.h>
#include <stdarg.h>
#include <time.h>

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
    "\"path\":{\"type\":\"string\",\"description\":\"Path to an APK (.apk) or DEX (.dex) file. This is the primary entry point for reverse-engineering. When the user says 'analyze this APK' or 'decompile this app', ALWAYS use the analyze tool (not decompile) — it does decompilation + call graph + DuckDB import in one shot.\"},"  \
    "\"output_dir\":{\"type\":\"string\",\"description\":\"Working directory for all outputs. Creates: decompiled/ (Java sources), cg/ (call graph CSVs), and analysis.duckdb (imported DuckDB database). Example: ./my_app_analysis\"}"  \
    "},\"required\":[\"path\",\"output_dir\"]}"

#define SCHEMA_ANDROID_MANIFEST  \
    "{\"type\":\"object\",\"properties\":{"  \
    "\"output_dir\":{\"type\":\"string\",\"description\":\"The output directory from a previous decompile or analyze run. Must contain AndroidManifest.xml which was extracted from the APK during decompilation. Returns the full XML content including permissions, activities, services, receivers, and intent filters.\"}"  \
    "},\"required\":[\"output_dir\"]}"

#define SCHEMA_TRACE_FLOW  \
    "{\"type\":\"object\",\"properties\":{"  \
    "\"db_path\":{\"type\":\"string\",\"description\":\"Path to the analysis DuckDB database.\"},"  \
    "\"keyword\":{\"type\":\"string\",\"description\":\"Keyword to search in Java method names (e.g. 'login', 'decrypt', 'encrypt'). Searches java_cg_nodes.method_raw with LIKE matching.\"},"  \
    "\"native_dir\":{\"type\":\"string\",\"description\":\"Path to native_libs directory (e.g. '<out>/native_libs'). Needed to read .dissembly files for disassembly output.\"},"  \
    "\"depth\":{\"type\":\"integer\",\"description\":\"How many levels of callers/callees to trace. Default 2, max 5.\"}"  \
    "},\"required\":[\"db_path\",\"keyword\"]}"

#define SCHEMA_NATIVE_METHOD_FIND  \
    "{\"type\":\"object\",\"properties\":{"  \
    "\"db_path\":{\"type\":\"string\",\"description\":\"Path to the analysis DuckDB database (.duckdb) created by the analyze tool.\"},"  \
    "\"name\":{\"type\":\"string\",\"description\":\"Method name or partial name to search. Accepts both Java-style (e.g. 'com.example.ClassName.methodName') and mangled JNI names (e.g. 'Java_com_example_ClassName_methodName_1foo'). The tool converts Java dots to underscores and searches native_exports with LIKE matching.\"}"  \
    "},\"required\":[\"db_path\",\"name\"]}"

#define SCHEMA_ANALYZE_ELF  \
    "{\"type\":\"object\",\"properties\":{"  \
    "\"path\":{\"type\":\"string\",\"description\":\"Path to an ELF (.so) or Mach-O (.dylib) native binary file. Analyzes the binary with the embedded rosemary engine: disassembly, exports, imports, function cross-references, and string analysis. Output files are created alongside the input file with suffixes like .exports, .func_xref, .dissembly, .entries, .imports, .strings, .pc_xrefs, .cfg_nodes, .cfg_edges.\"}"  \
    "},\"required\":[\"path\"]}"

#define SCHEMA_GENERATE_REPORT  \
    "{\"type\":\"object\",\"properties\":{"  \
    "\"analysis_dir\":{\"type\":\"string\",\"description\":\"Path to the analysis output directory (same as analyze output_dir). Must contain analysis.duckdb and decompiled/ cg/ native_libs/ subdirectories.\"},"  \
    "\"report_dir\":{\"type\":\"string\",\"description\":\"Directory to write report files into. Defaults to <analysis_dir>/report. Creates the directory if it doesn't exist. After generating, use start_report_http_server pointing to this directory to view the HTML report in a browser.\"}"  \
    "},\"required\":[\"analysis_dir\"]}"

#define SCHEMA_START_HTTP_SERVER  \
    "{\"type\":\"object\",\"properties\":{"  \
    "\"directory\":{\"type\":\"string\",\"description\":\"Directory to serve static files from (e.g. the analysis output_dir).\"},"  \
    "\"port\":{\"type\":\"integer\",\"description\":\"TCP port to listen on (default: 8080).\"}"  \
    "},\"required\":[\"directory\"]}"


const jd_mcp_tool MCP_TOOLS[] = {
    {
        .name         = "analyze",
        .description  = "STEP 1 — Analyze an APK or DEX file. This is the first step in any APK reverse-engineering workflow. "
                        "Decompiles to Java source, generates a call graph, and imports everything into DuckDB for SQL queries. "
                        "Takes 30-60 seconds for a typical APK. Requires 'duckdb' CLI on PATH. "
                        "Decompiles to Java source, generates call graph, imports into DuckDB. "
                        "Automatically produces report.md + report.html in <output_dir>/report/. "
                        "When the user wants to view the report, use start_report_http_server.",
        .input_schema = SCHEMA_ANALYZE,
    },
    {
        .name         = "decompile",
        .description  = "Decompile Java bytecode to readable Java source code only. "
                        "Supports .class / .jar / .dex / .apk files. "
                        "NOTE: this only does decompilation — no call graph, no DuckDB, no report. "
                        "For full analysis (decompile + call graph + SQL + report), use the analyze tool instead.",
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
    {
        .name         = "native_method_find",
        .description  = "Map a Java native method to its C/C++ implementation in an SO. Converts Java-style method names to JNI function names and searches the native_exports DuckDB table for matches. Use after analyze on an APK to trace Java native calls into their native implementations. Also returns callers/callees from native_func_xref and entry-point info. Requires the 'duckdb' CLI.",
        .input_schema = SCHEMA_NATIVE_METHOD_FIND,
    },
    {
        .name         = "trace_flow",
        .description  = "Trace the full execution path of a method through all layers: Java call graph → JNI native bridge → native function cross-references → disassembly. Search for a keyword (e.g. 'login', 'crypto', 'JNI') in java_cg_nodes, follow callers/callees, cross into native_exports for JNI boundaries, then follow native_func_xref. Disassembly is shown via grep on .dissembly files (NOT imported into DuckDB). Requires the 'duckdb' CLI.",
        .input_schema = SCHEMA_TRACE_FLOW,
    },
    {
        .name         = "analyze_elf",
        .description  = "Analyze a native ELF (.so) or Mach-O (.dylib) binary file. "
                        "Extracts: disassembly listing, exported/imported symbols, "
                        "function cross-references, string constants, entry points, "
                        "and control-flow graphs. "
                        "Output files are created alongside the input file with "
                        "suffixes: .exports, .func_xref, .dissembly, .entries, "
                        ".imports, .strings, .pc_xrefs, .cfg_nodes, .cfg_edges. "
                        "Use this when you have a standalone native library (.so/.dylib) "
                        "that was extracted from an APK or obtained separately. "
                        "For full APK analysis (Java+Native), use the analyze tool instead.",
        .input_schema = SCHEMA_ANALYZE_ELF,
    },
    {
        .name         = "generate_report",
        .description  = "STEP 2 — Generate an analysis report (Markdown + HTML) from a completed analyze run. "
                        "Queries the DuckDB database for: overview stats (methods, call edges, native libraries), "
                        "Android permissions, top .so exports, and URL endpoints from string analysis. "
                        "Writes report.md + report.html into the specified report_dir. "
                        "Takes ~2-5 seconds. Requires a completed analyze run first. "
                        "➡ Next step: use start_report_http_server to serve the report directory so the user "
                        "can open the HTML in their browser at http://localhost:<port>/report.html",
        .input_schema = SCHEMA_GENERATE_REPORT,
    },
    {
        .name         = "start_report_http_server",
        .description  = "Start a background HTTP server to view analysis reports in a browser. "
                        "Only use this when the user explicitly asks to 'view the report' or 'open in browser'. "
                        "Serves static files from the given directory with directory listing. "
                        "After starting, tell the user: open http://localhost:<port>/report.html",
        .input_schema = SCHEMA_START_HTTP_SERVER,
    },
};

const int MCP_TOOL_COUNT = (int)(sizeof(MCP_TOOLS) / sizeof(MCP_TOOLS[0]));

static char* mcp_read_dir_java(mem_pool *pool, const char *dir)
{
    DIR *d = opendir(dir);
    if (!d) return NULL;

    size_t cap = 4096, len = 0;
    char *out = mem_pool_alloc(pool, cap);
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
            char *sub = mcp_read_dir_java(pool, full);
            if (sub) {
                size_t slen = strlen(sub);
                if (len + slen + 1 > cap) {
                    cap = (len + slen + 1) * 2;
                    out = mem_pool_realloc(pool, out, (len + slen + 1), cap);
                    if (!out) { closedir(d); return NULL; }
                }
                memcpy(out + len, sub, slen);
                len += slen;
                out[len] = '\0';
            }
            continue;
        }

        if (!strstr(e->d_name, ".java")) continue;

        char *content = jd_mcp_read_file(full);
        if (!content) continue;

        int need = snprintf(NULL, 0, "// --- %s ---\n%s\n", e->d_name, content);
        if (need > 0 && (size_t)need + len + 1 > cap) {
            cap = ((size_t)need + len + 1) * 2;
            out = mem_pool_realloc(pool, out, (size_t)need + len, cap);
            if (!out) { free(content); closedir(d); return NULL; }
        }
        len += snprintf(out + len, cap - len,
                        "// --- %s ---\n%s\n", e->d_name, content);
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
    if (!path || path[0] == '\0')
        return false;

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

static char* duckdb_escape_path(mem_pool *pool, const char *path)
{
    size_t len = strlen(path);
    char *escaped = mem_pool_alloc(pool, len * 2 + 1);
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

static char* run_duckdb_script(mem_pool *pool, const char *db_path, const char *sql,
                               bool readonly_csv, int *exit_code)
{
    char *temp_dir = jd_mcp_create_temp_dir();
    if (!temp_dir) {
        if (exit_code) *exit_code = -1;
        return NULL;
    }

//    char script_path[4096];
//    snprintf(script_path, sizeof(script_path), "%s/query.sql", temp_dir);
    string script_path = str_create_in(pool, "%s/query.sql", temp_dir);
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

    const char *write_argv[] = {"duckdb", "-csv", db_path, NULL};
    const char *read_argv[] = {
        "duckdb", "-readonly", "-csv", db_path, NULL
    };
    char *raw = NULL;
    int rc = exec_process(readonly_csv ? read_argv : write_argv,
                          script_path, true, &raw);

    jd_mcp_remove_temp_dir(temp_dir);
    free(temp_dir);

    char *output = NULL;
    if (raw) {
        output = str_create_in(pool, "%s", raw);
        free(raw);
    }
    if (exit_code) *exit_code = rc;
    return output;
}

static char* process_error(mem_pool *pool, const char *operation, int rc, const char *detail)
{
    return str_create_in(pool, "err: %s failed (exit=%d)%s%s",
                         operation, rc,
                         detail ? ": " : "", detail ? detail : "");
}

static string tool_decompile(jd_mcp_server *server, const char *path, const char *output_dir)
{
    mem_pool *pool = server->pool;

    if (jd_mcp_detect_file_type(path) == JD_MCP_FILE_CLASS) {
        const char *argv[] = {garlic_bin(), path, NULL};
        char *raw = NULL;
        int rc = exec_process(argv, NULL, true, &raw);
        if (rc != 0 || !raw || raw[0] == '\0') {
            free(raw);
            return str_create_in(pool, "Error: decompilation failed (exit=%d)", rc);
        }
        char *result = str_create_in(pool, "%s", raw);
        free(raw);
        return result;
    }

    const char *save_dir = output_dir;
    char tmp_path[2048];
    if (!output_dir || output_dir[0] == '\0') {
        char *d = jd_mcp_create_temp_dir();
        if (!d) return str_create_in(pool, "Error: cannot create temp directory");
        snprintf(tmp_path, sizeof(tmp_path), "%s", d);
        free(d);
        save_dir = tmp_path;
    }

    if (!ensure_directory(save_dir)) {
        if (!output_dir || output_dir[0] == '\0')
            jd_mcp_remove_temp_dir(save_dir);
        return str_create_in(pool, "Error: cannot create output directory");
    }

    const char *argv[] = {garlic_bin(), path, "-o", save_dir, NULL};
    int rc = exec_process(argv, NULL, false, NULL);
    if (rc != 0) {
        if (!output_dir || output_dir[0] == '\0')
            jd_mcp_remove_temp_dir(save_dir);
        return str_create_in(pool, "Error: decompilation failed (exit=%d)", rc);
    }

    if (!output_dir || output_dir[0] == '\0') {
        char *result = mcp_read_dir_java(pool, save_dir);
        jd_mcp_remove_temp_dir(save_dir);
        if (!result)
            result = str_create_in(pool, "(decompilation produced no output)");
        return result;
    }

#ifdef _WIN32
    char abs[4096];
    char *p = _fullpath(abs, save_dir, sizeof(abs));
    return str_create_in(pool, "Decompiled to: %s", p ? abs : save_dir);
#else
    char *abs = realpath(save_dir, NULL);
    char *result = str_create_in(pool, "Decompiled to: %s", abs ? abs : save_dir);
    free(abs);
    return result;
#endif
}

static string tool_dump_info(jd_mcp_server *server, const char *path)
{
    mem_pool *pool = server->pool;
    const char *argv[] = {garlic_bin(), path, "-p", NULL};
    char *raw = NULL;
    int rc = exec_process(argv, NULL, true, &raw);
    if (rc != 0 || !raw) {
        free(raw);
        return str_create_in(pool,
            "Error: dump_info failed (exit=%d; file may be unsupported)", rc);
    }
    char *result = str_create_in(pool, "%s", raw);
    free(raw);
    return result;
}

static string tool_call_graph(jd_mcp_server *server, const char *path, const char *output_dir)
{
    mem_pool *pool = server->pool;
    const char *save_dir = output_dir;
    char tmp_path[2048];

    if (!output_dir || output_dir[0] == '\0') {
        char *d = jd_mcp_create_temp_dir();
        if (!d) return str_create_in(pool, "Error: cannot create temp directory");
        snprintf(tmp_path, sizeof(tmp_path), "%s", d);
        free(d);
        save_dir = tmp_path;
    }

    if (!ensure_directory(save_dir)) {
        if (!output_dir || output_dir[0] == '\0')
            jd_mcp_remove_temp_dir(save_dir);
        return str_create_in(pool, "Error: cannot create output directory");
    }

    const char *argv[] = {garlic_bin(), path, "-g", "-o", save_dir, NULL};
    int rc = exec_process(argv, NULL, false, NULL);

    if (rc != 0) {
        if (!output_dir || output_dir[0] == '\0')
            jd_mcp_remove_temp_dir(save_dir);
        return str_create_in(pool, "Error: call_graph failed (exit=%d)", rc);
    }

    return str_create_in(pool, "Call graph generated in: %s", save_dir);
}

#define JD_CALL_GRAPH_IMPORT_SUCCESS          0
#define JD_CALL_GRAPH_IMPORT_ERR_NO_DATA      1
#define JD_CALL_GRAPH_IMPORT_ERR_NO_DUCKDB    2
#define JD_CALL_GRAPH_IMPORT_ERR_NO_OUTPUT    3
#define JD_CALL_GRAPH_IMPORT_ERR_DUCKDB_OUT_MEMORY  4
#define JD_CALL_GRAPH_IMPORT_ERR_SQL_TOO_LARGE      5
#define JD_CALL_GRAPH_IMPORT_ERROR_DUCKDB_ERR       6

static int jd_mcp_cg_import(jd_mcp_server *server, string cg_dir, string db_path)
{
    mem_pool *pool = server->pool;

    string node_csv = str_create_in(pool, "%s/call_graph_node.csv", cg_dir);
    string edge_csv = str_create_in(pool, "%s/call_graph_edge.csv", cg_dir);
    string str_node_csv = str_create_in(pool, "%s/string_node.csv", cg_dir);
    string str_edge_csv = str_create_in(pool, "%s/string_edge.csv", cg_dir);

    if (access(node_csv, F_OK) != 0 || access(edge_csv, F_OK) != 0)
        return JD_CALL_GRAPH_IMPORT_ERR_NO_DATA;
    if (!duckdb_available())
        return JD_CALL_GRAPH_IMPORT_ERR_NO_DUCKDB;
    if (!ensure_parent_directory(db_path))
        return JD_CALL_GRAPH_IMPORT_ERR_NO_OUTPUT;

    char *node_path = duckdb_escape_path(pool, node_csv);
    char *edge_path = duckdb_escape_path(pool, edge_csv);
    char *str_node_path = duckdb_escape_path(pool, str_node_csv);
    char *str_edge_path = duckdb_escape_path(pool, str_edge_csv);
    if (!node_path || !edge_path || !str_node_path || !str_edge_path)
        return JD_CALL_GRAPH_IMPORT_ERR_DUCKDB_OUT_MEMORY;

    /* Build SQL in pool */
    size_t sql_cap = 65536 + strlen(node_path) + strlen(edge_path) +
                     strlen(str_node_path) + strlen(str_edge_path);
    size_t n = 0;
    char *sql = mem_pool_alloc(pool, sql_cap);
    if (!sql) return JD_CALL_GRAPH_IMPORT_ERR_DUCKDB_OUT_MEMORY;

    sql_cg_pragmas(sql, sql_cap, &n);
    sql_cg_tables(sql, sql_cap, &n, node_path, edge_path);

    if (access(str_node_csv, F_OK) == 0)
            sql_cg_string_nodes(sql, sql_cap, &n, str_node_path);
    if (access(str_edge_csv, F_OK) == 0)
        sql_cg_string_edges(sql, sql_cap, &n, str_edge_path);

    sql_cg_footer(sql, sql_cap, &n);

    int rc = -1;
    char *out = run_duckdb_script(pool, db_path, sql, false, &rc);
    if (rc != 0 || !out)
        return JD_CALL_GRAPH_IMPORT_ERROR_DUCKDB_ERR;
    return JD_CALL_GRAPH_IMPORT_SUCCESS;
}

static char* tool_cg_query(jd_mcp_server *server, const char *db_path, const char *sql)
{
    mem_pool *pool = server->pool;

    if (access(db_path, F_OK) != 0)
        return str_create_in(pool, "err: database file not found");

    if (!duckdb_available())
        return str_create_in(pool, "err: duckdb not found on PATH");

    int rc = -1;
    char *out = run_duckdb_script(pool, db_path, sql, true, &rc);
    if (rc != 0)
        return process_error(pool, "duckdb query", rc, out);
    if (!out || out[0] == '\0')
        return str_create_in(pool, "(no results)");
    return out;
}

/* Strip trailing whitespace from a text file in-place.
 * Returns 0 on success, -1 on error. */
static int strip_trailing_ws(const char *path)
{
    char tmp[4096];
    snprintf(tmp, sizeof(tmp), "%s.clean", path);

    FILE *in = fopen(path, "rb");
    if (!in) return -1;
    FILE *out = fopen(tmp, "wb");
    if (!out) { fclose(in); return -1; }

    char buf[65536];
    bool changed = false;
    while (fgets(buf, sizeof(buf), in)) {
        size_t len = strlen(buf);
        while (len > 0 && (buf[len - 1] == ' ' || buf[len - 1] == '\t' ||
                           buf[len - 1] == '\n' || buf[len - 1] == '\r'))
            buf[--len] = '\0';
        if (len > 0 || buf[0] != '\0') {
            buf[len] = '\n';
            len++;
        }
        changed = true;
        if (fwrite(buf, 1, len, out) != len) {
            fclose(in); fclose(out); remove(tmp);
            return -1;
        }
    }
    fclose(in); fclose(out);
    if (changed) rename(tmp, path);
    else         remove(tmp);
    return 0;
}

/* Ensure a tab-separated exports file has a 4th (descriptor) column,
 * padding with an empty field if it only has 3. */
static int pad_exports_4cols(const char *path)
{
    char tmp[4096];
    snprintf(tmp, sizeof(tmp), "%s.pad", path);

    FILE *in = fopen(path, "rb");
    if (!in) return -1;
    FILE *out = fopen(tmp, "wb");
    if (!out) { fclose(in); return -1; }

    char buf[65536];
    bool need_pad = false;
    while (fgets(buf, sizeof(buf), in)) {
        size_t len = strlen(buf);
        /* Count tab-separated fields in this line */
        int fields = 1;
        for (size_t i = 0; i < len; i++)
            if (buf[i] == '\t') fields++;

        if (fields == 3) {
            /* 3 fields: flag, address, name — add empty descriptor */
            size_t pos = len;
            if (pos > 0 && buf[pos-1] == '\n') pos--;
            buf[pos++] = '\t';
            buf[pos++] = '\n';
            len = pos;
            need_pad = true;
        }
        if (fwrite(buf, 1, len, out) != len) {
            fclose(in); fclose(out); remove(tmp);
            return -1;
        }
    }
    fclose(in); fclose(out);
    if (need_pad) rename(tmp, path);
    else          remove(tmp);
    return 0;
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

static void format_hms(char *buf, size_t sz)
{
    time_t now = time(NULL);
    struct tm tm;
    localtime_r(&now, &tm);
    strftime(buf, sz, "%H:%M:%S", &tm);
}

static int jd_mcp_do_decompile(jd_mcp_server *server, string path, string decompile_dir)
{
    (void)server;
    const char *decompile_argv[] = {
            garlic_bin(), path, "-o", decompile_dir, NULL
    };
    return exec_process(decompile_argv, NULL, false, NULL);
}

static int jd_mcp_do_call_graph(jd_mcp_server *server, string path, string cg_dir)
{
    const char *call_graph_argv[] = {
            garlic_bin(), path, "-g", "-o", cg_dir, NULL
    };
    int rc = exec_process(call_graph_argv, NULL, false, NULL);

    string node_csv_path = str_create_in(server->pool, "%s/call_graph_node.csv", cg_dir);
    string edge_csv_path = str_create_in(server->pool, "%s/call_graph_edge.csv", cg_dir);

    if (access(node_csv_path, F_OK) != 0 || access(edge_csv_path, F_OK) != 0)
        rc = 1;

    return rc;
}

/* ------------------------------------------------------------------ *
 *  Native analysis helpers for tool_analyze
 * ------------------------------------------------------------------ */

/* Extract .so files from APK into native_dir/ (arm64-v8a only).
 * Handles both direct .so entries and nested split-APK archives. */
static void analyze_extract_native_libs(const char *path,
                                        const char *native_dir)
{
    if (!ensure_directory(native_dir))
        return;

    const char *unzip_argv[] = {
        "unzip", "-o", path,
        "lib/arm64-v8a/*.so", "-d", native_dir, NULL
    };
    if (exec_process(unzip_argv, NULL, false, NULL) == 0) {
        /* unzip creates native_dir/lib/<arch>/xxx.so — rename up */
        char lib_subdir[4096];
        snprintf(lib_subdir, sizeof(lib_subdir), "%s/lib", native_dir);
        DIR *ld = opendir(lib_subdir);
        if (ld) {
            struct dirent *le;
            char src[4096], dst[4096];
            while ((le = readdir(ld)) != NULL) {
                if (STR_EQL(le->d_name, ".") ||
                    STR_EQL(le->d_name, ".."))
                    continue;
                snprintf(src, sizeof(src), "%s/%s", lib_subdir, le->d_name);
                snprintf(dst, sizeof(dst), "%s/%s", native_dir, le->d_name);
                rename(src, dst);
            }
            closedir(ld);
        }
        rmdir(lib_subdir);
    }

    /* ---- Nested APK entries (split APK) ---- */
    const char *list_argv[] = {"unzip", "-l", path, "*.apk", NULL};
    char *list_out = NULL;
    if (exec_process(list_argv, NULL, true, &list_out) != 0 || !list_out)
        return;

    char *line = list_out;
    while (line && *line) {
        char *nl = strchr(line, '\n');
        if (nl) *nl = '\0';

        if (line[0] != '\0'
            && !strstr(line, "Archive:")
            && !strstr(line, "Length")
            && !strstr(line, "----")
            && !strstr(line, "file")) {

            char *name = line, *last_space = NULL;
            for (char *p = line; *p; p++)
                if (*p == ' ' || *p == '\t') last_space = p;
            if (last_space) {
                name = last_space + 1;
                while (*name == ' ' || *name == '\t') name++;
            }

            size_t nlen = strlen(name);
            if (nlen > 4 && name[nlen-4] == '.'
                && (name[nlen-3] == 'a' || name[nlen-3] == 'A')
                && (name[nlen-2] == 'p' || name[nlen-2] == 'P')
                && (name[nlen-1] == 'k' || name[nlen-1] == 'K')) {

                char *tmpd = jd_mcp_create_temp_dir();
                if (!tmpd) goto next_line;

                char tmpapk[4096];
                snprintf(tmpapk, sizeof(tmpapk), "%s/n.apk", tmpd);

                char pcmd[8192];
#if defined(_WIN32)
                snprintf(pcmd, sizeof(pcmd),
                    "unzip -p \"%s\" \"%s\" 2>NUL:", path, name);
#else
                snprintf(pcmd, sizeof(pcmd),
                    "unzip -p '%s' '%s' 2>/dev/null", path, name);
#endif
                FILE *pfp = popen(pcmd, "r");
                if (pfp) {
                    FILE *ofp = fopen(tmpapk, "wb");
                    if (ofp) {
                        char buf[65536];
                        size_t nr;
                        while ((nr = fread(buf, 1, sizeof(buf), pfp)) > 0)
                            fwrite(buf, 1, nr, ofp);
                        fclose(ofp);

                        const char *uav[] = {
                            "unzip", "-o", tmpapk,
                            "lib/arm64-v8a/*.so", "-d", native_dir, NULL
                        };
                        exec_process(uav, NULL, false, NULL);

                        char libd[4096];
                        snprintf(libd, sizeof(libd), "%s/lib", native_dir);
                        DIR *ld = opendir(libd);
                        if (ld) {
                            struct dirent *le;
                            char src[4096], dst[4096];
                            while ((le = readdir(ld)) != NULL) {
                                if (STR_EQL(le->d_name, ".") ||
                                    STR_EQL(le->d_name, ".."))
                                    continue;
                                snprintf(src, sizeof(src), "%s/%s",
                                         libd, le->d_name);
                                snprintf(dst, sizeof(dst), "%s/%s",
                                         native_dir, le->d_name);
                                rename(src, dst);
                            }
                            closedir(ld);
                        }
                        rmdir(libd);
                    }
                    pclose(pfp);
                }
                jd_mcp_remove_temp_dir(tmpd);
                free(tmpd);
            }
        }
        next_line:
        line = nl ? nl + 1 : NULL;
    }
    free(list_out);
}

/* Run rosemary on every .so under native_dir/<arch>/.
 * Appends per-file results to result buffer.
 * Returns count of successfully analysed .so files. */
static size_t analyze_run_rosemary(mem_pool *pool,
    char *result, size_t cap, size_t *n,
    const char *native_dir, size_t *fail_count)
{
    size_t ok = 0;
    *fail_count = 0;

    DIR *d = opendir(native_dir);
    if (!d) return 0;

    *n += snprintf(result + *n, cap - *n, "\n  Native libs:\n");

    struct dirent *entry;
    while ((entry = readdir(d)) != NULL) {
        if (STR_EQL(entry->d_name, ".") ||
            STR_EQL(entry->d_name, ".."))
            continue;
        char *arch_path = str_create_in(pool, "%s/%s",
                                        native_dir, entry->d_name);
        struct stat st;
        if (stat(arch_path, &st) != 0 || !S_ISDIR(st.st_mode))
            continue;

        DIR *arch_dir = opendir(arch_path);
        if (!arch_dir) continue;
        struct dirent *so_entry;
        while ((so_entry = readdir(arch_dir)) != NULL) {
            if (!strstr(so_entry->d_name, ".so"))
                continue;
            char *so_path = str_create_in(pool, "%s/%s",
                                          arch_path, so_entry->d_name);

#ifndef _WIN32
            time_t so_start = time(NULL);
            pid_t so_pid = fork();
            if (so_pid == 0) {
                jd_elf *elf = jd_analysis_elf_from_path(so_path);
                if (elf) { jd_dump_all_csv(elf); _exit(0); }
                _exit(1);
            } else if (so_pid > 0) {
                int wstatus;
                waitpid(so_pid, &wstatus, 0);
                int so_sec = (int)difftime(time(NULL), so_start);
                if (WIFEXITED(wstatus) && WEXITSTATUS(wstatus) == 0) {
                    ok++;
                    *n += snprintf(result + *n, cap - *n,
                        "    %s/%s  [%ds]\n",
                        entry->d_name, so_entry->d_name, so_sec);
                } else {
                    (*fail_count)++;
                    const char *why = WIFSIGNALED(wstatus)
                        ? " (crashed)" : " (analysis failed)";
                    *n += snprintf(result + *n, cap - *n,
                        "    %s/%s%s  [%ds]\n",
                        entry->d_name, so_entry->d_name, why, so_sec);
                }
            } else {
                jd_elf *elf = jd_analysis_elf_from_path(so_path);
                if (elf) { jd_dump_all_csv(elf); ok++; }
                else (*fail_count)++;
            }
#else
            jd_elf *elf = jd_analysis_elf_from_path(so_path);
            if (elf) { jd_dump_all_csv(elf); ok++; }
            else (*fail_count)++;
#endif
        }
        closedir(arch_dir);
    }
    closedir(d);
    return ok;
}

/* Build DuckDB SQL for native tables, import analysis files,
 * parse the row counts and append a summary to the result buffer. */
static void analyze_import_native(mem_pool *pool,
    char *result, size_t cap, size_t *n,
    const char *native_dir, const char *db_path, size_t native_ok)
{
    (void)native_ok;
    time_t t_start = time(NULL);
    *n += snprintf(result + *n, cap - *n,
        "\n  Native DuckDB import:\n");

    size_t sql_cap = 1048576, sql_len = 0;
    char *sql = mem_pool_alloc(pool, sql_cap);
    if (!sql) {
        *n += snprintf(result + *n, cap - *n,
            "    (out of memory for SQL buffer)\n");
        return;
    }
    sql_native_header(sql, sql_cap, &sql_len);

    DIR *dd = opendir(native_dir);
    if (dd) {
        struct dirent *de;
        while ((de = readdir(dd)) != NULL) {
            if (STR_EQL(de->d_name, ".") ||
                STR_EQL(de->d_name, ".."))
                continue;
            char *ap = str_create_in(pool, "%s/%s", native_dir, de->d_name);
            struct stat st;
            if (stat(ap, &st) != 0 || !S_ISDIR(st.st_mode))
                continue;

            DIR *ad = opendir(ap);
            if (!ad) continue;
            struct dirent *se;
            while ((se = readdir(ad)) != NULL) {
                if (!strstr(se->d_name, ".so"))
                    continue;
                char *fe = str_create_in(pool, "%s/%s.exports", ap, se->d_name);
                if (access(fe, F_OK) != 0) continue;
                strip_trailing_ws(fe);
                pad_exports_4cols(fe);
                sql_native_insert_exports(sql, sql_cap, &sql_len, se->d_name, fe);

                char *fen = str_create_in(pool, "%s/%s.entries", ap, se->d_name);
                if (access(fen, F_OK) == 0) {
                    strip_trailing_ws(fen);
                    sql_native_insert_entries(sql, sql_cap, &sql_len, se->d_name, fen);
                }

                char *ffx = str_create_in(pool, "%s/%s.func_xref", ap, se->d_name);
                if (access(ffx, F_OK) == 0) {
                    strip_trailing_ws(ffx);
                    sql_native_insert_func_xref(sql, sql_cap, &sql_len, se->d_name, ffx);
                }

                char *fim = str_create_in(pool, "%s/%s.imports", ap, se->d_name);
                if (access(fim, F_OK) == 0) {
                    strip_trailing_ws(fim);
                    sql_native_insert_imports(sql, sql_cap, &sql_len, se->d_name, fim);
                }

                char *fpc = str_create_in(pool, "%s/%s.pc_xrefs", ap, se->d_name);
                if (access(fpc, F_OK) == 0) {
                    strip_trailing_ws(fpc);
                    sql_native_insert_pc_xrefs(sql, sql_cap, &sql_len, se->d_name, fpc);
                }

                char *fst = str_create_in(pool, "%s/%s.strings", ap, se->d_name);
                if (access(fst, F_OK) == 0) {
                    strip_trailing_ws(fst);
                    sql_native_insert_string_nodes(sql, sql_cap, &sql_len, fst);
                }
            }
            closedir(ad);
        }
        closedir(dd);
    }

    sql_native_footer(sql, sql_cap, &sql_len);

    int import_rc;
    char *import_out = run_duckdb_script(pool,
        db_path, sql, false, &import_rc);
    if (import_out) {
        char *line = import_out, *nl;
        while ((nl = strchr(line, '\n')) != NULL) {
            *nl = '\0';
            if (!strstr(line, "cnt") && !strstr(line, "---")) {
                char tbl[64]; long cnt;
                if (sscanf(line, "%63[^,],%ld", tbl, &cnt) == 2)
                    *n += snprintf(result + *n, cap - *n,
                        "    %-12s %ld\n", tbl, cnt);
            }
            line = nl + 1;
        }
    }

    /* Count native-specific strings */
    int rc;
    char *cnt_out = run_duckdb_script(pool, db_path,
        "SELECT COUNT(*) AS cnt FROM string_nodes"
        " WHERE is_so_name OR is_internal_class_desc",
        true, &rc);
    if (cnt_out) {
        char *cl = cnt_out;
        while (*cl && *cl != '\n' && (*cl < '0' || *cl > '9'))
            cl++;
        if (*cl && strchr(cl, '\n'))
            *strchr(cl, '\n') = '\0';
        *n += snprintf(result + *n, cap - *n,
            "    %-12s %s\n", "strings", *cl ? cl : "0");
    }

    int elapsed = (int)difftime(time(NULL), t_start);
    *n += snprintf(result + *n, cap - *n,
        "    --- native import took %ds ---\n", elapsed);
}

static char* tool_analyze(jd_mcp_server *server, string path, string out_dir)
{
    mem_pool *pool = server->pool;
    time_t t_total_start = time(NULL);

    if (!ensure_directory(out_dir))
        return str_create_in(pool, "err: cannot create analysis output directory");

    string decompile_dir = str_create_in(pool, "%s/decompiled", out_dir);
    string cg_dir = str_create_in(pool, "%s/cg", out_dir);
    string db_path = str_create_in(pool, "%s/analysis.duckdb", out_dir);

    if (!ensure_directory(decompile_dir) || !ensure_directory(cg_dir))
        return str_create_in(pool, "err: cannot create analysis subdirectories");

    /* Phase 1 — Decompile */
    time_t t_deco_start = time(NULL);
    int rc = jd_mcp_do_decompile(server, path, decompile_dir);
    int deco_sec = (int)difftime(time(NULL), t_deco_start);
    if (rc != 0)
        return str_create_in(pool, "err: decompilation failed (exit=%d)", rc);

    /* Phase 2 — Call graph */
    time_t t_cg_start = time(NULL);
    rc = jd_mcp_do_call_graph(server, path, cg_dir);
    int cg_sec = (int)difftime(time(NULL), t_cg_start);
    if (rc != 0)
        return str_create_in(pool, "err: call graph creation failed (exit=%d)", rc);

    /* Phase 3 — DuckDB import (Java CG) */
    time_t t_db_start = time(NULL);
    int import_result = jd_mcp_cg_import(server, cg_dir, db_path);
    int db_sec = (int)difftime(time(NULL), t_db_start);
    if (import_result != JD_CALL_GRAPH_IMPORT_SUCCESS)
        return str_create_in(pool, "err: duckdb import failed (code=%d)", import_result);

    /* Build result buffer in pool */
    size_t cap = 131072;
    size_t n = 0;
    char *result = mem_pool_alloc(pool, cap);
    if (!result)
        return str_create_in(pool, "err: out of memory");

    string node_csv = str_create_in(pool, "%s/call_graph_node.csv", cg_dir);
    string edge_csv = str_create_in(pool, "%s/call_graph_edge.csv", cg_dir);

    n += snprintf(result + n, cap - n,
        "Analysis complete for: %s\n\n"
        "  Decompiled:  %s/  [%ds]\n"
        "  Call graph:  %s/  [%ds]\n"
        "  DuckDB:      %s   [%ds]\n\n",
        path, decompile_dir, deco_sec,
        cg_dir, cg_sec, db_path, db_sec);

    size_t java_files = count_files_with_suffix(decompile_dir, ".java");
    size_t node_lines = count_file_lines(node_csv);
    size_t edge_lines = count_file_lines(edge_csv);
    size_t node_count = node_lines > 0 ? node_lines - 1 : 0;
    size_t edge_count = edge_lines > 0 ? edge_lines - 1 : 0;

    n += snprintf(result + n, cap - n,
        "  Java files:  %zu\n"
        "  CG nodes:    %zu\n"
        "  CG edges:    %zu\n",
        java_files, node_count, edge_count);

    /* Native library analysis (APK only) */
    if (jd_mcp_detect_file_type(path) == JD_MCP_FILE_APK) {
        char native_dir[4096];
        snprintf(native_dir, sizeof(native_dir), "%s/native_libs", out_dir);

        analyze_extract_native_libs(path, native_dir);

        size_t native_fail = 0;
        size_t native_ok = analyze_run_rosemary(pool,
            result, cap, &n, native_dir, &native_fail);

        n += snprintf(result + n, cap - n,
            "\n  Native summary:  %zu OK, %zu FAIL\n",
            native_ok, native_fail);

        if (native_ok > 0 && access(db_path, F_OK) == 0)
            analyze_import_native(pool,
                result, cap, &n, native_dir, db_path, native_ok);
    }

    int total_sec = (int)difftime(time(NULL), t_total_start);
    n += snprintf(result + n, cap - n,
        "\n--- Total time: %ds ---\n"
        "Ready: cg_query(db_path=\"%s\", sql=\"...\")\n\n",
        total_sec, db_path);

    /* Auto-generate markdown report into <out_dir>/report/ */
    char *rpt_dir = str_create_in(pool, "%s/report", out_dir);
    char *rpt_msg = report_generate(pool, out_dir, rpt_dir);
    if (rpt_msg) {
        n += snprintf(result + n, cap - n, "%s\n", rpt_msg);
    }

    return result;
}

static char* duckdb_query(mem_pool *pool, const char *db_path, const char *sql)
{
    return run_duckdb_script(pool, db_path, sql, true, NULL);
}

static char* tool_trace_flow(jd_mcp_server *server, const char *db_path, const char *keyword,
                              const char *native_dir, int depth)
{
    mem_pool *pool = server->pool;

    if (access(db_path, F_OK) != 0)
        return str_create_in(pool, "err: database file not found");
    if (!duckdb_available())
        return str_create_in(pool, "err: duckdb not found on PATH");
    if (depth <= 0) depth = 2;
    if (depth > 5)  depth = 5;

    /* Escape single-quotes */
    char safe_kw[2048];
    size_t sk = 0;
    for (const char *s = keyword; *s && sk < sizeof(safe_kw) - 2; s++) {
        if (*s == '\'') safe_kw[sk++] = '\'';
        safe_kw[sk++] = *s;
    }
    safe_kw[sk] = '\0';

    /* Output buffer */
    size_t cap = 65536;
    char *out = mem_pool_alloc(pool, cap);
    if (!out) return str_create_in(pool, "err: out of memory");
    size_t n = 0;

    n += snprintf(out + n, cap - n,
        "=== Trace flow for: \"%s\" ===\n\n", keyword);

    /* Phase 1 — Java methods matching the keyword */
    n += snprintf(out + n, cap - n,
        "--- Java methods (keyword match) ---\n");
    char *sql = str_create_in(pool,
        "SELECT node_id, method_raw, node_type, api_type "
        "FROM java_cg_nodes WHERE method_raw LIKE '%%%s%%' "
        "ORDER BY node_type, node_id LIMIT 50", safe_kw);
    char *java_rows = duckdb_query(pool, db_path, sql);
    if (!java_rows || strstr(java_rows, "0 rows") == java_rows) {
        n += snprintf(out + n, cap - n,
            "  No Java methods matching \"%s\" found.\n", keyword);
        return out;
    }

    struct { int64_t id; char raw[1024]; int ntype; int atype; } jids[50];
    int jcount = 0;
    char *line = java_rows, *nl;
    bool skip_hdr = true;
    while ((nl = strchr(line, '\n')) != NULL && jcount < 50) {
        *nl = '\0';
        if (skip_hdr) { skip_hdr = false; line = nl + 1; continue; }
        int64_t nid = 0; char mraw[1024] = ""; int nt = 0, at = 0;
        if (sscanf(line, "%lld,\"%1023[^\"]\",%d,%d",
                   (long long*)&nid, mraw, &nt, &at) >= 3) {
            size_t ml = strlen(mraw);
            if (ml > 0 && mraw[ml-1] == '"') mraw[ml-1] = '\0';
        } else {
            sscanf(line, "%lld,%1023[^,],%d,%d",
                   (long long*)&nid, mraw, &nt, &at);
        }
        jids[jcount].id = nid;
        strncpy(jids[jcount].raw, mraw, sizeof(jids[jcount].raw) - 1);
        jids[jcount].ntype = nt;
        jids[jcount].atype = at;
        jcount++;
        line = nl + 1;
    }

    for (int i = 0; i < jcount; i++) {
        bool is_native = (jids[i].ntype > 0 && jids[i].ntype < 10) ||
                          jids[i].atype == 3;
        n += snprintf(out + n, cap - n,
            "  [%lld] %s%s\n",
            (long long)jids[i].id, jids[i].raw,
            is_native ? "  ← NATIVE" : "");
    }
    n += snprintf(out + n, cap - n, "  (%d methods, depth=%d)\n\n", jcount, depth);

    /* Phase 2 — Callee chains for top matches */
    int show = jcount < 8 ? jcount : 8;
    for (int i = 0; i < show; i++) {
        n += snprintf(out + n, cap - n,
            "--- Callee chain: %s ---\n", jids[i].raw);

        int64_t cur_id = jids[i].id;
        bool is_native_chain = false;
        for (int d = 0; d < depth && !is_native_chain; d++) {
            sql = str_create_in(pool,
                "SELECT n.node_id, n.method_raw, n.node_type, n.api_type "
                "FROM java_cg_edges e JOIN java_cg_nodes n "
                "ON e.dst_id = n.node_id "
                "WHERE e.src_id = %lld ORDER BY e.dst_id LIMIT 20",
                (long long)cur_id);

            char *callees = duckdb_query(pool, db_path, sql);
            if (!callees) continue;

            char *cl = callees, *cnl;
            bool cskip = true;
            int cc = 0;
            int64_t next_id = 0;
            bool found_next = false;
            while ((cnl = strchr(cl, '\n')) != NULL && cc < 20) {
                *cnl = '\0';
                if (cskip) { cskip = false; cl = cnl + 1; continue; }
                int64_t cnid = 0; char craw[1024] = "";
                int cnt = 0, cat = 0;
                if (sscanf(cl, "%lld,\"%1023[^\"]\",%d,%d",
                           (long long*)&cnid, craw, &cnt, &cat) >= 3) {
                    size_t mrl = strlen(craw);
                    if (mrl > 0 && craw[mrl-1] == '"') craw[mrl-1] = '\0';
                } else {
                    sscanf(cl, "%lld,%1023[^,],%d,%d",
                           (long long*)&cnid, craw, &cnt, &cat);
                }
                bool cnative = (cnt > 0 && cnt < 10) || cat == 3;
                n += snprintf(out + n, cap - n,
                    "    %*s→ [%lld] %s%s\n",
                    d * 2, "", (long long)cnid, craw,
                    cnative ? "  ← NATIVE" : "");

                if (cnative) {
                    char nm_kw[512];
                    char *last_d = strrchr(craw, '.');
                    if (last_d) {
                        char *prev_d = last_d;
                        while (prev_d > craw && *(prev_d-1) != '.') prev_d--;
                        snprintf(nm_kw, sizeof(nm_kw), "%.*s",
                                 (int)(last_d - prev_d), prev_d);
                    } else {
                        snprintf(nm_kw, sizeof(nm_kw), "%s", craw);
                    }

                    char *ns = str_create_in(pool,
                        "SELECT so_name, printf('%%llx',address), name "
                        "FROM native_exports "
                        "WHERE name LIKE '%%%s%%' LIMIT 10", nm_kw);
                    char *nexp = duckdb_query(pool, db_path, ns);
                    if (nexp) {
                        char *el = nexp, *enl;
                        bool eskip = true;
                        while ((enl = strchr(el, '\n')) != NULL) {
                            *enl = '\0';
                            if (eskip) { eskip = false; el = enl + 1; continue; }
                            char so[256], adr[32], fn[512];
                            if (sscanf(el, "%255[^,],%31[^,],%511[^\n]",
                                       so, adr, fn) >= 2)
                                n += snprintf(out + n, cap - n,
                                    "        → NATIVE: %s @ %s  %s\n",
                                    so, adr, fn);
                            el = enl + 1;
                        }
                    }
                    is_native_chain = true;
                }

                if (!found_next) { next_id = cnid; found_next = true; }
                cc++;
                cl = cnl + 1;
            }
            if (found_next) cur_id = next_id;
            else break;
        }
        n += snprintf(out + n, cap - n, "\n");
        if (n > cap - 4096) break;
    }

    /* Phase 3 — Disassembly snippets */
    if (native_dir && access(native_dir, F_OK) == 0) {
        n += snprintf(out + n, cap - n,
            "--- Disassembly (grep from .dissembly) ---\n");

        const char *short_kw = strrchr(keyword, '.');
        if (!short_kw) short_kw = keyword;
        else short_kw++;

        DIR *native_d = opendir(native_dir);
        if (native_d) {
            struct dirent *dent;
            int gcnt = 0;
            while ((dent = readdir(native_d)) != NULL && gcnt < 3) {
                size_t dnlen = strlen(dent->d_name);
                if (dnlen <= 10) continue;
                if (strcmp(dent->d_name + dnlen - 10, ".dissembly") != 0)
                    continue;

                char dpath[4096];
                snprintf(dpath, sizeof(dpath), "%s/%s",
                         native_dir, dent->d_name);

                /* First pass: check if keyword appears */
                FILE *df = fopen(dpath, "r");
                if (!df) continue;
                bool found = false;
                char lbuf[4096];
                while (fgets(lbuf, sizeof(lbuf), df)) {
                    if (strstr(lbuf, short_kw)) { found = true; break; }
                }
                fclose(df);
                if (!found) continue;

                /* Second pass: extract matching lines */
                FILE *g2f = fopen(dpath, "r");
                if (g2f) {
                    char *so_name = strrchr(dpath, '/');
                    if (!so_name) so_name = dpath;
                    else so_name++;
                    char *dot = strstr(so_name, ".so.");
                    if (dot) *dot = '\0';
                    n += snprintf(out + n, cap - n,
                        "  --- %s ---\n", so_name);
                    char gline[2048];
                    int glc = 0, lineno = 0;
                    while (fgets(gline, sizeof(gline), g2f) && glc < 20) {
                        lineno++;
                        if (strstr(gline, short_kw)) {
                            size_t gl = strlen(gline);
                            if (gl > 0 && gline[gl-1] == '\n') gline[gl-1] = '\0';
                            n += snprintf(out + n, cap - n,
                                "    %d: %s\n", lineno, gline);
                            glc++;
                        }
                    }
                    fclose(g2f);
                    gcnt++;
                }
            }
            closedir(native_d);
        }
        n += snprintf(out + n, cap - n, "\n");
    }

    n += snprintf(out + n, cap - n,
        "Use cg_query for deeper exploration:\n"
        "  SELECT * FROM java_cg_edges WHERE src_id=<id>\n"
        "  SELECT * FROM native_func_xref WHERE so_name='<so>' AND ...\n");
    return out;
}

static char* tool_native_method_find(jd_mcp_server *server, const char *db_path, const char *name)
{
    mem_pool *pool = server->pool;

    if (access(db_path, F_OK) != 0)
        return str_create_in(pool, "err: database file not found");

    if (!duckdb_available())
        return str_create_in(pool, "err: duckdb not found on PATH");

    /* Escape single-quotes in the search string for SQL safety */
    char safe_name[2048];
    size_t sn_len = 0;
    for (const char *s = name; *s && sn_len < sizeof(safe_name) - 2; s++) {
        if (*s == '\'') safe_name[sn_len++] = '\'';
        safe_name[sn_len++] = *s;
    }
    safe_name[sn_len] = '\0';

    /* Convert Java dot-notation to JNI prefix */
    char jni_prefix[2048] = "Java_";
    size_t jp = 5;
    const char *last_dot = strrchr(name, '.');
    size_t pkg_len = last_dot ? (size_t)(last_dot - name) : strlen(name);
    for (size_t i = 0; i < pkg_len && jp < sizeof(jni_prefix) - 2; i++) {
        char ch = name[i];
        if (ch == '.')      jni_prefix[jp++] = '_';
        else if (ch == '/') jni_prefix[jp++] = '_';
        else                jni_prefix[jp++] = ch;
    }
    if (last_dot) jni_prefix[jp++] = '_';
    jni_prefix[jp] = '\0';

    /* SQL query */
    char *sql = str_create_in(pool,
        "SELECT so_name, printf('%%llx', address) AS addr_hex, name, descriptor "
        "FROM native_exports "
        "WHERE name LIKE '%s%%' OR name LIKE '%%%s%%' "
        "ORDER BY so_name, name LIMIT 200",
        jni_prefix, safe_name);

    char *exports = run_duckdb_script(pool, db_path, sql, true, NULL);
    if (!exports || exports[0] == '\0')
        return str_create_in(pool,
            "err: no matching exports found (does native_exports exist?)");

    /* Build result in pool */
    size_t rcap = 65536 + strlen(exports), rn = 0;
    char *result = mem_pool_alloc(pool, rcap);
    if (!result) return str_create_in(pool, "err: out of memory");

    rn += snprintf(result + rn, rcap - rn,
        "=== Native method search for: %s ===\nJNI prefix: %s\n\n",
        name, jni_prefix);

    int match_count = 0;
    char *line = exports, *nl;
    bool first = true;
    char prev_so[256] = "";

    while ((nl = strchr(line, '\n')) != NULL) {
        *nl = '\0';
        if (strstr(line, "so_name,addr_hex")) { line = nl + 1; continue; }
        char soname[256], addrstr[32], fname[1024], desc[256];
        int f = sscanf(line, "%255[^,],%31[^,],%1023[^,],%255[^\n]",
                       soname, addrstr, fname, desc);
        if (f >= 3) {
            if (strcmp(soname, prev_so) != 0) {
                if (!first)
                    rn += snprintf(result + rn, rcap - rn, "\n");
                rn += snprintf(result + rn, rcap - rn,
                    "  -- %s --\n", soname);
                strncpy(prev_so, soname, sizeof(prev_so) - 1);
                prev_so[sizeof(prev_so) - 1] = '\0';
            }
            rn += snprintf(result + rn, rcap - rn,
                "    0x%s  %s\n", addrstr, fname);
            match_count++;
            first = false;
        }
        line = nl + 1;
    }

    if (match_count == 0)
        rn += snprintf(result + rn, rcap - rn,
            "No matching exports found.\n\n"
            "Try a shorter name fragment or the raw JNI name.\n");
    else
        rn += snprintf(result + rn, rcap - rn,
            "\n--- %d match(es) ---\n"
            "Explore with cg_query:\n"
            "  SELECT * FROM native_func_xref WHERE so_name='<so>' ...\n"
            "  SELECT * FROM native_entries WHERE so_name='<so>' AND address=0x<addr>\n",
            match_count);

    return result;
}

static char* tool_android_manifest(jd_mcp_server *server, const char *output_dir)
{
    mem_pool *pool = server->pool;
    char manifest_path[4096];
    snprintf(manifest_path, sizeof(manifest_path), "%s/AndroidManifest.xml", output_dir);

    if (access(manifest_path, F_OK) != 0)
        return str_create_in(pool,
            "Error: AndroidManifest.xml not found in the specified output directory. "
            "Make sure you have decompiled an APK first.");

    char *raw = jd_mcp_read_file(manifest_path);
    char *result = str_create_in(pool, "%s", raw ? raw : "");
    free(raw);
    return result;
}

static string tool_analyze_elf(jd_mcp_server *server, const char *path)
{
    mem_pool *pool = server->pool;
    jd_elf *elf = jd_analysis_elf_from_path(path);
    if (!elf)
        return str_create_in(pool,
            "Error: ELF analysis failed for: %s\n"
            "(file may not be a valid ELF/Mach-O binary, "
            "or the embedded rosemary library could not be loaded)",
            path);

    jd_dump_all_csv(elf);

    static const char *suffixes[] = {
        ".exports", ".func_xref", ".dissembly", ".entries",
        ".imports", ".strings", ".pc_xrefs", ".cfg_nodes", ".cfg_edges"
    };

    size_t cap = 4096, len = 0;
    char *result = mem_pool_alloc(pool, cap);
    if (!result) return str_create_in(pool, "err: out of memory");

    len += snprintf(result + len, cap - len,
        "ELF analysis complete for: %s\n\n  Generated files:\n", path);

    int file_count = 0;
    for (size_t i = 0; i < sizeof(suffixes)/sizeof(suffixes[0]); i++) {
        char out_path[4096];
        snprintf(out_path, sizeof(out_path), "%s%s", path, suffixes[i]);
        if (access(out_path, F_OK) == 0) {
            len += snprintf(result + len, cap - len,
                "    %s%s\n", path, suffixes[i]);
            file_count++;
        }
    }

    len += snprintf(result + len, cap - len,
        "\n  (%d file(s) generated)\n", file_count);

    return result;
}

void mcp_handle_tools_call(jd_mcp_server *server, unsigned id,
                           const char *tool_name, cJSON *args)
{
    bool from_pool = false;  /* all outputs now pool-allocated */

    if (!args || !cJSON_IsObject(args)) {
        jd_mcp_send_error(id, JD_MCP_ERROR_INVALID_PARAMS,
                          "tools/call requires 'arguments' object");
        return;
    }

    char *output = NULL;

    if (STR_EQL(tool_name, "decompile") ||
        STR_EQL(tool_name, "dump_info") ||
        STR_EQL(tool_name, "call_graph") ||
        STR_EQL(tool_name, "analyze") ||
        STR_EQL(tool_name, "analyze_elf"))
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
            char *errmsg = str_create_in(server->pool,
                "File not found: %s", file_path);
            jd_mcp_send_error(id, JD_MCP_ERROR_INVALID_PARAMS, errmsg);
            mem_pool_free(server->pool);
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

        if (STR_EQL(tool_name, "decompile"))
            output = tool_decompile(server, file_path, output_dir);
        else if (STR_EQL(tool_name, "dump_info"))
            output = tool_dump_info(server, file_path);
        else if (STR_EQL(tool_name, "call_graph"))
            output = tool_call_graph(server, file_path, output_dir);
        else if (STR_EQL(tool_name, "analyze_elf"))
            output = tool_analyze_elf(server, file_path);
        else
            output = tool_analyze(server, file_path, output_dir);
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
        int cg_rc = jd_mcp_cg_import(server, cg_dir_json->valuestring,
                                      db_json->valuestring);
        output = str_create_in(server->pool, "cg_import %s (code=%d)",
                               cg_rc == JD_CALL_GRAPH_IMPORT_SUCCESS ? "ok" : "FAILED",
                               cg_rc);
    } else if (STR_EQL(tool_name, "cg_query")) {
        cJSON *db_json = cJSON_GetObjectItem(args, "db_path");
        cJSON *sql_json = cJSON_GetObjectItem(args, "sql");
        if (!db_json || !cJSON_IsString(db_json) ||
            !sql_json || !cJSON_IsString(sql_json)) {
            jd_mcp_send_error(id, JD_MCP_ERROR_INVALID_PARAMS,
                              "cg_query requires 'db_path' and 'sql' (strings)");
            return;
        }
        output = tool_cg_query(server, db_json->valuestring, sql_json->valuestring);
    } else if (STR_EQL(tool_name, "trace_flow")) {
        cJSON *db_json = cJSON_GetObjectItem(args, "db_path");
        cJSON *kw_json = cJSON_GetObjectItem(args, "keyword");
        cJSON *nd_json = cJSON_GetObjectItem(args, "native_dir");
        cJSON *dp_json = cJSON_GetObjectItem(args, "depth");
        if (!db_json || !cJSON_IsString(db_json) ||
            !kw_json || !cJSON_IsString(kw_json) ||
            strlen(kw_json->valuestring) == 0) {
            jd_mcp_send_error(id, JD_MCP_ERROR_INVALID_PARAMS,
                "trace_flow requires 'db_path' and 'keyword' (strings)");
            return;
        }
        const char *nd = (nd_json && cJSON_IsString(nd_json))
                          ? nd_json->valuestring : NULL;
        int dp = (dp_json && cJSON_IsNumber(dp_json))
                  ? dp_json->valueint : 2;
        output = tool_trace_flow(server, db_json->valuestring,
                                  kw_json->valuestring, nd, dp);
    } else if (STR_EQL(tool_name, "native_method_find")) {
        cJSON *db_json = cJSON_GetObjectItem(args, "db_path");
        cJSON *name_json = cJSON_GetObjectItem(args, "name");
        if (!db_json || !cJSON_IsString(db_json) ||
            !name_json || !cJSON_IsString(name_json) ||
            strlen(name_json->valuestring) == 0) {
            jd_mcp_send_error(id, JD_MCP_ERROR_INVALID_PARAMS,
                              "native_method_find requires 'db_path' and 'name' (strings)");
            return;
        }
        output = tool_native_method_find(server, db_json->valuestring,
                                          name_json->valuestring);
    } else if (STR_EQL(tool_name, "android_manifest")) {
        cJSON *outdir_json = cJSON_GetObjectItem(args, "output_dir");
        if (!outdir_json || !cJSON_IsString(outdir_json) ||
            strlen(outdir_json->valuestring) == 0) {
            jd_mcp_send_error(id, JD_MCP_ERROR_INVALID_PARAMS,
                              "android_manifest requires 'output_dir' (string)");
            return;
        }
        output = tool_android_manifest(server, outdir_json->valuestring);
    } else if (STR_EQL(tool_name, "generate_report")) {
        cJSON *adir_json = cJSON_GetObjectItem(args, "analysis_dir");
        cJSON *rdir_json = cJSON_GetObjectItem(args, "report_dir");
        if (!adir_json || !cJSON_IsString(adir_json) ||
            strlen(adir_json->valuestring) == 0) {
            jd_mcp_send_error(id, JD_MCP_ERROR_INVALID_PARAMS,
                              "generate_report requires 'analysis_dir' (string)");
            return;
        }
        const char *rdir = (rdir_json && cJSON_IsString(rdir_json))
                           ? rdir_json->valuestring : NULL;
        if (!rdir) {
            rdir = str_create_in(server->pool, "%s/report",
                                 adir_json->valuestring);
        }
        output = report_generate(server->pool, adir_json->valuestring, rdir);
    } else if (STR_EQL(tool_name, "start_report_http_server")) {
        cJSON *dir_json = cJSON_GetObjectItem(args, "directory");
        cJSON *port_json = cJSON_GetObjectItem(args, "port");
        if (!dir_json || !cJSON_IsString(dir_json) ||
            strlen(dir_json->valuestring) == 0) {
            jd_mcp_send_error(id, JD_MCP_ERROR_INVALID_PARAMS,
                              "start_report_http_server requires 'directory' (string)");
            return;
        }
        const char *dir = dir_json->valuestring;
        int port = (port_json && cJSON_IsNumber(port_json))
                   ? port_json->valueint : 8080;

#ifndef _WIN32
        pid_t pid = fork();
        if (pid == 0) {
            setsid();
            char port_str[16];
            snprintf(port_str, sizeof(port_str), "%d", port);
            execl(mcp_self_path, mcp_self_path,
                  "--serve", dir, port_str, (char *)NULL);
            _exit(1);
        } else if (pid > 0) {
            output = str_create_in(server->pool,
                "HTTP server started in background (pid=%d).\n"
                "Open http://localhost:%d/ in your browser to view reports.\n"
                "Run 'kill %d' to stop the server.",
                (int)pid, port, (int)pid);
        } else {
            output = str_create_in(server->pool,
                "err: fork failed, cannot start HTTP server");
        }
#else
        output = str_create_in(server->pool,
            "On Windows, run the HTTP server manually:\n"
            "  garlic --serve \"%s\" %d\n"
            "Then open http://localhost:%d/ in your browser.",
            dir, port, port);
#endif
    } else {
        jd_mcp_send_error(id, JD_MCP_ERROR_METHOD_NOT_FOUND,
                          "Unknown tool");
        return;
    }

    jd_mcp_send_tool_result(id, output ? output : "(no output)");
    mem_pool_free(server->pool);
}
