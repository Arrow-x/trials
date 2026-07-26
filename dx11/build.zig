const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.resolveTargetQuery(.{
        .cpu_arch = .x86_64,
        .os_tag = .windows,
    });

    const optimize = b.standardOptimizeOption(.{});

    // We now pass `link_libcpp = true` directly into the module definition
    const exe = b.addExecutable(.{
        .name = "triangle",
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
            .link_libcpp = true,
        }),
    });

    exe.root_module.addCSourceFile(.{
        .file = b.path("main.cpp"),
        .flags = &[_][]const u8{ "-std=c++17" },
    });

    exe.root_module.linkSystemLibrary("d3d11", .{});
    exe.root_module.linkSystemLibrary("dxgi", .{});
    exe.root_module.linkSystemLibrary("user32", .{});
    exe.root_module.linkSystemLibrary("gdi32", .{});

    b.installArtifact(exe);
}
