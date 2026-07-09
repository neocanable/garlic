# Garlic MCP Server

[English](MCP-SERVER.md) | 中文

[Garlic](https://github.com/neocanable/garlic) —— 世界最快的 APK/Java 开源反编译器 —— 现在也支持 **Model Context Protocol (MCP)**。

通过 `garlic -m` 启动一个 stdio 传输的 MCP 服务器，将反编译、类结构查看、调用图分析和 DuckDB SQL 查询能力暴露给所有兼容 MCP 的 AI 客户端（Claude Desktop、Cline、Continue 等）。

---

## 前置依赖

- **Garlic** v1.6 及以上版本（编译时包含 MCP 支持）
- **DuckDB** CLI — **必须安装**。MCP 服务器依赖 DuckDB 进行所有分析功能：
  ```sh
  https://duckdb.org/install/?platform=macos&environment=cli
  ```
  > 如果没有 DuckDB，`analyze`、`cg_import` 和 `cg_query` 工具将无法使用。

---

## 安装

### 从 GitHub Releases 下载

从 [GitHub Releases 页面](https://github.com/neocanable/garlic/releases) 下载最新版的二进制文件。选择对应操作系统和架构的压缩包（macOS、Linux、Windows；x86_64 或 aarch64）。

解压后将 `garlic` 放到 `PATH` 下，例如：

```sh
# macOS / Linux
tar xzf garlic-*.tar.gz
sudo mv garlic /usr/local/bin/
```

```sh
git clone https://github.com/neocanable/garlic.git
cd garlic
# 使用 CMake 编译
cmake -B build
cmake --build build

# 或使用 Zig 编译（跨平台）
zig build --release=fast
```

验证是否正常工作：
```sh
./build/garlic -m
# （没有输出说明启动成功 —— 它在等待 stdin 上的 JSON-RPC 消息）
```

---

## 客户端配置

### Claude Desktop

编辑 `claude_desktop_config.json`（macOS 路径 `~/Library/Application Support/Claude/claude_desktop_config.json`）：

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

如果 garlic 不在系统 `PATH` 中，使用完整路径：

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

### Cline / Roo Code（VS Code 扩展）

在 MCP 服务器设置中添加：

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

在 `config.json` 中添加：

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

### 直接测试（stdio）

你也可以直接通过管道与 MCP 服务器交互：

```sh
# 列出可用工具
echo '{"jsonrpc":"2.0","id":1,"method":"tools/list"}' | garlic -m
```

---

## 工具参考

Garlic MCP 提供 **7 个工具**：

### 1. `analyze`（一键分析）

一站式分析：反编译 + 生成调用图 + 导入 DuckDB。  
大多数场景下默认使用此工具。

必填参数 `path`（string）—— `.dex` 或 `.apk` 文件路径，以及 `output_dir`（string）—— 工作目录。会在其中创建 `decompiled/`、`cg/` 和 `analysis.duckdb`。

需要系统安装 `duckdb` CLI。

### 2. `decompile`（反编译）

将 `.class` / `.jar` / `.dex` / `.apk` 文件反编译为 Java 源代码。

必填参数 `path`（string）—— 目标文件路径。  
可选参数 `output_dir`（string）—— 输出目录，省略时结果内联返回，临时文件自动清理。

### 3. `dump_info`（结构信息）

显示类或 DEX 文件结构（类似 `javap` / `dexdump`）。

必填参数 `path`（string）—— `.class` 或 `.dex` 文件路径。

### 4. `call_graph`（调用图）

为 `.dex` 或 `.apk` 文件生成调用图。

必填参数 `path`（string）—— `.dex` 或 `.apk` 文件路径。  
可选参数 `output_dir`（string）—— CSV 文件输出目录，省略时使用临时目录。生成 `call_graph_node.csv` 和 `call_graph_edge.csv`。

### 5. `cg_import`（导入调用图到 DuckDB）

将调用图 CSV 文件导入 DuckDB 数据库。

必填参数 `cg_dir`（string）—— 包含 `call_graph_node.csv` 和 `call_graph_edge.csv` 的目录，以及 `db_path`（string）—— `.duckdb` 数据库文件输出路径。

需要系统安装 `duckdb` CLI。

### 6. `cg_query`（SQL 查询调用图）

对调用图 DuckDB 数据库执行 SQL 查询。

必填参数 `db_path`（string）—— `.duckdb` 数据库文件路径，以及 `sql`（string）—— 要执行的 SQL 查询。

需要系统安装 `duckdb` CLI。

### 7. `android_manifest`（读取 AndroidManifest）

从之前 analyze/decompile 的输出目录中读取 AndroidManifest.xml。

必填参数 `output_dir`（string）—— 与 `analyze` 或 `decompile` 工具使用的输出目录。返回 AndroidManifest.xml 的完整内容。

在 Claude Desktop 中询问：

> "help me analysis apk at '/path/to/apk'"

Garlic MCP 会自动反编译 APK、生成调用图并将数据导入 DuckDB 以供进一步分析。之后你可以问：

> "read the AndroidManifest.xml"

---

## 常见问题

- `garlic -m` 立即退出 — 没有 stdin 输入，请直接在 MCP 客户端中配置使用
- 工具返回空结果 — 文件不存在或路径错误，确认路径存在且 garlic 有读取权限
- `cg_import` / `cg_query` 失败 — 未安装 DuckDB，执行 `brew install duckdb`
- `duckdb: command not found` — DuckDB 不在 PATH 中，安装或提供完整路径
- `decompile` 返回错误 — 不支持的文件格式或文件损坏，检查文件类型（class/jar/dex/apk）
- 大 APK 响应慢 — 反编译很快（200MB APK 约 12 秒），但超大文件需要更多时间

### 调试日志

如果 garlic 以调试模式编译（未定义 `NDEBUG`），MCP 日志会输出到 **stderr**：

```sh
garlic -m 2>/dev/null   # 屏蔽日志
garlic -m               # 在终端显示日志
```

---

## 协议信息

- **服务器名称**：`garlic-mcp`
- **版本**：`1.0.0`
- **MCP 协议版本**：`2024-11-05`
- **传输方式**：stdio（JSON-RPC 2.0）
- **最大消息大小**：1 MB

---

## 许可证

Apache 2.0 —— 详见 [LICENSE](LICENSE)。
