/*
 * whois - Domain and IP address WHOIS lookup utility
 * Part of QCO MoreUtils - Quality Control Operations More Utilities
 * 
 * Copyright 2025 AnmiTaliDev
 * Licensed under the Apache License, Version 2.0
 * 
 * Repository: https://github.com/Qainar-Projects/MoreUtils
 * 
 * A comprehensive WHOIS client for querying domain registration information,
 * IP address allocation data, and network information with support for
 * multiple WHOIS servers and output formats.
 */

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <regex>
#include <sstream>
#include <algorithm>
#include <getopt.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

class WhoisClient {
private:
    std::string server = "";
    int port = 43;
    bool follow_referrals = true;
    bool raw_output = false;
    bool json_output = false;
    bool verbose = false;
    bool quiet = false;
    int timeout = 30;
    std::string output_file = "";
    
    // Default WHOIS servers by TLD
    std::map<std::string, std::string> tld_servers = {
        {"com", "whois.verisign-grs.com"},
        {"net", "whois.verisign-grs.com"},
        {"org", "whois.pir.org"},
        {"info", "whois.afilias.net"},
        {"biz", "whois.neulevel.biz"},
        {"us", "whois.nic.us"},
        {"uk", "whois.nic.uk"},
        {"de", "whois.denic.de"},
        {"fr", "whois.afnic.fr"},
        {"jp", "whois.jprs.jp"},
        {"cn", "whois.cnnic.cn"},
        {"ru", "whois.tcinet.ru"},
        {"br", "whois.registro.br"},
        {"au", "whois.auda.org.au"},
        {"ca", "whois.cira.ca"},
        {"edu", "whois.educause.edu"},
        {"gov", "whois.dotgov.gov"},
        {"mil", "whois.nic.mil"},
        {"int", "whois.iana.org"}
    };
    
    // IP WHOIS servers
    std::vector<std::string> ip_servers = {
        "whois.arin.net",
        "whois.ripe.net", 
        "whois.apnic.net",
        "whois.lacnic.net",
        "whois.afrinic.net"
    };
    
public:
    void setServer(const std::string& srv) { server = srv; }
    void setPort(int p) { port = p; }
    void setFollowReferrals(bool follow) { follow_referrals = follow; }
    void setRawOutput(bool raw) { raw_output = raw; }
    void setJsonOutput(bool json) { json_output = json; }
    void setVerbose(bool verb) { verbose = verb; }
    void setQuiet(bool q) { quiet = q; }
    void setTimeout(int t) { timeout = t; }
    void setOutputFile(const std::string& file) { output_file = file; }
    
    bool isIPAddress(const std::string& query) {
        std::regex ipv4_pattern(R"(^(?:[0-9]{1,3}\.){3}[0-9]{1,3}$)");
        std::regex ipv6_pattern(R"(^(?:[0-9a-fA-F]{1,4}:){7}[0-9a-fA-F]{1,4}$)");
        return std::regex_match(query, ipv4_pattern) || std::regex_match(query, ipv6_pattern);
    }
    
    std::string extractTLD(const std::string& domain) {
        size_t last_dot = domain.find_last_of('.');
        if (last_dot != std::string::npos && last_dot < domain.length() - 1) {
            return domain.substr(last_dot + 1);
        }
        return "";
    }
    
    std::string selectServer(const std::string& query) {
        if (!server.empty()) {
            return server;
        }
        
        if (isIPAddress(query)) {
            return "whois.arin.net"; // Start with ARIN for IP queries
        }
        
        std::string tld = extractTLD(query);
        std::transform(tld.begin(), tld.end(), tld.begin(), ::tolower);
        
        if (tld_servers.count(tld)) {
            return tld_servers[tld];
        }
        
        return "whois.internic.net"; // Default fallback
    }
    
    std::string performQuery(const std::string& query, const std::string& whois_server) {
        if (verbose && !quiet) {
            std::cerr << "Querying " << whois_server << " for: " << query << std::endl;
        }
        
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            throw std::runtime_error("Failed to create socket");
        }
        
        struct hostent* host = gethostbyname(whois_server.c_str());
        if (!host) {
            close(sock);
            throw std::runtime_error("Failed to resolve WHOIS server: " + whois_server);
        }
        
        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        server_addr.sin_addr = *((struct in_addr*)host->h_addr);
        
