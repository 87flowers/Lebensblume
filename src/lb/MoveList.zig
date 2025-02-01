moves: Moves = Moves.init(0) catch unreachable,

pub fn generateMoves(self: *MoveList, board: *const Board) void {
    switch (board.checkers.count()) {
        0 => generateMovesNoCheckers(self, board),
        1 => generateMovesOneChecker(self, board),
        else => generateMovesTwoCheckers(self, board),
    }
}

fn generateMovesNoCheckers(self: *MoveList, board: *const Board) void {
    _ = .{ self, board };
}

fn generateMovesOneChecker(self: *MoveList, board: *const Board) void {
    _ = .{ self, board };
}

fn generateMovesTwoCheckers(self: *MoveList, board: *const Board) void {
    _ = .{ self, board };
}

const Moves = std.BoundedArray(lb.Move, lb.max_legal_moves);

const MoveList = @This();
const std = @import("std");
const assert = std.debug.assert;
const lb = @import("../lb.zig");
const Board = lb.Board;
