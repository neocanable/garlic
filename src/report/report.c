#include "report/report.h"
#include "str_tools.h"
#include "md4c-html.h"

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>

#ifdef _WIN32
  #include <windows.h>
  #include <io.h>
  #define popen  _popen
  #define pclose _pclose
  #ifndef S_ISDIR
    #define S_ISDIR(m) (((m) & _S_IFMT) == _S_IFDIR)
  #endif
#else
  #include <unistd.h>
#endif

/* ================================================================== */
/*  DuckDB query helper                                                */
/* ================================================================== */

static char *run_query(mem_pool *pool, const char *db_path, const char *sql) {
    char cmd[16384];
    snprintf(cmd, sizeof(cmd),
        "duckdb -readonly -csv '%s' -c \"%s\" 2>/dev/null", db_path, sql);
    FILE *fp = popen(cmd, "r");
    if (!fp) return NULL;
    size_t cap = 4096, len = 0;
    char *buf = mem_pool_alloc(pool, cap);
    if (!buf) { pclose(fp); return NULL; }
    buf[0] = '\0';
    char tmp[4096];
    size_t nr;
    while ((nr = fread(tmp, 1, sizeof(tmp), fp)) > 0) {
        if (len + nr + 1 > cap) {
            cap = (len + nr + 1) * 2;
            char *nb = mem_pool_realloc(pool, buf, len + nr, cap);
            if (!nb) { pclose(fp); return buf; }
            buf = nb;
        }
        memcpy(buf + len, tmp, nr);
        len += nr;
        buf[len] = '\0';
    }
    pclose(fp);
    return buf;
}

static void csv_skip_header(const char **csv) {
    if (!csv || !*csv) return;
    const char *nl = strchr(*csv, '\n');
    *csv = nl ? nl + 1 : *csv;
}

/* ================================================================== */
/*  md4c output callback                                               */
/* ================================================================== */

typedef struct {
    mem_pool *pool;
    char     *buf;
    size_t    cap;
    size_t    len;
} md_output_ctx_t;

static void md_output_cb(const MD_CHAR *text, MD_SIZE size, void *userdata) {
    md_output_ctx_t *ctx = (md_output_ctx_t *)userdata;
    if (ctx->len + size + 1 > ctx->cap) {
        size_t new_cap = (ctx->len + size + 1) * 2;
        char *nb = mem_pool_realloc(ctx->pool, ctx->buf, ctx->cap, new_cap);
        if (!nb) return;
        ctx->buf = nb;
        ctx->cap = new_cap;
    }
    memcpy(ctx->buf + ctx->len, text, size);
    ctx->len += size;
    ctx->buf[ctx->len] = '\0';
}

/* ================================================================== */
/*  File counter                                                       */
/* ================================================================== */

static size_t count_files(const char *dir, const char *suffix) {
    DIR *d = opendir(dir);
    if (!d) return 0;
    size_t n = 0;
    struct dirent *e;
    char path[4096];
    while ((e = readdir(d)) != NULL) {
        if (e->d_name[0] == '.') continue;
        snprintf(path, sizeof(path), "%s/%s", dir, e->d_name);
        struct stat st;
        if (stat(path, &st) != 0) continue;
        if (S_ISDIR(st.st_mode))
            n += count_files(path, suffix);
        else if (str_end_with(e->d_name, suffix))
            n++;
    }
    closedir(d);
    return n;
}

/* ================================================================== */
/*  AndroidManifest permission extraction                              */
/* ================================================================== */

