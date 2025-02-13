moves: Moves = Moves.init(0) catch unreachable,

pub fn generateMoves(self: *MoveList, board: *const Board) void {
    switch (board.checkers.count()) {
        0 => generateMovesNoCheckers(self, board),
        1 => generateMovesOneChecker(self, board),
        else => generateMovesTwoCheckers(self, board),
    }
}

fn generateMovesNoCheckers(self: *MoveList, board: *const Board) void {
    const valid_dests = board.getColor(board.active_color).invert();
    const valid_drop_dests = board.getOccupied().invert();
    generateNonKingMoves(self, board, valid_dests);
    generateKingMoves(self, board);
    generateDrops(self, board, valid_drop_dests);
}

fn generateMovesOneChecker(self: *MoveList, board: *const Board) void {
    const valid_dests = lb.rays.rayBetween(board.getKingSq(board.active_color), board.checkers.toSq()).@"or"(board.checkers);
    generateNonKingMoves(self, board, valid_dests);
    generateKingMoves(self, board);
    generateDrops(self, board, valid_dests.@"and"(board.checkers.invert()));
}

fn generateMovesTwoCheckers(self: *MoveList, board: *const Board) void {
    generateKingMoves(self, board);
}

fn generateDrops(self: *MoveList, board: *const Board, valid_dests: Bitboard) void {
    var hand_ptypes = board.hand[@intFromEnum(board.active_color)];

    // Pawn Drops
    if (hand_ptypes & 1 == 1) {
        hand_ptypes &= hand_ptypes - 1;
        const valid_normal_dests = validNormalDests(board.active_color, .pawn);
        const nifu_restriction = board.getPieces(board.active_color, .pawn).fillFile().invert();
        const enemy_king = board.getKing(board.active_color.invert());
        const potential_uchifuzume = enemy_king.shiftRelative(.n, board.active_color.invert());

        var drops = valid_dests.@"and"(valid_normal_dests).@"and"(nifu_restriction);
        if (!drops.@"and"(potential_uchifuzume).empty() and isUchifuzume(board, enemy_king.toSq(), potential_uchifuzume)) {
            drops = drops.@"and"(potential_uchifuzume.invert());
        }
        self.splatDrops(.pawn, drops);
    }

    // All other drops
    while (hand_ptypes != 0) : (hand_ptypes &= hand_ptypes - 1) {
        const ptype: PieceType = @enumFromInt(@ctz(hand_ptypes) + 1);
        const valid_normal_dests = validNormalDests(board.active_color, ptype);
        self.splatDrops(ptype, valid_dests.@"and"(valid_normal_dests));
    }
}

fn isUchifuzume(board: *const Board, enemy_king: Square, drop_bb: Bitboard) bool {
    const pawn_attackers = board.getAllNonKingAttackers(drop_bb.toSq(), board.active_color.invert());
    const nonpinned_pawn_attackers = pawn_attackers.@"and"(board.getPinned(board.active_color.invert()).invert());
    if (!nonpinned_pawn_attackers.empty()) return false;

    const ring = lb.attacks.king(enemy_king);
    const attack_map = board.getAttackMap(board.active_color).@"or"(board.getColor(board.active_color.invert()));
    return attack_map.@"and"(ring).raw == ring.raw;
}

