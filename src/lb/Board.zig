colors: [2]Bitboard,
pieces: [PieceType.bitboards_count]Bitboard,
hand: [2]u7,
board_mailbox: [81]packed struct { color: Color, ptype: PieceType },
hand_mailbox: [2]Hand,
active_color: Color = .sente,
ply: usize = 0,

checkers: Bitboard,
pinned: Bitboard,

pub fn defaultBoard() Board {
    @setEvalBranchQuota(100_000);
    return comptime parse("lnsgkgsnl/1r5b1/ppppppppp/9/9/9/PPPPPPPPP/1B5R1/LNSGKGSNL b - 1") catch unreachable;
}

pub fn verify(board: *const Board) void {
    _ = board;
}

pub fn move(board: *Board, m: Move) void {
    _ = .{ board, m };
}

pub fn getColor(board: *const Board, color: Color) Bitboard {
    return board.colors[@intFromEnum(color)];
}

pub fn getOccupied(board: *const Board) Bitboard {
    return Bitboard.@"or"(board.colors[0], board.colors[1]);
}

pub fn getPieces(board: *const Board, color: Color, ptype: PieceType) Bitboard {
    return Bitboard.@"and"(board.colors[@intFromEnum(color)], board.pieces[ptype.toBitboardIndex()]);
}

pub fn getPromoteds(board: *const Board, color: Color) Bitboard {
    return Bitboard.@"and"(board.colors[@intFromEnum(color)], board.pieces[board.pieces.len - 1]);
}

fn generateCheckersAndPinned(board: *Board) void {
    const friendly_color = board.active_color;
    const enemy_color = board.active_color.invert();

    const friendly_king = board.getPieces(friendly_color, .king);
    const friendly_king_sq = friendly_king.toSq();

    const orthogonals = Bitboard.@"or"(board.getPieces(enemy_color, .rook), board.getPieces(enemy_color, .dragon));
    const diagonals = Bitboard.@"or"(board.getPieces(enemy_color, .bishop), board.getPieces(enemy_color, .horse));

    // Checkers
    {
        board.checkers = .{};
        const blockers = board.getOccupied();

        const orthogonal_checkers = lb.attacks.rook(friendly_king_sq, blockers).@"and"(orthogonals);
        board.checkers.orWith(orthogonal_checkers);

        const diagonal_checkers = lb.attacks.bishop(friendly_king_sq, blockers).@"and"(diagonals);
        board.checkers.orWith(diagonal_checkers);

        const pawn_checkers = lb.attacks.pawn(friendly_king_sq, friendly_color).@"and"(board.getPieces(enemy_color, .pawn));
        board.checkers.orWith(pawn_checkers);

        const lance_checkers = lb.attacks.lance(friendly_king_sq, friendly_color, blockers).@"and"(board.getPieces(enemy_color, .lance));
        board.checkers.orWith(lance_checkers);

        const knight_checkers = lb.attacks.knight(friendly_king_sq, friendly_color).@"and"(board.getPieces(enemy_color, .knight));
        board.checkers.orWith(knight_checkers);

        const silver_checkers = lb.attacks.silver(friendly_king_sq, friendly_color).@"and"(board.getPieces(enemy_color, .silver));
        board.checkers.orWith(silver_checkers);

        const golds = board.getPieces(enemy_color, .gold).@"or"(board.getPieces(enemy_color, .tokin)).@"or"(board.getPromoteds(enemy_color));
        const golds_checkers = lb.attacks.gold(friendly_king_sq, friendly_color).@"and"(golds);
        board.checkers.orWith(golds_checkers);

        const rings = Bitboard.@"or"(board.getPieces(enemy_color, .horse), board.getPieces(enemy_color, .dragon));
        const rings_checkers = lb.attacks.king(friendly_king_sq).@"and"(rings);
        board.checkers.orWith(rings_checkers);
    }

    // Pinned
    {
        board.pinned = .{};
        const friendly = board.getColor(friendly_color);
        const enemy = board.getColor(enemy_color);

        const orthogonal_rays = lb.attacks.rook(friendly_king_sq, enemy);
        const diagonal_rays = lb.attacks.bishop(friendly_king_sq, enemy);

        const orthogonal_pinners = Bitboard.@"or"(
            orthogonal_rays.@"and"(orthogonals),
            lb.attacks.lance(friendly_king_sq, friendly_color, enemy).@"and"(board.getPieces(enemy_color, .lance)),
        );
        const diagonal_pinners = diagonal_rays.@"and"(diagonals);

        var orthogonal_pinners_iter = orthogonal_pinners.iterateSquares();
        while (orthogonal_pinners_iter.next()) |pinner| {
            const ray = orthogonal_rays.@"and"(lb.attacks.rook(pinner, friendly_king));
            const potential_pinned = ray.@"and"(friendly);
            if (potential_pinned.count() == 1) board.pinned.orWith(potential_pinned);
        }

        var diagonal_pinners_iter = diagonal_pinners.iterateSquares();
        while (diagonal_pinners_iter.next()) |pinner| {
            const ray = diagonal_rays.@"and"(lb.attacks.bishop(pinner, friendly_king));
            const potential_pinned = ray.@"and"(friendly);
            if (potential_pinned.count() == 1) board.pinned.orWith(potential_pinned);
        }
    }
}

