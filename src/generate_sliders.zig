pub fn main() !void {
    var bufw = std.io.bufferedWriter(std.io.getStdOut().writer());
    const output = bufw.writer();
    try output.print("{s}\n", .{
        \\pub const SliderTable = struct {
        \\    blocker_mask: u128,
        \\    dest_table: []const u128,
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
        const piece = Bitboard.fromSq(Square.make(@intCast(sq)));
        const blockers = genPotentialBlockers(piece, directions);
        // const moves_no_blockers = genMoves(piece, directions, Bitboard{});
        try output.print("{s}.{{\n", .{indent});
        try output.print("{s}    .blocker_mask = 0x{x},\n", .{ indent, blockers.raw });
        try output.print("{s}    .dest_table = &[_]u128{{\n", .{indent});
        const m = compress(blockers.raw, blockers.raw);
        var i: u64 = 0;
        while (true) {
            const current = Bitboard.make(decompress(pdep(i, m), blockers.raw));
            if (current.raw == 0 and i != 0) break;
            const moves = genMoves(piece, directions, current);
            try output.print("{s}        0x{x},\n", .{ indent, moves.raw });
            assert(i == pext(compress(current.raw, blockers.raw), compress(blockers.raw, blockers.raw)));
            i += 1;
        }
        try output.print("{s}    }},\n", .{indent});
        try output.print("{s}}},\n", .{indent});
    }
}

inline fn pext(x: u64, m: u64) usize {
    return asm ("pext %[m], %[x], %[result]"
        : [result] "=r" (-> u64),
        : [x] "r" (x),
          [m] "r" (m),
    );
}

inline fn pdep(x: u64, m: u64) usize {
    return asm ("pdep %[m], %[x], %[result]"
        : [result] "=r" (-> u64),
        : [x] "r" (x),
          [m] "r" (m),
    );
}

const compression_shift = 1;

fn compress(x: u128, mask: u128) u64 {
    assert(x & ~mask == 0);
    const top: u64 = @as(u64, @intCast(x >> 64)) << compression_shift;
    const bot: u64 = @truncate(x);
    // std.debug.print("{b} {b} {b} {b}\n", .{top, bot, x, mask});
    assert(bot & top == 0);
    assert(top & mask == 0);
    const result = bot | top;
    return result;
}

fn decompress(y: u64, mask: u128) u128 {
    const bot: u128 = y & mask;
    const top: u128 = (@as(u128, y >> compression_shift) << 64) & mask;
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
    const piece = Bitboard.fromSq(Square.make(0));
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
const Square = lb.Square;
