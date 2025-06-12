/*
 * sleep - Flexible program execution pausing utility
 * Part of QCO MoreUtils - Quality Control Operations More Utilities
 * 
 * Copyright 2025 AnmiTaliDev
 * Licensed under the Apache License, Version 2.0
 * 
 * Repository: https://github.com/Qainar-Projects/MoreUtils
 * 
 * Provides flexible functions for program execution pausing with support
 * for multiple time units and high precision timing capabilities.
 */

#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <algorithm>
#include <regex>
#include <getopt.h>
#include <unistd.h>

/// Time units supported by the sleep utility
enum class TimeUnit {
    NANOSECOND,
    MICROSECOND,
    MILLISECOND,
    SECOND,
    MINUTE,
    HOUR,
    DAY
};

class SleepUtility {
private:
    bool verbose = false;
    bool quiet = false;
    
public:
    void setVerbose(bool v) { verbose = v; }
    void setQuiet(bool q) { quiet = q; }
    
    /// Sleep for specified duration in milliseconds
    void sleepMs(uint64_t milliseconds) {
        std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
    }
    
    /// Sleep for specified duration in seconds
    void sleepSec(uint64_t seconds) {
        std::this_thread::sleep_for(std::chrono::seconds(seconds));
    }
    
    /// Sleep for specified duration in minutes
    void sleepMin(uint64_t minutes) {
        std::this_thread::sleep_for(std::chrono::minutes(minutes));
    }
    
    /// Sleep for specified duration with custom time unit
    void sleepDuration(uint64_t value, TimeUnit unit) {
        if (verbose && !quiet) {
            std::cerr << "Sleeping for " << value << " " << unitToString(unit) << std::endl;
        }
        
        switch (unit) {
            case TimeUnit::NANOSECOND:
                std::this_thread::sleep_for(std::chrono::nanoseconds(value));
                break;
            case TimeUnit::MICROSECOND:
                std::this_thread::sleep_for(std::chrono::microseconds(value));
                break;
            case TimeUnit::MILLISECOND:
                std::this_thread::sleep_for(std::chrono::milliseconds(value));
                break;
            case TimeUnit::SECOND:
                std::this_thread::sleep_for(std::chrono::seconds(value));
                break;
            case TimeUnit::MINUTE:
                std::this_thread::sleep_for(std::chrono::minutes(value));
                break;
            case TimeUnit::HOUR:
                std::this_thread::sleep_for(std::chrono::hours(value));
                break;
            case TimeUnit::DAY:
                std::this_thread::sleep_for(std::chrono::hours(value * 24));
                break;
        }
        
        if (verbose && !quiet) {
            std::cerr << "Sleep completed" << std::endl;
        }
    }
    
    /// Parse combined duration string (e.g., "5s", "100ms", "2h30m")
    void sleepCombined(const std::string& duration_str) {
        std::regex pattern(R"((\d+(?:\.\d+)?)([a-zA-Z]+))");
        std::sregex_iterator iter(duration_str.begin(), duration_str.end(), pattern);
        std::sregex_iterator end;
        
        if (iter == end) {
            // No unit specified, try parsing as number with default seconds
            try {
                double value = std::stod(duration_str);
                auto microseconds = static_cast<uint64_t>(value * 1000000);
                std::this_thread::sleep_for(std::chrono::microseconds(microseconds));
                return;
            } catch (...) {
                throw std::runtime_error("Invalid duration format: " + duration_str);
            }
        }
        
        uint64_t total_microseconds = 0;
        
        for (; iter != end; ++iter) {
            std::string value_str = (*iter)[1].str();
            std::string unit_str = (*iter)[2].str();
            
            double value = std::stod(value_str);
            TimeUnit unit = parseUnit(unit_str);
            
            uint64_t microseconds = convertToMicroseconds(value, unit);
            total_microseconds += microseconds;
        }
        
        if (verbose && !quiet) {
            std::cerr << "Total sleep duration: " << total_microseconds << " microseconds" << std::endl;
        }
        
        std::this_thread::sleep_for(std::chrono::microseconds(total_microseconds));
    }
    
private:
    TimeUnit parseUnit(const std::string& unit_str) {
        std::string unit = unit_str;
        std::transform(unit.begin(), unit.end(), unit.begin(), ::tolower);
        
        if (unit == "ns" || unit == "nanoseconds" || unit == "nanosecond") {
            return TimeUnit::NANOSECOND;
        } else if (unit == "us" || unit == "microseconds" || unit == "microsecond") {
            return TimeUnit::MICROSECOND;
        } else if (unit == "ms" || unit == "milliseconds" || unit == "millisecond") {
            return TimeUnit::MILLISECOND;
        } else if (unit == "s" || unit == "seconds" || unit == "second") {
            return TimeUnit::SECOND;
        } else if (unit == "m" || unit == "minutes" || unit == "minute") {
            return TimeUnit::MINUTE;
        } else if (unit == "h" || unit == "hours" || unit == "hour") {
            return TimeUnit::HOUR;
        } else if (unit == "d" || unit == "days" || unit == "day") {
            return TimeUnit::DAY;
        } else {
            throw std::runtime_error("Unknown time unit: " + unit_str);
        }
    }
    