        // Set timeout
        struct timeval tv;
        tv.tv_sec = timeout;
        tv.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof(tv));
        
        if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            close(sock);
            throw std::runtime_error("Failed to connect to WHOIS server: " + whois_server);
        }
        
        // Send query
        std::string query_string = query + "\r\n";
        if (send(sock, query_string.c_str(), query_string.length(), 0) < 0) {
            close(sock);
            throw std::runtime_error("Failed to send query");
        }
        
        // Receive response
        std::string response;
        char buffer[4096];
        ssize_t bytes_received;
        
        while ((bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
            buffer[bytes_received] = '\0';
            response += buffer;
        }
        
        close(sock);
        
        if (response.empty()) {
            throw std::runtime_error("No response from WHOIS server");
        }
        
        return response;
    }
    
    std::string extractReferralServer(const std::string& response) {
        std::vector<std::string> patterns = {
            R"(ReferralServer:\s*whois://([^\s]+))",
            R"(Whois Server:\s*([^\s]+))",
            R"(whois:\s*([^\s]+))",
            R"(refer:\s*([^\s]+))"
        };
        
        for (const auto& pattern : patterns) {
            std::regex regex(pattern, std::regex_constants::icase);
            std::smatch match;
            if (std::regex_search(response, match, regex)) {
                return match[1].str();
            }
        }
        
        return "";
    }
    
    std::map<std::string, std::string> parseResponse(const std::string& response) {
        std::map<std::string, std::string> data;
        std::istringstream stream(response);
        std::string line;
        
        while (std::getline(stream, line)) {
            // Remove carriage return
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            
            // Skip empty lines and comments
            if (line.empty() || line[0] == '%' || line[0] == '#') {
                continue;
            }
            
            size_t colon_pos = line.find(':');
            if (colon_pos != std::string::npos) {
                std::string key = line.substr(0, colon_pos);
                std::string value = line.substr(colon_pos + 1);
                
                // Trim whitespace
                key.erase(0, key.find_first_not_of(" \t"));
                key.erase(key.find_last_not_of(" \t") + 1);
                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \t") + 1);
                
                if (!key.empty() && !value.empty()) {
                    data[key] = value;
                }
            }
        }
        
        return data;
    }
    
    void outputJSON(const std::string& query, const std::string& response, 
                   const std::string& server_used) {
        auto parsed_data = parseResponse(response);
        
        std::cout << "{\n";
        std::cout << "  \"query\": \"" << query << "\",\n";
        std::cout << "  \"server\": \"" << server_used << "\",\n";
        std::cout << "  \"data\": {\n";
        
        bool first = true;
        for (const auto& pair : parsed_data) {
            if (!first) std::cout << ",\n";
            std::cout << "    \"" << pair.first << "\": \"" << pair.second << "\"";
            first = false;
        }
        
        std::cout << "\n  },\n";
        std::cout << "  \"raw_response\": \"" << escapeJSON(response) << "\"\n";
        std::cout << "}\n";
    }
    
    std::string escapeJSON(const std::string& str) {
        std::string result;
        for (char c : str) {
            switch (c) {
                case '"': result += "\\\""; break;
                case '\\': result += "\\\\"; break;
                case '\n': result += "\\n"; break;
                case '\r': result += "\\r"; break;
                case '\t': result += "\\t"; break;
                default: result += c; break;
            }
        }
        return result;
    }
    
    void outputSummary(const std::string& response) {
        auto data = parseResponse(response);
        
        // Key fields to display in summary
        std::vector<std::string> important_fields = {
            "Domain Name", "domain", "domain_name",
            "Registrar", "registrar", 
            "Registration Date", "Created", "created", "Creation Date",
            "Expiration Date", "Expires", "expires", "Registry Expiry Date",
            "Status", "status", "Domain Status",
            "Name Server", "name_server", "Name Servers",
            "Organization", "org", "Organization Name",
            "Country", "country", "Country Code",
            "Updated Date", "updated", "Last Updated"
        };
        
        std::cout << "WHOIS Summary:\n";
        std::cout << "==============\n\n";
        
        for (const auto& field : important_fields) {
            if (data.count(field)) {
                std::cout << field << ": " << data[field] << "\n";
            }
        }
    }
    
    int query(const std::string& target) {
        try {
            std::string whois_server = selectServer(target);
            std::string response = performQuery(target, whois_server);
            
            // Check for referral and follow if enabled
            if (follow_referrals && !isIPAddress(target)) {
                std::string referral_server = extractReferralServer(response);
                if (!referral_server.empty() && referral_server != whois_server) {
                    if (verbose && !quiet) {
                        std::cerr << "Following referral to: " << referral_server << std::endl;
                    }
                    response = performQuery(target, referral_server);
                    whois_server = referral_server;
                }
            }
            
            // Output results
            if (json_output) {
                outputJSON(target, response, whois_server);
            } else if (raw_output) {
                std::cout << response;
            } else {
                outputSummary(response);
                if (verbose) {
                    std::cout << "\nFull Response:\n";
                    std::cout << "==============\n";
                    std::cout << response;
                }
            }
            
            return 0;
            
        } catch (const std::exception& e) {
            if (!quiet) {
                std::cerr << "whois: error: " << e.what() << std::endl;
            }
            return 1;
        }
    }
};