test isUchifuzume {
    const cases = [_]struct { []const u8, bool, []const u8 } {
        .{ "9/9/7gp/7pk/9/7G1/9/PPPPPPPP1/K8 b P 1", true, "P*1e" },
        .{ "9/9/7pp/7sk/9/7G1/9/PPPPPPPP1/K8 b P 1", false, "P*1e" },
        .{ "9/9/8p/6K1k/9/7G1/9/PPPPPPPP1/9 b P 1", true, "P*1e" },
        .{ "9/9/8p/6K1k/9/9/9/PPPPPPPP1/9 b P 1", false, "P*1e" },
        .{ "9/9/7gp/1R5gk/9/7G1/9/PPPPPPPP1/K8 b P 1", true, "P*1e" },
        .{ "9/9/7gp/7gk/9/7G1/9/PPPPPPPP1/K8 b P 1", false, "P*1e" },
    };
    for (cases) |case| {
        const sfen, const is_uchifuzume, const move = case;
        const board = try Board.parse(sfen);
        var moves = MoveList{};
        moves.generateMoves(&board);
        const has_move = std.mem.count(Move, moves.moves.slice(), &.{try Move.parse(move)});
        try std.testing.expectEqual(!is_uchifuzume, has_move > 0);
    }
}

fn generateKingMoves(self: *MoveList, board: *const Board) void {
    const king_sq = board.getKingSq(board.active_color);
    const king_moves = lb.attacks.king(king_sq).@"and"(board.danger.invert()).@"and"(board.getColor(board.active_color).invert());
    self.splatNormalMoves(king_sq, king_moves);
}

fn generateNonKingMoves(self: *MoveList, board: *const Board, valid_dests: Bitboard) void {
    const color = board.active_color;
    const occupied = board.getOccupied();
    const pinned = board.pinned;
    const king_sq = board.getKingSq(board.active_color);

    self.generatePieceMoves(board, board.getPieces(color, .rook), king_sq, pinned, valid_dests, occupied, .rook, struct {
        fn op(from: Square, _: Color, blockers: Bitboard) Bitboard {
            return lb.attacks.rook(from, blockers);
        }
    }.op);

    self.generatePieceMoves(board, board.getPieces(color, .dragon), king_sq, pinned, valid_dests, occupied, .dragon, struct {
        fn op(from: Square, _: Color, blockers: Bitboard) Bitboard {
            return Bitboard.@"or"(lb.attacks.rook(from, blockers), lb.attacks.king(from));
        }
    }.op);

    self.generatePieceMoves(board, board.getPieces(color, .bishop), king_sq, pinned, valid_dests, occupied, .bishop, struct {
        fn op(from: Square, _: Color, blockers: Bitboard) Bitboard {
            return lb.attacks.bishop(from, blockers);
        }
    }.op);

    self.generatePieceMoves(board, board.getPieces(color, .horse), king_sq, pinned, valid_dests, occupied, .horse, struct {
        fn op(from: Square, _: Color, blockers: Bitboard) Bitboard {
            return Bitboard.@"or"(lb.attacks.bishop(from, blockers), lb.attacks.king(from));
        }
    }.op);

    self.generatePieceMoves(board, board.getPieces(color, .lance), king_sq, pinned, valid_dests, occupied, .lance, struct {
        fn op(from: Square, piece_color: Color, blockers: Bitboard) Bitboard {
            return lb.attacks.lance(from, piece_color, blockers);
        }
    }.op);

    const golds = board.getPieces(color, .gold).@"or"(board.getPieces(color, .tokin)).@"or"(board.getPromoteds(color));
    self.generatePieceMoves(board, golds, king_sq, pinned, valid_dests, occupied, .gold, struct {
        fn op(from: Square, piece_color: Color, _: Bitboard) Bitboard {
            return lb.attacks.gold(from, piece_color);
        }
    }.op);

    self.generatePieceMoves(board, board.getPieces(color, .silver), king_sq, pinned, valid_dests, occupied, .silver, struct {
        fn op(from: Square, piece_color: Color, _: Bitboard) Bitboard {
            return lb.attacks.silver(from, piece_color);
        }
    }.op);

    self.generatePieceMoves(board, board.getPieces(color, .knight), king_sq, pinned, valid_dests, occupied, .knight, struct {
        fn op(from: Square, piece_color: Color, _: Bitboard) Bitboard {
            return lb.attacks.knight(from, piece_color);
        }
    }.op);

    self.generatePieceMoves(board, board.getPieces(color, .pawn), king_sq, pinned, valid_dests, occupied, .pawn, struct {
        fn op(from: Square, piece_color: Color, _: Bitboard) Bitboard {
            return lb.attacks.pawn(from, piece_color);
        }
    }.op);
}

