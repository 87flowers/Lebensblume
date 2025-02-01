pub inline fn rayBetween(from: Square, to: Square) Bitboard {
    return ray_between_table[from.raw][to.raw];
}

pub inline fn rayInfinite(from: Square, to: Square) Bitboard {
    return ray_infinite_table[from.raw][to.raw];
}

fn genRayBetween() [81][81]Bitboard {
    comptime {
        @setEvalBranchQuota(100_000);
        var result: [81][81]Bitboard = @splat(@splat(.{}));
        for (0..81) |from_sq| {
            const from = Bitboard.fromSq(Square.make(from_sq));
            for ([_]Direction{ .n, .ne, .e, .se, .s, .sw, .w, .nw }) |dir| {
                var bb = Bitboard{};
                var to = from.shift(dir);
                while (to.raw != 0) : (to = to.shift(dir)) {
                    result[from.toSq().raw][to.toSq().raw] = bb;
                    bb.orWith(to);
                }
            }
        }
        return result;
    }
}

fn genRayInfinite() [81][81]Bitboard {
    comptime {
        @setEvalBranchQuota(100_000);
        var result: [81][81]Bitboard = @splat(@splat(.{}));
        for (0..81) |from_sq| {
            const from = Bitboard.fromSq(Square.make(from_sq));
            for ([_]Direction{ .n, .ne, .e, .se, .s, .sw, .w, .nw }) |dir| {
                var bb = Bitboard{};
                var to = from.shift(dir);
                while (to.raw != 0) : (to = to.shift(dir)) {
                    bb.orWith(to);
                }
                to = from.shift(dir);
                while (to.raw != 0) : (to = to.shift(dir)) {
                    result[from.toSq().raw][to.toSq().raw] = bb;
                }
            }
        }
        return result;
    }
}

const ray_between_table: [81][81]Bitboard = genRayBetween();
const ray_infinite_table: [81][81]Bitboard = genRayInfinite();

const lb = @import("../lb.zig");
const Bitboard = lb.Bitboard;
const Direction = lb.Bitboard.Direction;
const Square = lb.Square;
