const std = @import("std");

const core_source_dirs = [_][]const u8{
    "common",
    "libs/memory",
    "libs/hashmap",
    "libs/list",
    "libs/bitset",
    "libs/queue",
    "libs/str",
    "libs/zip",
    "libs/threadpool",
    "libs/trie",
    "decompiler",
    "parser/class",
    "parser/dex",
    "jar",
    "jvm",
    "apk",
    "dalvik",
    "analyzer",
    "report",
    "parser/pe",
    "libs/md4c",
};

const cli_source_dirs = [_][]const u8{
    "ai",
    "libs/cjson",
};

const core_include_dirs = [_][]const u8{
    "src",
    "src/common",
    "src/libs/memory",
    "src/libs/hashmap",
    "src/libs/list",
    "src/libs/bitset",
    "src/libs/queue",
    "src/libs/str",
    "src/libs/zip",
    "src/libs/threadpool",
    "src/jar",
    "src/dalvik",
    "src/libs/md4c",
    "libs/include",
};

const cli_include_dirs = [_][]const u8{
    "src/ai",
    "src/libs/cjson",
};

fn collectCFiles(
    b: *std.Build,
    files: *std.ArrayList([]const u8),
    source_dirs: []const []const u8,
) void {
    for (source_dirs) |relative_dir| {
        const source_dir = b.fmt("src/{s}", .{relative_dir});
        var dir = b.build_root.handle.openDir(b.graph.io, source_dir, .{ .iterate = true }) catch |err| {
            std.debug.panic("unable to open {s}: {}", .{ source_dir, err });
        };
        defer dir.close(b.graph.io);

        var walker = dir.walk(b.allocator) catch @panic("OOM");
        defer walker.deinit();

        while (walker.next(b.graph.io) catch @panic("walk failed")) |entry| {
            if (entry.kind != .file or !std.mem.endsWith(u8, entry.basename, ".c"))
                continue;
            files.append(b.allocator, b.fmt("{s}/{s}", .{ relative_dir, entry.path })) catch @panic("OOM");
        }
    }
}

fn addIncludeDirs(
    b: *std.Build,
    module: *std.Build.Module,
    include_dirs: []const []const u8,
) void {
    for (include_dirs) |dir|
        module.addIncludePath(b.path(dir));
}

fn determineRosemaryLib(target: std.Target) []const u8 {
    return switch (target.os.tag) {
        .linux => switch (target.cpu.arch) {
            .aarch64 => "libs/linux-aarch64/librosemarylib.so",
            .x86 => "libs/linux-i686/librosemarylib.so",
            .x86_64 => "libs/linux-x64/librosemarylib.so",
            else => @panic("Unsupported Linux architecture for rosemary"),
        },
        .macos => switch (target.cpu.arch) {
            .aarch64 => "libs/macos-aarch64/librosemarylib.dylib",
            .x86_64 => "libs/macos-x64/librosemarylib.dylib",
            else => @panic("Unsupported macOS architecture for rosemary"),
        },
        .windows => switch (target.cpu.arch) {
            .x86_64 => "libs/win64/librosemarylib.dll",
            .x86 => "libs/win32/librosemarylib.dll",
            else => @panic("Unsupported Windows architecture for rosemary"),
        },
        else => @panic("Unsupported OS for rosemary"),
    };
}