static char *read_permissions(mem_pool *pool, const char *analysis_dir) {
    char path[4096];
    snprintf(path, sizeof(path), "%s/decompiled/AndroidManifest.xml", analysis_dir);
    FILE *fp = fopen(path, "r");
    if (!fp) return NULL;
    fseek(fp, 0, SEEK_END);
    long sz = ftell(fp);
    rewind(fp);
    char *buf = mem_pool_alloc(pool, (size_t)sz + 1);
    if (!buf) { fclose(fp); return NULL; }
    size_t n = fread(buf, 1, (size_t)sz, fp);
    fclose(fp);
    buf[n] = '\0';

    size_t rcap = 65536, rlen = 0;
    char *result = mem_pool_alloc(pool, rcap);
    char *line = buf, *nl;
    while ((nl = strchr(line, '\n')) != NULL) {
        *nl = '\0';
        const char *name = strstr(line, "android:name=\"");
        if (name && strstr(line, "uses-permission")) {
            name += 14;
            const char *end = strchr(name, '"');
            if (end) {
                size_t nlen = (size_t)(end - name);
                if (rlen + nlen + 4 > rcap) {
                    rcap = (rlen + nlen + 4) * 2;
                    result = mem_pool_realloc(pool, result, rlen + nlen + 3, rcap);
                }
                if (rlen > 0) result[rlen++] = '\n';
                memcpy(result + rlen, name, nlen);
                rlen += nlen;
                result[rlen] = '\0';
            }
        }
        line = nl + 1;
    }
    return (rlen > 0) ? result : NULL;
}

/* ================================================================== */
/*  Main report generator                                              */
/* ================================================================== */

