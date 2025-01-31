pub const sliders = @import("attacks/sliders.zig");

pub const pawn = genRelative(struct {
    fn op(piece: Bitboard, active_color: Color) Bitboard {
        return piece.shiftRelative(.n, active_color);
    }
}.op);

pub const knight = genRelative(struct {
    fn op(piece: Bitboard, active_color: Color) Bitboard {
        var result = Bitboard{};
        result.orWith(piece.shiftRelative(.n, active_color).shiftRelative(.nw, active_color));
        result.orWith(piece.shiftRelative(.n, active_color).shiftRelative(.ne, active_color));
        return result;
    }
}.op);

pub const silver = genRelative(struct {
    fn op(piece: Bitboard, active_color: Color) Bitboard {
        var result = Bitboard{};
        result.orWith(piece.shiftRelative(.nw, active_color));
        result.orWith(piece.shiftRelative(.n, active_color));
        result.orWith(piece.shiftRelative(.ne, active_color));
        result.orWith(piece.shiftRelative(.sw, active_color));
        result.orWith(piece.shiftRelative(.se, active_color));
        return result;
    }
}.op);

pub const gold = genRelative(struct {
    fn op(piece: Bitboard, active_color: Color) Bitboard {
        var result = Bitboard{};
        result.orWith(piece.shiftRelative(.nw, active_color));
        result.orWith(piece.shiftRelative(.n, active_color));
        result.orWith(piece.shiftRelative(.ne, active_color));
        result.orWith(piece.shiftRelative(.w, active_color));
        result.orWith(piece.shiftRelative(.e, active_color));
        result.orWith(piece.shiftRelative(.s, active_color));
        return result;
    }
}.op);

pub const king = gen(struct {
    fn op(piece: Bitboard) Bitboard {
        var result = Bitboard{};
        result.orWith(piece.shift(.n));
        result.orWith(piece.shift(.ne));
        result.orWith(piece.shift(.e));
        result.orWith(piece.shift(.se));
        result.orWith(piece.shift(.s));
        result.orWith(piece.shift(.sw));
        result.orWith(piece.shift(.w));
        result.orWith(piece.shift(.nw));
        return result;
    }
}.op);

pub fn rook(sq: lb.Square, blockers: Bitboard) Bitboard {
    const mask = sliders.rook[sq].blocker_mask;
    const x = compressBlockers(blockers.raw & mask);
    const m = compressBlockers(mask);
    const index = pext(x, m);
    return Bitboard.make(sliders.rook[sq].dest_table[index]);
}

pub fn bishop(sq: lb.Square, blockers: Bitboard) Bitboard {
    const mask = sliders.bishop[sq].blocker_mask;
    const x = compressBlockers(blockers.raw & mask);
    const m = compressBlockers(mask);
    const index = pext(x, m);
    return Bitboard.make(sliders.bishop[sq].dest_table[index]);
}

pub fn lance(sq: lb.Square, lance_color: Color, blockers: Bitboard) Bitboard {
    const mask = sliders.lance[@intFromEnum(lance_color)][sq].blocker_mask;
    const x = compressBlockers(blockers.raw & mask);
    const m = compressBlockers(mask);
    const index = pext(x, m);
    return Bitboard.make(sliders.lance[@intFromEnum(lance_color)][sq].dest_table[index]);
}

inline fn pext(x: u64, m: u64) usize {
    if (@inComptime()) return pextComptime(x, m);
    return asm ("pext %[m], %[x], %[result]"
        : [result] "=r" (-> u64),
        : [x] "r" (x),
          [m] "r" (m),
    );
}

fn pextComptime(x: u64, m: u64) usize {
    var result: usize = 0;
    var bb: u64 = 1;
    var mask = m;
    while (mask != 0) : (bb += bb) {
        if (x & mask & -%mask != 0) result |= bb;
        mask &= mask - 1;
    }
    return result;
}

fn compressBlockers(bb: u81) u64 {
    const top: u64 = @as(u64, @intCast(bb >> 64)) << 1;
    const bot: u64 = @truncate(bb);
    return bot | top;
}

fn gen(comptime op: fn (Bitboard) Bitboard) [81]Bitboard {
    comptime {
        @setEvalBranchQuota(100_000);
        var result: [81]Bitboard = @splat(.{});
        for (0..81) |sq| {
            const piece = Bitboard.fromSq(sq);
            result[sq] = op(piece);
        }
        return result;
    }
}

fn genRelative(comptime op: fn (Bitboard, Color) Bitboard) [2][81]Bitboard {
    comptime {
        @setEvalBranchQuota(100_000);
        var result: [2][81]Bitboard = @splat(@splat(.{}));
        for ([2]Color{ .sente, .gote }) |active_color| {
            for (0..81) |sq| {
                const piece = Bitboard.fromSq(sq);
                result[@intFromEnum(active_color)][sq] = op(piece, active_color);
            }
        }
        return result;
    }
}

const lb = @import("../lb.zig");
const Bitboard = lb.Bitboard;
const Color = lb.Color;
