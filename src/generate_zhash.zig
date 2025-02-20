pub fn main() !void {
    var output = std.io.getStdOut().writer();

    const ptype_label = [_][]const u8{ "Pawn", "Bishop", "Rook", "Lance", "Knight", "Silver", "Gold", "King", "Tokin", "Horse", "Dragon", "Promoted Lance/Knight/Silver" };
    const color_label = [_][]const u8{ "Sente", "Gote" };

    const bm = bch.genBasisMatrix(11, 64);
    const pm = bch.genParityMatrix(11, bm);
    var pm_index: usize = 0;

    try output.print("pub const Hash = u64;\n\n", .{});

    try output.print(
        \\pub fn board(color: Color, ptype: PieceType, sq: Square) Hash {{
        \\    const index = ptype.toBitboardIndex() * 81 * 2 + @as(usize, @intFromEnum(color)) * 81 + @as(usize, sq.raw);
        \\    return board_table[index];
        \\}}
        \\
        \\pub fn hand(color: Color, ptype: PieceType, count: usize) Hash {{
        \\    const index = ptype.toBitboardIndex() * 19 * 2 + @as(usize, @intFromEnum(color)) * 19 + @as(usize, count);
        \\    assert(count == 0 or hand_table[index] != 0);
        \\    return hand_table[index];
        \\}}
        \\
        \\pub fn handIncremental(color: Color, ptype: PieceType, count: usize) Hash {{
        \\    const index = ptype.toBitboardIndex() * 19 * 2 + @as(usize, @intFromEnum(color)) * 19 + @as(usize, count);
        \\    assert(count == 0 or hand_incr_table[index] != 0);
        \\    return hand_incr_table[index];
        \\}}
        \\
        \\
    , .{});

    try output.print("const board_table = [2 * 0o14 * 81]Hash{{\n", .{});
    for (0..24) |pc| {
        const ptype = pc >> 1;
        const color = pc & 1;
        try output.print("    // {s} {s}\n", .{ color_label[color], ptype_label[ptype] });
        for (0..81) |where| {
            if (where % 9 == 0) {
                try output.print("   ", .{});
            }

            const h = toZhash(pm[pm_index]);
            pm_index += 1;
            try output.print(" 0x{X:016},", .{h});

            if (where % 9 == 8) {
                try output.print("\n", .{});
            }
        }
    }
    try output.print("}};\n\n", .{});

    const hand_table_start_index = pm_index;
    try output.print("const hand_table = [2 * 7 * 19]Hash{{\n", .{});
    for (0..7) |ptype| {
        const counts = [_]usize{ 18, 2, 2, 4, 4, 4, 4 };
        const count = counts[ptype];
        for (0..2) |color| {
            try output.print("    // {s} {s}\n", .{ color_label[color], ptype_label[ptype] });
            try output.print("    0x{X:016},\n", .{0});
            for (0..18) |i| {
                if (i % 9 == 0) {
                    try output.print("   ", .{});
                }

                if (i < count) {
                    const h = toZhash(pm[pm_index]);
                    pm_index += 1;
                    try output.print(" 0x{X:016},", .{h});
                } else {
                    try output.print(" 0x{X:016},", .{0});
                }

                if (i % 9 == 8) {
                    try output.print("\n", .{});
                }
            }
        }
    }
    try output.print("}};\n\n", .{});
    const hand_table_end_index = pm_index;

    pm_index = hand_table_start_index;
    try output.print("const hand_incr_table = [2 * 7 * 19]Hash{{\n", .{});
    for (0..7) |ptype| {
        const counts = [_]usize{ 18, 2, 2, 4, 4, 4, 4 };
        const count = counts[ptype];
        for (0..2) |color| {
            try output.print("    // {s} {s}\n", .{ color_label[color], ptype_label[ptype] });
            var prev_h: Hash = 0;
            try output.print("    0x{X:016},\n", .{0});
            for (0..18) |i| {
                if (i % 9 == 0) {
                    try output.print("   ", .{});
                }

                if (i < count) {
                    const h = toZhash(pm[pm_index]);
                    pm_index += 1;
                    try output.print(" 0x{X:016},", .{h ^ prev_h});
                    prev_h = h;
                } else {
                    try output.print(" 0x{X:016},", .{0});
                }

                if (i % 9 == 8) {
                    try output.print("\n", .{});
                }
            }
        }
    }
    try output.print("}};\n\n", .{});
    assert(hand_table_end_index == pm_index);

    {
        const h = toZhash(pm[pm_index]);
        pm_index += 1;
        try output.print("pub const move: Hash = 0x{X:016};\n\n", .{h});
    }

    try output.print(
        \\const std = @import("std");
        \\const assert = std.debug.assert;
        \\const lb = @import("../lb.zig");
        \\const Color = lb.Color;
        \\const PieceType = lb.PieceType;
        \\const Square = lb.Square;
        \\
    , .{});
}

fn toZhash(row: bch.Row(11)) Hash {
    return @bitReverse(@as(Hash, @truncate(row)));
}

const Hash = u64;

const std = @import("std");
const assert = std.debug.assert;
const util = @import("util.zig");
const bch = util.bch;