string report_generate(mem_pool *pool, const char *analysis_dir,
                       const char *report_dir) {
    char db_path[4096], deco_dir[4096], native_dir[4096];
    snprintf(db_path, sizeof(db_path), "%s/analysis.duckdb", analysis_dir);
    snprintf(deco_dir, sizeof(deco_dir), "%s/decompiled", analysis_dir);
    snprintf(native_dir, sizeof(native_dir), "%s/native_libs", analysis_dir);

    if (access(db_path, F_OK) != 0)
        return str_create_in(pool, "err: %s not found — run analyze first", db_path);

#ifdef _WIN32
    CreateDirectoryA(report_dir, NULL);
#else
    mkdir(report_dir, 0755);
#endif

    /* ---- Queries ---- */
    const char *qs[] = {
        "SELECT COUNT(*) AS cnt FROM java_cg_nodes",
        "SELECT COUNT(*) AS cnt FROM java_cg_edges",
        "SELECT COUNT(DISTINCT so_name) AS cnt FROM native_exports",
        "SELECT COUNT(*) AS cnt FROM native_exports",
        "SELECT COUNT(*) AS cnt FROM native_func_xref",
        "SELECT COUNT(*) AS cnt FROM native_imports",
        "SELECT COUNT(*) AS cnt FROM native_entries",
        "SELECT node_type, COUNT(*) AS cnt FROM java_cg_nodes "
            "GROUP BY node_type ORDER BY node_type",
        "SELECT so_name, COUNT(*) AS n FROM native_exports "
            "GROUP BY so_name ORDER BY n DESC LIMIT 10",
        "SELECT so_name, name FROM native_exports "
            "ORDER BY so_name LIMIT 200",
        "SELECT str FROM string_nodes WHERE is_url LIMIT 50",
    };
    char *r[11];
    for (int i = 0; i < 11; i++)
        r[i] = run_query(pool, db_path, qs[i]);

    size_t n_java_files = count_files(deco_dir, ".java");
    char *permissions = read_permissions(pool, analysis_dir);

    /* ---- Build Markdown ---- */
    size_t md_cap = 524288, md_len = 0;
    char *md = mem_pool_alloc(pool, md_cap);
    if (!md) return str_create_in(pool, "err: out of memory");

    char now_str[32];
    time_t now = time(NULL);
    strftime(now_str, sizeof(now_str), "%Y-%m-%d %H:%M:%S", localtime(&now));

    #define MD(...) md_len += snprintf(md + md_len, md_cap - md_len, __VA_ARGS__)

    char *labels[] = {
        "Java Methods (CG nodes)","Call Edges","Native .so Libraries",
        "Native Export Symbols","Native Func XRefs","Native Imports","Native Entries"
    };

    MD("# Garlic Analysis Report\n\n");
    MD("**Analysis:** `%s`  \n", analysis_dir);
    MD("**Generated:** %s  \n\n", now_str);
    MD("---\n\n## Overview\n\n");
    MD("| Metric | Value |\n|--------|-------|\n");
    MD("| Java source files | %zu |\n", n_java_files);
    for (int i = 0; i < 7; i++) {
        if (r[i]) {
            const char *s = r[i];
            csv_skip_header(&s);
            char val[64];
            strncpy(val, s, sizeof(val)-1);
            val[sizeof(val)-1] = '\0';
            char *nl = strchr(val, '\n');
            if (nl) *nl = '\0';
            MD("| %s | %s |\n", labels[i], val);
        }
    }
    MD("\n");

    /* Node types */
    if (r[7]) {
        MD("### Call Graph Node Types\n\n```\n%s```\n\n", r[7]);
    }

    /* Permissions */
    MD("## Android Permissions\n\n");
    if (permissions) {
        MD("```\n%s\n```\n\n", permissions);
    } else {
        MD("*(AndroidManifest.xml not found)*\n\n");
    }

    /* Top SO */
    if (r[8]) {
        MD("## Native Libraries\n\n");
        MD("### Top .so by Export Count\n\n```\n%s```\n\n", r[8]);
    }

    /* All exports */
    if (r[9]) {
        MD("### All Native Exports (first 200)\n\n```\n%s```\n\n", r[9]);
    }

    /* URL strings */
    if (r[10]) {
        MD("## URL Endpoints (string analysis)\n\n```\n%s```\n\n", r[10]);
    }

    MD("---\n\n*Generated by [garlic](https://github.com/neocanable/garlic)*\n");
    #undef MD

    /* ---- Convert to HTML ---- */
    md_output_ctx_t ctx = { .pool = pool, .cap = 131072 };
    ctx.buf = mem_pool_alloc(pool, ctx.cap);
    if (ctx.buf) {
        ctx.buf[0] = '\0';
        md_html(md, (MD_SIZE)md_len, md_output_cb, &ctx,
                MD_FLAG_TABLES | MD_FLAG_TASKLISTS, 0);
    }

    /* ---- Write files ---- */
    char md_path[4096], html_path[4096];
    snprintf(md_path, sizeof(md_path), "%s/report.md", report_dir);
    snprintf(html_path, sizeof(html_path), "%s/report.html", report_dir);

    FILE *fp = fopen(md_path, "w");
    if (fp) { fwrite(md, 1, md_len, fp); fclose(fp); }

    fp = fopen(html_path, "w");
    if (fp) {
        fputs("<!DOCTYPE html>\n<html lang=\"en\"><head><meta charset=\"UTF-8\">\n"
              "<title>Garlic Analysis Report</title>\n"
              "<style>body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;"
              "max-width:1000px;margin:0 auto;padding:2em;background:#fff;color:#333;}"
              "table{border-collapse:collapse;width:100%;margin:1em 0}"
              "th,td{border:1px solid #ddd;padding:8px 12px;text-align:left}"
              "th{background:#f0f0f0;font-weight:600}tr:hover{background:#fafafa}"
              "code{background:#f0f0f0;padding:2px 6px;border-radius:3px}"
              "pre{background:#f8f8f8;padding:1em;border-radius:6px;overflow-x:auto;"
              "font-size:0.85em;line-height:1.5;max-height:400px;overflow-y:auto}"
              "h1{font-size:2em;border-bottom:3px solid #333;padding-bottom:.3em}"
              "h2{border-bottom:1px solid #ddd;padding-bottom:.3em;margin-top:1.8em}"
              "</style>\n</head>\n<body>\n", fp);
        if (ctx.buf) fwrite(ctx.buf, 1, ctx.len, fp);
        fputs("</body>\n</html>\n", fp);
        fclose(fp);
    }

    return str_create_in(pool,
        "Report saved to %s/\n"
        "  report.md   — Markdown\n"
        "  report.html  — HTML\n\n"
        "Overview: %zu java files, %d queries run",
        report_dir, n_java_files, 11);
}
