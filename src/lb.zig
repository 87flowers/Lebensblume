pub const attacks = @import("lb/attacks.zig");
pub const bit_util = @import("lb/bit_util.zig");
pub const perft = @import("lb/perft.zig");
pub const Bitboard = @import("lb/Bitboard.zig");
pub const Board = @import("lb/Board.zig");
pub const Game = @import("lb/Game.zig");
pub const MoveList = @import("lb/MoveList.zig");

pub const max_game_ply = 1024;
pub const max_legal_moves = 600;
pub const max_search_ply = 64;

pub const Score = i32;

pub const Square = u7;

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
        return pt >= @intFromEnum(PieceType.pawn) or pt <= @intFromEnum(PieceType.silver);
    }

    pub fn promote(ptype: PieceType) PieceType {
        assert(ptype != .none and ptype != .gold);
        return @enumFromInt(@intFromEnum(ptype) | 0o10);
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
            .src = f,
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

    pub fn from(m: Move) Square {
        assert(!m.drop);
        return m.src;
    }

    pub fn ptype(m: Move) PieceType {
        assert(m.drop);
        return @enumFromInt(m.src);
    }
};

const std = @import("std");
const assert = std.debug.assert;
