const std = @import("std");
const builtin = @import("builtin");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const exe = b.addExecutable(.{
        .name = "garlic",
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
        }),
    });

    exe.root_module.link_libc = true;

    exe.root_module.addIncludePath(b.path("src"));
    exe.root_module.addIncludePath(b.path("src/common"));
    exe.root_module.addIncludePath(b.path("src/libs/memory"));
    exe.root_module.addIncludePath(b.path("src/libs/hashmap"));
    exe.root_module.addIncludePath(b.path("src/libs/list"));
    exe.root_module.addIncludePath(b.path("src/libs/bitset"));
    exe.root_module.addIncludePath(b.path("src/libs/queue"));
    exe.root_module.addIncludePath(b.path("src/libs/str"));
    exe.root_module.addIncludePath(b.path("src/libs/zip"));
    exe.root_module.addIncludePath(b.path("src/libs/threadpool"));
    exe.root_module.addIncludePath(b.path("src/jar"));
    exe.root_module.addIncludePath(b.path("src/dalvik"));

    var c_flags_list = std.ArrayList([]const u8).empty;
    defer c_flags_list.deinit(b.allocator);

    c_flags_list.appendSlice(b.allocator, &.{
        "-std=c99",
        "-fvisibility=hidden",
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

    var c_files: std.ArrayList([]const u8) = .empty;
    while (walker.next(b.graph.io) catch @panic("walk failed")) |entry| {
        if (entry.kind == .file and std.mem.endsWith(u8, entry.basename, ".c")) {
            const normalized = b.allocator.dupe(u8, entry.path) catch @panic("OOM");
            c_files.append(b.allocator, normalized) catch @panic("OOM");
        }
    }

    exe.root_module.addCSourceFiles(.{
        .root = b.path("src"),
        .flags = c_flags_list.items,
        .files = c_files.items,
    });

    if (target.result.os.tag == .windows) {
        exe.root_module.linkSystemLibrary("ws2_32", .{});
    } else {
        exe.root_module.linkSystemLibrary("pthread", .{});
    }

    b.installArtifact(exe);

    const run_step = b.step("run", "Run the app");
    const run_cmd = b.addRunArtifact(exe);
    run_step.dependOn(&run_cmd.step);
    run_cmd.step.dependOn(b.getInstallStep());
    if (b.args) |args| {
        run_cmd.addArgs(args);
    }
}
