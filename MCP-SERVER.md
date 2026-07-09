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
# List available tools
echo '{"jsonrpc":"2.0","id":1,"method":"tools/list"}' | garlic -m
```

---

## Tools Reference

Garlic MCP provides **7 tools**:

### 1. `analyze`

One-shot analysis: decompile + generate call graph + import into DuckDB.  
This is the default tool for most use cases.

Requires `path` (string) — path to `.dex` or `.apk` file, and `output_dir` (string) — working directory. Creates `decompiled/`, `cg/`, and `analysis.duckdb` inside it.

Requires `duckdb` CLI installed.

### 2. `decompile`

Decompile a `.class` / `.jar` / `.dex` / `.apk` file to Java source code.

Requires `path` (string) — path to the target file.  
Accepts optional `output_dir` (string) — if omitted, results are returned inline and temp files are cleaned.

### 3. `dump_info`

Display class or DEX file structure (like `javap` / `dexdump`).

Requires `path` (string) — path to `.class` or `.dex` file.

### 4. `call_graph`

Generate a call graph for a `.dex` or `.apk` file.

Requires `path` (string) — path to `.dex` or `.apk` file.  
Accepts optional `output_dir` (string) — if omitted, uses a temp directory. Produces `call_graph_node.csv` and `call_graph_edge.csv`.

### 5. `cg_import`

Import call graph CSV files into a DuckDB database for SQL analysis.

Requires `cg_dir` (string) — directory containing `call_graph_node.csv` and `call_graph_edge.csv`, and `db_path` (string) — output path for the `.duckdb` database file.

Requires `duckdb` CLI installed.

### 6. `cg_query`

Run a SQL query against a call graph DuckDB database.

Requires `db_path` (string) — path to the `.duckdb` database file, and `sql` (string) — the SQL query to execute.

Requires `duckdb` CLI installed.

### 7. `android_manifest`

Read AndroidManifest.xml from a previous decompile/analyze output directory.

Requires `output_dir` (string) — the output directory used with `analyze` or `decompile`. Returns the full contents of AndroidManifest.xml.

In Claude Desktop, ask:

> "help me analysis apk at '/path/to/apk'"

Garlic MCP will decompile the APK, generate the call graph, and import everything into DuckDB for further analysis. You can then ask:

> "read the AndroidManifest.xml"

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
