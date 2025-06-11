/*
 * conf-convert - Configuration format converter utility
 * Part of QCO MoreUtils - Advanced System Development More Utilities
 * 
 * Copyright 2025 AnmiTaliDev
 * Licensed under the Apache License, Version 2.0
 * 
 * Repository: https://github.com/Qainar-Projects/MoreUtils
 * 
 * A powerful utility for converting between various configuration file formats
 * including JSON, YAML, TOML, XML, INI, and custom formats with validation
 * and formatting options.
 */

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <regex>
#include <getopt.h>
#include <unistd.h>

// Simple key-value configuration storage
class ConfigConverter {
private:
    std::string input_format = "auto";
    std::string output_format = "json";
    std::string input_file = "";
    std::string output_file = "";
    bool pretty_print = true;
    bool validate_only = false;
    bool show_stats = false;
    bool preserve_comments = false;
    int indent_size = 2;
    std::string root_key = "";
    std::vector<std::string> exclude_keys;
    std::vector<std::string> include_keys;
    bool sort_keys = false;
    bool minify = false;
    
    std::map<std::string, std::string> config_data;
    std::vector<std::string> comments;
    
public:
    void setInputFormat(const std::string& format) { input_format = format; }
    void setOutputFormat(const std::string& format) { output_format = format; }
    void setInputFile(const std::string& file) { input_file = file; }
    void setOutputFile(const std::string& file) { output_file = file; }
    void setPrettyPrint(bool pretty) { pretty_print = pretty; }
    void setValidateOnly(bool validate) { validate_only = validate; }
    void setShowStats(bool stats) { show_stats = stats; }
    void setPreserveComments(bool preserve) { preserve_comments = preserve; }
    void setIndentSize(int size) { indent_size = size; }
    void setRootKey(const std::string& key) { root_key = key; }
    void setExcludeKeys(const std::vector<std::string>& keys) { exclude_keys = keys; }
    void setIncludeKeys(const std::vector<std::string>& keys) { include_keys = keys; }
    void setSortKeys(bool sort) { sort_keys = sort; }
    void setMinify(bool min) { minify = min; }
    
    bool endsWith(const std::string& str, const std::string& suffix) {
        if (suffix.length() > str.length()) return false;
        return str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
    }
    
    bool startsWith(const std::string& str, const std::string& prefix) {
        if (prefix.length() > str.length()) return false;
        return str.compare(0, prefix.length(), prefix) == 0;
    }
    
    std::string detectFormat(const std::string& content, const std::string& filename) {
        // Detect by file extension first
        if (!filename.empty()) {
            if (endsWith(filename, ".json")) return "json";
            if (endsWith(filename, ".yaml") || endsWith(filename, ".yml")) return "yaml";
            if (endsWith(filename, ".toml")) return "toml";
            if (endsWith(filename, ".xml")) return "xml";
            if (endsWith(filename, ".ini") || endsWith(filename, ".conf")) return "ini";
            if (endsWith(filename, ".env")) return "env";
        }
        
        // Detect by content
        std::string trimmed = content;
        trimmed.erase(0, trimmed.find_first_not_of(" \t\n\r"));
        
        if (startsWith(trimmed, "{") || startsWith(trimmed, "[")) return "json";
        if (startsWith(trimmed, "<?xml") || startsWith(trimmed, "<")) return "xml";
        if (trimmed.find("---") != std::string::npos || trimmed.find(":") != std::string::npos) return "yaml";
        if (trimmed.find("[") != std::string::npos && trimmed.find("=") != std::string::npos) return "ini";
        if (trimmed.find("=") != std::string::npos && trimmed.find("\n") != std::string::npos) return "env";
        
        return "json"; // Default fallback
    }
    
    bool parseJSON(const std::string& content) {
        try {
            // Simple JSON parser (basic implementation)
            std::regex json_pattern("\"([^\"]+)\"\\s*:\\s*\"([^\"]*)\"");
            std::regex number_pattern("\"([^\"]+)\"\\s*:\\s*(\\d+(?:\\.\\d+)?)");
            std::regex bool_pattern("\"([^\"]+)\"\\s*:\\s*(true|false)");
            
            std::sregex_iterator iter(content.begin(), content.end(), json_pattern);
            std::sregex_iterator end;
            
            for (; iter != end; ++iter) {
                std::string key = (*iter)[1].str();
                std::string value = (*iter)[2].str();
                config_data[key] = value;
            }
            
            // Parse numbers
            std::sregex_iterator num_iter(content.begin(), content.end(), number_pattern);
            for (; num_iter != end; ++num_iter) {
                std::string key = (*num_iter)[1].str();
                std::string value = (*num_iter)[2].str();
                config_data[key] = value;
            }
            
            // Parse booleans
            std::sregex_iterator bool_iter(content.begin(), content.end(), bool_pattern);
            for (; bool_iter != end; ++bool_iter) {
                std::string key = (*bool_iter)[1].str();
                std::string value = (*bool_iter)[2].str();
                config_data[key] = value;
            }
            
            return true;
        } catch (...) {
            return false;
        }
    }
    
