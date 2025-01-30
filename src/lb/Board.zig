colors: [2]Bitboard,
pieces: [PieceType.bitboards_count]Bitboard,
hand: [2]u7,
board_mailbox: [81]PieceType,
hand_mailbox: [2]Hand,
active_color: Color = .sente,
ply: usize = 0,

pub fn emptyBoard() Board {
    return .{
        .colors = @splat(.{}),
        .pieces = @splat(.{}),
        .hand = @splat(0),
        .board_mailbox = @splat(.none),
        .hand_mailbox = @splat(.{}),
        .active_color = .sente,
        .ply = 0,
    };
}

pub fn verify(board: *const Board) void {
    _ = board;
}

fn ones(count: usize) u64 {
    return (@as(u64, 1) << @intCast(count)) -% 1;
}

pub fn move(board: *Board, m: Move) void {
    _ = .{ board, m };
}

pub fn format(board: *const Board, comptime _: []const u8, _: std.fmt.FormatOptions, writer: anytype) !void {
    _ = .{ board, writer };
}

pub fn parse(str: []const u8) !Board {
    var it = std.mem.tokenizeAny(u8, str, " \t\r\n");
    const board_str = it.next() orelse return lb.ParseError.InvalidLength;
    const color = it.next() orelse return lb.ParseError.InvalidLength;
    const hand = it.next() orelse return lb.ParseError.InvalidLength;
    const ply = it.next() orelse "1";
    if (it.next() != null) return lb.ParseError.InvalidLength;
    return Board.parseParts(board_str, color, hand, ply);
}

pub fn parseParts(board_str: []const u8, color_str: []const u8, hand_str: []const u8, ply_str: []const u8) !Board {
    var result = emptyBoard();

    // Parse Pieces
    {
        var place_index: usize = 0;
        var i: usize = 0;
        board_loop: while (place_index < 81 and i < board_str.len) : (i += 1) {
            const file = place_index % 9;
            const sq: lb.Square = @intCast(72 - (place_index - file) + file);
            const ch = board_str[i];
            switch (ch) {
                '/' => {
                    if (file != 0 or place_index == 0) return lb.ParseError.InvalidChar;
                    continue :board_loop;
                },
                '1'...'9' => {
                    place_index += ch - '0';
                    continue :board_loop;
                },
                'p' => result.placeBoard(.gote, .pawn, sq),
                'b' => result.placeBoard(.gote, .bishop, sq),
                'r' => result.placeBoard(.gote, .rook, sq),
                'l' => result.placeBoard(.gote, .lance, sq),
                'n' => result.placeBoard(.gote, .knight, sq),
                's' => result.placeBoard(.gote, .silver, sq),
                'g' => result.placeBoard(.gote, .gold, sq),
                'k' => result.placeBoard(.gote, .king, sq),
                'P' => result.placeBoard(.sente, .pawn, sq),
                'B' => result.placeBoard(.sente, .bishop, sq),
                'R' => result.placeBoard(.sente, .rook, sq),
                'L' => result.placeBoard(.sente, .lance, sq),
                'N' => result.placeBoard(.sente, .knight, sq),
                'S' => result.placeBoard(.sente, .silver, sq),
                'G' => result.placeBoard(.sente, .gold, sq),
                'K' => result.placeBoard(.sente, .king, sq),
                '+' => {
                    i += 1;
                    if (i >= board_str.len) return lb.ParseError.InvalidLength;
                    switch (board_str[i]) {
                        'p' => result.placeBoard(.gote, .tokin, sq),
                        'b' => result.placeBoard(.gote, .horse, sq),
                        'r' => result.placeBoard(.gote, .dragon, sq),
                        'l' => result.placeBoard(.gote, .nari_lance, sq),
                        'n' => result.placeBoard(.gote, .nari_knight, sq),
                        's' => result.placeBoard(.gote, .nari_silver, sq),
                        'P' => result.placeBoard(.sente, .tokin, sq),
                        'B' => result.placeBoard(.sente, .horse, sq),
                        'R' => result.placeBoard(.sente, .dragon, sq),
                        'L' => result.placeBoard(.sente, .nari_lance, sq),
                        'N' => result.placeBoard(.sente, .nari_knight, sq),
                        'S' => result.placeBoard(.sente, .nari_silver, sq),
                        else => return lb.ParseError.InvalidChar,
                    }
                },
                else => return lb.ParseError.InvalidChar,
            }
            place_index += 1;
        }
        if (place_index != 81 or i != board_str.len) return lb.ParseError.InvalidLength;
    }

    // Parse Color
    {
        if (color_str.len != 1) return lb.ParseError.InvalidLength;
        result.active_color = switch (color_str[0]) {
            'b' => .sente,
            'w' => .gote,
            else => return lb.ParseError.InvalidChar,
        };
    }

    // Parse Hand
    if (!std.mem.eql(u8, hand_str, "-")) {
        var modifier: ?usize = null;
        hand_loop: for (hand_str) |ch| {
            switch (ch) {
                '0'...'9' => {
                    if (modifier == null and ch == '0') return lb.ParseError.InvalidChar;
                    modifier = (modifier orelse 0) * 10 + (ch - '0');
                    if (modifier.? > 18) return lb.ParseError.OutOfRange;
                    continue :hand_loop;
                },
                'p' => try result.placeHandFromParse(.gote, .pawn, modifier orelse 1),
                'b' => try result.placeHandFromParse(.gote, .bishop, modifier orelse 1),
                'r' => try result.placeHandFromParse(.gote, .rook, modifier orelse 1),
                'l' => try result.placeHandFromParse(.gote, .lance, modifier orelse 1),
                'n' => try result.placeHandFromParse(.gote, .knight, modifier orelse 1),
                's' => try result.placeHandFromParse(.gote, .silver, modifier orelse 1),
                'g' => try result.placeHandFromParse(.gote, .gold, modifier orelse 1),
                'P' => try result.placeHandFromParse(.sente, .pawn, modifier orelse 1),
                'B' => try result.placeHandFromParse(.sente, .bishop, modifier orelse 1),
                'R' => try result.placeHandFromParse(.sente, .rook, modifier orelse 1),
                'L' => try result.placeHandFromParse(.sente, .lance, modifier orelse 1),
                'N' => try result.placeHandFromParse(.sente, .knight, modifier orelse 1),
                'S' => try result.placeHandFromParse(.sente, .silver, modifier orelse 1),
                'G' => try result.placeHandFromParse(.sente, .gold, modifier orelse 1),
                else => return lb.ParseError.InvalidChar,
            }
            modifier = null;
        }
        if (modifier != null) return lb.ParseError.InvalidLength;
    }

    // Parse ply
    {
        result.ply = try std.fmt.parseUnsigned(u16, ply_str, 10);
        if (result.ply < 1 or result.ply > 10000) return lb.ParseError.OutOfRange;
        result.ply = (result.ply - 1) * 2 + @intFromEnum(result.active_color);
    }

    // King count validation
    if (@popCount(Bitboard.@"and"(result.pieces[PieceType.king.toBitboardIndex()], result.colors[0]).raw) != 1) return lb.ParseError.InvalidBoard;
    if (@popCount(Bitboard.@"and"(result.pieces[PieceType.king.toBitboardIndex()], result.colors[1]).raw) != 1) return lb.ParseError.InvalidBoard;

    return result;
}

