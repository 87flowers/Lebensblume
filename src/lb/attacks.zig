pub const sliders = @import("attacks/sliders.zig");

pub fn allPawns(pieces: Bitboard, pawn_color: Color) Bitboard {
    return pieces.shiftRelative(.n, pawn_color);
}

pub fn pawn(sq: lb.Square, pawn_color: Color) Bitboard {
    return pawn_table[@intFromEnum(pawn_color)][sq.raw];
}

pub fn allKnights(pieces: Bitboard, knight_color: Color) Bitboard {
    var result = Bitboard{};
    result.orWith(pieces.shiftRelative(.n, knight_color).shiftRelative(.nw, knight_color));
    result.orWith(pieces.shiftRelative(.n, knight_color).shiftRelative(.ne, knight_color));
    return result;
}

pub fn knight(sq: lb.Square, knight_color: Color) Bitboard {
    return knight_table[@intFromEnum(knight_color)][sq.raw];
}

pub fn allSilvers(pieces: Bitboard, silver_color: Color) Bitboard {
    var result = Bitboard{};
    result.orWith(pieces.shiftRelative(.nw, silver_color));
    result.orWith(pieces.shiftRelative(.n, silver_color));
    result.orWith(pieces.shiftRelative(.ne, silver_color));
    result.orWith(pieces.shiftRelative(.sw, silver_color));
    result.orWith(pieces.shiftRelative(.se, silver_color));
    return result;
}

pub fn silver(sq: lb.Square, silver_color: Color) Bitboard {
    return silver_table[@intFromEnum(silver_color)][sq.raw];
}

pub fn allGolds(pieces: Bitboard, gold_color: Color) Bitboard {
    var result = Bitboard{};
    result.orWith(pieces.shiftRelative(.nw, gold_color));
    result.orWith(pieces.shiftRelative(.n, gold_color));
    result.orWith(pieces.shiftRelative(.ne, gold_color));
    result.orWith(pieces.shiftRelative(.w, gold_color));
    result.orWith(pieces.shiftRelative(.e, gold_color));
    result.orWith(pieces.shiftRelative(.s, gold_color));
    return result;
}

pub fn gold(sq: lb.Square, gold_color: Color) Bitboard {
    return gold_table[@intFromEnum(gold_color)][sq.raw];
}

pub fn allKings(pieces: Bitboard) Bitboard {
    var result = Bitboard{};
    result.orWith(pieces.shift(.n));
    result.orWith(pieces.shift(.ne));
    result.orWith(pieces.shift(.e));
    result.orWith(pieces.shift(.se));
    result.orWith(pieces.shift(.s));
    result.orWith(pieces.shift(.sw));
    result.orWith(pieces.shift(.w));
    result.orWith(pieces.shift(.nw));
    return result;
}

pub fn king(sq: lb.Square) Bitboard {
    return king_table[sq.raw];
}

pub fn allRooks(pieces: Bitboard, blockers: Bitboard) Bitboard {
    var result = Bitboard{};
    var pieces_iterator = pieces.iterateSquares();
    while (pieces_iterator.next()) |sq| {
        result.orWith(rook(sq, blockers));
    }
    return result;
}

pub fn rook(sq: lb.Square, blockers: Bitboard) Bitboard {
    const mask = sliders.rook[sq.raw].blocker_mask;
    const x = compressBlockers(blockers.raw & mask);
    const m = compressBlockers(mask);
    const index = pext(x, m);
    return Bitboard.make(sliders.rook[sq.raw].dest_table[index]);
}

pub fn allBishops(pieces: Bitboard, blockers: Bitboard) Bitboard {
    var result = Bitboard{};
    var pieces_iterator = pieces.iterateSquares();
    while (pieces_iterator.next()) |sq| {
        result.orWith(bishop(sq, blockers));
    }
    return result;
}

pub fn bishop(sq: lb.Square, blockers: Bitboard) Bitboard {
    const mask = sliders.bishop[sq.raw].blocker_mask;
    const x = compressBlockers(blockers.raw & mask);
    const m = compressBlockers(mask);
    const index = pext(x, m);
    return Bitboard.make(sliders.bishop[sq.raw].dest_table[index]);
}

pub fn allLances(pieces: Bitboard, lance_color: Color, blockers: Bitboard) Bitboard {
    var result = Bitboard{};
    var pieces_iterator = pieces.iterateSquares();
    while (pieces_iterator.next()) |sq| {
        result.orWith(lance(sq, lance_color, blockers));
    }
    return result;
}

pub fn lance(sq: lb.Square, lance_color: Color, blockers: Bitboard) Bitboard {
    const mask = sliders.lance[@intFromEnum(lance_color)][sq.raw].blocker_mask;
    const x = compressBlockers(blockers.raw & mask);
    const m = compressBlockers(mask);
    const index = pext(x, m);
    return Bitboard.make(sliders.lance[@intFromEnum(lance_color)][sq.raw].dest_table[index]);
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
    comptime {
        var result: usize = 0;
        var bb: u64 = 1;
        var mask = m;
        while (mask != 0) : (bb += bb) {
            if (x & mask & -%mask != 0) result |= bb;
            mask &= mask - 1;
        }
        return result;
    }
}

inline fn compressBlockers(bb: u81) u64 {
    const top: u64 = @as(u64, @intCast(bb >> 64)) << 1;
    const bot: u64 = @truncate(bb);
    return bot | top;
}

fn gen(comptime op: fn (Bitboard) Bitboard) [81]Bitboard {
    comptime {
        @setEvalBranchQuota(100_000);
        var result: [81]Bitboard = @splat(.{});
        for (0..81) |sq| {
            const piece = Bitboard.fromSq(Square.make(sq));
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
                const piece = Bitboard.fromSq(Square.make(sq));
                result[@intFromEnum(active_color)][sq] = op(piece, active_color);
            }
        }
        return result;
    }
}

const pawn_table = genRelative(allPawns);
const knight_table = genRelative(allKnights);
const silver_table = genRelative(allSilvers);
const gold_table = genRelative(allGolds);
const king_table = gen(allKings);

const lb = @import("../lb.zig");
const Bitboard = lb.Bitboard;
const Color = lb.Color;
const Square = lb.Square;
