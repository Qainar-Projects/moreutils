/*
 * ping - Advanced network connectivity testing utility
 * Part of QCO MoreUtils - Advanced System Development More Utilities
 * 
 * Copyright 2025 AnmiTaliDev
 * Licensed under the Apache License, Version 2.0
 * 
 * Repository: https://github.com/Qainar-Projects/MoreUtils
 * 
 * An enhanced ping utility with advanced features including
 * continuous monitoring, statistics, multiple target support,
 * and flexible output formats for network diagnostics.
 */

#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <iomanip>
#include <getopt.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <sys/time.h>

class PingUtility {
private:
    std::vector<std::string> targets;
    int count = 4;
    int interval = 1000; // milliseconds
    int timeout = 3000;  // milliseconds
    int packet_size = 56;
    bool continuous = false;
    bool quiet = false;
    bool verbose = false;
    bool use_colors = true;
    bool show_statistics = true;
    bool timestamp = false;
    bool resolve_dns = true;
    std::string output_format = "default";
    std::string log_file = "";
    int ttl = 64;
    bool flood_mode = false;
    bool audible = false;
    
    // Statistics
    std::vector<double> response_times;
    int packets_sent = 0;
    int packets_received = 0;
    int packets_lost = 0;
    
    // Color codes
    const std::string RESET = "\033[0m";
    const std::string BOLD = "\033[1m";
    const std::string RED = "\033[31m";
    const std::string GREEN = "\033[32m";
    const std::string YELLOW = "\033[33m";
    const std::string BLUE = "\033[34m";
    const std::string CYAN = "\033[36m";
    const std::string DIM = "\033[2m";
    
    volatile bool running = true;
    
public:
    void setTargets(const std::vector<std::string>& t) { targets = t; }
    void setCount(int c) { count = c; }
    void setInterval(int i) { interval = i; }
    void setTimeout(int t) { timeout = t; }
    void setPacketSize(int size) { packet_size = size; }
    void setContinuous(bool cont) { continuous = cont; }
    void setQuiet(bool q) { quiet = q; }
    void setVerbose(bool v) { verbose = v; }
    void setColors(bool colors) { use_colors = colors; }
    void setShowStatistics(bool stats) { show_statistics = stats; }
    void setTimestamp(bool ts) { timestamp = ts; }
    void setResolveDNS(bool resolve) { resolve_dns = resolve; }
    void setOutputFormat(const std::string& format) { output_format = format; }
    void setLogFile(const std::string& file) { log_file = file; }
    void setTTL(int t) { ttl = t; }
    void setFloodMode(bool flood) { flood_mode = flood; }
    void setAudible(bool audio) { audible = audio; }
    void setRunning(bool r) { running = r; }
    
    std::string getColor(const std::string& color) {
        return use_colors ? color : "";
    }
    
