pub fn iterate(bb: anytype) struct {
    bitboard: @TypeOf(bb),
    pub fn next(self: *@This()) ?@TypeOf(@ctz(bb)) {
        if (self.bitboard == 0) return null;
        const result = @ctz(self.bitboard);
        self.bitboard &= self.bitboard - 1;
        return result;
    }
    pub fn nextBit(self: *@This()) ?@TypeOf(bb) {
        if (self.bitboard == 0) return null;
        const old = self.bitboard;
        self.bitboard &= self.bitboard - 1;
        return old & ~self.bitboard;
    }
} {
    comptime assert(@typeInfo(@TypeOf(bb)).int.signedness == .unsigned);
    return .{ .bitboard = bb };
}

// From: https://graphics.stanford.edu/~seander/bithacks.html#NextBitPermutation
pub fn nextPerm(x: anytype) @TypeOf(x) {
    comptime assert(@typeInfo(@TypeOf(x)).int.signedness == .unsigned);
    const t = x | (x -% 1);
    return (t +% 1) | (((~t & -%~t) - 1) >> (@ctz(x) + 1));
}

const std = @import("std");
const assert = std.debug.assert;
