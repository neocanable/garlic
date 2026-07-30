#ifndef GARLIC_JD_MCP_SQL_H
#define GARLIC_JD_MCP_SQL_H

#include "mem_pool.h"
#include "str_tools.h"

/* ================================================================== *
 *  Java Call-Graph  DuckDB  Import SQL
 * ================================================================== */

static inline void sql_cg_pragmas(char *buf, size_t cap, size_t *len)
{
    *len += snprintf(buf + *len, cap - *len,
        "PRAGMA threads=4;\n"
        "PRAGMA memory_limit='4GB';\n"
        "PRAGMA preserve_insertion_order=false;\n"
        "BEGIN TRANSACTION;\n");
}

static inline void sql_cg_tables(char *buf, size_t cap, size_t *len,
                                 const char *node_csv, const char *edge_csv)
{
    *len += snprintf(buf + *len, cap - *len,
        "CREATE TABLE IF NOT EXISTS java_cg_nodes("
        "node_id BIGINT, method_raw VARCHAR, node_type BIGINT, api_type BIGINT);\n"
        "INSERT INTO java_cg_nodes "
        "SELECT CAST(id AS BIGINT), CAST(method AS VARCHAR), "
        "CAST(COALESCE(type,0) AS BIGINT), CAST(COALESCE(api_type,0) AS BIGINT) "
        "FROM read_csv_auto('%s', HEADER=TRUE, SAMPLE_SIZE=-1, ignore_errors=true);\n"
        "CREATE TABLE IF NOT EXISTS java_cg_edges("
        "src_id BIGINT, dst_id BIGINT);\n"
        "INSERT INTO java_cg_edges "
        "SELECT DISTINCT CAST(src_id AS BIGINT), CAST(dst_id AS BIGINT) "
        "FROM read_csv_auto('%s', HEADER=TRUE, SAMPLE_SIZE=-1, ignore_errors=true);\n",
        node_csv, edge_csv);
}

static inline void sql_cg_string_nodes(char *buf, size_t cap, size_t *len,
                                       const char *str_node_csv)
{
    *len += snprintf(buf + *len, cap - *len,
        "CREATE TABLE IF NOT EXISTS string_nodes("
        "id BIGINT, pc BIGINT, str VARCHAR,"
        " is_class_desc BOOLEAN, is_field_name BOOLEAN,"
        " is_method_name BOOLEAN, is_return_type BOOLEAN,"
        " is_method_param_type BOOLEAN, is_internal_class_desc BOOLEAN,"
        " is_url BOOLEAN, is_enc_dec BOOLEAN, is_uuid BOOLEAN,"
        " is_pem_key BOOLEAN, is_so_name BOOLEAN, is_ipv4 BOOLEAN);\n"
        "INSERT INTO string_nodes "
        "SELECT * FROM read_csv_auto('%s',"
        " HEADER=TRUE, SAMPLE_SIZE=-1, ignore_errors=true);\n",
        str_node_csv);
}

static inline void sql_cg_string_edges(char *buf, size_t cap, size_t *len,
                                       const char *str_edge_csv)
{
    *len += snprintf(buf + *len, cap - *len,
        "CREATE TABLE IF NOT EXISTS string_edges("
        "string_id BIGINT, method_id BIGINT);\n"
        "INSERT INTO string_edges "
        "SELECT src_id AS string_id, dst_id AS method_id "
        "FROM read_csv_auto('%s', HEADER=TRUE, SAMPLE_SIZE=-1, ignore_errors=true);\n",
        str_edge_csv);
}

static inline void sql_cg_footer(char *buf, size_t cap, size_t *len)
{
    *len += snprintf(buf + *len, cap - *len,
        "CREATE INDEX IF NOT EXISTS idx_nodes_id ON java_cg_nodes(node_id);\n"
        "CREATE INDEX IF NOT EXISTS idx_edges_src ON java_cg_edges(src_id);\n"
        "CREATE INDEX IF NOT EXISTS idx_edges_dst ON java_cg_edges(dst_id);\n"
        "COMMIT;\n"
        "SELECT 'nodes' AS tbl, COUNT(*) AS cnt FROM java_cg_nodes "
        "UNION ALL SELECT 'edges', COUNT(*) FROM java_cg_edges;\n");
}

/* ================================================================== *
 *  Native  DuckDB  Import SQL
 * ================================================================== */

static inline void sql_native_header(char *buf, size_t cap, size_t *len)
{
    *len += snprintf(buf + *len, cap - *len,
        "PRAGMA threads=4;\n"
        "BEGIN TRANSACTION;\n\n"
        "CREATE TABLE IF NOT EXISTS native_exports("
        "  so_name VARCHAR, address BIGINT, name VARCHAR, descriptor VARCHAR);\n"
        "CREATE TABLE IF NOT EXISTS native_entries("
        "  so_name VARCHAR, address BIGINT, name VARCHAR);\n"
        "CREATE TABLE IF NOT EXISTS native_func_xref("
        "  so_name VARCHAR, caller_addr BIGINT, callee_addr BIGINT);\n"
        "CREATE TABLE IF NOT EXISTS native_imports("
        "  so_name VARCHAR, address BIGINT, name VARCHAR);\n"
        "CREATE TABLE IF NOT EXISTS native_pc_xrefs("
        "  so_name VARCHAR, ref_pc BIGINT, func_pc BIGINT, inst_pc BIGINT);\n");
}