    std::string getCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%H:%M:%S");
        ss << "." << std::setfill('0') << std::setw(3) << ms.count();
        return ss.str();
    }
    
    std::string resolveHostname(const std::string& hostname) {
        if (!resolve_dns) return hostname;
        
        struct hostent* host_entry = gethostbyname(hostname.c_str());
        if (host_entry == nullptr) {
            return hostname; // Return original if resolution fails
        }
        
        struct in_addr addr;
        addr.s_addr = *((unsigned long *)host_entry->h_addr_list[0]);
        return inet_ntoa(addr);
    }
    
    double pingHost(const std::string& target) {
        int sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
        if (sock < 0) {
            // Fallback to system ping if raw socket fails (need root)
            return systemPing(target);
        }
        
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        inet_pton(AF_INET, resolveHostname(target).c_str(), &addr.sin_addr);
        
        // Create ICMP packet
        struct icmp icmp_hdr;
        icmp_hdr.icmp_type = ICMP_ECHO;
        icmp_hdr.icmp_code = 0;
        icmp_hdr.icmp_id = getpid();
        icmp_hdr.icmp_seq = packets_sent;
        icmp_hdr.icmp_cksum = 0;
        
        // Calculate checksum
        icmp_hdr.icmp_cksum = calculateChecksum(&icmp_hdr, sizeof(icmp_hdr));
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        if (sendto(sock, &icmp_hdr, sizeof(icmp_hdr), 0, 
                   (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            close(sock);
            return -1.0;
        }
        
        // Wait for reply with timeout
        fd_set readfds;
        struct timeval tv;
        tv.tv_sec = timeout / 1000;
        tv.tv_usec = (timeout % 1000) * 1000;
        
        FD_ZERO(&readfds);
        FD_SET(sock, &readfds);
        
        if (select(sock + 1, &readfds, nullptr, nullptr, &tv) <= 0) {
            close(sock);
            return -1.0; // Timeout
        }
        
        char buffer[1024];
        if (recv(sock, buffer, sizeof(buffer), 0) < 0) {
            close(sock);
            return -1.0;
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
            end_time - start_time);
        
        close(sock);
        return duration.count() / 1000.0; // Return milliseconds
    }
    
    double systemPing(const std::string& target) {
        // Fallback to system ping command for non-root users
        std::string cmd = "ping -c 1 -W " + std::to_string(timeout / 1000) + 
                         " " + target + " 2>/dev/null | grep 'time=' | " +
                         "sed 's/.*time=\\([0-9.]*\\).*/\\1/'";
        
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) return -1.0;
        
        char buffer[128];
        std::string result;
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result += buffer;
        }
        pclose(pipe);
        
        try {
            return std::stod(result);
        } catch (...) {
            return -1.0;
        }
    }
    
    uint16_t calculateChecksum(void* data, int len) {
        uint16_t* ptr = (uint16_t*)data;
        uint32_t sum = 0;
        
        while (len > 1) {
            sum += *ptr++;
            len -= 2;
        }
        
        if (len == 1) {
            sum += *(uint8_t*)ptr;
        }
        
        while (sum >> 16) {
            sum = (sum & 0xFFFF) + (sum >> 16);
        }
        
        return ~sum;
    }
    
    void printPingResult(const std::string& target, double time_ms, int seq) {
        if (quiet && time_ms > 0) return;
        
        std::string timestamp_str = "";
        if (timestamp) {
            timestamp_str = "[" + getCurrentTimestamp() + "] ";
        }
        
        if (time_ms < 0) {
            // Timeout or error
            std::cout << timestamp_str << getColor(RED) << "Request timeout for " 
                      << target << " (seq=" << seq << ")" << getColor(RESET) << "\n";
            packets_lost++;
            if (audible) std::cout << "\a";
        } else {
            // Success
            std::string color = getColor(GREEN);
            if (time_ms > 100) color = getColor(YELLOW);
            if (time_ms > 500) color = getColor(RED);
            
            if (output_format == "json") {
                std::cout << "{\"target\":\"" << target << "\",\"time\":" 
                          << std::fixed << std::setprecision(3) << time_ms 
                          << ",\"seq\":" << seq << "}\n";
            } else if (output_format == "csv") {
                std::cout << target << "," << std::fixed << std::setprecision(3) 
                          << time_ms << "," << seq << "\n";
            } else {
                std::cout << timestamp_str << color << "Reply from " << target 
                          << ": time=" << std::fixed << std::setprecision(3) 
                          << time_ms << "ms seq=" << seq << getColor(RESET) << "\n";
            }
            
            response_times.push_back(time_ms);
            packets_received++;
        }
        
        packets_sent++;
    }
    
    void printStatistics() {
        if (!show_statistics || quiet) return;
        
        std::cout << "\n" << getColor(BOLD) << "--- Ping Statistics ---" << getColor(RESET) << "\n";
        
        double loss_percentage = packets_sent > 0 ? 
            (static_cast<double>(packets_lost) / packets_sent) * 100.0 : 0.0;
        
        std::cout << packets_sent << " packets transmitted, " 
                  << packets_received << " received, ";
        
        if (loss_percentage > 0) {
            std::cout << getColor(RED) << std::fixed << std::setprecision(1) 
                      << loss_percentage << "% packet loss" << getColor(RESET);
        } else {
            std::cout << getColor(GREEN) << "0% packet loss" << getColor(RESET);
        }
        std::cout << "\n";
        
        if (!response_times.empty()) {
            double min_time = *std::min_element(response_times.begin(), response_times.end());
            double max_time = *std::max_element(response_times.begin(), response_times.end());
            double avg_time = std::accumulate(response_times.begin(), response_times.end(), 0.0) 
                             / response_times.size();
            
            // Calculate standard deviation
            double variance = 0.0;
            for (double time : response_times) {
                variance += (time - avg_time) * (time - avg_time);
            }
            variance /= response_times.size();
            double std_dev = std::sqrt(variance);
            
            std::cout << "round-trip min/avg/max/stddev = " 
                      << std::fixed << std::setprecision(3)
                      << min_time << "/" << avg_time << "/" 
                      << max_time << "/" << std_dev << " ms\n";
        }
    }
    
    void printHeader() {
        if (quiet) return;
        
        for (const auto& target : targets) {
            std::string resolved = resolveHostname(target);
            std::cout << getColor(BOLD) << "PING " << target;
            if (resolved != target) {
                std::cout << " (" << resolved << ")";
            }
            std::cout << " " << packet_size << " bytes of data" << getColor(RESET) << "\n";
        }
        std::cout << "\n";
    }
    
    int run() {
        if (targets.empty()) {
            std::cerr << "ping: no targets specified\n";
            return 1;
        }
        
        printHeader();
        
        int sequence = 1;
        int completed_pings = 0;
        
        while (running && (continuous || completed_pings < count)) {
            for (const auto& target : targets) {
                if (!running) break;
                
                double ping_time = pingHost(target);
                printPingResult(target, ping_time, sequence);
                
                if (!continuous && !flood_mode) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(interval));
                }
                
                sequence++;
            }
            
            completed_pings++;
            
            if (!continuous && completed_pings >= count) {
                break;
            }
            
            if (flood_mode) {
                // Minimal delay in flood mode
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            } else if (continuous) {
                std::this_thread::sleep_for(std::chrono::milliseconds(interval));
            }
        }
        
        printStatistics();
        return 0;
    }
};

