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

    exe.root_module.addCSourceFiles(.{
        .root = b.path("src"),
        .flags = c_flags_list.items,
        .files = &.{
            "garlic.c",

            // libs/list
            "libs/list/list.c",

            // libs/hashmap
            "libs/hashmap/hashmap.c",
            "libs/hashmap/hashmap_tools.c",

            // libs/queue
            "libs/queue/queue.c",

            // libs/str
            "libs/str/str.c",

            // libs/memory
            "libs/memory/mem_pool.c",

            // libs/zip
            "libs/zip/zip.c",

            // libs/threadpool
            "libs/threadpool/threadpool.c",

            // libs/bitset
            "libs/bitset/bitset.c",

            // libs/trie
            "libs/trie/trie_tree.c",

            // parser/class
            "parser/class/class_tools.c",
            "parser/class/metadata.c",
            "parser/class/opcode.c",
            "parser/class/writter.c",

            // parser/dex
            "parser/dex/metadata.c",
            "parser/dex/opcode.c",

            // parser/pe
            "parser/pe/metadata.c",

            // decompiler
            "decompiler/control_flow.c",
            "decompiler/descriptor.c",
            "decompiler/dominator_tree.c",
            "decompiler/exception.c",
            "decompiler/expression.c",
            "decompiler/expression_analyse.c",
            "decompiler/expression_array.c",
            "decompiler/expression_assert.c",
            "decompiler/expression_assign.c",
            "decompiler/expression_branches.c",
            "decompiler/expression_chain.c",
            "decompiler/expression_copy_propgation.c",
            "decompiler/expression_enum.c",
            "decompiler/expression_exception.c",
            "decompiler/expression_goto.c",
            "decompiler/expression_if.c",
            "decompiler/expression_inline.c",
            "decompiler/expression_inner_class.c",
            "decompiler/expression_local_variable.c",
            "decompiler/expression_logical.c",
            "decompiler/expression_loop.c",
            "decompiler/expression_loop_type.c",
            "decompiler/expression_new.c",
            "decompiler/expression_node.c",
            "decompiler/expression_node_param.c",
            "decompiler/expression_remove_useless.c",
            "decompiler/expression_return.c",
            "decompiler/expression_synchronized.c",
            "decompiler/expression_ternary.c",
            "decompiler/expression_visitor.c",
            "decompiler/expression_writter.c",
            "decompiler/field.c",
            "decompiler/instruction.c",
            "decompiler/klass.c",
            "decompiler/method.c",
            "decompiler/scc.c",
            "decompiler/signature.c",
            "decompiler/ssa.c",
            "decompiler/stack.c",

            // decompiler/transformer
            "decompiler/transformer/anonymous.c",
            "decompiler/transformer/array_load.c",
            "decompiler/transformer/array_store.c",
            "decompiler/transformer/arraylength.c",
            "decompiler/transformer/assert.c",
            "decompiler/transformer/assignment.c",
            "decompiler/transformer/assignment_chain.c",
            "decompiler/transformer/athrow.c",
            "decompiler/transformer/cast.c",
            "decompiler/transformer/const.c",
            "decompiler/transformer/declaration.c",
            "decompiler/transformer/define_stack_var.c",
            "decompiler/transformer/enum.c",
            "decompiler/transformer/get_field.c",
            "decompiler/transformer/get_static.c",
            "decompiler/transformer/goto.c",
            "decompiler/transformer/if.c",
            "decompiler/transformer/iinc.c",
            "decompiler/transformer/initialize.c",
            "decompiler/transformer/instanceof.c",
            "decompiler/transformer/invoke.c",
            "decompiler/transformer/invokedynamic.c",
            "decompiler/transformer/invokeinterface.c",
            "decompiler/transformer/invokespecial.c",
            "decompiler/transformer/invokestatic.c",
            "decompiler/transformer/invokevirtual.c",
            "decompiler/transformer/lambda.c",
            "decompiler/transformer/local_variable.c",
            "decompiler/transformer/logic_not.c",
            "decompiler/transformer/loop.c",
            "decompiler/transformer/lvalue.c",
            "decompiler/transformer/monitorenter.c",
            "decompiler/transformer/monitorexit.c",
            "decompiler/transformer/new_array.c",
            "decompiler/transformer/operator.c",
            "decompiler/transformer/put_field.c",
            "decompiler/transformer/put_static.c",
            "decompiler/transformer/return.c",
            "decompiler/transformer/single_list.c",
            "decompiler/transformer/single_operator.c",
            "decompiler/transformer/stack_value.c",
            "decompiler/transformer/stack_var.c",
            "decompiler/transformer/store.c",
            "decompiler/transformer/str_concat.c",
            "decompiler/transformer/switch.c",
            "decompiler/transformer/ternary.c",
            "decompiler/transformer/transformer.c",
            "decompiler/transformer/uninitialize.c",

            // jar
            "jar/jar.c",

            // jvm
            "jvm/jvm_annotation.c",
            "jvm/jvm_class.c",
            "jvm/jvm_decompile.c",
            "jvm/jvm_descriptor.c",
            "jvm/jvm_exception.c",
            "jvm/jvm_expression_builder.c",
            "jvm/jvm_ins.c",
            "jvm/jvm_ins_action.c",
            "jvm/jvm_lambda.c",
            "jvm/jvm_method.c",
            "jvm/jvm_optimizer.c",
            "jvm/jvm_simulator.c",
            "jvm/jvm_type_analyse.c",

            // apk
            "apk/apk.c",
            "apk/apk_manifest.c",

            // dalvik
            "dalvik/dex_annotation.c",
            "dalvik/dex_class.c",
            "dalvik/dex_decompile.c",
            "dalvik/dex_descriptor.c",
            "dalvik/dex_dump.c",
            "dalvik/dex_exception.c",
            "dalvik/dex_expression_builder.c",
            "dalvik/dex_ins.c",
            "dalvik/dex_ins_action.c",
            "dalvik/dex_lambda.c",
            "dalvik/dex_meta_helper.c",
            "dalvik/dex_method.c",
            "dalvik/dex_optimizer.c",
            "dalvik/dex_pre_optimizer.c",
            "dalvik/dex_simulator.c",
            "dalvik/dex_smali.c",
            "dalvik/dex_type_analyse.c",
            "dalvik/dex_writter.c",
        },
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