static inline void sql_native_insert_exports(char *buf, size_t cap, size_t *len,
        const char *so_name, const char *exports_path)
{
    *len += snprintf(buf + *len, cap - *len,
        "-- %s\n"
        "INSERT INTO native_exports SELECT '%s',"
        "  column2::BIGINT, column3::VARCHAR, COALESCE(column4,'')::VARCHAR"
        " FROM read_csv('%s', auto_detect=false,"
        "  columns={'column1':'VARCHAR','column2':'VARCHAR',"
        "          'column3':'VARCHAR','column4':'VARCHAR'},"
        "  delim='\\t', header=false, ignore_errors=true);\n",
        so_name, so_name, exports_path);
}

static inline void sql_native_insert_entries(char *buf, size_t cap, size_t *len,
        const char *so_name, const char *entries_path)
{
    *len += snprintf(buf + *len, cap - *len,
        "INSERT INTO native_entries SELECT '%s',"
        "  column1::BIGINT, column2::VARCHAR"
        " FROM read_csv('%s', auto_detect=false,"
        "  columns={'column1':'VARCHAR','column2':'VARCHAR'},"
        "  delim='\\t', header=false, ignore_errors=true);\n",
        so_name, entries_path);
}

static inline void sql_native_insert_func_xref(char *buf, size_t cap, size_t *len,
        const char *so_name, const char *fxref_path)
{
    *len += snprintf(buf + *len, cap - *len,
        "INSERT INTO native_func_xref SELECT '%s',"
        "  column1::BIGINT, column2::BIGINT"
        " FROM read_csv('%s', auto_detect=false,"
        "  columns={'column1':'VARCHAR','column2':'VARCHAR'},"
        "  delim='\\t', header=false, ignore_errors=true);\n",
        so_name, fxref_path);
}

static inline void sql_native_insert_imports(char *buf, size_t cap, size_t *len,
        const char *so_name, const char *imports_path)
{
    *len += snprintf(buf + *len, cap - *len,
        "INSERT INTO native_imports SELECT '%s',"
        "  column1::BIGINT, column2::VARCHAR"
        " FROM read_csv('%s', auto_detect=false,"
        "  columns={'column1':'VARCHAR','column2':'VARCHAR'},"
        "  delim=',', header=false, ignore_errors=true);\n",
        so_name, imports_path);
}

static inline void sql_native_insert_pc_xrefs(char *buf, size_t cap, size_t *len,
        const char *so_name, const char *pcxref_path)
{
    *len += snprintf(buf + *len, cap - *len,
        "INSERT INTO native_pc_xrefs SELECT '%s',"
        "  column1::BIGINT, column2::BIGINT, column3::BIGINT"
        " FROM read_csv('%s', auto_detect=false,"
        "  columns={'column1':'VARCHAR','column2':'VARCHAR','column3':'VARCHAR'},"
        "  delim=',', header=false, ignore_errors=true);\n",
        so_name, pcxref_path);
}

static inline void sql_native_insert_string_nodes(char *buf, size_t cap, size_t *len,
        const char *strings_path)
{
    *len += snprintf(buf + *len, cap - *len,
        "CREATE TABLE IF NOT EXISTS string_nodes("
        "id BIGINT, pc BIGINT, str VARCHAR,"
        " is_class_desc BOOLEAN, is_field_name BOOLEAN,"
        " is_method_name BOOLEAN, is_return_type BOOLEAN,"
        " is_method_param_type BOOLEAN, is_internal_class_desc BOOLEAN,"
        " is_url BOOLEAN, is_enc_dec BOOLEAN, is_uuid BOOLEAN,"
        " is_pem_key BOOLEAN, is_so_name BOOLEAN, is_ipv4 BOOLEAN);\n"
        "INSERT INTO string_nodes "
        "SELECT * FROM read_csv_auto('%s', HEADER=TRUE,"
        " SAMPLE_SIZE=-1, ignore_errors=true);\n",
        strings_path);
}

static inline void sql_native_footer(char *buf, size_t cap, size_t *len)
{
    *len += snprintf(buf + *len, cap - *len,
        "CREATE INDEX IF NOT EXISTS idx_native_exports_name"
        " ON native_exports(name);\n"
        "CREATE INDEX IF NOT EXISTS idx_native_entries_so"
        " ON native_entries(so_name, address);\n"
        "CREATE INDEX IF NOT EXISTS idx_native_func_xref_caller"
        " ON native_func_xref(so_name, caller_addr);\n"
        "CREATE INDEX IF NOT EXISTS idx_native_func_xref_callee"
        " ON native_func_xref(so_name, callee_addr);\n"
        "CREATE INDEX IF NOT EXISTS idx_native_imports_name"
        " ON native_imports(so_name, name);\n"
        "CREATE INDEX IF NOT EXISTS idx_native_pc_xrefs_ref"
        " ON native_pc_xrefs(so_name, ref_pc);\n"
        "COMMIT;\n"
        "SELECT 'exports' AS tbl, COUNT(*) AS cnt FROM native_exports"
        " UNION ALL SELECT 'entries', COUNT(*) FROM native_entries"
        " UNION ALL SELECT 'func_xref', COUNT(*) FROM native_func_xref"
        " UNION ALL SELECT 'imports', COUNT(*) FROM native_imports;\n");
}

#endif /* GARLIC_JD_MCP_SQL_H */
