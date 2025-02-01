pub inline fn ray(from: Square, to: Square) Bitboard {
    return ray_table[from.raw][to.raw];
}

fn gen() [81][81]Bitboard {
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
                    result[from.toSq().raw][to.toSq().raw] = bb;
                }
            }
        }
        return result;
    }
}

const ray_table: [81][81]Bitboard = gen();

const lb = @import("../lb.zig");
const Bitboard = lb.Bitboard;
const Direction = lb.Bitboard.Direction;
const Square = lb.Square;
