pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const lebensblume_exe = add(b, target, optimize, "run", "Run shogi engine", "lebensblume", "src/main.zig");
    _ = add(b, target, optimize, "generate-sliders", "Generate slider tables", "lb-generate-sliders", "src/generate_sliders.zig");

    addTests(b);

    const bench_cmd = b.addRunArtifact(lebensblume_exe);
    bench_cmd.addArg("bench");
    const bench_cmd_step = b.step("bench", "Run shogi engine internal benchmark");
    bench_cmd_step.dependOn(&bench_cmd.step);
}

fn add(b: *std.Build, target: ResolvedTarget, optimize: OptimizeMode, step_cmd: []const u8, description: []const u8, exe_name: []const u8, root_source_file: []const u8) *std.Build.Step.Compile {
    const exe = b.addExecutable(.{
        .name = exe_name,
        .root_source_file = b.path(root_source_file),
        .target = target,
        .optimize = optimize,
    });
    b.installArtifact(exe);

    const run_cmd = b.addRunArtifact(exe);
    if (b.args) |args| {
        run_cmd.addArgs(args);
    }
    const run_step = b.step(step_cmd, description);
    run_step.dependOn(&run_cmd.step);

    return exe;
}

fn addTests(b: *std.Build) void {
    const exe_unit_tests = b.addTest(.{
        .root_source_file = b.path("src/tests.zig"),
    });
    const run_exe_unit_tests = b.addRunArtifact(exe_unit_tests);
    const test_step = b.step("test", "Run unit tests");
    test_step.dependOn(&run_exe_unit_tests.step);
}

const std = @import("std");
const OptimizeMode = std.builtin.OptimizeMode;
const ResolvedTarget = std.Build.ResolvedTarget;