void printUsage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [OPTIONS] QUERY\n\n"
              << "Domain and IP address WHOIS lookup utility.\n\n"
              << "Options:\n"
              << "  -h, --host=SERVER        WHOIS server to query\n"
              << "  -p, --port=PORT          Port number (default: 43)\n"
              << "  -r, --raw                Raw output (don't parse)\n"
              << "  -j, --json               JSON output format\n"
              << "  -f, --no-follow          Don't follow referrals\n"
              << "  -v, --verbose            Verbose output\n"
              << "  -q, --quiet              Quiet mode (errors only)\n"
              << "  -t, --timeout=SECONDS    Query timeout (default: 30)\n"
              << "  -o, --output=FILE        Save output to file\n"
              << "  --help                   Show this help message\n"
              << "  --version                Show version information\n\n"
              << "Query Types:\n"
              << "  Domain names             example.com, google.org\n"
              << "  IP addresses             192.168.1.1, 8.8.8.8\n"
              << "  IPv6 addresses           2001:4860:4860::8888\n\n"
              << "Examples:\n"
              << "  " << program_name << " example.com\n"
              << "  " << program_name << " 8.8.8.8\n"
              << "  " << program_name << " -h whois.nic.uk example.co.uk\n"
              << "  " << program_name << " --json --verbose google.com\n"
              << "  " << program_name << " --raw --no-follow domain.org\n"
              << "  " << program_name << " -t 60 slow-server.example\n\n"
              << "Supported TLDs:\n"
              << "  .com, .net, .org, .info, .biz, .us, .uk, .de, .fr, .jp,\n"
              << "  .cn, .ru, .br, .au, .ca, .edu, .gov, .mil, .int\n\n"
              << "Part of QCO MoreUtils by AnmiTaliDev\n"
              << "Repository: https://github.com/Qainar-Projects/MoreUtils\n";
}

void printVersion() {
    std::cout << "whois 1.0.0\n"
              << "Part of QCO MoreUtils - Quality Control Operations More Utilities\n"
              << "Copyright 2025 AnmiTaliDev\n"
              << "Licensed under the Apache License, Version 2.0\n"
              << "Repository: https://github.com/Qainar-Projects/MoreUtils\n";
}

int main(int argc, char* argv[]) {
    WhoisClient whois;
    
    static struct option long_options[] = {
        {"host",       required_argument, 0, 'h'},
        {"port",       required_argument, 0, 'p'},
        {"raw",        no_argument,       0, 'r'},
        {"json",       no_argument,       0, 'j'},
        {"no-follow",  no_argument,       0, 'f'},
        {"verbose",    no_argument,       0, 'v'},
        {"quiet",      no_argument,       0, 'q'},
        {"timeout",    required_argument, 0, 't'},
        {"output",     required_argument, 0, 'o'},
        {"help",       no_argument,       0, 7001},
        {"version",    no_argument,       0, 7002},
        {0, 0, 0, 0}
    };
    
    int opt;
    int option_index = 0;
    
    while ((opt = getopt_long(argc, argv, "h:p:rjfvqt:o:", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'h':
                whois.setServer(optarg);
                break;
            case 'p':
                try {
                    whois.setPort(std::stoi(optarg));
                } catch (...) {
                    std::cerr << "whois: invalid port number: " << optarg << std::endl;
                    return 1;
                }
                break;
            case 'r':
                whois.setRawOutput(true);
                break;
            case 'j':
                whois.setJsonOutput(true);
                break;
            case 'f':
                whois.setFollowReferrals(false);
                break;
            case 'v':
                whois.setVerbose(true);
                break;
            case 'q':
                whois.setQuiet(true);
                break;
            case 't':
                try {
                    whois.setTimeout(std::stoi(optarg));
                } catch (...) {
                    std::cerr << "whois: invalid timeout value: " << optarg << std::endl;
                    return 1;
                }
                break;
            case 'o':
                whois.setOutputFile(optarg);
                break;
            case 7001: // help
                printUsage(argv[0]);
                return 0;
            case 7002: // version
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
        std::cerr << "whois: missing query target\n";
        std::cerr << "Try '" << argv[0] << " --help' for more information.\n";
        return 1;
    }
    
    std::string query = argv[optind];
    return whois.query(query);
}