inline fn generatePieceMoves(self: *MoveList, board: *const Board, from_bb: Bitboard, king_sq: Square, pinned: Bitboard, valid_dests: Bitboard, blockers: Bitboard, comptime ptype: PieceType, op: anytype) void {
    var nonpinned_from_iterator = from_bb.@"and"(pinned.invert()).iterateSquares();
    while (nonpinned_from_iterator.next()) |from| {
        const to_bb = op(from, board.active_color, blockers).@"and"(valid_dests);
        if (comptime ptype.promotable()) {
            self.splatMaybePromoMoves(board.active_color, from, to_bb, ptype);
        } else {
            self.splatNormalMoves(from, to_bb);
        }
    }

    var pinned_from_iterator = from_bb.@"and"(pinned).iterateSquares();
    while (pinned_from_iterator.next()) |from| {
        const pin_ray = lb.rays.rayInfinite(king_sq, from);
        const to_bb = op(from, board.active_color, blockers).@"and"(valid_dests).@"and"(pin_ray);
        if (comptime ptype.promotable()) {
            self.splatMaybePromoMoves(board.active_color, from, to_bb, ptype);
        } else {
            self.splatNormalMoves(from, to_bb);
        }
    }
}

fn splatNormalMoves(self: *MoveList, from: Square, to_bb: Bitboard) void {
    var iterator = to_bb.iterateSquares();
    while (iterator.next()) |to| {
        self.moves.appendAssumeCapacity(Move.makeMove(from, to, false));
    }
}

fn splatMaybePromoMoves(self: *MoveList, active_color: Color, from: Square, to_bb: Bitboard, comptime ptype: PieceType) void {
    const valid_normal_dests = switch (active_color) {
        inline else => |color| comptime validNormalDests(color, ptype),
    };

    if (from.isPromoSquare(active_color)) {
        self.splatPromoMoves(from, to_bb);
        self.splatNormalMoves(from, Bitboard.@"and"(to_bb, valid_normal_dests));
    } else {
        self.splatPromoMoves(from, Bitboard.@"and"(to_bb, Bitboard.promoZone(active_color)));
        self.splatNormalMoves(from, Bitboard.@"and"(to_bb, valid_normal_dests));
    }
}

fn splatPromoMoves(self: *MoveList, from: Square, to_bb: Bitboard) void {
    var iterator = to_bb.iterateSquares();
    while (iterator.next()) |to| {
        self.moves.appendAssumeCapacity(Move.makeMove(from, to, true));
    }
}

fn splatDrops(self: *MoveList, ptype: PieceType, to_bb: Bitboard) void {
    var iterator = to_bb.iterateSquares();
    while (iterator.next()) |to| {
        self.moves.appendAssumeCapacity(Move.makeDrop(ptype, to));
    }
}

fn validNormalDests(active_color: Color, ptype: PieceType) Bitboard {
    return switch (active_color) {
        inline else => |color| switch (ptype) {
            .none => unreachable,
            .pawn, .lance => comptime Bitboard.rankRelative(8, color).invert(),
            .knight => comptime Bitboard.@"or"(Bitboard.rankRelative(7, color), Bitboard.rankRelative(8, color)).invert(),
            else => comptime (Bitboard{}).invert(),
        },
    };
}

const Moves = std.BoundedArray(lb.Move, lb.max_legal_moves);

const MoveList = @This();
const std = @import("std");
const assert = std.debug.assert;
const lb = @import("../lb.zig");
const Bitboard = lb.Bitboard;
const Board = lb.Board;
const Color = lb.Color;
const Move = lb.Move;
const PieceType = lb.PieceType;
const Square = lb.Square;
