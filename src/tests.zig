test {
    std.testing.refAllDeclsRecursive(@import("lb.zig"));
    std.testing.refAllDeclsRecursive(@import("main.zig"));
    std.testing.refAllDeclsRecursive(@import("util.zig"));
}

const std = @import("std");
