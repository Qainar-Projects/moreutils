//! Sleep utility for QCO MoreUtils package
//! Provides flexible functions for program execution pausing
//!
//! Author: AnmiTaliDev
//! License: Apache 2.0

const std = @import("std");

/// Main module for the sleep utility
pub const Sleep = struct {
    /// Sleep for specified duration in milliseconds
    pub fn ms(milliseconds: u64) void {
        std.time.sleep(milliseconds * std.time.ns_per_ms);
    }

    /// Sleep for specified duration in seconds
    pub fn sec(seconds: u64) void {
        std.time.sleep(seconds * std.time.ns_per_s);
    }

    /// Sleep for specified duration in minutes
    pub fn min(minutes: u64) void {
        std.time.sleep(minutes * std.time.ns_per_min);
    }

    /// Sleep for specified duration with custom timeunit
    pub fn duration(value: u64, unit: TimeUnit) void {
        switch (unit) {
            .nanosecond => std.time.sleep(value),
            .microsecond => std.time.sleep(value * std.time.ns_per_us),
            .millisecond => std.time.sleep(value * std.time.ns_per_ms),
            .second => std.time.sleep(value * std.time.ns_per_s),
            .minute => std.time.sleep(value * std.time.ns_per_min),
            .hour => std.time.sleep(value * std.time.ns_per_hour),
        }
    }
};

/// Time units supported by the sleep utility
pub const TimeUnit = enum {
    nanosecond,
    microsecond, 
    millisecond,
    second,
    minute,
    hour,
};

/// Module information
pub const info = struct {
    pub const package = "QCO MoreUtils";
    pub const version = "1.0.0";
    pub const author = "AnmiTaliDev";
    pub const license = "Apache 2.0";
};

// Command-line functionality
pub fn main() !void {
    // Parse command line arguments
    var general_purpose_allocator = std.heap.GeneralPurposeAllocator(.{}){};
    const gpa = general_purpose_allocator.allocator();
    defer _ = general_purpose_allocator.deinit();

    const args = try std.process.argsAlloc(gpa);
    defer std.process.argsFree(gpa, args);

    if (args.len < 2) {
        try printUsage();
        return;
    }

    // Parse duration
    const duration_str = args[1];
    const duration = std.fmt.parseInt(u64, duration_str, 10) catch |err| {
        try std.io.getStdErr().writer().print("Error parsing duration: {s}\n", .{@errorName(err)});
        try printUsage();
        return;
    };

    // Default to seconds if no unit specified
    var unit = TimeUnit.second;

    // Check if unit was specified
    if (args.len > 2) {
        const unit_str = args[2];
        unit = parseUnit(unit_str) catch {
            try std.io.getStdErr().writer().print("Error: Unknown time unit '{s}'\n", .{unit_str});
            try printUsage();
            return;
        };
    }

    // Sleep for specified duration
    Sleep.duration(duration, unit);
}

fn parseUnit(unit_str: []const u8) !TimeUnit {
    if (std.mem.eql(u8, unit_str, "ns") or std.mem.eql(u8, unit_str, "nanoseconds")) {
        return TimeUnit.nanosecond;
    } else if (std.mem.eql(u8, unit_str, "us") or std.mem.eql(u8, unit_str, "microseconds")) {
        return TimeUnit.microsecond;
    } else if (std.mem.eql(u8, unit_str, "ms") or std.mem.eql(u8, unit_str, "milliseconds")) {
        return TimeUnit.millisecond;
    } else if (std.mem.eql(u8, unit_str, "s") or std.mem.eql(u8, unit_str, "seconds")) {
        return TimeUnit.second;
    } else if (std.mem.eql(u8, unit_str, "m") or std.mem.eql(u8, unit_str, "minutes")) {
        return TimeUnit.minute;
    } else if (std.mem.eql(u8, unit_str, "h") or std.mem.eql(u8, unit_str, "hours")) {
        return TimeUnit.hour;
    } else {
        return error.UnknownUnit;
    }
}

fn printUsage() !void {
    const stderr = std.io.getStdErr().writer();
    try stderr.print("Usage: sleep DURATION [UNIT]\n", .{});
    try stderr.print("\n", .{});
    try stderr.print("DURATION is a positive integer\n", .{});
    try stderr.print("UNIT can be one of:\n", .{});
    try stderr.print("  ns, nanoseconds  - sleep for nanoseconds\n", .{});
    try stderr.print("  us, microseconds - sleep for microseconds\n", .{});
    try stderr.print("  ms, milliseconds - sleep for milliseconds\n", .{});
    try stderr.print("  s, seconds       - sleep for seconds (default)\n", .{});
    try stderr.print("  m, minutes       - sleep for minutes\n", .{});
    try stderr.print("  h, hours         - sleep for hours\n", .{});
    try stderr.print("\n", .{});
    try stderr.print("Examples:\n", .{});
    try stderr.print("  sleep 5      - sleep for 5 seconds\n", .{});
    try stderr.print("  sleep 100 ms - sleep for 100 milliseconds\n", .{});
    try stderr.print("\n", .{});
    try stderr.print("QCO MoreUtils package - By AnmiTaliDev\n", .{});
    try stderr.print("License: Apache 2.0\n", .{});
}