// Global ping instance for signal handling
PingUtility* global_ping = nullptr;

void signalHandler(int signal) {
    if (global_ping) {
        global_ping->setRunning(false);
    }
}

void printUsage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [OPTIONS] TARGET [TARGET...]\n\n"
              << "Advanced network connectivity testing utility.\n\n"
              << "Basic Options:\n"
              << "  -c, --count COUNT        Number of pings to send (default: 4)\n"
              << "  -i, --interval MS        Interval between pings in ms (default: 1000)\n"
              << "  -W, --timeout MS         Timeout for each ping in ms (default: 3000)\n"
              << "  -s, --size BYTES         Packet size in bytes (default: 56)\n"
              << "  -t, --ttl TTL            Time to live (default: 64)\n\n"
              << "Continuous Options:\n"
              << "  -o, --continuous         Ping continuously until stopped\n"
              << "  -f, --flood              Flood ping (minimal interval)\n\n"
              << "Output Options:\n"
              << "  -q, --quiet              Quiet mode (errors only)\n"
              << "  -v, --verbose            Verbose output\n"
              << "  -T, --timestamp          Add timestamps to output\n"
              << "  -a, --audible            Audible ping (beep on timeout)\n"
              << "  --no-colors              Disable colored output\n"
              << "  --no-stats               Disable statistics\n"
              << "  --no-dns                 Don't resolve hostnames\n\n"
              << "Format Options:\n"
              << "  --format FORMAT          Output format (default/json/csv)\n"
              << "  --log FILE               Log results to file\n\n"
              << "Standard Options:\n"
              << "  -h, --help               Show this help message\n"
              << "  -V, --version            Show version information\n\n"
              << "Examples:\n"
              << "  " << program_name << " google.com\n"
              << "  " << program_name << " -c 10 -i 500 8.8.8.8\n"
              << "  " << program_name << " -o -T google.com cloudflare.com\n"
              << "  " << program_name << " --format json -c 5 example.com\n"
              << "  " << program_name << " -f --no-stats 192.168.1.1\n"
              << "  " << program_name << " -a -v -W 5000 slow-server.com\n\n"
              << "Part of QCO MoreUtils by AnmiTaliDev\n"
              << "Repository: https://github.com/Qainar-Projects/MoreUtils\n";
}

