const std = @import("std");

pub fn build(b: *std.Build) void {
    const optimize = .ReleaseFast;

    const targets = [_]struct {
        os_tag: std.Target.Os.Tag,
        cpu_arch: std.Target.Cpu.Arch,
        abi: ?std.Target.Abi = null, // Default to null (auto-detect)
    }{
        // Linux (explicitly use GNU libc)
        .{ .os_tag = .linux, .cpu_arch = .x86_64, .abi = .gnu },
        .{ .os_tag = .linux, .cpu_arch = .aarch64, .abi = .gnu },
        // Windows
        .{ .os_tag = .windows, .cpu_arch = .x86_64 },
        .{ .os_tag = .windows, .cpu_arch = .aarch64 },
        // macOS
        .{ .os_tag = .macos, .cpu_arch = .x86_64 },
        .{ .os_tag = .macos, .cpu_arch = .aarch64 },
    };

    for (targets) |target| {
        const os_name = @tagName(target.os_tag);
        const arch_name = @tagName(target.cpu_arch);
        const lib_name = b.fmt("clippy-{s}-{s}", .{os_name, arch_name});

        const lib = b.addSharedLibrary(.{
            .name = lib_name,
            .target = b.resolveTargetQuery(.{
                .os_tag = target.os_tag,
                .cpu_arch = target.cpu_arch,
                .abi = target.abi, // <-- Pass ABI (gnu/musl)
            }),
            .optimize = optimize,
            .strip = true,
        });

        lib.linkLibC();
        lib.addCSourceFiles(.{
            .files = &.{
                "src/clippy.c",
                "src/bitboard.c",
                "src/move.c",
                "src/movegen.c",
                "src/position.c",
                "src/search.c",
                "src/tt.c",
                "src/eval.c",
            },
            .flags = &.{
                "-std=c11",
                "-Wall",
                "-Wextra",
            },
        });

        // Link libm only on Linux and Windows
        if (target.os_tag == .linux or target.os_tag == .windows) {
            lib.linkSystemLibrary("m");
        }

        // macOS-specific settings
        if (target.os_tag == .macos) {
            lib.install_name = b.fmt("@rpath/lib{s}.dylib", .{lib_name});
        }

        b.installArtifact(lib);
    }
}