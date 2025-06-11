/*
 * uptime - Flexible system uptime utility
 * Part of QCO MoreUtils - Quality Control Operations More Utilities
 * 
 * Copyright 2025 AnmiTaliDev
 * Licensed under the Apache License, Version 2.0
 * 
 * Repository: https://github.com/Qainar-Projects/MoreUtils
 * 
 * A comprehensive system uptime utility that displays system uptime,
 * load averages, and user information with flexible output formats.
 */

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <algorithm>
#include <getopt.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <utmp.h>

class UptimeUtility {
private:
    bool brief = false;
    bool pretty = false;
    bool load_only = false;
    bool uptime_only = false;
    bool users_only = false;
    bool quiet = false;
    
public:
    void setBrief(bool b) { brief = b; }
    void setPretty(bool p) { pretty = p; }
    void setLoadOnly(bool l) { load_only = l; }
    void setUptimeOnly(bool u) { uptime_only = u; }
    void setUsersOnly(bool u) { users_only = u; }
    void setQuiet(bool q) { quiet = q; }
    
    double getUptime() {
        std::ifstream file("/proc/uptime");
        if (!file.is_open()) {
            throw std::runtime_error("Error reading uptime from /proc/uptime");
        }
        
        double uptime;
        file >> uptime;
        return uptime;
    }
    
    std::vector<std::string> getLoadAverage() {
        std::ifstream file("/proc/loadavg");
        if (!file.is_open()) {
            throw std::runtime_error("Error reading load average from /proc/loadavg");
        }
        
        std::vector<std::string> loads(3);
        file >> loads[0] >> loads[1] >> loads[2];
        return loads;
    }
    
    int getUserCount() {
        setutent();
        
        std::vector<std::string> unique_users;
        struct utmp *entry;
        
        while ((entry = getutent()) != nullptr) {
            if (entry->ut_type == USER_PROCESS) {
                std::string username(entry->ut_user);
                if (std::find(unique_users.begin(), unique_users.end(), username) == unique_users.end()) {
                    unique_users.push_back(username);
                }
            }
        }
        
        endutent();
        return unique_users.size();
    }
    
    std::string formatUptime(double seconds, bool pretty_format = false) {
        if (!pretty_format) {
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(2) << seconds << " seconds";
            return oss.str();
        }
        
        std::vector<std::string> components;
        
        int days = static_cast<int>(seconds / 86400);
        if (days > 0) {
            components.push_back(std::to_string(days) + " day" + (days != 1 ? "s" : ""));
        }
        
        int hours = static_cast<int>((static_cast<int>(seconds) % 86400) / 3600);
        if (hours > 0) {
            components.push_back(std::to_string(hours) + " hour" + (hours != 1 ? "s" : ""));
        }
        
        int minutes = static_cast<int>((static_cast<int>(seconds) % 3600) / 60);
        if (minutes > 0 || components.empty()) {
            components.push_back(std::to_string(minutes) + " minute" + (minutes != 1 ? "s" : ""));
        }
        
        std::string result;
        for (size_t i = 0; i < components.size(); ++i) {
            if (i > 0) result += ", ";
            result += components[i];
        }
        
        return result;
    }
    
    std::string getCurrentTime() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto tm = *std::localtime(&time_t);
        
        std::ostringstream oss;
        oss << std::put_time(&tm, "%H:%M:%S");
        return oss.str();
    }
    
    std::string joinVector(const std::vector<std::string>& vec, const std::string& delimiter) {
        if (vec.empty()) return "";
        
        std::string result = vec[0];
        for (size_t i = 1; i < vec.size(); ++i) {
            result += delimiter + vec[i];
        }
        return result;
    }
    
    int run() {
        try {
            // Get system data
            double uptime = getUptime();
            std::vector<std::string> load = getLoadAverage();
            int users = getUserCount();
            
            // Handle special output modes
            if (uptime_only || load_only || users_only) {
                std::vector<std::string> outputs;
                
                if (uptime_only) {
                    if (brief) {
                        std::ostringstream oss;
                        oss << std::fixed << std::setprecision(2) << uptime;
                        outputs.push_back(oss.str());
                    } else {
                        outputs.push_back(formatUptime(uptime, pretty));
                    }
                }
                
                if (load_only) {
                    if (brief) {
                        outputs.push_back(joinVector(load, ","));
                    } else {
                        outputs.push_back(joinVector(load, " "));
                    }
                }
                
                if (users_only) {
                    if (brief) {
                        outputs.push_back(std::to_string(users));
                    } else {
                        outputs.push_back(std::to_string(users) + " user" + (users != 1 ? "s" : ""));
                    }
                }
                
                for (const auto& output : outputs) {
                    std::cout << output << std::endl;
                }
            } else {
                // Default output format
                std::string time_str = getCurrentTime();
                std::string uptime_str = formatUptime(uptime, true);
                std::string user_str = std::to_string(users) + " user" + (users != 1 ? "s" : "");
                std::string load_str = joinVector(load, ", ");
                
                std::cout << time_str << " up " << uptime_str << ", " 
                          << user_str << ", load average: " << load_str << std::endl;
            }
            
            return 0;
            
        } catch (const std::exception& e) {
            if (!quiet) {
                std::cerr << "uptime: error: " << e.what() << std::endl;
            }
            return 1;
        }
    }
};