    uint64_t convertToMicroseconds(double value, TimeUnit unit) {
        switch (unit) {
            case TimeUnit::NANOSECOND:
                return static_cast<uint64_t>(value / 1000);
            case TimeUnit::MICROSECOND:
                return static_cast<uint64_t>(value);
            case TimeUnit::MILLISECOND:
                return static_cast<uint64_t>(value * 1000);
            case TimeUnit::SECOND:
                return static_cast<uint64_t>(value * 1000000);
            case TimeUnit::MINUTE:
                return static_cast<uint64_t>(value * 60000000);
            case TimeUnit::HOUR:
                return static_cast<uint64_t>(value * 3600000000);
            case TimeUnit::DAY:
                return static_cast<uint64_t>(value * 86400000000);
        }
        return 0;
    }
    
    std::string unitToString(TimeUnit unit) {
        switch (unit) {
            case TimeUnit::NANOSECOND: return "nanoseconds";
            case TimeUnit::MICROSECOND: return "microseconds";
            case TimeUnit::MILLISECOND: return "milliseconds";
            case TimeUnit::SECOND: return "seconds";
            case TimeUnit::MINUTE: return "minutes";
            case TimeUnit::HOUR: return "hours";
            case TimeUnit::DAY: return "days";
        }
        return "unknown";
    }
};

void printUsage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [OPTIONS] DURATION [UNIT]\n\n"
              << "Flexible program execution pausing utility.\n\n"
              << "Options:\n"
              << "  -v, --verbose            Verbose output\n"
              << "  -q, --quiet              Quiet mode\n"
              << "  -h, --help               Show this help message\n"
              << "  --version                Show version information\n\n"
              << "Duration Formats:\n"
              << "  NUMBER [UNIT]            Sleep for NUMBER in specified UNIT\n"
              << "  COMBINED                 Combined format (e.g., 2h30m15s)\n\n"
              << "Time Units:\n"
              << "  ns, nanoseconds          Sleep for nanoseconds\n"
              << "  us, microseconds         Sleep for microseconds\n"
              << "  ms, milliseconds         Sleep for milliseconds\n"
              << "  s, seconds               Sleep for seconds (default)\n"
              << "  m, minutes               Sleep for minutes\n"
              << "  h, hours                 Sleep for hours\n"
              << "  d, days                  Sleep for days\n\n"
              << "Examples:\n"
              << "  " << program_name << " 5                    # 5 seconds\n"
              << "  " << program_name << " 100 ms               # 100 milliseconds\n"
              << "  " << program_name << " 2.5 s                # 2.5 seconds\n"
              << "  " << program_name << " 1h30m                # 1 hour 30 minutes\n"
              << "  " << program_name << " 2h30m15s             # Complex duration\n"
              << "  " << program_name << " 0.001 s              # 1 millisecond\n"
              << "  " << program_name << " 500000 us            # 500 milliseconds\n\n"
              << "Precision:\n"
              << "  The utility supports sub-second precision and can handle\n"
              << "  fractional values for all time units.\n\n"
              << "Part of QCO MoreUtils by AnmiTaliDev\n"
              << "Repository: https://github.com/Qainar-Projects/MoreUtils\n";
}

