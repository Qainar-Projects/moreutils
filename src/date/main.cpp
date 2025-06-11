/*
 * date - Advanced date/time utility with extensive formatting and calculation features
 * Part of QCO MoreUtils - Advanced System Development More Utilities
 * 
 * Copyright 2025 AnmiTaliDev
 * Licensed under the Apache License, Version 2.0
 * 
 * Repository: https://github.com/Qainar-Projects/MoreUtils
 * 
 * A powerful date/time utility supporting multiple formats, timezones,
 * date arithmetic, parsing, and advanced formatting options.
 */

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <regex>
#include <ctime>
#include <getopt.h>
#include <unistd.h>

class DateUtility {
private:
    std::string format = "%Y-%m-%d %H:%M:%S";
    std::string timezone = "local";
    bool utc = false;
    bool iso_format = false;
    bool rfc_format = false;
    bool unix_timestamp = false;
    bool relative_format = false;
    std::string set_date = "";
    std::string add_time = "";
    std::string subtract_time = "";
    std::string reference_date = "";
    bool verbose = false;
    bool json_output = false;
    bool quiet = false;
    std::vector<std::string> input_dates;
    
    // Predefined formats
    std::map<std::string, std::string> predefined_formats = {
        {"iso", "%Y-%m-%dT%H:%M:%S"},
        {"iso-date", "%Y-%m-%d"},
        {"iso-time", "%H:%M:%S"},
        {"rfc", "%a, %d %b %Y %H:%M:%S %z"},
        {"short", "%Y-%m-%d"},
        {"long", "%A, %B %d, %Y %H:%M:%S"},
        {"time", "%H:%M:%S"},
        {"date", "%Y-%m-%d"},
        {"us", "%m/%d/%Y"},
        {"eu", "%d/%m/%Y"},
        {"compact", "%Y%m%d%H%M%S"},
        {"log", "%Y-%m-%d %H:%M:%S.%f"},
        {"sql", "%Y-%m-%d %H:%M:%S"}
    };
    
public:
    void setFormat(const std::string& fmt) { 
        if (predefined_formats.count(fmt)) {
            format = predefined_formats[fmt];
        } else {
            format = fmt;
        }
    }
    void setTimezone(const std::string& tz) { timezone = tz; }
    void setUTC(bool utc_flag) { utc = utc_flag; }
    void setISO(bool iso_flag) { iso_format = iso_flag; }
    void setRFC(bool rfc_flag) { rfc_format = rfc_flag; }
    void setUnixTimestamp(bool unix_flag) { unix_timestamp = unix_flag; }
    void setRelative(bool rel_flag) { relative_format = rel_flag; }
    void setDate(const std::string& date) { set_date = date; }
    void setAddTime(const std::string& time) { add_time = time; }
    void setSubtractTime(const std::string& time) { subtract_time = time; }
    void setReferenceDate(const std::string& date) { reference_date = date; }
    void setVerbose(bool verb) { verbose = verb; }
    void setJSON(bool json) { json_output = json; }
    void setQuiet(bool q) { quiet = q; }
    void addInputDate(const std::string& date) { input_dates.push_back(date); }
    
    std::chrono::system_clock::time_point parseDate(const std::string& date_str) {
        // Common date formats to try
        std::vector<std::string> formats = {
            "%Y-%m-%d %H:%M:%S",
            "%Y-%m-%dT%H:%M:%S",
            "%Y-%m-%d",
            "%m/%d/%Y",
            "%d/%m/%Y",
            "%Y%m%d",
            "%Y%m%d%H%M%S",
            "%a %b %d %H:%M:%S %Y"
        };
        
        std::tm tm = {};
        for (const auto& fmt : formats) {
            std::istringstream ss(date_str);
            ss >> std::get_time(&tm, fmt.c_str());
            if (!ss.fail()) {
                auto time_t_val = std::mktime(&tm);
                return std::chrono::system_clock::from_time_t(time_t_val);
            }
        }
        
        // Try parsing as Unix timestamp
        try {
            long long timestamp = std::stoll(date_str);
            return std::chrono::system_clock::from_time_t(timestamp);
        } catch (...) {
            throw std::runtime_error("Unable to parse date: " + date_str);
        }
    }
    
