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
# 列出可用工具（显示所有工具的名称、描述和 JSON Schema）
echo '{"jsonrpc":"2.0","id":1,"method":"tools/list"}' | garlic -m

# 测试反编译/结构查看
echo '{"jsonrpc":"2.0","id":2,"method":"tools/call","params":{"name":"dump_info","arguments":{"path":"/path/to/Hello.class"}}}' | garlic -m
```

---

## 工具参考

Garlic MCP 提供 **7 个工具**。以下按典型工作流顺序排列 —— **大多数逆向任务从 `analyze` 开始**，再根据需要深入使用其他工具。

### 1. `analyze`（一键分析）

**推荐入口。** 一站式完成反编译 + 调用图生成 + DuckDB 导入三个步骤。

- **参数**：`path`（string）—— `.dex` 或 `.apk` 文件路径；`output_dir`（string）—— 工作目录。
- **输出**：在 `output_dir` 下创建三项内容：
  - `decompiled/` — Java 源文件（保留包目录结构）
  - `cg/` — 调用图 CSV 文件（`call_graph_node.csv`、`call_graph_edge.csv`）
  - `analysis.duckdb` — 带索引的 DuckDB 数据库（含 `java_cg_nodes` / `java_cg_edges` 表）
- **返回**：包含 Java 文件数、CG 节点数、CG 边数的摘要，以及可直接使用的 `cg_query(...)` 示例。
- **前置依赖**：需要系统安装 `duckdb` CLI 并加入 PATH。
- **典型用法**：大多数逆向任务首选此工具。完成后可接着使用：
  - `android_manifest` 查看应用权限和入口点
  - `cg_query` 通过 SQL 分析调用图
- **示例**：
  ```
  analyze(path="/path/to/app.apk", output_dir="./app_analysis")
  ```

### 2. `decompile`（反编译）

将编译后的 Java/Android 二进制文件还原为可读的 Java 源代码。

- **参数**：`path`（string）—— `.class`、`.jar`、`.dex` 或 `.apk` 文件路径。
  - 可选 `output_dir`（string）—— 省略时结果内联返回，临时文件自动清理。
- **输出**：保留包目录结构的 Java 源文件。对于 APK 输入，还会一并提取 `AndroidManifest.xml`。
- **前置依赖**：无（独立运行，不依赖 DuckDB）。
- **典型用法**：只需要还原 Java 源码，不需要调用图或 SQL 分析时使用。如需完整分析，建议使用 `analyze`。
- **示例**：
  ```
  decompile(path="/path/to/app.apk", output_dir="./sources")
  ```

### 3. `dump_info`（结构信息）

快速查看类/DEX 文件内部结构 —— 方法、字段、常量、注解和签名，无需完整反编译。类似 `javap` 或 `dexdump`。

- **参数**：`path`（string）—— `.class` 或 `.dex` 文件路径。
- **输出**：展示类/字节码结构的格式化文本。
- **前置依赖**：无。
- **典型用法**：快速验证文件类型、检查 API 接口、检测混淆模式。
- **示例**：
  ```
  dump_info(path="/path/to/classes.dex")
  ```

### 4. `call_graph`（调用图）

为 `.dex` 或 `.apk` 文件生成完整的调用图。

- **参数**：`path`（string）—— `.dex` 或 `.apk` 文件路径。
  - 可选 `output_dir`（string）—— 省略时使用临时目录。
- **输出**：两个（或四个）CSV 文件：
  - `call_graph_node.csv` — 每个方法一行，包含 `node_id`、`method_raw`、`node_type`、`api_type` 列
  - `call_graph_edge.csv` — 每个调用关系一行（`src_id`、`dst_id`）
  - `string_node.csv` / `string_edge.csv` — 字符串引用分析数据（存在时生成）
- **前置依赖**：无（独立运行，不依赖 DuckDB）。
- **典型用法**：理解方法依赖关系、寻找入口点、追踪数据流、检测未使用的代码。CSV 输出专为通过 `cg_import` 导入 DuckDB 而设计。
- **示例**：
  ```
  call_graph(path="/path/to/app.apk", output_dir="./cg_output")
  ```

### 5. `cg_import`（导入调用图到 DuckDB）

将调用图 CSV 文件导入 DuckDB 数据库，建立索引以便快速 SQL 查询。

- **参数**：
  - `cg_dir`（string）—— `call_graph` 的输出目录，需包含 `call_graph_node.csv` 和 `call_graph_edge.csv`
  - `db_path`（string）—— `.duckdb` 数据库文件的输出路径
- **输出**：在 `db_path` 位置创建 DuckDB 数据库，包含：
  - `java_cg_nodes(node_id, method_raw, node_type, api_type)` — 方法元数据（已建索引）
  - `java_cg_edges(src_id, dst_id)` — 调用关系（两端均已建索引）
  - `string_nodes` / `string_edges` — 仅当对应的 CSV 文件存在时创建
- **前置依赖**：需要系统安装 `duckdb` CLI 并加入 PATH。
- **典型用法**：介于 `call_graph` 和 `cg_query` 之间的中间步骤。先运行 `call_graph`，再用此工具导入，最后用 `cg_query` 分析。
- **示例**：
  ```
  cg_import(cg_dir="./cg_output", db_path="./analysis.duckdb")
  ```

### 6. `cg_query`（SQL 查询调用图）

对 `cg_import` 创建的 DuckDB 数据库执行 SQL 查询。

- **参数**：
  - `db_path`（string）—— `cg_import` 创建的 `.duckdb` 数据库文件路径
  - `sql`（string）—— 要执行的 SQL 查询
- **输出**：CSV 格式的查询结果。
- **前置依赖**：需要系统安装 `duckdb` CLI 并加入 PATH。
- **数据库结构**：
  - `java_cg_nodes` — `node_id BIGINT, method_raw VARCHAR, node_type BIGINT, api_type BIGINT`
  - `java_cg_edges` — `src_id BIGINT, dst_id BIGINT`
  - `string_nodes` — 字符串常量元数据（如已导入）
  - `string_edges` — 字符串到方法的引用（如已导入）
- **典型查询场景**：
  - 查找某个方法调用了哪些方法：`SELECT * FROM java_cg_edges WHERE src_id = ?`
  - 查找哪些方法调用了某方法：`SELECT * FROM java_cg_edges WHERE dst_id = ?`
  - 发现应用入口点：`SELECT * FROM java_cg_nodes WHERE api_type > 0`
  - 查找未被调用的方法：`SELECT n.* FROM java_cg_nodes n LEFT JOIN java_cg_edges e ON n.node_id = e.dst_id WHERE e.dst_id IS NULL`
  - 按类型统计 API 调用：`SELECT node_type, COUNT(*) FROM java_cg_nodes GROUP BY node_type`
- **重要提示**：查询以只读模式运行，不会修改数据库。
- **示例**：
  ```
  cg_query(db_path="./analysis.duckdb", sql="SELECT method_raw, node_type FROM java_cg_nodes LIMIT 20")
  ```

### 7. `android_manifest`（读取 AndroidManifest）

从之前的 `decompile` 或 `analyze` 输出目录中读取 AndroidManifest.xml。

- **参数**：`output_dir`（string）—— 与 `decompile` 或 `analyze` 工具使用的输出目录。
- **输出**：AndroidManifest.xml 的完整 XML 内容，包括：
  - 应用权限（`<uses-permission>`）
  - Activity、Service、BroadcastReceiver、ContentProvider 声明
  - Intent 过滤器和组件导出标志
- **前置依赖**：无。
- **典型用法**：反编译 APK 后，使用此工具了解应用的攻击面、入口点、声明的权限和组件间通信（ICC）接口。
- **示例**：
  ```
  android_manifest(output_dir="./app_analysis")
  ```

---

### 工作流总结

```
analyze(path, output_dir)          ← 大多数任务从此开始
  ├─ decompile(path, output_dir?)   ← 或单独使用
  ├─ dump_info(path)                ← 快速查看结构
  └─ call_graph(path, output_dir?)  ← 生成调用图 CSV
       └─ cg_import(cg_dir, db_path)  ← 导入 DuckDB
             └─ cg_query(db_path, sql) ← SQL 分析
 android_manifest(output_dir)      ← 读取应用权限（在 decompile/analyze 之后）
```

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
