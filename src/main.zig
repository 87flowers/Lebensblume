var g = lb.Game{};

pub fn main() !void {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    const allocator = gpa.allocator();

    var usi = Usi{ .out = UsiOutput.init(std.io.bufferedWriter(std.io.getStdOut().writer())) };

    // Handle command line arguments
    {
        var args = try std.process.argsWithAllocator(allocator);
        defer args.deinit();
        // skip program name
        _ = args.skip();

        var has_arguments = false;
        while (args.next()) |arg| {
            has_arguments = true;
            try usi.usiParseCommand(arg);
            try usi.out.flush();
        }
        if (has_arguments) return;
    }

    // Handle stdin
    const buffer_size = lb.max_game_ply * 8;
    var input = util.lineReader(buffer_size, std.io.getStdIn().reader());
    while (try input.readLine()) |input_line| {
        try usi.usiParseCommand(input_line);
        try usi.out.flush();
    }
}

const Usi = struct {
    out: UsiOutput,

    const Iterator = std.mem.TokenIterator(u8, .any);

    pub fn usiParseCommand(self: *Usi, input_line: []const u8) !void {
        var it = std.mem.tokenizeAny(u8, input_line, " \t\r\n");
        const command = it.next() orelse return;
        if (std.mem.eql(u8, command, "position")) {
            try self.usiParsePosition(&it);
        } else if (std.mem.eql(u8, command, "perft")) {
            try self.usiParsePerft(&it);
        } else if (std.mem.eql(u8, command, "quit")) {
            std.process.exit(0);
        } else if (std.mem.eql(u8, command, "d")) {
            try g.board.prettyPrint(&self.out, .ja);
        } else if (std.mem.eql(u8, command, "de")) {
            try g.board.prettyPrint(&self.out, .en);
        } else if (std.mem.eql(u8, command, "danger")) {
            try g.board.danger.prettyPrint(&self.out, "    ");
        } else {
            try self.out.protocolError(command, "unknown command", .{});
        }
    }

    fn usiParsePosition(self: *Usi, it: *Iterator) !void {
        const pos_type = it.next() orelse
            return self.out.protocolError("position", "no position provided", .{});

        if (std.mem.eql(u8, pos_type, "startpos")) {
            g.setPositionDefault();
        } else if (std.mem.eql(u8, pos_type, "sfen")) {
            const board_str = it.next() orelse "";
            const color = it.next() orelse "";
            const hand = it.next() orelse "";
            const ply = it.next() orelse "";
            g.setPosition(lb.Board.parseParts(board_str, color, hand, ply) catch |err|
                return self.out.protocolError("position", "invalid sfen provided: {}", .{err}));
        } else {
            try self.out.unrecognisedToken("position", pos_type);
            return;
        }

        if (try self.expectToken("position", it, "moves")) {
            try self.usiParseMoves(it);
        }
    }

    fn usiParseMoves(self: *Usi, it: *Iterator) !void {
        while (it.next()) |move_str| {
            const m = lb.Move.parse(move_str) catch return self.out.illegalMoveString(move_str);
            g.board.move(m);
        }
    }

    fn usiParsePerft(self: *Usi, it: *Iterator) !void {
        const depth_str = it.next() orelse "1";
        const depth = std.fmt.parseUnsigned(usize, depth_str, 10) catch return self.out.unrecognisedToken("perft", depth_str);
        _ = try lb.perft.go(&self.out, &g.board, depth);
    }

    fn expectToken(self: *Usi, comptime command: []const u8, it: *Iterator, comptime token: []const u8) !bool {
        if (it.next()) |token_str| {
            if (std.mem.eql(u8, token_str, token)) return true;
            try self.out.unrecognisedToken(command, token_str);
        }
        return false;
    }
};

const UsiOutput = struct {
    writer: std.io.BufferedWriter(4096, std.fs.File.Writer),

    pub inline fn init(writer: std.io.BufferedWriter(4096, std.fs.File.Writer)) UsiOutput {
        return .{ .writer = writer };
    }

    pub inline fn raw(self: *UsiOutput, comptime fmt: []const u8, args: anytype) !void {
        try self.writer.writer().print(fmt, args);
    }

    pub inline fn flush(self: *UsiOutput) !void {
        try self.writer.flush();
    }

    pub fn protocolError(self: *UsiOutput, command: []const u8, comptime fmt: []const u8, args: anytype) !void {
        try self.raw("error ({s}): ", .{command});
        try self.raw(fmt ++ "\n", args);
        try self.flush();
    }

    pub fn unrecognisedToken(self: *UsiOutput, comptime command: []const u8, token: []const u8) !void {
        try self.raw("error (" ++ command ++ "): unrecognised token '{s}'\n", .{token});
        try self.flush();
    }

    pub fn illegalMoveString(self: *UsiOutput, move: []const u8) !void {
        try self.raw("error (illegal move): {s}\n", .{move});
        try self.flush();
    }

    pub fn illegalMove(self: *UsiOutput, move: lb.Move) !void {
        try self.raw("error (illegal move): {}\n", .{move});
        try self.flush();
    }

    pub inline fn pong(self: *UsiOutput) !void {
        try self.raw("readyok\n", .{});
        try self.flush();
    }

    pub inline fn bestmove(self: *UsiOutput, move: ?lb.Move) !void {
        try self.raw("bestmove {?}\n", .{move});
        try self.flush();
    }

    pub inline fn eval(self: *UsiOutput, score: lb.Score) !void {
        try self.printEval(score);
        try self.flush();
    }

    pub inline fn info(self: *UsiOutput, depth: i32, score: lb.Score, ctrl: anytype, pv: anytype, comptime info_type: enum { normal, early_termination }) !void {
        const trailing = switch (info_type) {
            .normal => "\n",
            .early_termination => " string [search terminated]\n",
        };

        const elapsed = ctrl.timer.read();
        const nps = ctrl.nodes * std.time.ns_per_s / elapsed;
        try self.raw("info depth {} ", .{depth});
        try self.printEval(score);
        try self.raw(" time {} nodes {} nps {} pv {}" ++ trailing, .{ elapsed / std.time.ns_per_ms, ctrl.nodes, nps, pv });
        try self.flush();
    }

    inline fn printEval(self: *UsiOutput, score: lb.Score) !void {
        if (lb.eval.distanceToMate(score)) |md| {
            try self.raw("score mate {}", .{md});
        } else {
            try self.raw("score cp {}", .{score});
        }
    }
};

const std = @import("std");
const lb = @import("lb.zig");
const util = @import("util.zig");