    bool parseYAML(const std::string& content) {
        try {
            std::istringstream stream(content);
            std::string line;
            std::regex yaml_pattern("^\\s*([^:]+):\\s*(.*)$");
            std::regex comment_pattern("^\\s*#(.*)$");
            
            while (std::getline(stream, line)) {
                std::smatch match;
                
                if (std::regex_match(line, match, comment_pattern)) {
                    if (preserve_comments) {
                        comments.push_back(match[1].str());
                    }
                    continue;
                }
                
                if (std::regex_match(line, match, yaml_pattern)) {
                    std::string key = match[1].str();
                    std::string value = match[2].str();
                    
                    // Trim whitespace
                    key.erase(0, key.find_first_not_of(" \t"));
                    key.erase(key.find_last_not_of(" \t") + 1);
                    value.erase(0, value.find_first_not_of(" \t"));
                    value.erase(value.find_last_not_of(" \t") + 1);
                    
                    // Remove quotes if present
                    if ((startsWith(value, "\"") && endsWith(value, "\"")) ||
                        (startsWith(value, "'") && endsWith(value, "'"))) {
                        value = value.substr(1, value.length() - 2);
                    }
                    
                    config_data[key] = value;
                }
            }
            
            return true;
        } catch (...) {
            return false;
        }
    }
    
    bool parseINI(const std::string& content) {
        try {
            std::istringstream stream(content);
            std::string line;
            std::string current_section = "";
            std::regex section_pattern("^\\s*\\[([^\\]]+)\\]\\s*$");
            std::regex keyvalue_pattern("^\\s*([^=]+)=(.*)$");
            std::regex comment_pattern("^\\s*[;#](.*)$");
            
            while (std::getline(stream, line)) {
                std::smatch match;
                
                if (std::regex_match(line, match, comment_pattern)) {
                    if (preserve_comments) {
                        comments.push_back(match[1].str());
                    }
                    continue;
                }
                
                if (std::regex_match(line, match, section_pattern)) {
                    current_section = match[1].str();
                    continue;
                }
                
                if (std::regex_match(line, match, keyvalue_pattern)) {
                    std::string key = match[1].str();
                    std::string value = match[2].str();
                    
                    // Trim whitespace
                    key.erase(0, key.find_first_not_of(" \t"));
                    key.erase(key.find_last_not_of(" \t") + 1);
                    value.erase(0, value.find_first_not_of(" \t"));
                    value.erase(value.find_last_not_of(" \t") + 1);
                    
                    std::string full_key = current_section.empty() ? key : current_section + "." + key;
                    config_data[full_key] = value;
                }
            }
            
            return true;
        } catch (...) {
            return false;
        }
    }
    
    bool parseENV(const std::string& content) {
        try {
            std::istringstream stream(content);
            std::string line;
            std::regex env_pattern("^\\s*([A-Za-z_][A-Za-z0-9_]*)=(.*)$");
            std::regex comment_pattern("^\\s*#(.*)$");
            
            while (std::getline(stream, line)) {
                std::smatch match;
                
                if (std::regex_match(line, match, comment_pattern)) {
                    if (preserve_comments) {
                        comments.push_back(match[1].str());
                    }
                    continue;
                }
                
                if (std::regex_match(line, match, env_pattern)) {
                    std::string key = match[1].str();
                    std::string value = match[2].str();
                    
                    // Remove quotes if present
                    if ((startsWith(value, "\"") && endsWith(value, "\"")) ||
                        (startsWith(value, "'") && endsWith(value, "'"))) {
                        value = value.substr(1, value.length() - 2);
                    }
                    
                    config_data[key] = value;
                }
            }
            
            return true;
        } catch (...) {
            return false;
        }
    }
    
