pub const attacks = @import("lb/attacks.zig");
pub const bit_util = @import("lb/bit_util.zig");
pub const perft = @import("lb/perft.zig");
pub const rays = @import("lb/rays.zig");
pub const Bitboard = @import("lb/Bitboard.zig");
pub const Board = @import("lb/Board.zig");
pub const Game = @import("lb/Game.zig");
pub const MoveList = @import("lb/MoveList.zig");

pub const max_game_ply = 1024;
pub const max_legal_moves = 600;
pub const max_search_ply = 64;

pub const Score = i32;

pub const Square = packed struct(u7) {
    raw: u7,

    pub fn make(raw: u7) Square {
        return .{ .raw = raw };
    }

    pub fn parse(str: [2]u8) !Square {
        if (str[0] < '1' or str[0] > '9') return ParseError.InvalidChar;
        const file = 9 - (str[0] - '0');
        if (str[1] < 'a' or str[1] > 'i') return ParseError.InvalidChar;
        const rank = 9 - (str[1] - 'a' + 1);
        return Square.make(@intCast(rank * 9 + file));
    }

    pub fn bitboard(sq: Square) Bitboard {
        return Bitboard.fromSq(sq);
    }

    pub fn format(sq: Square, comptime _: []const u8, _: std.fmt.FormatOptions, writer: anytype) !void {
        const file = 9 - (sq.raw % 9);
        const rank = 9 - (sq.raw / 9);
        try writer.print("{c}{c}", .{ '0' + file, 'a' + rank - 1 });
    }

    pub fn isPromoSquare(sq: Square, color: Color) bool {
        return switch (color) {
            .sente => (80 - sq.raw) < 27,
            .gote => sq.raw < 27,
        };
    }
};

pub const ParseError = error{
    InvalidChar,
    InvalidLength,
    OutOfRange,
    InvalidHand,
    InvalidBoard,
};

pub const PieceType = enum(u4) {
    none = 0o00,
    pawn = 0o01,
    bishop = 0o02,
    rook = 0o03,
    lance = 0o04,
    knight = 0o05,
    silver = 0o06,
    gold = 0o07,
    king = 0o10,
    tokin = 0o11,
    horse = 0o12,
    dragon = 0o13,
    nari_lance = 0o14,
    nari_knight = 0o15,
    nari_silver = 0o16,

    // Promoted lance/knight/silver share a bitboard
    pub const bitboards_count = 0o14;

    pub fn promotable(ptype: PieceType) bool {
        const pt = @intFromEnum(ptype);
        return pt >= @intFromEnum(PieceType.pawn) and pt <= @intFromEnum(PieceType.silver);
    }

    pub fn promote(ptype: PieceType) PieceType {
        assert(ptype != .none and ptype != .gold);
        return @enumFromInt(@intFromEnum(ptype) | 0o10);
    }

    pub fn demote(ptype: PieceType) PieceType {
        if (ptype.promoted()) return @enumFromInt(@intFromEnum(ptype) & 0o07);
        return ptype;
    }

    pub fn promoted(ptype: PieceType) bool {
        return @intFromEnum(ptype) > 0o10;
    }

    pub fn toBitboardIndex(ptype: PieceType) usize {
        assert(ptype != .none);
        return @min(bitboards_count, @intFromEnum(ptype)) - 1;
    }

    pub const piece_strings = [15][2][]const u8{
        .{ " ", " " },
        .{ "P", "p" },
        .{ "B", "b" },
        .{ "R", "r" },
        .{ "L", "l" },
        .{ "N", "n" },
        .{ "S", "s" },
        .{ "G", "g" },
        .{ "K", "k" },
        .{ "+P", "+p" },
        .{ "+B", "+b" },
        .{ "+R", "+r" },
        .{ "+L", "+l" },
        .{ "+N", "+n" },
        .{ "+S", "+s" },
    };

    pub const ja_piece_strings = [15][2][]const u8{
        .{ "　", "　" },
        .{ "歩", "歩" },
        .{ "角", "角" },
        .{ "飛", "飛" },
        .{ "香", "香" },
        .{ "桂", "桂" },
        .{ "銀", "銀" },
        .{ "金", "金" },
        .{ "玉", "王" },
        .{ "と", "と" },
        .{ "馬", "馬" },
        .{ "龍", "龍" },
        .{ "杏", "杏" },
        .{ "圭", "圭" },
        .{ "全", "全" },
    };

    pub fn format(self: PieceType, comptime _: []const u8, _: std.fmt.FormatOptions, writer: anytype) !void {
        try writer.print("{s}", .{piece_strings[@intFromEnum(self)][0]});
    }
};

pub const Color = enum(u1) {
    sente = 0,
    gote = 1,

    pub fn invert(self: Color) Color {
        return @enumFromInt(~@intFromEnum(self));
    }

    pub fn toChar(self: Color) u8 {
        return switch (self) {
            .sente => 'b',
            .gote => 'w',
        };
    }

    pub fn format(self: Color, comptime _: []const u8, _: std.fmt.FormatOptions, writer: anytype) !void {
        try writer.print("{c}", .{self.toChar()});
    }
};

pub const Move = packed struct(u16) {
    src: u7,
    drop: bool,
    to: Square,
    promo: bool,

    pub const none: Move = @bitCast(@as(u16, 0));

    pub fn makeMove(f: Square, to: Square, promo: bool) Move {
        return .{
            .src = f.raw,
            .drop = false,
            .to = to,
            .promo = promo,
        };
    }

    pub fn makeDrop(pt: PieceType, to: Square) Move {
        return .{
            .src = @intFromEnum(pt),
            .drop = true,
            .to = to,
            .promo = false,
        };
    }

    pub fn parse(str: []const u8) !Move {
        if (str.len < 4 or str.len > 5) return ParseError.InvalidLength;
        if (str[1] == '*') {
            if (str.len != 4) return ParseError.InvalidLength;
            const pt: PieceType = switch (str[0]) {
                'P' => .pawn,
                'N' => .knight,
                'L' => .lance,
                'S' => .silver,
                'G' => .gold,
                'B' => .bishop,
                'R' => .rook,
                else => return ParseError.InvalidChar,
            };
            const to = try Square.parse(str[2..][0..2].*);
            return Move.makeDrop(pt, to);
        } else {
            if (str.len == 5 and str[4] != '+') return ParseError.InvalidChar;
            const promo = str.len == 5;
            const f = try Square.parse(str[0..][0..2].*);
            const to = try Square.parse(str[2..][0..2].*);
            return Move.makeMove(f, to, promo);
        }
    }

    pub fn from(m: Move) Square {
        assert(!m.drop);
        return Square.make(m.src);
    }

    pub fn ptype(m: Move) PieceType {
        assert(m.drop and !m.promo);
        return @enumFromInt(m.src);
    }

    pub fn format(m: Move, comptime _: []const u8, _: std.fmt.FormatOptions, writer: anytype) !void {
        if (m.drop) {
            try writer.print("{}*{}", .{ m.ptype(), m.to });
        } else {
            try writer.print("{}{}{s}", .{ m.from(), m.to, if (m.promo) "+" else "" });
        }
    }
};

pub fn displayIndexToSquare(index: usize) struct { usize, Square } {
    const file = index % 9;
    const sq = Square.make(@intCast(72 - (index - file) + file));
    return .{ file, sq };
}

const std = @import("std");
const assert = std.debug.assert;
