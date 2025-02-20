pub const LineReaderError = error{BufferTooSmall};

pub fn LineReader(comptime ReaderType: type) type {
    return struct {
        reader: ReaderType,
        buf: std.ArrayList(u8),

        pub const Error = ReaderType.Error;

        const Self = @This();

        pub fn readLine(self: *Self) !?[]u8 {
            self.buf.clearRetainingCapacity();
            self.reader.reader().streamUntilDelimiter(self.buf.writer(), '\n', null) catch |err| switch (err) {
                error.EndOfStream => return null,
                else => return err,
            };
            return self.buf.items;
        }
    };
}

pub fn lineReader(allocator: std.mem.Allocator, unbuffered_reader: anytype) LineReader(@TypeOf(std.io.bufferedReader(unbuffered_reader))) {
    return .{
        .reader = std.io.bufferedReader(unbuffered_reader),
        .buf = std.ArrayList(u8).init(allocator),
    };
}

const std = @import("std");
