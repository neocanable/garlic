# Garlic on Windows

Garlic supports Windows natively via [MinGW-w64](http://mingw-w64.org/) cross-compilation. This guide covers building, running, and common Windows-specific issues.

---

## Quick Start

Download the latest `win64` or `win32` binary from the [GitHub Releases page](https://github.com/neocanable/garlic/releases), extract it, and run:

```sh
garlic.exe app.apk -o output
```

Or build from source (see below).

---

## Build

### Option 1: Cross-compile from macOS/Linux (recommended)

Using the provided `build.sh` script:

```sh
# Windows 64-bit
./build.sh win64

# Windows 32-bit
./build.sh win32
```

Output goes to `build/build-win64/garlic.exe` or `build/build-win32/garlic.exe`.

Requires MinGW-w64 toolchain:

```sh
brew install mingw-w64           # macOS
sudo apt install mingw-w64       # Debian/Ubuntu
```

### Option 2: Build with Zig (cross-platform)

```sh
# Windows x86_64
zig build --release=fast -Dtarget=x86_64-windows

# Windows 32-bit
zig build --release=fast -Dtarget=x86-windows
```

Zig bundles its own cross-linkers — no toolchain needed. Output goes to `zig-out/bin/garlic.exe`.

### Option 3: Build natively on Windows (MSYS2)

1. Install [MSYS2](https://www.msys2.org/)
2. Install MinGW-w64 GCC and CMake:
   ```sh
   pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake make
   ```
3. Build:
   ```sh
   mkdir build && cd build
   cmake .. -G "MinGW Makefiles"
   mingw32-make
   ```

---

## Performance

### Windows Defender slows down decompilation

By default, Windows Defender scans every `.java` file that garlic writes. For large APKs (50,000+ files), this can make decompilation **5-6x slower** than on macOS/Linux.

**Fix**: Add garlic's output directory to Defender's exclusion list.

```
Windows Security → Virus & threat protection → Manage settings
→ Exclusions → Add exclusion → Folder
```

Or using PowerShell (run as administrator):

```powershell
Add-MpPreference -ExclusionPath "C:\path\to\your\output"
```

### Console output

On Windows, frequent `printf`/`fflush(stdout)` for progress display can be slower due to console (`conhost.exe`) overhead. If you notice slowdown during decompilation, redirect output to a file or use `-t 1` to reduce thread contention on the console:

```sh
garlic.exe app.apk -o output > nul
```

---

## Known Issues

### MCP Server on Windows

The MCP server (`garlic.exe -m`) runs over stdio and works on Windows. Configure it in Claude Desktop:

```json
{
  "mcpServers": {
    "garlic": {
      "command": "C:\\path\\to\\garlic.exe",
      "args": ["-m"]
    }
  }
}
```

**DuckDB requirement**: The MCP server's `analyze`, `cg_import`, and `cg_query` tools require [DuckDB CLI](https://duckdb.org/docs/installation/) installed and in `%PATH%`.

### Call graph (`-g`)

- CSV files are written in binary mode (`"wb"`) for Windows compatibility
- `call_graph_node.csv` and `method_node.csv` have correct line endings for Windows
- DuckDB import works natively

### MCP call graph tools

The `cg_import` and `cg_query` tools shell out to `duckdb.exe`. Make sure DuckDB is in your `%PATH%` or the tools will fail with "duckdb: command not found".

---

## Tips

- **Use absolute paths** — Windows drive letters (`C:\...`) work correctly with garlic's path handling
- **Output directory** — always use `-o <output_dir>` to avoid writing to the same directory as the input file
- **Thread count** — the default of 4 threads works well on most Windows machines. 3900X users can try `-t 8` or `-t 12`
