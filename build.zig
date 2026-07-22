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

    exe.root_module.addCSourceFiles(.{
        .root = b.path("src"),
        .language = .c,
        .flags = c_flags_list.items,
        .files = cli_c_files.items,
    });

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
