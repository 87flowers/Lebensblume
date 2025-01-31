pub fn main() !void {
    var bufw = std.io.bufferedWriter(std.io.getStdOut().writer());
    const output = bufw.writer();
    try output.print("{s}\n", .{
        \\pub const SliderTable = struct {
        \\    blocker_mask: u81,
        \\    dest_table: []const u81,
        \\};
    });
    try output.print("\n", .{});
    try output.print("pub const rook: [81]SliderTable = .{{\n", .{});
    try gen(output, "    ", &.{ .n, .e, .s, .w });
    try output.print("}};\n", .{});
    try output.print("\n", .{});
    try output.print("pub const bishop: [81]SliderTable = .{{\n", .{});
    try gen(output, "    ", &.{ .ne, .se, .sw, .nw });
    try output.print("}};\n", .{});
    try output.print("\n", .{});
    try output.print("pub const lance: [2][81]SliderTable = .{{\n", .{});
    try output.print("    .{{\n", .{});
    try gen(output, "        ", &.{.n});
    try output.print("    }},\n", .{});
    try output.print("    .{{\n", .{});
    try gen(output, "        ", &.{.s});
    try output.print("    }},\n", .{});
    try output.print("}};\n", .{});
    try bufw.flush();
}

fn gen(output: anytype, indent: []const u8, directions: []const Direction) !void {
    for (0..81) |sq| {
        const piece = Bitboard.fromSq(@intCast(sq));
        const blockers = genPotentialBlockers(piece, directions);
        // const moves_no_blockers = genMoves(piece, directions, Bitboard{});
        try output.print("{s}.{{\n", .{indent});
        try output.print("{s}    .blocker_mask = 0x{x},\n", .{ indent, blockers.raw });
        try output.print("{s}    .dest_table = &[_]u81{{\n", .{indent});
        var subs = subsets(compress(blockers.raw, blockers.raw));
        while (subs.next()) |sub| {
            const current = Bitboard.make(decompress(sub, blockers.raw));
            const moves = genMoves(piece, directions, current);
            try output.print("{s}        0x{x},\n", .{ indent, moves.raw });
        }
        try output.print("{s}    }},\n", .{indent});
        try output.print("{s}}},\n", .{indent});
    }
}

pub inline fn subsets(x: u64) struct {
    set: u64,
    current: u64,
    pub fn next(self: *@This()) ?u64 {
        if (self.current == 0) return null;
        const result = self.current;
        self.current = (self.current -% self.set) & self.set;
        return result;
    }
} {
    return .{ .set = x, .current = -%x & x };
}

const compression_shift = 1;

fn compress(x: u81, mask: u81) u64 {
    assert(x & ~mask == 0);
    const top: u64 = @as(u64, @intCast(x >> 64)) << compression_shift;
    const bot: u64 = @truncate(x);
    // std.debug.print("{b} {b} {b} {b}\n", .{top, bot, x, mask});
    assert(bot & top == 0);
    const result = bot | top;
    return result;
}

fn decompress(y: u64, mask: u81) u81 {
    const bot: u81 = y & mask;
    const top: u81 = (@as(u81, y >> compression_shift) << 64) & mask;
    assert(bot & top == 0);
    const result = bot | top;
    assert(result & ~mask == 0);
    assert(compress(result, mask) == y);
    return result;
}

fn genMoves(piece: Bitboard, directions: []const Direction, blockers: Bitboard) Bitboard {
    var moves = Bitboard{};
    for (directions) |dir| {
        var current = piece.shift(dir);
        while (current.raw != 0) {
            moves.orWith(current);
            if (!Bitboard.@"and"(current, blockers).empty()) break;
            current = current.shift(dir);
        }
    }
    return moves;
}

fn genPotentialBlockers(piece: Bitboard, directions: []const Direction) Bitboard {
    var moves = Bitboard{};
    for (directions) |dir| {
        var current = piece.shift(dir);
        while (current.shift(dir).raw != 0) {
            moves.orWith(current);
            current = current.shift(dir);
        }
    }
    return moves;
}

test "rook on 9i" {
    const piece = Bitboard.fromSq(0);
    const moves = genMoves(piece, &.{ .n, .e, .w, .s }, Bitboard{});
    const blockers = genPotentialBlockers(piece, &.{ .n, .e, .w, .s });
    try std.testing.expectEqual(0b000000001_000000001_000000001_000000001_000000001_000000001_000000001_000000001_111111110, moves.raw);
    try std.testing.expectEqual(0b000000000_000000001_000000001_000000001_000000001_000000001_000000001_000000001_011111110, blockers.raw);
}

const std = @import("std");
const assert = std.debug.assert;
const lb = @import("lb.zig");
const Bitboard = lb.Bitboard;
const Direction = lb.Bitboard.Direction;