    std::chrono::seconds parseTimeAmount(const std::string& time_str) {
        std::regex pattern(R"((\d+)([smhdwy]))");
        std::smatch match;
        std::chrono::seconds total(0);
        
        std::string str = time_str;
        while (std::regex_search(str, match, pattern)) {
            int amount = std::stoi(match[1].str());
            char unit = match[2].str()[0];
            
            switch (unit) {
                case 's': total += std::chrono::seconds(amount); break;
                case 'm': total += std::chrono::minutes(amount); break;
                case 'h': total += std::chrono::hours(amount); break;
                case 'd': total += std::chrono::hours(24 * amount); break;
                case 'w': total += std::chrono::hours(24 * 7 * amount); break;
                case 'y': total += std::chrono::hours(24 * 365 * amount); break;
                default: throw std::runtime_error("Invalid time unit: " + std::string(1, unit));
            }
            
            str = match.suffix();
        }
        
        if (total.count() == 0) {
            throw std::runtime_error("Invalid time format: " + time_str);
        }
        
        return total;
    }
    
    std::string formatRelativeTime(const std::chrono::system_clock::time_point& tp) {
        auto now = std::chrono::system_clock::now();
        auto diff = std::chrono::duration_cast<std::chrono::seconds>(now - tp);
        
        if (diff.count() < 0) {
            diff = -diff;
            if (diff.count() < 60) return "in " + std::to_string(diff.count()) + " seconds";
            if (diff.count() < 3600) return "in " + std::to_string(diff.count() / 60) + " minutes";
            if (diff.count() < 86400) return "in " + std::to_string(diff.count() / 3600) + " hours";
            return "in " + std::to_string(diff.count() / 86400) + " days";
        } else {
            if (diff.count() < 60) return std::to_string(diff.count()) + " seconds ago";
            if (diff.count() < 3600) return std::to_string(diff.count() / 60) + " minutes ago";
            if (diff.count() < 86400) return std::to_string(diff.count() / 3600) + " hours ago";
            return std::to_string(diff.count() / 86400) + " days ago";
        }
    }
    
    std::string formatTime(const std::chrono::system_clock::time_point& tp) {
        auto time_t_val = std::chrono::system_clock::to_time_t(tp);
        std::tm* tm_ptr = utc ? std::gmtime(&time_t_val) : std::localtime(&time_t_val);
        
        if (!tm_ptr) {
            throw std::runtime_error("Failed to convert time");
        }
        
        if (unix_timestamp) {
            return std::to_string(time_t_val);
        }
        
        if (iso_format) {
            std::ostringstream oss;
            oss << std::put_time(tm_ptr, "%Y-%m-%dT%H:%M:%S");
            if (utc) oss << "Z";
            return oss.str();
        }
        
        if (rfc_format) {
            std::ostringstream oss;
            oss << std::put_time(tm_ptr, "%a, %d %b %Y %H:%M:%S");
            if (utc) oss << " +0000";
            return oss.str();
        }
        
        if (relative_format) {
            return formatRelativeTime(tp);
        }
        
        // Handle microseconds for log format
        if (format.find("%f") != std::string::npos) {
            auto epoch = tp.time_since_epoch();
            auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(epoch) % 1000000;
            std::string fmt = format;
            std::string microsec_str = std::to_string(microseconds.count());
            microsec_str = std::string(6 - microsec_str.length(), '0') + microsec_str;
            size_t pos = fmt.find("%f");
            fmt.replace(pos, 2, microsec_str);
            
            std::ostringstream oss;
            oss << std::put_time(tm_ptr, fmt.c_str());
            return oss.str();
        }
        
        std::ostringstream oss;
        oss << std::put_time(tm_ptr, format.c_str());
        return oss.str();
    }
    
    void outputJSON(const std::vector<std::pair<std::string, std::chrono::system_clock::time_point>>& results) {
        std::cout << "{\n";
        std::cout << "  \"results\": [\n";
        
        for (size_t i = 0; i < results.size(); ++i) {
            auto time_t_val = std::chrono::system_clock::to_time_t(results[i].second);
            std::cout << "    {\n";
            std::cout << "      \"formatted\": \"" << results[i].first << "\",\n";
            std::cout << "      \"unix_timestamp\": " << time_t_val << ",\n";
            std::cout << "      \"iso\": \"" << formatTimeISO(results[i].second) << "\",\n";
            std::cout << "      \"relative\": \"" << formatRelativeTime(results[i].second) << "\"\n";
            std::cout << "    }";
            if (i < results.size() - 1) std::cout << ",";
            std::cout << "\n";
        }
        
        std::cout << "  ],\n";
        std::cout << "  \"timezone\": \"" << (utc ? "UTC" : "local") << "\",\n";
        std::cout << "  \"format\": \"" << format << "\"\n";
        std::cout << "}\n";
    }
    