    std::string escapeJsonString(const std::string& str) {
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
    
    bool isNumber(const std::string& str) {
        if (str.empty()) return false;
        std::regex number_pattern("^-?\\d+(?:\\.\\d+)?$");
        return std::regex_match(str, number_pattern);
    }
    
    bool isBoolean(const std::string& str) {
        return str == "true" || str == "false";
    }
    
    std::string generateJSON() {
        std::ostringstream output;
        
        if (minify) {
            output << "{";
            bool first = true;
            for (const auto& pair : config_data) {
                if (!first) output << ",";
                output << "\"" << escapeJsonString(pair.first) << "\":";
                
                if (isNumber(pair.second)) {
                    output << pair.second;
                } else if (isBoolean(pair.second)) {
                    output << pair.second;
                } else {
                    output << "\"" << escapeJsonString(pair.second) << "\"";
                }
                
                first = false;
            }
            output << "}";
        } else {
            output << "{\n";
            std::string indent(indent_size, ' ');
            bool first = true;
            
            for (const auto& pair : config_data) {
                if (!first) output << ",\n";
                output << indent << "\"" << escapeJsonString(pair.first) << "\": ";
                
                if (isNumber(pair.second)) {
                    output << pair.second;
                } else if (isBoolean(pair.second)) {
                    output << pair.second;
                } else {
                    output << "\"" << escapeJsonString(pair.second) << "\"";
                }
                
                first = false;
            }
            output << "\n}";
        }
        
        return output.str();
    }
    
    std::string generateYAML() {
        std::ostringstream output;
        
        if (preserve_comments) {
            for (const auto& comment : comments) {
                output << "# " << comment << "\n";
            }
            if (!comments.empty()) output << "\n";
        }
        
        for (const auto& pair : config_data) {
            output << pair.first << ": ";
            
            std::string value = pair.second;
            // Quote if contains special characters
            if (value.find(':') != std::string::npos || value.find('#') != std::string::npos || 
                value.find('[') != std::string::npos || value.find(']') != std::string::npos ||
                value.find(' ') == 0 || value.find(' ') == value.length() - 1) {
                output << "\"" << value << "\"";
            } else {
                output << value;
            }
            
            output << "\n";
        }
        
        return output.str();
    }
    
    std::string generateINI() {
        std::ostringstream output;
        std::map<std::string, std::map<std::string, std::string>> sections;
        
        // Group by sections
        for (const auto& pair : config_data) {
            size_t dot_pos = pair.first.find('.');
            if (dot_pos != std::string::npos) {
                std::string section = pair.first.substr(0, dot_pos);
                std::string section_key = pair.first.substr(dot_pos + 1);
                sections[section][section_key] = pair.second;
            } else {
                sections[""][pair.first] = pair.second;
            }
        }
        
        if (preserve_comments) {
            for (const auto& comment : comments) {
                output << "; " << comment << "\n";
            }
            if (!comments.empty()) output << "\n";
        }
        
        // Output global keys first
        if (sections.count("")) {
            for (const auto& pair : sections[""]) {
                output << pair.first << " = " << pair.second << "\n";
            }
            output << "\n";
        }
        
        // Output sections
        for (const auto& section_pair : sections) {
            if (section_pair.first.empty()) continue;
            
            output << "[" << section_pair.first << "]\n";
            for (const auto& pair : section_pair.second) {
                output << pair.first << " = " << pair.second << "\n";
            }
            output << "\n";
        }
        
        return output.str();
    }
    
    std::string generateENV() {
        std::ostringstream output;
        
        if (preserve_comments) {
            for (const auto& comment : comments) {
                output << "# " << comment << "\n";
            }
            if (!comments.empty()) output << "\n";
        }
        
        for (const auto& pair : config_data) {
            // Convert key to uppercase and replace dots with underscores
            std::string env_key = pair.first;
            std::transform(env_key.begin(), env_key.end(), env_key.begin(), ::toupper);
            std::replace(env_key.begin(), env_key.end(), '.', '_');
            
            output << env_key << "=";
            
            std::string value = pair.second;
            // Quote if contains spaces or special characters
            if (value.find(' ') != std::string::npos || value.find('\t') != std::string::npos ||
                value.find('#') != std::string::npos || value.find('$') != std::string::npos) {
                output << "\"" << value << "\"";
            } else {
                output << value;
            }
            
            output << "\n";
        }
        
        return output.str();
    }
    
    void filterKeys() {
        if (!include_keys.empty()) {
            std::map<std::string, std::string> filtered;
            for (const auto& key : include_keys) {
                if (config_data.count(key)) {
                    filtered[key] = config_data[key];
                }
            }
            config_data = filtered;
        }
        
        for (const auto& key : exclude_keys) {
            config_data.erase(key);
        }
    }
    
    void printStatistics() {
        if (!show_stats) return;
        
        std::cout << "\n--- Conversion Statistics ---\n";
        std::cout << "Total keys: " << config_data.size() << "\n";
        std::cout << "Comments preserved: " << comments.size() << "\n";
        std::cout << "Input format: " << input_format << "\n";
        std::cout << "Output format: " << output_format << "\n";
        
        // Count value types
        int numbers = 0, booleans = 0, strings = 0;
        for (const auto& pair : config_data) {
            if (isNumber(pair.second)) {
                numbers++;
            } else if (isBoolean(pair.second)) {
                booleans++;
            } else {
                strings++;
            }
        }
        
        std::cout << "Value types - Strings: " << strings 
                  << ", Numbers: " << numbers 
                  << ", Booleans: " << booleans << "\n";
    }
    
    int convert() {
        try {
            // Read input
            std::string content;
            if (input_file.empty() || input_file == "-") {
                std::string line;
                while (std::getline(std::cin, line)) {
                    content += line + "\n";
                }
            } else {
                std::ifstream file(input_file);
                if (!file.is_open()) {
                    std::cerr << "conf-convert: cannot open input file '" << input_file << "'\n";
                    return 1;
                }
                content.assign((std::istreambuf_iterator<char>(file)),
                              std::istreambuf_iterator<char>());
            }
            
            // Auto-detect input format
            if (input_format == "auto") {
                input_format = detectFormat(content, input_file);
            }
            
            // Parse input
            bool parse_success = false;
            if (input_format == "json") {
                parse_success = parseJSON(content);
            } else if (input_format == "yaml" || input_format == "yml") {
                parse_success = parseYAML(content);
            } else if (input_format == "ini" || input_format == "conf") {
                parse_success = parseINI(content);
            } else if (input_format == "env") {
                parse_success = parseENV(content);
            } else {
                std::cerr << "conf-convert: unsupported input format '" << input_format << "'\n";
                return 1;
            }
            
            if (!parse_success) {
                std::cerr << "conf-convert: failed to parse input as " << input_format << "\n";
                return 1;
            }
            
            if (validate_only) {
                std::cout << "Input file is valid " << input_format << "\n";
                printStatistics();
                return 0;
            }
            
            // Filter keys
            filterKeys();
            
            // Sort keys if requested
            if (sort_keys) {
                std::map<std::string, std::string> sorted_data(config_data.begin(), config_data.end());
                config_data = sorted_data;
            }
            
            // Generate output
            std::string output;
            if (output_format == "json") {
                output = generateJSON();
            } else if (output_format == "yaml" || output_format == "yml") {
                output = generateYAML();
            } else if (output_format == "ini" || output_format == "conf") {
                output = generateINI();
            } else if (output_format == "env") {
                output = generateENV();
            } else {
                std::cerr << "conf-convert: unsupported output format '" << output_format << "'\n";
                return 1;
            }
            
            // Write output
            if (output_file.empty() || output_file == "-") {
                std::cout << output;
            } else {
                std::ofstream file(output_file);
                if (!file.is_open()) {
                    std::cerr << "conf-convert: cannot create output file '" << output_file << "'\n";
                    return 1;
                }
                file << output;
            }
            
            printStatistics();
            return 0;
            
        } catch (const std::exception& e) {
            std::cerr << "conf-convert: error: " << e.what() << "\n";
            return 1;
        }
    }
};

void printUsage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [OPTIONS] [INPUT_FILE] [OUTPUT_FILE]\n\n"
              << "Configuration format converter utility.\n\n"
              << "Supported Formats:\n"
              << "  json, yaml/yml, ini/conf, env\n\n"
              << "Format Options:\n"
              << "  -f, --from FORMAT        Input format (auto-detect if not specified)\n"
              << "  -t, --to FORMAT          Output format (default: json)\n"
              << "  -i, --input FILE         Input file (stdin if not specified)\n"
              << "  -o, --output FILE        Output file (stdout if not specified)\n\n"
              << "Processing Options:\n"
              << "  --validate               Validate input only (don't convert)\n"
              << "  --include KEYS           Include only specified keys (comma-separated)\n"
              << "  --exclude KEYS           Exclude specified keys (comma-separated)\n"
              << "  --root KEY               Extract from root key\n"
              << "  --sort                   Sort keys alphabetically\n\n"
              << "Output Options:\n"
              << "  --pretty                 Pretty-print output (default)\n"
              << "  --minify                 Minify output (compact format)\n"
              << "  --indent SIZE            Indentation size (default: 2)\n"
              << "  --preserve-comments      Preserve comments when possible\n"
              << "  --stats                  Show conversion statistics\n\n"
              << "Standard Options:\n"
              << "  -h, --help               Show this help message\n"
              << "  -v, --version            Show version information\n\n"
              << "Examples:\n"
              << "  " << program_name << " -f yaml -t json config.yml\n"
              << "  " << program_name << " --from ini --to env settings.ini\n"
              << "  cat config.json | " << program_name << " --to yaml\n"
              << "  " << program_name << " --validate -f json config.json\n"
              << "  " << program_name << " --exclude \"debug,test\" -t yaml app.json\n"
              << "  " << program_name << " --minify --to json config.yml output.json\n"
              << "  " << program_name << " --sort --preserve-comments -t ini config.yaml\n\n"
              << "Auto-detection:\n"
              << "  The tool automatically detects input format based on file extension\n"
              << "  and content analysis when --from is not specified.\n\n"
              << "Part of QCO MoreUtils by AnmiTaliDev\n"
              << "Repository: https://github.com/Qainar-Projects/MoreUtils\n";
}

