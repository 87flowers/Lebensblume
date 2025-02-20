pub fn Control(comptime config: struct {
    time: bool = false,
    depth: bool = false,
    nodes: bool = false,
    stats: bool = false,
}) type {
    return struct {
        timer: std.time.Timer,
        last_time_check: u64,
        nodes: u64,
        time_limit: if (config.time) struct { soft: u64, hard: u64 } else void,
        depth_limit: if (config.depth) struct { target_depth: i32 } else void,
        nodes_limit: if (config.nodes) struct { soft: u64, hard: u64 } else void,

        pub fn init(args: anytype) @This() {
            return .{
                .timer = std.time.Timer.start() catch @panic("timer unsupported on platform"),
                .nodes = 0,
                .last_time_check = 0,
                .time_limit = if (config.time) .{ .soft = args.soft_time, .hard = args.hard_time } else {},
                .depth_limit = if (config.depth) .{ .target_depth = args.target_depth } else {},
                .nodes_limit = if (config.nodes) .{ .soft = args.soft_nodes, .hard = args.hard_nodes } else {},
            };
        }

        pub fn reset(self: *@This()) void {
            _ = self.timer.lap();
            self.last_time_check = 0;
            self.nodes = 0;
        }

        pub fn nodeVisited(self: *@This()) void {
            self.nodes += 1;
        }

        /// Returns true if we should terminate the search
        pub fn checkSoftTermination(self: *@This(), depth: i32) bool {
            if (config.time and self.time_limit.soft <= self.timer.read()) return true;
            if (config.depth and depth >= self.depth_limit.target_depth) return true;
            if (config.nodes and self.nodes >= self.nodes_limit.soft) return true;
            return false;
        }

        /// Raises SearchError.EarlyTermination if we should terminate the search
        pub fn checkHardTermination(self: *@This()) SearchError!void {
            if (config.time and self.last_time_check + 1000 < self.nodes) {
                if (self.time_limit.hard <= self.timer.read()) {
                    @branchHint(.unlikely);
                    return SearchError.EarlyTermination;
                }
                self.last_time_check = self.nodes;
            }
            if (config.nodes and self.nodes >= self.nodes_limit.hard) {
                @branchHint(.unlikely);
                return SearchError.EarlyTermination;
            }
        }
    };
}

pub const TimeControl = Control(.{ .time = true });
pub const DepthControl = Control(.{ .depth = true });

pub fn go(out: anytype, game: *Game, ctrl: anytype, pv: anytype) !i32 {
    comptime assert(@typeInfo(@TypeOf(ctrl)) == .pointer);
    comptime assert(@typeInfo(@TypeOf(pv)) == .pointer);
    var depth: i32 = 1;
    var score: i32 = undefined;
    var current_pv = pv.new();
    while (depth < lb.max_search_ply) : (depth += 1) {
        score = forDepth(game, ctrl, &current_pv, depth, score) catch {
            try out.info(depth, score, ctrl, pv, .early_termination);
            break;
        };
        pv.copyFrom(&current_pv);
        try out.info(depth, score, ctrl, pv, .normal);
        if (ctrl.checkSoftTermination(depth)) break;
    }
    return score;
}

fn forDepth(game: *Game, ctrl: anytype, pv: anytype, depth: i32, prev_score: i32) SearchError!i32 {
    _ = prev_score;

    const min_window = -std.math.maxInt(i32);
    const max_window = std.math.maxInt(i32);
    return try search(game, ctrl, pv, min_window, max_window, 0, depth);
}

fn eval(board: *const Board) lb.Score {
    const piece_values = [_]lb.Score{
        100,
        800,
        1000,
        300,
        400,
        500,
        600,
        0,
        650,
        1000,
        1200,
        600,
    };

    var result: lb.Score = 0;
    for (piece_values, board.pieces) |value, bb| {
        const sente_count = Bitboard.@"and"(bb, board.getColor(.sente)).count();
        const gote_count = Bitboard.@"and"(bb, board.getColor(.sente)).count();
        result += @as(lb.Score, @intCast(sente_count)) * value;
        result -= @as(lb.Score, @intCast(gote_count)) * value;
    }

    result += @as(lb.Score, board.hand_mailbox[0].pawn) * 100;
    result -= @as(lb.Score, board.hand_mailbox[1].pawn) * 100;
    result += @as(lb.Score, board.hand_mailbox[0].bishop) * 800;
    result -= @as(lb.Score, board.hand_mailbox[1].bishop) * 800;
    result += @as(lb.Score, board.hand_mailbox[0].rook) * 1000;
    result -= @as(lb.Score, board.hand_mailbox[1].rook) * 1000;
    result += @as(lb.Score, board.hand_mailbox[0].lance) * 300;
    result -= @as(lb.Score, board.hand_mailbox[1].lance) * 300;
    result += @as(lb.Score, board.hand_mailbox[0].knight) * 400;
    result -= @as(lb.Score, board.hand_mailbox[1].knight) * 400;
    result += @as(lb.Score, board.hand_mailbox[0].silver) * 500;
    result -= @as(lb.Score, board.hand_mailbox[1].silver) * 500;
    result += @as(lb.Score, board.hand_mailbox[0].gold) * 600;
    result -= @as(lb.Score, board.hand_mailbox[1].gold) * 600;

    return switch (board.active_color) {
        .sente => result,
        .gote => -result,
    };
}

fn search(game: *Game, ctrl: anytype, pv: anytype, alpha_orig: i32, beta: i32, ply: u32, depth: i32) SearchError!lb.Score {
    var alpha = alpha_orig;

    if (depth <= 0) return eval(game.board()); // TODO: return eval
    if (ply >= lb.max_search_ply) return eval(game.board()); // TODO: return eval if not in check

    try ctrl.checkHardTermination();

    var best_score: lb.Score = std.math.minInt(lb.Score);

    var moves = MoveList{};
    moves.generateMoves(game.board());

    if (moves.moves.len == 0) return -std.math.maxInt(lb.Score);

    for (moves.moves.slice()) |m| {
        game.move(m);
        defer game.unmove();

        var child_pv = pv.newChild();
        const child_score = blk: {
            if (game.checkRepetition()) |repscore| break :blk repscore;
            break :blk -try search(game, ctrl, &child_pv, -beta, -alpha, ply + 1, depth - 1);
        };

        ctrl.nodeVisited();

        if (child_score > best_score) {
            best_score = child_score;
            if (child_score > alpha) {
                alpha = child_score;
                pv.write(m, &child_pv);
                if (alpha >= beta) break;
            }
        }
    }

    return best_score;
}

const SearchError = error{EarlyTermination};

const std = @import("std");
const assert = std.debug.assert;
const lb = @import("../lb.zig");
const Bitboard = lb.Bitboard;
const Board = lb.Board;
const Game = lb.Game;
const MoveList = lb.MoveList;