test "checkers" {
    const board = try Board.parse("b5k1r/1b7/3n1n3/4p4/b3K3r/3sp+s3/3n1n3/9/9 b - 1");
    const expect = try Bitboard.fromSqList(&.{ "6f", "1e", "5d", "6c", "4c", "8b" });
    try std.testing.expectEqual(expect, board.checkers);
}

pub fn prettyPrint(board: *const Board, writer: anytype, language: PrintLanguage) !void {
    const indent = "    ";
    try writer.raw("\n", .{});
    try writer.raw(indent ++ " ９ ８ ７ ６ ５ ４ ３ ２ １ \n", .{});
    try writer.raw(indent ++ "┏━━┯━━┯━━┯━━┯━━┯━━┯━━┯━━┯━━┓\n", .{});
    for (0..81) |place_index| {
        const file, const sq = displayIndexToSquare(place_index);
        try writer.raw("{s}", .{if (file == 0) indent ++ "┃" else "│"});
        const place = board.board_mailbox[sq.raw];
        const is_red = place.ptype.promoted() and language == .ja;
        if (place.ptype != .none) try writer.raw("{s}", .{switch (place.color) {
            .sente => if (is_red) "\x1b[38;2;255;100;100;48;2;10;10;10m" else "\x1b[38;2;220;220;220;48;2;10;10;10m",
            .gote => if (is_red) "\x1b[38;2;220;0;0;48;2;255;255;255m" else "\x1b[38;2;80;80;80;48;2;255;255;255m",
        }});
        switch (language) {
            .ja => try writer.raw("{s}", .{PieceType.ja_piece_strings[@intFromEnum(place.ptype)][@intFromEnum(place.color)]}),
            .en => try writer.raw("{s:2}", .{PieceType.piece_strings[@intFromEnum(place.ptype)][@intFromEnum(place.color)]}),
        }
        try writer.raw("\x1b[39m\x1b[49m", .{});
        if (file == 8) {
            const rank_ch = @as(u8, @intCast('a' + place_index / 9));
            try writer.raw("┃ {c}\n", .{rank_ch});
            if (place_index != 80) {
                try writer.raw(indent ++ "┠──┼──┼──┼──┼──┼──┼──┼──┼──┨", .{});
                if (rank_ch == 'c') {
                    try writer.raw("      ☖ 持駒: ", .{});
                    try board.hand_mailbox[1].prettyPrint(writer, .gote, language);
                }
                if (rank_ch == 'f') {
                    try writer.raw("      ☗ 持駒: ", .{});
                    try board.hand_mailbox[0].prettyPrint(writer, .sente, language);
                }
                try writer.raw("\n", .{});
            }
        }
    }
    try writer.raw(indent ++ "┗━━┷━━┷━━┷━━┷━━┷━━┷━━┷━━┷━━┛\n", .{});
    try writer.raw("\n", .{});
    try writer.raw("sfen: {}\n", .{board});
    {
        try writer.raw("checkers:", .{});
        if (board.checkers.empty()) try writer.raw(" -", .{});
        var checkers_iterator = board.checkers.iterateSquares();
        while (checkers_iterator.next()) |checker| try writer.raw(" {}", .{checker});
        try writer.raw("\n", .{});
    }
    {
        try writer.raw("pinned:", .{});
        if (board.pinned.empty()) try writer.raw(" -", .{});
        var pinned_iterator = board.pinned.iterateSquares();
        while (pinned_iterator.next()) |pinned| try writer.raw(" {}", .{pinned});
        try writer.raw("\n", .{});
    }
    try writer.flush();
}