    std::string formatTimeISO(const std::chrono::system_clock::time_point& tp) {
        auto time_t_val = std::chrono::system_clock::to_time_t(tp);
        std::tm* tm_ptr = std::gmtime(&time_t_val);
        std::ostringstream oss;
        oss << std::put_time(tm_ptr, "%Y-%m-%dT%H:%M:%SZ");
        return oss.str();
    }
    
    int run() {
        try {
            std::vector<std::pair<std::string, std::chrono::system_clock::time_point>> results;
            
            // Determine the base time point
            std::chrono::system_clock::time_point base_time;
            
            if (!set_date.empty()) {
                base_time = parseDate(set_date);
                if (verbose && !quiet) {
                    std::cerr << "Using set date: " << set_date << std::endl;
                }
            } else if (!reference_date.empty()) {
                base_time = parseDate(reference_date);
                if (verbose && !quiet) {
                    std::cerr << "Using reference date: " << reference_date << std::endl;
                }
            } else {
                base_time = std::chrono::system_clock::now();
            }
            
            // Apply time arithmetic
            if (!add_time.empty()) {
                auto duration = parseTimeAmount(add_time);
                base_time += duration;
                if (verbose && !quiet) {
                    std::cerr << "Added: " << add_time << std::endl;
                }
            }
            
            if (!subtract_time.empty()) {
                auto duration = parseTimeAmount(subtract_time);
                base_time -= duration;
                if (verbose && !quiet) {
                    std::cerr << "Subtracted: " << subtract_time << std::endl;
                }
            }
            
            // Process input dates or use base time
            if (!input_dates.empty()) {
                for (const auto& input_date : input_dates) {
                    try {
                        auto tp = parseDate(input_date);
                        std::string formatted = formatTime(tp);
                        results.push_back({formatted, tp});
                    } catch (const std::exception& e) {
                        if (!quiet) {
                            std::cerr << "Error parsing date '" << input_date << "': " << e.what() << std::endl;
                        }
                        return 1;
                    }
                }
            } else {
                std::string formatted = formatTime(base_time);
                results.push_back({formatted, base_time});
            }
            
            // Output results
            if (json_output) {
                outputJSON(results);
            } else {
                for (const auto& result : results) {
                    std::cout << result.first << std::endl;
                }
            }
            
            return 0;
            
        } catch (const std::exception& e) {
            if (!quiet) {
                std::cerr << "date: error: " << e.what() << std::endl;
            }
            return 1;
        }
    }
};

void printUsage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [OPTIONS] [DATE...]\n\n"
              << "Advanced date/time utility with extensive formatting and calculation features.\n\n"
              << "Format Options:\n"
              << "  -f, --format=FORMAT      Custom format string (strftime-style)\n"
              << "  --preset=PRESET          Use predefined format (iso, rfc, short, long, etc.)\n"
              << "  --iso                    ISO 8601 format (YYYY-MM-DDTHH:MM:SS)\n"
              << "  --rfc                    RFC 2822 format\n"
              << "  --unix                   Unix timestamp\n"
              << "  --relative               Relative time (e.g., '2 hours ago')\n\n"
              << "Time Options:\n"
              << "  -u, --utc                Use UTC timezone\n"
              << "  -s, --set=DATE           Set base date/time\n"
              << "  -d, --date=DATE          Parse and format specific date\n"
              << "  -r, --reference=FILE     Use file modification time as reference\n\n"
              << "Arithmetic Options:\n"
              << "  --add=DURATION           Add time duration (e.g., 2h30m, 1d, 1w)\n"
              << "  --subtract=DURATION      Subtract time duration\n\n"
              << "Output Options:\n"
              << "  --json                   JSON output format\n"
              << "  -v, --verbose            Verbose output\n"
              << "  -q, --quiet              Quiet mode (errors only)\n\n"
              << "Standard Options:\n"
              << "  -h, --help               Show this help message\n"
              << "  -V, --version            Show version information\n\n"
              << "Predefined Formats:\n"
              << "  iso        - %Y-%m-%dT%H:%M:%S\n"
              << "  iso-date   - %Y-%m-%d\n"
              << "  iso-time   - %H:%M:%S\n"
              << "  rfc        - %a, %d %b %Y %H:%M:%S %z\n"
              << "  short      - %Y-%m-%d\n"
              << "  long       - %A, %B %d, %Y %H:%M:%S\n"
              << "  us         - %m/%d/%Y\n"
              << "  eu         - %d/%m/%Y\n"
              << "  compact    - %Y%m%d%H%M%S\n"
              << "  log        - %Y-%m-%d %H:%M:%S.%f\n\n"
              << "Duration Format:\n"
              << "  s = seconds, m = minutes, h = hours\n"
              << "  d = days, w = weeks, y = years\n"
              << "  Examples: 30s, 5m, 2h30m, 1d, 2w, 1y6m\n\n"
              << "Examples:\n"
              << "  " << program_name << " --iso\n"
              << "  " << program_name << " -f '%A, %B %d, %Y'\n"
              << "  " << program_name << " --preset=long\n"
              << "  " << program_name << " --add=2h30m\n"
              << "  " << program_name << " -s '2025-01-01' --add=1w\n"
              << "  " << program_name << " --relative -d '2025-01-01'\n"
              << "  " << program_name << " --json --unix\n"
              << "  " << program_name << " -u --rfc\n\n"
              << "Part of QCO MoreUtils by AnmiTaliDev\n"
              << "Repository: https://github.com/Qainar-Projects/MoreUtils\n";
}

