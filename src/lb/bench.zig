pub fn run(out: anytype, game: *Game) !void {
    var time: u64 = 0;
    var nodes: u64 = 0;

    var ctrl = lb.search.DepthControl.init(.{ .target_depth = 4 });
    for (sfens) |sfen| {
        game.reset();
        try out.raw("benching {s}\n", .{sfen});
        game.setPosition(try Board.parse(sfen));

        ctrl.reset();
        var pv = lb.line.Line{};
        _ = try lb.search.go(out, game, &ctrl, &pv);

        time += ctrl.timer.read();
        nodes += ctrl.nodes;
        try out.raw("\n", .{});
    }

    try out.raw(
        \\bench results:
        \\nodes: {} nodes
        \\time:  {} milliseconds
        \\nps:   {} nps
        \\
    , .{
        nodes,
        time / std.time.ns_per_ms,
        nodes * std.time.ns_per_s / time,
    });
    try out.flush();

    game.reset();
}

const sfens = [_][]const u8{
    "lnsgkgsnl/1r5b1/ppppppppp/9/9/9/PPPPPPPPP/1B5R1/LNSGKGSNL b - 1",
    "l6nl/5+P1gk/2np1S3/p1p4Pp/3P2Sp1/1PPb2P1P/P5GS1/R8/LN4bKL w GR5pnsg 1",
    "9/9/9/3k5/9/5K3/9/9/9 b RB2G2S2N2L9Prb2g2s2n2l9p 1",
    "8l/1l+R2P3/p2pBG1pp/kps1p4/Nn1P2G2/P1P1P2PP/1PS6/1KSG3+r1/LN2+p3L w Sbgn3p 124",

    "lnsgk2nl/1r4gs1/p1pppp1pp/1p4p2/7P1/2P6/PP1PPPP1P/1SG4R1/LN2KGSNL b Bb",
    "l8/2n1g4/1p1p1s2+R/p2np1pK1/3N5/1SkP1G3/B1+n1L4/5+rs2/L4s2L b BG3Pg9p 1",

    "+Pn1k2S+P+P/7P1/pL+B+r+LP3/sg4p1G/P5r2/1PS3G1p/L1P1pK1+pL/4P4/SN1b1+n+n2 w G3Pp 206",
    "lnsgkgsnl/9/1ppp1p1p1/1r6p/p3p4/5P3/6P2/3K5/4B1+b2 b r2g2s2n2l8p 108",
    "3g2k2/3sp4/l1n1rp2+B/p8/1pP4P1/P2P5/1P2PPP2/2S1G4/LN1RKGSNL w BGSNL6P 119",
};

const std = @import("std");
const lb = @import("../lb.zig");
const Board = lb.Board;
const Game = lb.Game;
