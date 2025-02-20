board_stack: Stack(Board),
hash_stack: Stack(Hash),
search_stack: Stack(SS),
tt: TT,

pub fn reset(game: *Game) void {
    game.setPositionDefault();
    game.tt.clear();
}

pub fn board(game: *Game) *const Board {
    return game.mutBoard();
}

pub fn ss(game: *Game) *SS {
    return &game.search_stack.slice()[game.search_stack.len - 1];
}

fn mutBoard(game: *Game) *Board {
    return &game.board_stack.slice()[game.board_stack.len - 1];
}

pub fn move(game: *Game, m: Move) void {
    game.board_stack.append(game.board().*) catch @panic("board stack overflow");
    const new_board: *Board = game.mutBoard();
    new_board.move(m);

    game.hash_stack.append(new_board.hash) catch @panic("hash stack overflow");

    const old_check_count = game.ss().continuous_check_count;
    game.search_stack.append(.{}) catch @panic("search stack overflow");
    if (new_board.isInCheck()) {
        if (old_check_count) |old_count| {
            game.ss().continuous_check_count = old_count + 2;
        } else {
            game.ss().continuous_check_count = 0;
        }
    }
}

pub fn unmove(game: *Game) void {
    _ = game.board_stack.pop();
    _ = game.hash_stack.pop();
    _ = game.search_stack.pop();
}

pub fn checkRepetition(game: *Game) ?Score {
    const hash = game.board().hash;
    const check_for_check = game.board().isInCheck() and game.ss().continuous_check_count != null;

    var i: usize = if (game.hash_stack.len >= 16)
        game.hash_stack.len - 16
    else
        @intFromBool(game.board().active_color != game.board_stack.get(0).active_color);

    while (i < game.hash_stack.len - 1) : (i += 2) {
        if (game.hash_stack.get(i) == hash) {
            const dist = game.hash_stack.len - i;
            if (check_for_check and dist >= game.ss().continuous_check_count.?) return std.math.minInt(Score);
            return 0;
        }
    }

    return null;
}

pub fn ttLoad(game: *Game) TT.Entry {
    return game.tt.load(game.board().hash);
}

pub fn ttStore(game: *Game, arg: struct {
    depth: u7,
    best_move: Move,
    bound: TT.Bound,
    score: Score,
}) void {
    game.tt.store(game.board().hash, arg.depth, arg.best_move, arg.bound, arg.score);
}

pub fn sortMoves(_: *Game, moves: *MoveList, tt_move: Move) void {
    if (std.mem.indexOfScalar(Move, moves.moves.slice(), tt_move)) |index| {
        std.mem.swap(Move, &moves.moves.slice()[0], &moves.moves.slice()[index]);
    }
}

pub fn setPositionDefault(game: *Game) void {
    game.board_stack.resize(1) catch unreachable;
    game.hash_stack.resize(1) catch unreachable;
    game.search_stack.resize(1) catch unreachable;
    game.board_stack.set(0, Board.defaultBoard());
    game.hash_stack.set(0, Board.defaultBoard().hash);
    game.search_stack.set(0, .{});
}

pub fn setPosition(game: *Game, position: lb.Board) void {
    game.board_stack.resize(1) catch unreachable;
    game.hash_stack.resize(1) catch unreachable;
    game.search_stack.resize(1) catch unreachable;
    game.board_stack.set(0, position);
    game.hash_stack.set(0, position.hash);
    game.search_stack.set(0, .{ .continuous_check_count = if (position.isInCheck()) 0 else null });
}

const SS = struct {
    continuous_check_count: ?u8 = null,
};

fn Stack(T: type) type {
    return std.BoundedArray(T, lb.max_game_ply);
}

const Game = @This();
const std = @import("std");
const lb = @import("../lb.zig");
const Board = lb.Board;
const Hash = lb.Hash;
const Move = lb.Move;
const MoveList = lb.MoveList;
const Score = lb.Score;
const TT = lb.TT;
