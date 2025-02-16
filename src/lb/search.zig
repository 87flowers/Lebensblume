pub fn go(out: anytype, game: *Game, pv: anytype) !i32 {
    comptime assert(@typeInfo(@TypeOf(pv)) == .pointer);
    var moves = MoveList{};
    moves.generateMoves(&game.board);
    const move = moves.moves.get(game.rand.random().uintLessThan(usize, moves.moves.len));
    pv.write(move, &.{});
    try out.raw("info depth 0 score 0 nodes {} pv {}\n", .{ moves.moves.len, move });
    return 0;
}

const std = @import("std");
const assert = std.debug.assert;
const lb = @import("../lb.zig");
const Game = lb.Game;
const MoveList = lb.MoveList;
