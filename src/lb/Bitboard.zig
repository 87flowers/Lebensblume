// Represents a board with the following convention:
//
//   9   8   7   6   5   4   3   2   1
// +---+---+---+---+---+---+---+---+---+
// |   |   |   |   |   |   |   |   |   | a
// +---+---+---+---+---+---+---+---+---+
// |   |   |   |   |   |   |   |   |   | b
// +---+---+---+---+---+---+---+---+---+
// |   |   |   |   |   |   |   |   |   | c
// +---+---+---+---+---+---+---+---+---+
// |   |   |   |   |   |   |   |   |   | d
// +---+---+---+---+---+---+---+---+---+
// | … |   |   |   |   |   |   |   |   | e
// +---+---+---+---+---+---+---+---+---+
// |27 | … |   |   |   |   |   |   |   | f
// +---+---+---+---+---+---+---+---+---+
// |18 | … |   |   |   |   |   |   |   | g
// +---+---+---+---+---+---+---+---+---+
// | 9 |10 |11 |12 | … |   |   |   |   | h
// +---+---+---+---+---+---+---+---+---+
// | 0 | 1 | 2 | 3 | 4 | … |   |   |   | i
// +---+---+---+---+---+---+---+---+---+
//

raw: u81 = 0,

pub inline fn make(raw: u81) Bitboard {
    return .{ .raw = raw };
}

pub inline fn set(bb: *Bitboard, sq: lb.Square) void {
    bb.raw |= @as(u81, 1) << sq;
}

pub inline fn clear(bb: *Bitboard, sq: lb.Square) void {
    bb.raw &= ~(@as(u81, 1) << sq);
}

pub inline fn put(bb: *Bitboard, sq: lb.Square, value: u1) void {
    bb.clear(sq);
    bb.raw |= @as(u81, value) << sq;
}

pub inline fn get(bb: Bitboard, sq: lb.Square) u1 {
    return @truncate(bb.raw >> sq);
}

pub inline fn or_with(bb: *Bitboard, other: Bitboard) *Bitboard {
    bb.raw |= other.raw;
    return bb;
}

pub inline fn shift(bb: Bitboard, dir: Direction) Bitboard {
    return switch (dir) {
        .n => .{ .raw = bb.raw << 9 },
        .s => .{ .raw = bb.raw >> 9 },
        .e => .{ .raw = (bb.raw & ~file_1) << 1 },
        .w => .{ .raw = (bb.raw & ~file_9) >> 1 },
        .ne => .{ .raw = (bb.raw & ~file_1) << 10 },
        .nw => .{ .raw = (bb.raw & ~file_9) << 8 },
        .se => .{ .raw = (bb.raw & ~file_1) >> 8 },
        .sw => .{ .raw = (bb.raw & ~file_9) >> 10 },
    };
}

test shift {
    const base = Bitboard.make(0x800);
    try std.testing.expectEqual(0x100000, base.shift(.n).raw);
    try std.testing.expectEqual(0x200000, base.shift(.ne).raw);
    try std.testing.expectEqual(0x1000, base.shift(.e).raw);
    try std.testing.expectEqual(0x8, base.shift(.se).raw);
    try std.testing.expectEqual(0x4, base.shift(.s).raw);
    try std.testing.expectEqual(0x2, base.shift(.sw).raw);
    try std.testing.expectEqual(0x400, base.shift(.w).raw);
    try std.testing.expectEqual(0x80000, base.shift(.nw).raw);
}

const rank_a: u81 = rank_i << (8 * 9);
const rank_i: u81 = 0x1FF;
const file_1: u81 = file_9 << 8;
const file_9: u81 = 0x001008040201008040201;
pub const Direction = enum { n, ne, e, se, s, sw, w, nw };

const Bitboard = @This();
const std = @import("std");
const lb = @import("../lb.zig");