pub fn format(board: *const Board, comptime _: []const u8, _: std.fmt.FormatOptions, writer: anytype) !void {
    var blanks: usize = 0;
    for (0..81) |place_index| {
        const file, const sq = displayIndexToSquare(place_index);
        const place = board.board_mailbox[sq.raw];
        if (place.ptype == .none) {
            blanks += 1;
        } else {
            if (blanks != 0) {
                try writer.print("{}", .{blanks});
                blanks = 0;
            }
            try writer.print("{s}", .{PieceType.piece_strings[@intFromEnum(place.ptype)][@intFromEnum(place.color)]});
        }
        if (file == 8) {
            if (blanks != 0) {
                try writer.print("{}", .{blanks});
                blanks = 0;
            }
            if (place_index != 80) try writer.print("/", .{});
        }
    }
    try writer.print(" {} ", .{board.active_color});
    if (board.hand[0] == 0 and board.hand[1] == 0) {
        try writer.print("-", .{});
    } else {
        const op = struct {
            fn op(w: anytype, count: anytype, ch: u8) !void {
                if (count == 0) return;
                if (count == 1) return w.print("{c}", .{ch});
                return w.print("{}{c}", .{ count, ch });
            }
        }.op;
        try op(writer, board.hand_mailbox[@intFromEnum(Color.sente)].rook, 'R');
        try op(writer, board.hand_mailbox[@intFromEnum(Color.sente)].bishop, 'B');
        try op(writer, board.hand_mailbox[@intFromEnum(Color.sente)].gold, 'G');
        try op(writer, board.hand_mailbox[@intFromEnum(Color.sente)].silver, 'S');
        try op(writer, board.hand_mailbox[@intFromEnum(Color.sente)].knight, 'N');
        try op(writer, board.hand_mailbox[@intFromEnum(Color.sente)].lance, 'L');
        try op(writer, board.hand_mailbox[@intFromEnum(Color.sente)].pawn, 'P');
        try op(writer, board.hand_mailbox[@intFromEnum(Color.gote)].rook, 'r');
        try op(writer, board.hand_mailbox[@intFromEnum(Color.gote)].bishop, 'b');
        try op(writer, board.hand_mailbox[@intFromEnum(Color.gote)].gold, 'g');
        try op(writer, board.hand_mailbox[@intFromEnum(Color.gote)].silver, 's');
        try op(writer, board.hand_mailbox[@intFromEnum(Color.gote)].knight, 'n');
        try op(writer, board.hand_mailbox[@intFromEnum(Color.gote)].lance, 'l');
        try op(writer, board.hand_mailbox[@intFromEnum(Color.gote)].pawn, 'p');
    }
    try writer.print(" {}", .{(board.ply >> 1) + 1});
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

test "roundtrip sfen" {
    const cases = [_][]const u8{
        "lnsgkgsn1/1r5b1/ppppppppp/9/9/9/PPPPPPPPP/1B5R1/LNSGKGSNL w - 1",
        "lnsgk2nl/1r4gs1/p1pppp1pp/1p4p2/7P1/2P6/PP1PPPP1P/1SG4R1/LN2KGSNL b Bb 1",
        "ln1g5/1r2S1k2/p2pppn2/2ps2p2/1p7/2P6/PPSPPPPLP/2G2K1pr/LN4G1b w BGSLPnp 62",
        "8l/1l+R2P3/p2pBG1pp/kps1p4/Nn1P2G2/P1P1P2PP/1PS6/1KSG3+r1/LN2+p3L w Sbgn3p 124",
        "ln1g3nl/2s2k1+P1/p3pg3/1np2p2p/3p5/1SP3P1P/P1KPPSp2/2G6/L2b1G2L w RBSN2Pr2p 66",
    };
    for (cases) |case| {
        const board = try Board.parse(case);
        var tmp: [128]u8 = undefined;
        const sfen = try std.fmt.bufPrint(&tmp, "{}", .{board});
        try std.testing.expectEqualStrings(case, sfen);
    }
}

pub fn parseParts(board_str: []const u8, color_str: []const u8, hand_str: []const u8, ply_str: []const u8) !Board {
    var result = Board{
        .colors = @splat(.{}),
        .pieces = @splat(.{}),
        .hand = @splat(0),
        .board_mailbox = @splat(.{ .color = .sente, .ptype = .none }),
        .hand_mailbox = @splat(.{}),
        .active_color = .sente,
        .ply = 0,
        .checkers = .{},
        .pinned = .{},
    };

    // Parse Pieces
    {
        var place_index: usize = 0;
        var i: usize = 0;
        board_loop: while (place_index < 81 and i < board_str.len) : (i += 1) {
            const file, const sq = displayIndexToSquare(place_index);
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

    // Precompute
    result.generateCheckersAndPinned();

    return result;
}

fn displayIndexToSquare(index: usize) struct { usize, Square } {
    const file = index % 9;
    const sq = Square.make(@intCast(72 - (index - file) + file));
    return .{ file, sq };
}

fn placeBoard(self: *Board, color: Color, ptype: PieceType, sq: Square) void {
    self.colors[@intFromEnum(color)].orWith(Bitboard.fromSq(sq));
    self.pieces[ptype.toBitboardIndex()].orWith(Bitboard.fromSq(sq));
    self.board_mailbox[sq.raw] = .{ .color = color, .ptype = ptype };
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

    pub fn prettyPrint(hand: *const Hand, writer: anytype, color: Color, language: PrintLanguage) !void {
        const table = switch (language) {
            .ja => PieceType.ja_piece_strings,
            .en => PieceType.piece_strings,
        };
        if (hand.pawn == 0 and hand.bishop == 0 and hand.rook == 0 and hand.lance == 0 and hand.knight == 0 and hand.silver == 0 and hand.gold == 0) {
            try writer.raw("-", .{});
            return;
        }
        const op = struct {
            fn op(w: anytype, count: anytype, t: anytype, c: Color, ptype: PieceType) !void {
                const s = t[@intFromEnum(ptype)][@intFromEnum(c)];
                if (count == 0) return;
                if (count == 1) return w.raw("{s} ", .{s});
                return w.raw("{s}×{} ", .{ s, count });
            }
        }.op;
        try op(writer, hand.rook, table, color, .rook);
        try op(writer, hand.bishop, table, color, .bishop);
        try op(writer, hand.gold, table, color, .gold);
        try op(writer, hand.silver, table, color, .silver);
        try op(writer, hand.knight, table, color, .knight);
        try op(writer, hand.lance, table, color, .lance);
        try op(writer, hand.pawn, table, color, .pawn);
    }
};

pub const PrintLanguage = enum { ja, en };

const Board = @This();
const std = @import("std");
const assert = std.debug.assert;
const lb = @import("../lb.zig");
const Bitboard = lb.Bitboard;
const Color = lb.Color;
const Move = lb.Move;
const PieceType = lb.PieceType;
const Square = lb.Square;
