moves: Moves = Moves.init(0) catch unreachable,

pub fn generateMoves(self: *MoveList, board: *const Board) void {
    switch (board.checkers.count()) {
        0 => generateMovesNoCheckers(self, board),
        1 => generateMovesOneChecker(self, board),
        else => generateMovesTwoCheckers(self, board),
    }
}

fn generateMovesNoCheckers(self: *MoveList, board: *const Board) void {
    generateKingMoves(self, board);
}

fn generateMovesOneChecker(self: *MoveList, board: *const Board) void {
    generateKingMoves(self, board);
}

fn generateMovesTwoCheckers(self: *MoveList, board: *const Board) void {
    _ = .{ self, board };
}

fn generateKingMoves(self: *MoveList, board: *const Board) void {
    const king_sq = board.getPieces(board.active_color, .king).toSq();
    const king_moves = lb.attacks.king(king_sq).@"and"(board.danger.invert());
    self.splatNormalMoves(king_sq, king_moves);
}

fn splatNormalMoves(self: *MoveList, from: Square, to_bb: Bitboard) void {
    var iterator = to_bb.iterateSquares();
    while (iterator.next()) |to| {
        self.moves.appendAssumeCapacity(Move.makeMove(from, to, false));
    }
}

const Moves = std.BoundedArray(lb.Move, lb.max_legal_moves);

const MoveList = @This();
const std = @import("std");
const assert = std.debug.assert;
const lb = @import("../lb.zig");
const Bitboard = lb.Bitboard;
const Board = lb.Board;
const Move = lb.Move;
const PieceType = lb.PieceType;
const Square = lb.Square;