fn embedRosemaryLib(b: *std.Build, rosemary_path: []const u8) std.Build.LazyPath {
    const data = b.build_root.handle.readFileAlloc(
        b.graph.io, rosemary_path, b.allocator, @enumFromInt(std.math.maxInt(usize)),
    ) catch |err| {
        std.debug.panic("Failed to read {s}: {}", .{ rosemary_path, err });
    };
    defer b.allocator.free(data);
    const file_size = data.len;

    var c_source = std.ArrayList(u8).initCapacity(b.allocator, file_size * 6) catch @panic("OOM");
    c_source.appendSliceAssumeCapacity("/* Auto-generated from ");
    c_source.appendSliceAssumeCapacity(rosemary_path);
    c_source.appendSliceAssumeCapacity(" */\n");
    c_source.appendSliceAssumeCapacity("#include <stddef.h>\n");
    c_source.appendSliceAssumeCapacity("const unsigned char rosemarylib_data[] = {\n  ");

    for (data, 0..) |byte, i| {
        if (i > 0) {
            if (i % 16 == 0) {
                c_source.appendSliceAssumeCapacity(",\n  ");
            } else {
                c_source.appendSliceAssumeCapacity(",");
            }
        }
        c_source.print(b.allocator, "0x{X:0>2}", .{byte}) catch @panic("OOM");
    }

    c_source.print(b.allocator, "\n}};\nconst size_t rosemarylib_data_len = {d};\n", .{file_size}) catch @panic("OOM");

    const write_files = b.addWriteFiles();
    return write_files.add("rosemarylib_data.c", c_source.items);
}

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    if (target.result.abi == .msvc) {
        std.log.err("MSVC ABI is not supported", .{});
        std.process.exit(1);
    }

    var c_flags_list = std.ArrayList([]const u8).empty;
    defer c_flags_list.deinit(b.allocator);

    c_flags_list.appendSlice(b.allocator, &.{
        "-std=c99",
        "-Wall",
        "-Wno-implicit-function-declaration",
        "-Wno-incompatible-pointer-types",
        "-Wno-misleading-indentation",
        "-Wno-format",
    }) catch @panic("OOM");

    if (target.result.os.tag == .linux) {
        c_flags_list.appendSlice(b.allocator, &.{
            "-D_GNU_SOURCE",
        }) catch @panic("OOM");
    }

    var lib_c_files: std.ArrayList([]const u8) = .empty;
    defer lib_c_files.deinit(b.allocator);
    var cli_c_files: std.ArrayList([]const u8) = .empty;
    defer cli_c_files.deinit(b.allocator);

    collectCFiles(b, &lib_c_files, &core_source_dirs);
    cli_c_files.append(b.allocator, "garlic.c") catch @panic("OOM");
    cli_c_files.append(b.allocator, "rosemary/rosemary_embed.c") catch @panic("OOM");
    collectCFiles(b, &cli_c_files, &cli_source_dirs);

    const lib = b.addLibrary(.{
        .linkage = .static,
        .name = "garlic",
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
            .link_libc = true,
        }),
    });

    const exe = b.addExecutable(.{
        .name = "garlic-cli",
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
            .link_libc = true,
        }),
    });

    addIncludeDirs(b, lib.root_module, &core_include_dirs);
    addIncludeDirs(b, exe.root_module, &core_include_dirs);
    addIncludeDirs(b, exe.root_module, &cli_include_dirs);

    lib.root_module.addCSourceFiles(.{
        .root = b.path("src"),
        .language = .c,
        .flags = c_flags_list.items,
        .files = lib_c_files.items,
    });

    // Embed rosemary native analysis library as a C byte array
    const rosemary_src_path = determineRosemaryLib(target.result);
    const rosemary_data_lazy_path = embedRosemaryLib(b, rosemary_src_path);

    exe.root_module.addCSourceFiles(.{
        .root = b.path("src"),
        .language = .c,
        .flags = c_flags_list.items,
        .files = cli_c_files.items,
    });

    // Add the generated rosemarylib_data.c
    exe.root_module.addCSourceFile(.{
        .file = rosemary_data_lazy_path,
        .flags = c_flags_list.items,
    });
    exe.root_module.addCMacro("ROSEMARYLIB_EMBEDDED", "1");

    exe.root_module.linkLibrary(lib);

    if (optimize != .Debug) {
        exe.root_module.strip = true;
    }

    if (target.result.os.tag == .windows) {
        exe.root_module.linkSystemLibrary("ws2_32", .{});
    } else {
        exe.root_module.linkSystemLibrary("pthread", .{});
    }

    b.installArtifact(lib);

    const installed_exe_name = if (target.result.os.tag == .windows)
        "garlic.exe"
    else
        "garlic";
    const install_exe = b.addInstallArtifact(exe, .{
        .dest_sub_path = installed_exe_name,
    });
    b.getInstallStep().dependOn(&install_exe.step);

    for (core_source_dirs) |relative_dir| {
        b.installDirectory(.{
            .source_dir = b.path(b.fmt("src/{s}", .{relative_dir})),
            .install_dir = .header,
            .install_subdir = b.fmt("garlic/{s}", .{relative_dir}),
            .include_extensions = &.{".h"},
        });
    }

    const run_step = b.step("run", "Run the app");
    const run_cmd = b.addRunArtifact(exe);
    run_step.dependOn(&run_cmd.step);
    run_cmd.step.dependOn(b.getInstallStep());
    if (b.args) |args| {
        run_cmd.addArgs(args);
    }
}
