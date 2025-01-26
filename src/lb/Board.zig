active_color: lb.Color = .sente,
ply: usize = 0,

pub fn verify(board: *const Board) void {
    _ = board;
}

fn ones(count: usize) u64 {
    return (@as(u64, 1) << @intCast(count)) -% 1;
}

pub fn move(board: *Board, m: lb.Move) void {
    _ = .{ board, m };
}

pub fn format(board: *const Board, comptime _: []const u8, _: std.fmt.FormatOptions, writer: anytype) !void {
    _ = .{ board, writer };
}

pub fn parse(str: []const u8) !Board {
    var it = std.mem.tokenizeAny(u8, str, " \t\r\n");
    const board_str = it.next() orelse return lb.ParseError.InvalidLength;
    const color = it.next() orelse return lb.ParseError.InvalidLength;
    const ply = it.next() orelse return lb.ParseError.InvalidLength;
    if (it.next() != null) return lb.ParseError.InvalidLength;
    return Board.parseParts(board_str, color, ply);
}

pub fn parseParts(board_str: []const u8, color_str: []const u8, ply_str: []const u8) !Board {
    _ = .{ board_str, color_str, ply_str };
    return undefined;
}

const Board = @This();
const std = @import("std");
const assert = std.debug.assert;
const lb = @import("../lb.zig");
