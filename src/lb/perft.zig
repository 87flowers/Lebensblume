fn core(board: *lb.Board, depth: usize) usize {
    if (depth == 0) return 1;
    var result: usize = 0;
    var moves = lb.MoveList{};
    moves.generateMoves(board);
    for (moves.moves.slice()) |m| {
        var new_board = board.*;
        new_board.move(m);
        result += core(&new_board, depth - 1);
    }
    return result;
}

pub fn go(out: anytype, board: *lb.Board, depth: usize) !usize {
    if (depth == 0) return 1;

    var result: usize = 0;
    var moves = lb.MoveList{};
    var timer = std.time.Timer.start() catch @panic("no timer available");
    moves.generateMoves(board);
    for (moves.moves.slice()) |m| {
        var new_board = board.*;
        new_board.move(m);
        const p = core(&new_board, depth - 1);
        result += p;
        try out.raw("{}: {}\n", .{ m, p });
        try out.flush();
    }
    const elapsed: f64 = @floatFromInt(timer.read());
    try out.raw("Nodes searched (depth {}): {}\n", .{ depth, result });
    try out.raw("Search completed in {d:.1}ms\n", .{elapsed / std.time.ns_per_ms});
    try out.flush();

    return result;
}

const std = @import("std");
const lb = @import("../lb.zig");
const util = @import("../util.zig");
