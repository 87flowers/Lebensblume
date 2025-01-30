pub const pawn = genRelative(struct {
    fn op(piece: Bitboard, active_color: Color) Bitboard {
        return piece.shiftRelative(.n, active_color);
    }
}.op);

pub const knight = genRelative(struct {
    fn op(piece: Bitboard, active_color: Color) Bitboard {
        var result = Bitboard{};
        result.or_with(piece.shiftRelative(.n, active_color).shiftRelative(.nw, active_color));
        result.or_with(piece.shiftRelative(.n, active_color).shiftRelative(.ne, active_color));
        return result;
    }
}.op);

pub const silver = genRelative(struct {
    fn op(piece: Bitboard, active_color: Color) Bitboard {
        var result = Bitboard{};
        result.or_with(piece.shiftRelative(.nw, active_color));
        result.or_with(piece.shiftRelative(.n, active_color));
        result.or_with(piece.shiftRelative(.ne, active_color));
        result.or_with(piece.shiftRelative(.sw, active_color));
        result.or_with(piece.shiftRelative(.se, active_color));
        return result;
    }
}.op);

pub const gold = genRelative(struct {
    fn op(piece: Bitboard, active_color: Color) Bitboard {
        var result = Bitboard{};
        result.or_with(piece.shiftRelative(.nw, active_color));
        result.or_with(piece.shiftRelative(.n, active_color));
        result.or_with(piece.shiftRelative(.ne, active_color));
        result.or_with(piece.shiftRelative(.w, active_color));
        result.or_with(piece.shiftRelative(.e, active_color));
        result.or_with(piece.shiftRelative(.s, active_color));
        return result;
    }
}.op);

pub const king = gen(struct {
    fn op(piece: Bitboard) Bitboard {
        var result = Bitboard{};
        result.or_with(piece.shift(.n));
        result.or_with(piece.shift(.ne));
        result.or_with(piece.shift(.e));
        result.or_with(piece.shift(.se));
        result.or_with(piece.shift(.s));
        result.or_with(piece.shift(.sw));
        result.or_with(piece.shift(.w));
        result.or_with(piece.shift(.nw));
        return result;
    }
}.op);

fn gen(op: fn (Bitboard) Bitboard) [81]Bitboard {
    var result: [81]Bitboard = @splat(.{});
    for (0..81) |sq| {
        const piece = Bitboard.fromSq(sq);
        result[sq] = op(piece);
    }
    return result;
}

fn genRelative(op: fn (Bitboard, Color) Bitboard) [2][81]Bitboard {
    var result: [2][81]Bitboard = @splat(@splat(.{}));
    for ([2]Color{ .sente, .gote }) |active_color| {
        for (0..81) |sq| {
            const piece = Bitboard.fromSq(sq);
            result[@intFromEnum(active_color)][sq] = op(piece, active_color);
        }
    }
    return result;
}

const lb = @import("../lb.zig");
const Bitboard = lb.Bitboard;
const Color = lb.Color;