void printVersion() {
    std::cout << "ping 1.0.0\n"
              << "Part of QCO MoreUtils - Advanced System Development More Utilities\n"
              << "Copyright 2025 AnmiTaliDev\n"
              << "Licensed under the Apache License, Version 2.0\n"
              << "Repository: https://github.com/Qainar-Projects/MoreUtils\n";
}

int main(int argc, char* argv[]) {
    PingUtility ping;
    global_ping = &ping;
    
    // Set up signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    static struct option long_options[] = {
        {"count",       required_argument, 0, 'c'},
        {"interval",    required_argument, 0, 'i'},
        {"timeout",     required_argument, 0, 'W'},
        {"size",        required_argument, 0, 's'},
        {"ttl",         required_argument, 0, 't'},
        {"continuous",  no_argument,       0, 'o'},
        {"flood",       no_argument,       0, 'f'},
        {"quiet",       no_argument,       0, 'q'},
        {"verbose",     no_argument,       0, 'v'},
        {"timestamp",   no_argument,       0, 'T'},
        {"audible",     no_argument,       0, 'a'},
        {"no-colors",   no_argument,       0, 3001},
        {"no-stats",    no_argument,       0, 3002},
        {"no-dns",      no_argument,       0, 3003},
        {"format",      required_argument, 0, 3004},
        {"log",         required_argument, 0, 3005},
        {"help",        no_argument,       0, 'h'},
        {"version",     no_argument,       0, 'V'},
        {0, 0, 0, 0}
    };
    
    int opt;
    int option_index = 0;
    
    while ((opt = getopt_long(argc, argv, "c:i:W:s:t:ofqvTahV", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'c':
                ping.setCount(std::stoi(optarg));
                break;
            case 'i':
                ping.setInterval(std::stoi(optarg));
                break;
            case 'W':
                ping.setTimeout(std::stoi(optarg));
                break;
            case 's':
                ping.setPacketSize(std::stoi(optarg));
                break;
            case 't':
                ping.setTTL(std::stoi(optarg));
                break;
            case 'o':
                ping.setContinuous(true);
                break;
            case 'f':
                ping.setFloodMode(true);
                break;
            case 'q':
                ping.setQuiet(true);
                break;
            case 'v':
                ping.setVerbose(true);
                break;
            case 'T':
                ping.setTimestamp(true);
                break;
            case 'a':
                ping.setAudible(true);
                break;
            case 3001: // no-colors
                ping.setColors(false);
                break;
            case 3002: // no-stats
                ping.setShowStatistics(false);
                break;
            case 3003: // no-dns
                ping.setResolveDNS(false);
                break;
            case 3004: // format
                ping.setOutputFormat(optarg);
                break;
            case 3005: // log
                ping.setLogFile(optarg);
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
    
    // Collect target hosts
    std::vector<std::string> targets;
    for (int i = optind; i < argc; i++) {
        targets.push_back(argv[i]);
    }
    
    if (targets.empty()) {
        std::cerr << "ping: missing target host\n";
        std::cerr << "Try '" << argv[0] << " --help' for more information.\n";
        return 1;
    }
    
    ping.setTargets(targets);
    
    try {
        return ping.run();
    } catch (const std::exception& e) {
        std::cerr << "ping: error: " << e.what() << "\n";
        return 1;
    }
}