void printVersion() {
    std::cout << "sleep 1.0.0\n"
              << "Part of QCO MoreUtils - Quality Control Operations More Utilities\n"
              << "Copyright 2025 AnmiTaliDev\n"
              << "Licensed under the Apache License, Version 2.0\n"
              << "Repository: https://github.com/Qainar-Projects/MoreUtils\n";
}

int main(int argc, char* argv[]) {
    SleepUtility sleep_util;
    
    static struct option long_options[] = {
        {"verbose",    no_argument, 0, 'v'},
        {"quiet",      no_argument, 0, 'q'},
        {"help",       no_argument, 0, 'h'},
        {"version",    no_argument, 0, 8001},
        {0, 0, 0, 0}
    };
    
    int opt;
    int option_index = 0;
    
    while ((opt = getopt_long(argc, argv, "vqh", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'v':
                sleep_util.setVerbose(true);
                break;
            case 'q':
                sleep_util.setQuiet(true);
                break;
            case 'h':
                printUsage(argv[0]);
                return 0;
            case 8001: // version
                printVersion();
                return 0;
            case '?':
                std::cerr << "Try '" << argv[0] << " --help' for more information.\n";
                return 1;
            default:
                return 1;
        }
    }
    
    if (optind >= argc) {
        std::cerr << "sleep: missing duration argument\n";
        std::cerr << "Try '" << argv[0] << " --help' for more information.\n";
        return 1;
    }
    
    try {
        std::string duration_arg = argv[optind];
        
        // Check if we have a separate unit argument
        if (optind + 1 < argc) {
            // Format: sleep DURATION UNIT
            double value = std::stod(duration_arg);
            std::string unit_str = argv[optind + 1];
            
            TimeUnit unit = TimeUnit::SECOND; // Default
            std::transform(unit_str.begin(), unit_str.end(), unit_str.begin(), ::tolower);
            
            if (unit_str == "ns" || unit_str == "nanoseconds") {
                unit = TimeUnit::NANOSECOND;
            } else if (unit_str == "us" || unit_str == "microseconds") {
                unit = TimeUnit::MICROSECOND;
            } else if (unit_str == "ms" || unit_str == "milliseconds") {
                unit = TimeUnit::MILLISECOND;
            } else if (unit_str == "s" || unit_str == "seconds") {
                unit = TimeUnit::SECOND;
            } else if (unit_str == "m" || unit_str == "minutes") {
                unit = TimeUnit::MINUTE;
            } else if (unit_str == "h" || unit_str == "hours") {
                unit = TimeUnit::HOUR;
            } else if (unit_str == "d" || unit_str == "days") {
                unit = TimeUnit::DAY;
            } else {
                std::cerr << "sleep: unknown time unit '" << unit_str << "'\n";
                return 1;
            }
            
            uint64_t duration = static_cast<uint64_t>(value);
            if (value != static_cast<double>(duration)) {
                // Handle fractional values by converting to microseconds
                uint64_t microseconds = 0;
                switch (unit) {
                    case TimeUnit::NANOSECOND:
                        microseconds = static_cast<uint64_t>(value / 1000);
                        break;
                    case TimeUnit::MICROSECOND:
                        microseconds = static_cast<uint64_t>(value);
                        break;
                    case TimeUnit::MILLISECOND:
                        microseconds = static_cast<uint64_t>(value * 1000);
                        break;
                    case TimeUnit::SECOND:
                        microseconds = static_cast<uint64_t>(value * 1000000);
                        break;
                    case TimeUnit::MINUTE:
                        microseconds = static_cast<uint64_t>(value * 60000000);
                        break;
                    case TimeUnit::HOUR:
                        microseconds = static_cast<uint64_t>(value * 3600000000);
                        break;
                    case TimeUnit::DAY:
                        microseconds = static_cast<uint64_t>(value * 86400000000);
                        break;
                }
                std::this_thread::sleep_for(std::chrono::microseconds(microseconds));
            } else {
                sleep_util.sleepDuration(duration, unit);
            }
        } else {
            // Format: sleep DURATION or sleep COMBINED
            sleep_util.sleepCombined(duration_arg);
        }
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "sleep: error: " << e.what() << std::endl;
        return 1;
    }
}