fn placeBoard(self: *Board, color: Color, ptype: PieceType, sq: Square) void {
    self.colors[@intFromEnum(color)].orWith(Bitboard.fromSq(sq));
    self.pieces[ptype.toBitboardIndex()].orWith(Bitboard.fromSq(sq));
    self.board_mailbox[sq] = ptype;
}

fn placeHandFromParse(self: *Board, color: Color, ptype: PieceType, count: usize) !void {
    const max_count: usize = switch (ptype) {
        .pawn => 18,
        .bishop => 2,
        .rook => 2,
        .lance => 4,
        .knight => 4,
        .silver => 4,
        .gold => 4,
        else => unreachable,
    };
    if (count > max_count or count == 0) return lb.ParseError.InvalidHand;
    const index = ptype.toBitboardIndex();
    self.hand[@intFromEnum(color)] = @as(u7, 1) << @intCast(index);
    switch (ptype) {
        .pawn => {
            if (self.hand_mailbox[@intFromEnum(color)].pawn != 0) return lb.ParseError.InvalidHand;
            self.hand_mailbox[@intFromEnum(color)].pawn = @intCast(count);
        },
        .bishop => {
            if (self.hand_mailbox[@intFromEnum(color)].bishop != 0) return lb.ParseError.InvalidHand;
            self.hand_mailbox[@intFromEnum(color)].bishop = @intCast(count);
        },
        .rook => {
            if (self.hand_mailbox[@intFromEnum(color)].rook != 0) return lb.ParseError.InvalidHand;
            self.hand_mailbox[@intFromEnum(color)].rook = @intCast(count);
        },
        .lance => {
            if (self.hand_mailbox[@intFromEnum(color)].lance != 0) return lb.ParseError.InvalidHand;
            self.hand_mailbox[@intFromEnum(color)].lance = @intCast(count);
        },
        .knight => {
            if (self.hand_mailbox[@intFromEnum(color)].knight != 0) return lb.ParseError.InvalidHand;
            self.hand_mailbox[@intFromEnum(color)].knight = @intCast(count);
        },
        .silver => {
            if (self.hand_mailbox[@intFromEnum(color)].silver != 0) return lb.ParseError.InvalidHand;
            self.hand_mailbox[@intFromEnum(color)].silver = @intCast(count);
        },
        .gold => {
            if (self.hand_mailbox[@intFromEnum(color)].gold != 0) return lb.ParseError.InvalidHand;
            self.hand_mailbox[@intFromEnum(color)].gold = @intCast(count);
        },
        else => unreachable,
    }
}

pub const Hand = packed struct(u32) {
    pawn: u8 = 0,
    bishop: u4 = 0,
    rook: u4 = 0,
    lance: u4 = 0,
    knight: u4 = 0,
    silver: u4 = 0,
    gold: u4 = 0,
};

const Board = @This();
const std = @import("std");
const assert = std.debug.assert;
const lb = @import("../lb.zig");
const Bitboard = lb.Bitboard;
const Color = lb.Color;
const Move = lb.Move;
const PieceType = lb.PieceType;
const Square = lb.Square;
