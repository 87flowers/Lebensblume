board: lb.Board = lb.Board.defaultBoard(),

pub fn setPositionDefault(self: *Game) void {
    self.board = lb.Board.defaultBoard();
}

pub fn setPosition(self: *Game, board: lb.Board) void {
    self.board = board;
}

const Game = @This();
const lb = @import("../lb.zig");