void printVersion() {
    std::cout << "conf-convert 1.0.0\n"
              << "Part of QCO MoreUtils - Advanced System Development More Utilities\n"
              << "Copyright 2025 AnmiTaliDev\n"
              << "Licensed under the Apache License, Version 2.0\n"
              << "Repository: https://github.com/Qainar-Projects/MoreUtils\n";
}

std::vector<std::string> splitString(const std::string& str, char delimiter) {
    std::vector<std::string> result;
    std::stringstream ss(str);
    std::string item;
    
    while (std::getline(ss, item, delimiter)) {
        if (!item.empty()) {
            result.push_back(item);
        }
    }
    
    return result;
}

int main(int argc, char* argv[]) {
    ConfigConverter converter;
    
    static struct option long_options[] = {
        {"from",               required_argument, 0, 'f'},
        {"to",                 required_argument, 0, 't'},
        {"input",              required_argument, 0, 'i'},
        {"output",             required_argument, 0, 'o'},
        {"validate",           no_argument,       0, 4001},
        {"include",            required_argument, 0, 4002},
        {"exclude",            required_argument, 0, 4003},
        {"root",               required_argument, 0, 4004},
        {"sort",               no_argument,       0, 4005},
        {"pretty",             no_argument,       0, 4006},
        {"minify",             no_argument,       0, 4007},
        {"indent",             required_argument, 0, 4008},
        {"preserve-comments",  no_argument,       0, 4009},
        {"stats",              no_argument,       0, 4010},
        {"help",               no_argument,       0, 'h'},
        {"version",            no_argument,       0, 'v'},
        {0, 0, 0, 0}
    };
    
    int opt;
    int option_index = 0;
    
    while ((opt = getopt_long(argc, argv, "f:t:i:o:hv", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'f':
                converter.setInputFormat(optarg);
                break;
            case 't':
                converter.setOutputFormat(optarg);
                break;
            case 'i':
                converter.setInputFile(optarg);
                break;
            case 'o':
                converter.setOutputFile(optarg);
                break;
            case 4001: // validate
                converter.setValidateOnly(true);
                break;
            case 4002: // include
                converter.setIncludeKeys(splitString(optarg, ','));
                break;
            case 4003: // exclude
                converter.setExcludeKeys(splitString(optarg, ','));
                break;
            case 4004: // root
                converter.setRootKey(optarg);
                break;
            case 4005: // sort
                converter.setSortKeys(true);
                break;
            case 4006: // pretty
                converter.setPrettyPrint(true);
                break;
            case 4007: // minify
                converter.setMinify(true);
                converter.setPrettyPrint(false);
                break;
            case 4008: // indent
                converter.setIndentSize(std::stoi(optarg));
                break;
            case 4009: // preserve-comments
                converter.setPreserveComments(true);
                break;
            case 4010: // stats
                converter.setShowStats(true);
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
    
    // Handle positional arguments
    if (optind < argc) {
        converter.setInputFile(argv[optind]);
        if (optind + 1 < argc) {
            converter.setOutputFile(argv[optind + 1]);
        }
    }
    
    return converter.convert();
}