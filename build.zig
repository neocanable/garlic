const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

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
        .name = "garlic",
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
            .link_libc = true,
        }),
    });

    const include_dirs = [_][]const u8{
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
    for (include_dirs) |dir| {
        lib.root_module.addIncludePath(b.path(dir));
        exe.root_module.addIncludePath(b.path(dir));
    }

    var c_flags_list = std.ArrayList([]const u8).empty;
    defer c_flags_list.deinit(b.allocator);

    c_flags_list.appendSlice(b.allocator, &.{
        "-std=c99",
        "-Wall",
        "-Wno-unused-variable",
        "-Wno-unused-function",
        "-Wno-unused-parameter",
        "-Wno-unused-but-set-variable",
        "-Wno-implicit-function-declaration",
        "-Wno-incompatible-pointer-types",
        "-Wno-misleading-indentation",
        "-Wno-format",
    }) catch @panic("OOM");

    if (target.result.os.tag == .linux) {
        c_flags_list.appendSlice(b.allocator, &.{
            "-D_GNU_SOURCE",
            "-flto",
            "-Wno-ignored-optimization-argument",
        }) catch @panic("OOM");
    }

    const dir = std.Io.Dir.cwd();
    var src_dir = dir.openDir(b.graph.io, "src", .{ .iterate = true }) catch |err| {
        std.debug.panic("unable to open src directory: {}", .{err});
    };
    defer src_dir.close(b.graph.io);

    var walker = src_dir.walk(b.allocator) catch @panic("OOM");
    defer walker.deinit();

    var lib_c_files: std.ArrayList([]const u8) = .empty;
    while (walker.next(b.graph.io) catch @panic("walk failed")) |entry| {
        if (entry.kind == .file and std.mem.endsWith(u8, entry.basename, ".c")) {
            const path = b.allocator.dupe(u8, entry.path) catch @panic("OOM");
            if (!std.mem.eql(u8, entry.basename, "garlic.c")) {
                lib_c_files.append(b.allocator, path) catch @panic("OOM");
            }
        }
    }

    lib.root_module.addCSourceFiles(.{
        .root = b.path("src"),
        .flags = c_flags_list.items,
        .files = lib_c_files.items,
    });

    exe.root_module.addCSourceFiles(.{
        .root = b.path("src"),
        .flags = c_flags_list.items,
        .files = &.{
            "garlic.c",
        },
    });
    exe.root_module.linkLibrary(lib);

    if (target.result.os.tag == .windows) {
        exe.root_module.linkSystemLibrary("ws2_32", .{});
    } else {
        exe.root_module.linkSystemLibrary("pthread", .{});
    }

    b.installArtifact(lib);
    b.installArtifact(exe);

    const run_step = b.step("run", "Run the app");
    const run_cmd = b.addRunArtifact(exe);
    run_step.dependOn(&run_cmd.step);
    run_cmd.step.dependOn(b.getInstallStep());
    if (b.args) |args| {
        run_cmd.addArgs(args);
    }
}