void printUsage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [OPTIONS]\n\n"
              << "Flexible system uptime utility.\n\n"
              << "Options:\n"
              << "  -b, --brief              Machine-friendly numerical output\n"
              << "  -p, --pretty             Human-readable time format\n"
              << "  -l, --load               Display load averages only\n"
              << "  -u, --uptime             Display uptime only\n"
              << "  -w, --users              Display user count only\n"
              << "  -s, --since              Show boot time\n"
              << "  -q, --quiet              Quiet mode (errors only)\n"
              << "  -h, --help               Show this help message\n"
              << "  -v, --version            Show version information\n\n"
              << "Output Modes:\n"
              << "  Default                  Full uptime information\n"
              << "  --brief                  Numerical values only\n"
              << "  --pretty                 Human-readable format\n\n"
              << "Individual Components:\n"
              << "  --uptime                 System uptime only\n"
              << "  --load                   Load averages only\n"
              << "  --users                  User count only\n\n"
              << "Examples:\n"
              << "  " << program_name << "                      # Standard output\n"
              << "  " << program_name << " --brief --uptime     # Raw uptime seconds\n"
              << "  " << program_name << " --pretty --uptime    # Human-readable uptime\n"
              << "  " << program_name << " --load               # Load averages only\n"
              << "  " << program_name << " --users              # User count only\n"
              << "  " << program_name << " --since              # Boot time\n\n"
              << "Standard Output Format:\n"
              << "  HH:MM:SS up X days, Y hours, Z minutes, N users, load average: A, B, C\n\n"
              << "Part of QCO MoreUtils by AnmiTaliDev\n"
              << "Repository: https://github.com/Qainar-Projects/MoreUtils\n";
}

void printVersion() {
    std::cout << "uptime 1.0.0\n"
              << "Part of QCO MoreUtils - Quality Control Operations More Utilities\n"
              << "Copyright 2025 AnmiTaliDev\n"
              << "Licensed under the Apache License, Version 2.0\n"
              << "Repository: https://github.com/Qainar-Projects/MoreUtils\n";
}

void printSince() {
    try {
        struct sysinfo si;
        if (sysinfo(&si) != 0) {
            throw std::runtime_error("Failed to get system information");
        }
        
        auto now = std::chrono::system_clock::now();
        auto boot_time = now - std::chrono::seconds(si.uptime);
        auto boot_time_t = std::chrono::system_clock::to_time_t(boot_time);
        auto boot_tm = *std::localtime(&boot_time_t);
        
        std::cout << std::put_time(&boot_tm, "%Y-%m-%d %H:%M:%S") << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "uptime: error getting boot time: " << e.what() << std::endl;
    }
}

int main(int argc, char* argv[]) {
    UptimeUtility uptime;
    bool show_since = false;
    
    static struct option long_options[] = {
        {"brief",    no_argument, 0, 'b'},
        {"pretty",   no_argument, 0, 'p'},
        {"load",     no_argument, 0, 'l'},
        {"uptime",   no_argument, 0, 'u'},
        {"users",    no_argument, 0, 'w'},
        {"since",    no_argument, 0, 's'},
        {"quiet",    no_argument, 0, 'q'},
        {"help",     no_argument, 0, 'h'},
        {"version",  no_argument, 0, 'v'},
        {0, 0, 0, 0}
    };
    
    int opt;
    int option_index = 0;
    
    while ((opt = getopt_long(argc, argv, "bpluwsqhv", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'b':
                uptime.setBrief(true);
                break;
            case 'p':
                uptime.setPretty(true);
                break;
            case 'l':
                uptime.setLoadOnly(true);
                break;
            case 'u':
                uptime.setUptimeOnly(true);
                break;
            case 'w':
                uptime.setUsersOnly(true);
                break;
            case 's':
                show_since = true;
                break;
            case 'q':
                uptime.setQuiet(true);
                break;
            case 'h':
                printUsage(argv[0]);
                return 0;
            case 'v':
                printVersion();
                return 0;
            case '?':
                std::cerr << "Try '" << argv[0] << " --help' for more information.\n";
                return 1;
            default:
                return 1;
        }
    }
    
    if (show_since) {
        printSince();
        return 0;
    }
    
    return uptime.run();
}