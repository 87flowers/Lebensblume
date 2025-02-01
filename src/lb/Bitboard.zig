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

pub inline fn fromSq(sq: Square) Bitboard {
    return .{ .raw = @as(u81, 1) << sq.raw };
}

pub inline fn toSq(bb: Bitboard) Square {
    assert(bb.count() == 1);
    return Square.make(@intCast(@ctz(bb.raw)));
}

pub fn fromSqList(list: []const *const [2]u8) !Bitboard {
    var result = Bitboard{};
    for (list) |str| {
        const sq = try Square.parse(str.*);
        result.orWith(sq.bitboard());
    }
    return result;
}

pub inline fn promoZone(color: Color) Bitboard {
    return Bitboard.make(switch (color) {
        .sente => rank_a | (rank_a >> 9) | (rank_a >> 18),
        .gote => rank_i | (rank_i << 9) | (rank_i << 18),
    });
}

pub inline fn rank(id: usize) Bitboard {
    assert(id < 9);
    return Bitboard.make(rank_i << (id * 9));
}

pub inline fn rankRelative(id: usize, perspective: Color) Bitboard {
    return switch (perspective) {
        .sente => rank(id),
        .gote => rank(8 - id),
    };
}

pub inline fn set(bb: *Bitboard, sq: Square) void {
    bb.raw |= @as(u81, 1) << sq.raw;
}

pub inline fn clear(bb: *Bitboard, sq: Square) void {
    bb.raw &= ~(@as(u81, 1) << sq.raw);
}

pub inline fn put(bb: *Bitboard, sq: Square, value: u1) void {
    bb.clear(sq);
    bb.raw |= @as(u81, value) << sq.raw;
}

pub inline fn get(bb: Bitboard, sq: Square) u1 {
    return @truncate(bb.raw >> sq.raw);
}

pub inline fn empty(bb: Bitboard) bool {
    return bb.raw == 0;
}

pub inline fn count(bb: Bitboard) usize {
    return @popCount(bb.raw);
}

pub inline fn invert(bb: Bitboard) Bitboard {
    return .{ .raw = ~bb.raw };
}

pub inline fn @"and"(a: Bitboard, b: Bitboard) Bitboard {
    return .{ .raw = a.raw & b.raw };
}

pub inline fn @"or"(a: Bitboard, b: Bitboard) Bitboard {
    return .{ .raw = a.raw | b.raw };
}

pub inline fn orWith(bb: *Bitboard, other: Bitboard) void {
    bb.raw |= other.raw;
}

pub inline fn fillFile(bb: Bitboard) Bitboard {
    var up = bb.raw;
    var down = bb.raw;
    up |= up << 9;
    down |= down >> 9;
    up |= up << 18;
    down |= down >> 18;
    up |= up << 36;
    down |= down >> 36;
    up |= up << 72;
    down |= down >> 72;
    return .{ .raw = up | down };
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

pub inline fn shiftRelative(bb: Bitboard, dir: Direction, perspective: Color) Bitboard {
    return switch (perspective) {
        .sente => bb.shift(dir),
        .gote => bb.shift(@enumFromInt(@intFromEnum(dir) +% 4)),
    };
}

test shift {
    const base = Bitboard.make(0x800);
    try std.testing.expectEqual(0x100000, base.shift(.n).raw);
    try std.testing.expectEqual(0x200000, base.shift(.ne).raw);
    try std.testing.expectEqual(0x001000, base.shift(.e).raw);
    try std.testing.expectEqual(0x000008, base.shift(.se).raw);
    try std.testing.expectEqual(0x000004, base.shift(.s).raw);
    try std.testing.expectEqual(0x000002, base.shift(.sw).raw);
    try std.testing.expectEqual(0x000400, base.shift(.w).raw);
    try std.testing.expectEqual(0x080000, base.shift(.nw).raw);
}

pub inline fn iterate(bb: Bitboard) struct {
    bits: u81,
    pub fn next(self: *@This()) ?Bitboard {
        if (self.bits == 0) return null;
        const lsb = self.bits & -%self.bits;
        self.bits ^= lsb;
        return Bitboard.make(lsb);
    }
} {
    return .{ .bits = bb.raw };
}

pub inline fn iterateSquares(bb: Bitboard) struct {
    bits: u81,
    pub fn next(self: *@This()) ?Square {
        if (self.bits == 0) return null;
        const lsb = self.bits & -%self.bits;
        self.bits ^= lsb;
        return Square.make(@intCast(@ctz(lsb)));
    }
} {
    return .{ .bits = bb.raw };
}

pub fn prettyPrint(bb: Bitboard, writer: anytype, prefix: []const u8) !void {
    try writer.raw("{s}┏━━┯━━┯━━┯━━┯━━┯━━┯━━┯━━┯━━┓\n", .{prefix});
    for (0..81) |place_index| {
        const file, const sq = lb.displayIndexToSquare(place_index);
        if (file == 0) {
            try writer.raw("{s}┃", .{prefix});
        } else {
            try writer.raw("│", .{});
        }
        try writer.raw("{s}", .{switch (bb.get(sq)) {
            0 => "０",
            1 => "１",
        }});
        if (file == 8) {
            try writer.raw("┃\n", .{});
            if (place_index != 80) try writer.raw("{s}┠──┼──┼──┼──┼──┼──┼──┼──┼──┨\n", .{prefix});
        }
    }
    try writer.raw("{s}┗━━┷━━┷━━┷━━┷━━┷━━┷━━┷━━┷━━┛\n", .{prefix});
    try writer.raw("{s}hex: 0x{x:021}\n", .{ prefix, bb.raw });
    try writer.flush();
}

const rank_a: u81 = rank_i << (8 * 9);
const rank_i: u81 = 0x1FF;
const file_1: u81 = file_9 << 8;
const file_9: u81 = 0x001008040201008040201;
pub const Direction = enum(u3) { n = 0, ne = 1, e = 2, se = 3, s = 4, sw = 5, w = 6, nw = 7 };

const Bitboard = @This();
const std = @import("std");
const assert = std.debug.assert;
const lb = @import("../lb.zig");
const Color = lb.Color;
const Square = lb.Square;
