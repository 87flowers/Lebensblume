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

fn eval(board: *const Board) Score {
    const piece_values = [_]Score{
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

    var result: Score = 0;
    for (piece_values, board.pieces) |value, bb| {
        const sente_count = Bitboard.@"and"(bb, board.getColor(.sente)).count();
        const gote_count = Bitboard.@"and"(bb, board.getColor(.sente)).count();
        result += @as(Score, @intCast(sente_count)) * value;
        result -= @as(Score, @intCast(gote_count)) * value;
    }

    result += @as(Score, board.hand_mailbox[0].pawn) * 100;
    result -= @as(Score, board.hand_mailbox[1].pawn) * 100;
    result += @as(Score, board.hand_mailbox[0].bishop) * 800;
    result -= @as(Score, board.hand_mailbox[1].bishop) * 800;
    result += @as(Score, board.hand_mailbox[0].rook) * 1000;
    result -= @as(Score, board.hand_mailbox[1].rook) * 1000;
    result += @as(Score, board.hand_mailbox[0].lance) * 300;
    result -= @as(Score, board.hand_mailbox[1].lance) * 300;
    result += @as(Score, board.hand_mailbox[0].knight) * 400;
    result -= @as(Score, board.hand_mailbox[1].knight) * 400;
    result += @as(Score, board.hand_mailbox[0].silver) * 500;
    result -= @as(Score, board.hand_mailbox[1].silver) * 500;
    result += @as(Score, board.hand_mailbox[0].gold) * 600;
    result -= @as(Score, board.hand_mailbox[1].gold) * 600;

    const sente_king = board.getKingSq(.sente);
    const sente_king_rank = sente_king.raw / 9;
    const sente_king_file = sente_king.raw % 9;
    const gote_king = board.getKingSq(.gote);
    const gote_king_rank = gote_king.raw / 9;
    const gote_king_file = gote_king.raw % 9;

    const sente_attack = board.getAttackMap(.sente);
    const gote_attack = board.getAttackMap(.gote);

    for (0..9) |rank| {
        for (0..9) |file| {
            const sq = lb.Square.make(@intCast(file + rank * 9)).bitboard();
            const sente_count: usize = @intFromBool(!sente_attack.@"and"(sq).empty());
            const gote_count: usize = @intFromBool(!gote_attack.@"and"(sq).empty());

            const sente_dist = @max(abs_diff(sente_king_rank, rank), abs_diff(sente_king_file, file));
            const gote_dist = @max(abs_diff(gote_king_rank, rank), abs_diff(gote_king_file, file));

            result += @intCast(sente_count * 50 / (1 + sente_dist));
            result -= @intCast(gote_count * 90 / (1 + sente_dist));
            result -= @intCast(gote_count * 50 / (1 + gote_dist));
            result += @intCast(sente_count * 90 / (1 + gote_dist));
        }
    }

    return switch (board.active_color) {
        .sente => result,
        .gote => -result,
    };
}

fn abs_diff(a: usize, b: usize) usize {
    return if (a > b) a - b else b - a;
}

fn search(game: *Game, ctrl: anytype, pv: anytype, alpha_orig: i32, beta: i32, ply: u32, depth: i32) SearchError!Score {
    var alpha = alpha_orig;

    if (depth <= 0) return eval(game.board()); // TODO: return eval
    if (ply >= lb.max_search_ply) return eval(game.board()); // TODO: return eval if not in check

    try ctrl.checkHardTermination();

    var best_move = Move.none;
    var best_score: Score = std.math.minInt(Score);

    const tte = game.ttLoad();

    var moves = MoveList{};
    moves.generateMoves(game.board());
    game.sortMoves(&moves, tte.move);

    if (moves.moves.len == 0) return -std.math.maxInt(Score);

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
            best_move = m;
            if (child_score > alpha) {
                alpha = child_score;
                pv.write(m, &child_pv);
                if (alpha >= beta) break;
            }
        }
    }

    if (@abs(best_score) >= 1073741824) best_score -= std.math.sign(best_score);

    game.ttStore(.{
        .depth = @intCast(depth),
        .best_move = best_move,
        .bound = if (best_score >= beta)
            .lower
        else if (best_score <= alpha_orig)
            .upper
        else
            .exact,
        .score = best_score,
    });

    return best_score;
}

const SearchError = error{EarlyTermination};

const std = @import("std");
const assert = std.debug.assert;
const lb = @import("../lb.zig");
const Bitboard = lb.Bitboard;
const Board = lb.Board;
const Game = lb.Game;
const Move = lb.Move;
const MoveList = lb.MoveList;
const Score = lb.Score;
