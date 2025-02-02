pub inline fn raw(_: @This(), comptime fmt: []const u8, arg: anytype) !void { std.debug.print(fmt, arg); }
pub inline fn flush(_: @This()) !void {}
pub fn protocolError(_: @This(), _: []const u8, comptime _: []const u8, _: anytype) !void {}
pub fn unrecognisedToken(_: @This(), comptime _: []const u8, _: []const u8) !void {}
pub fn illegalMoveString(_: @This(), _: []const u8) !void {}
pub fn illegalMove(_: @This(), _: lb.Move) !void {}
pub inline fn pong(_: @This()) !void {}
pub inline fn bestmove(_: @This(), _: ?lb.Move) !void {}
pub inline fn eval(_: @This(), _: lb.Score) !void {}
pub inline fn info(_: @This(), _: i32, _: lb.Score, _: anytype, _: anytype, comptime _: enum { normal, early_termination }) !void {}

const std = @import("std");
const lb = @import("../lb.zig");
