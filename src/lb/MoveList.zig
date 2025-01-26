moves: Moves = Moves.init(0) catch unreachable,

const Moves = std.BoundedArray(lb.Move, lb.max_legal_moves);

const MoveList = @This();
const std = @import("std");
const assert = std.debug.assert;
const lb = @import("../lb.zig");