void printVersion() {
    std::cout << "date 1.0.0\n"
              << "Part of QCO MoreUtils - Advanced System Development More Utilities\n"
              << "Copyright 2025 AnmiTaliDev\n"
              << "Licensed under the Apache License, Version 2.0\n"
              << "Repository: https://github.com/Qainar-Projects/MoreUtils\n";
}

int main(int argc, char* argv[]) {
    DateUtility date_util;
    
    static struct option long_options[] = {
        {"format",      required_argument, 0, 'f'},
        {"preset",      required_argument, 0, 5001},
        {"iso",         no_argument,       0, 5002},
        {"rfc",         no_argument,       0, 5003},
        {"unix",        no_argument,       0, 5004},
        {"relative",    no_argument,       0, 5005},
        {"utc",         no_argument,       0, 'u'},
        {"set",         required_argument, 0, 's'},
        {"date",        required_argument, 0, 'd'},
        {"reference",   required_argument, 0, 'r'},
        {"add",         required_argument, 0, 5006},
        {"subtract",    required_argument, 0, 5007},
        {"json",        no_argument,       0, 5008},
        {"verbose",     no_argument,       0, 'v'},
        {"quiet",       no_argument,       0, 'q'},
        {"help",        no_argument,       0, 'h'},
        {"version",     no_argument,       0, 'V'},
        {0, 0, 0, 0}
    };
    
    int opt;
    int option_index = 0;
    
    while ((opt = getopt_long(argc, argv, "f:us:d:r:vqhV", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'f':
                date_util.setFormat(optarg);
                break;
            case 5001: // preset
                date_util.setFormat(optarg);
                break;
            case 5002: // iso
                date_util.setISO(true);
                break;
            case 5003: // rfc
                date_util.setRFC(true);
                break;
            case 5004: // unix
                date_util.setUnixTimestamp(true);
                break;
            case 5005: // relative
                date_util.setRelative(true);
                break;
            case 'u':
                date_util.setUTC(true);
                break;
            case 's':
                date_util.setDate(optarg);
                break;
            case 'd':
                date_util.addInputDate(optarg);
                break;
            case 'r':
                // TODO: Implement file reference time
                std::cerr << "Reference file option not yet implemented\n";
                break;
            case 5006: // add
                date_util.setAddTime(optarg);
                break;
            case 5007: // subtract
                date_util.setSubtractTime(optarg);
                break;
            case 5008: // json
                date_util.setJSON(true);
                break;
            case 'v':
                date_util.setVerbose(true);
                break;
            case 'q':
                date_util.setQuiet(true);
                break;
            case 'h':
                printUsage(argv[0]);
                return 0;
            case 'V':
                printVersion();
                return 0;
            case '?':
                std::cerr << "Try '" << argv[0] << " --help' for more information.\n";
                return 1;
            default:
                return 1;
        }
    }
    
    // Process remaining arguments as dates
    for (int i = optind; i < argc; i++) {
        date_util.addInputDate(argv[i]);
    }
    
    return date_util.run();
}