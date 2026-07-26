# Garlic MCP Server

English | [中文](MCP-SERVER.CN.md)

[Garlic](https://github.com/neocanable/garlic) — the world's fastest APK/Java decompiler — also speaks the **Model Context Protocol (MCP)**.

By running `garlic -m`, it starts an MCP server over stdio that exposes decompilation, class inspection, call graph analysis, and DuckDB-powered SQL querying to any MCP-compatible AI client (Claude Desktop, Cline, Continue, etc.).

---

## Prerequisites

- **Garlic** v1.6 or later (built with MCP support)
- **DuckDB** CLI — **required**. The MCP server uses DuckDB for all analysis features:
  ```sh
  https://duckdb.org/install/?platform=macos&environment=cli
  ```
  > Without DuckDB, `analyze`, `cg_import` and `cg_query` tools will fail.

---

## Installation

### Download from GitHub Releases

Download the latest binary for your platform from the [GitHub Releases page](https://github.com/neocanable/garlic/releases). Choose the archive matching your OS and architecture (macOS, Linux, Windows; x86_64 or aarch64).

Extract and place the `garlic` binary somewhere in your `PATH`, for example:

```sh
# macOS / Linux
tar xzf garlic-*.tar.gz
sudo mv garlic /usr/local/bin/
```

```sh
git clone https://github.com/neocanable/garlic.git
cd garlic
# Build with CMake
cmake -B build
cmake --build build

# Or with Zig (cross-platform)
zig build --release=fast
```

Verify it works:
```sh
./build/garlic -m
# (no output means it started successfully — it's waiting for JSON-RPC messages on stdin)
```

---

## Configuration

### Claude Desktop

Add to your `claude_desktop_config.json` (`~/Library/Application Support/Claude/claude_desktop_config.json` on macOS):

```json
{
  "mcpServers": {
    "garlic": {
      "command": "garlic",
      "args": ["-m"]
    }
  }
}
```

If Garlic is not in your system `PATH`, use the full path:

```json
{
  "mcpServers": {
    "garlic": {
      "command": "/usr/local/bin/garlic",
      "args": ["-m"]
    }
  }
}
```

### Cline / Roo Code (VS Code extension)

Add in MCP server settings:

```json
{
  "mcpServers": {
    "garlic": {
      "command": "garlic",
      "args": ["-m"]
    }
  }
}
```

### Continue.dev

Add to `config.json`:

```json
{
  "experimental": {
    "mcpServers": {
      "garlic": {
        "command": "garlic",
        "args": ["-m"]
      }
    }
  }
}
```

### Direct stdio test

You can interact with the MCP server directly:

```sh
# List available tools (shows all tool names, descriptions, and JSON schemas)
echo '{"jsonrpc":"2.0","id":1,"method":"tools/list"}' | garlic -m

# Try a decompilation
echo '{"jsonrpc":"2.0","id":2,"method":"tools/call","params":{"name":"dump_info","arguments":{"path":"/path/to/Hello.class"}}}' | garlic -m
```

---

## Tools Reference

Garlic MCP provides **7 tools**. The tools are listed below in typical workflow order — **start with `analyze`** for most reverse-engineering tasks, then drill down with the other tools as needed.

### 1. `analyze`

**RECOMMENDED ENTRY POINT.** One-shot comprehensive analysis: decompile + generate call graph + import into DuckDB, all in one step.

- **Input**: `path` (string) — path to `.dex` or `.apk` file; `output_dir` (string) — working directory for all outputs.
- **Output**: Creates three items inside `output_dir`:
  - `decompiled/` — Java source files (package-directory hierarchy preserved)
  - `cg/` — call graph CSV files (`call_graph_node.csv`, `call_graph_edge.csv`)
  - `analysis.duckdb` — DuckDB database with indexed `java_cg_nodes` / `java_cg_edges` tables
- **Returns**: A summary with Java file count, CG node count, and CG edge count, plus a ready-to-use `cg_query(...)` example.
- **Dependencies**: Requires `duckdb` CLI installed on PATH.
- **Use cases**: Run this first for most reverse-engineering tasks. After it completes, use:
  - `android_manifest` to read app permissions / entry points
  - `cg_query` to explore the call graph via SQL
- **Example**:
  ```
  analyze(path="/path/to/app.apk", output_dir="./app_analysis")
  ```

### 2. `decompile`

Recover readable Java source code from compiled Java/Android binaries.

- **Input**: `path` (string) — path to `.class`, `.jar`, `.dex`, or `.apk` file.
  - Optional `output_dir` (string) — when omitted, sources are returned inline and temp files cleaned.
- **Output**: Java source files preserving package directory structure. For APK inputs, `AndroidManifest.xml` is also extracted alongside the sources.
- **Dependencies**: None (standalone — does not require DuckDB).
- **Use cases**: Use when you only need Java source recovery without call graph or SQL analysis. For full analysis, prefer `analyze` instead.
- **Example**:
  ```
  decompile(path="/path/to/app.apk", output_dir="./sources")
  ```

### 3. `dump_info`

Quick-inspect internal class/DEX file structure — methods, fields, constants, annotations, and signatures — without performing full decompilation. Similar to `javap` or `dexdump`.

- **Input**: `path` (string) — path to `.class` or `.dex` file.
- **Output**: Structured text showing class/bytecode structure.
- **Dependencies**: None.
- **Use cases**: Fast validation of file type, API-surface inspection, checking for obfuscation patterns.
- **Example**:
  ```
  dump_info(path="/path/to/classes.dex")
  ```

### 4. `call_graph`

Generate a full call graph for a `.dex` or `.apk` file.

- **Input**: `path` (string) — path to `.dex` or `.apk` file.
  - Optional `output_dir` (string) — when omitted, uses a temp directory.
- **Output**: Two (or four) CSV files:
  - `call_graph_node.csv` — each row is one method, with columns for `node_id`, `method_raw`, `node_type`, and `api_type`
  - `call_graph_edge.csv` — each row is one caller→callee edge (`src_id`, `dst_id`)
  - `string_node.csv` / `string_edge.csv` — present when string-reference analysis data is available
- **Dependencies**: None (standalone — does not require DuckDB).
- **Use cases**: Understanding method dependencies, finding entry points, tracing data flow, detecting unused code. The CSV output is designed for import into DuckDB via `cg_import`.
- **Example**:
  ```
  call_graph(path="/path/to/app.apk", output_dir="./cg_output")
  ```

### 5. `cg_import`

Import call graph CSV files into a DuckDB database with indexed tables for fast SQL analysis.

- **Input**:
  - `cg_dir` (string) — directory produced by `call_graph`, containing `call_graph_node.csv` and `call_graph_edge.csv`
  - `db_path` (string) — output path for the `.duckdb` database file
- **Output**: A DuckDB database at `db_path` containing:
  - `java_cg_nodes(node_id, method_raw, node_type, api_type)` — method metadata, indexed
  - `java_cg_edges(src_id, dst_id)` — call relationships, indexed on both ends
  - `string_nodes` / `string_edges` — created only when the corresponding CSV files exist
- **Dependencies**: Requires `duckdb` CLI installed on PATH.
- **Use cases**: Intermediate step between `call_graph` and `cg_query`. Run this after `call_graph`, then use `cg_query` to analyse the database.
- **Example**:
  ```
  cg_import(cg_dir="./cg_output", db_path="./analysis.duckdb")
  ```

### 6. `cg_query`

Run a SQL query against a call graph DuckDB database (created by `cg_import`).

- **Input**:
  - `db_path` (string) — path to `.duckdb` database file (from `cg_import`)
  - `sql` (string) — SQL query to execute
- **Output**: Query results in CSV format.
- **Dependencies**: Requires `duckdb` CLI installed on PATH.
- **Database schema**:
  - `java_cg_nodes` — `node_id BIGINT, method_raw VARCHAR, node_type BIGINT, api_type BIGINT`
  - `java_cg_edges` — `src_id BIGINT, dst_id BIGINT`
  - `string_nodes` — string constants metadata (if imported)
  - `string_edges` — string-to-method references (if imported)
- **Use cases**:
  - Find all methods called by a given method: `SELECT * FROM java_cg_edges WHERE src_id = ?`
  - Find all callers of a method: `SELECT * FROM java_cg_edges WHERE dst_id = ?`
  - Discover app entry points: `SELECT * FROM java_cg_nodes WHERE api_type > 0`
  - Find unused (zero-callee) methods: `SELECT n.* FROM java_cg_nodes n LEFT JOIN java_cg_edges e ON n.node_id = e.dst_id WHERE e.dst_id IS NULL`
  - Count API calls by type: `SELECT node_type, COUNT(*) FROM java_cg_nodes GROUP BY node_type`
- **Important**: Queries run in read-only mode — the database is never modified.
- **Example**:
  ```
  cg_query(db_path="./analysis.duckdb", sql="SELECT method_raw, node_type FROM java_cg_nodes LIMIT 20")
  ```

### 7. `android_manifest`

Read AndroidManifest.xml from a previous decompile or analyze output directory.

- **Input**: `output_dir` (string) — the directory used with `decompile` or `analyze`.
- **Output**: Full XML content of AndroidManifest.xml, including:
  - App permissions (`<uses-permission>`)
  - Activity, service, broadcast receiver, and content provider declarations
  - Intent filters and exported-component flags
- **Dependencies**: None.
- **Use cases**: After decompiling an APK, use this to understand the app's attack surface, entry points, declared permissions, and inter-component communication (ICC) surface.
- **Example**:
  ```
  android_manifest(output_dir="./app_analysis")
  ```

---

### Workflow Summary

```
analyze(path, output_dir)          ← Start here for most tasks
  ├─ decompile(path, output_dir?)   ← Or use individually
  ├─ dump_info(path)                ← Quick peek at structure
  └─ call_graph(path, output_dir?)  ← Generate call graph CSVs
       └─ cg_import(cg_dir, db_path)  ← Import into DuckDB
             └─ cg_query(db_path, sql) ← Analyse with SQL
 android_manifest(output_dir)      ← Read app permissions (after decompile/analyze)
```

---

## Troubleshooting

- `garlic -m` exits immediately — no input on stdin when used interactively. Configure it in your MCP client instead.
- Tool returns empty results — file not found or wrong path. Verify the path exists and garlic can read it.
- `cg_import` / `cg_query` fail — DuckDB not installed. Install with `brew install duckdb`.
- `duckdb: command not found` — DuckDB not in PATH. Install DuckDB or provide full path.
- `decompile` returns error — unsupported or malformed file. Check the file type (class/jar/dex/apk).
- Slow response on large APK — large file processing. Decompilation is fast (200MB APK in ~12s), but very large files may take longer.

### Debug logging

If garlic was built in debug mode (without `NDEBUG`), MCP log messages go to **stderr**:

```sh
garlic -m 2>/dev/null   # suppress
garlic -m               # show in terminal
```

---

## Protocol

- **Server name**: `garlic-mcp`
- **Version**: `1.0.0`
- **MCP Protocol**: `2024-11-05`
- **Transport**: stdio (JSON-RPC 2.0)
- **Max message size**: 1 MB

---

## License

Apache 2.0 — see [LICENSE](LICENSE).
