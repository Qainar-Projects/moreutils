/*
 * lower - Convert text to lowercase
 * Part of QCO MoreUtils - Advanced System Development More Utilities
 * 
 * Copyright 2025 AnmiTaliDev
 * Licensed under the Apache License, Version 2.0
 * 
 * Repository: https://github.com/Qainar-Projects/MoreUtils
 * 
 * A flexible and functional utility for converting text to lowercase,
 * supporting various input sources and output formats with proper
 * Unicode handling and performance optimization.
 */

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <locale>
#include <codecvt>
#include <cstring>
#include <cerrno>
#include <getopt.h>
#include <unistd.h>

class LowerConverter {
private:
    bool preserve_whitespace = true;
    bool line_numbers = false;
    bool only_first_char = false;
    bool only_first_word = false;
    std::string delimiter = "";
    std::locale loc;
    
public:
    LowerConverter() {
        try {
            loc = std::locale("en_US.UTF-8");
        } catch (...) {
            loc = std::locale::classic();
        }
    }
    
    void setPreserveWhitespace(bool preserve) { preserve_whitespace = preserve; }
    void setLineNumbers(bool numbers) { line_numbers = numbers; }
    void setOnlyFirstChar(bool first) { only_first_char = first; }
    void setOnlyFirstWord(bool first_word) { only_first_word = first_word; }
    void setDelimiter(const std::string& delim) { delimiter = delim; }
    
    std::string convertLine(const std::string& line, size_t line_num = 0) {
        std::string result = line;
        
        if (only_first_char) {
            if (!result.empty()) {
                result[0] = std::tolower(result[0], loc);
            }
        } else if (only_first_word) {
            bool found_word = false;
            for (char& c : result) {
                if (std::isalpha(c, loc) && !found_word) {
                    c = std::tolower(c, loc);
                    found_word = true;
                } else if (found_word && std::isspace(c, loc)) {
                    break;
                }
            }
        } else {
            std::transform(result.begin(), result.end(), result.begin(),
                          [this](char c) { return std::tolower(c, loc); });
        }
        
        if (!preserve_whitespace) {
            // Remove leading/trailing whitespace
            result.erase(0, result.find_first_not_of(" \t\n\r\f\v"));
            result.erase(result.find_last_not_of(" \t\n\r\f\v") + 1);
        }
        
        if (line_numbers) {
            return std::to_string(line_num) + ": " + result;
        }
        
        return result;
    }
    
    void processStream(std::istream& input, std::ostream& output) {
        std::string line;
        size_t line_num = 1;
        
        while (std::getline(input, line)) {
            output << convertLine(line, line_num);
            if (!delimiter.empty()) {
                output << delimiter;
            } else {
                output << "\n";
            }
            line_num++;
        }
    }
};

void printUsage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [OPTIONS] [FILE...]\n\n"
              << "Convert text to lowercase.\n\n"
              << "Options:\n"
              << "  -c, --first-char     Convert only first character\n"
              << "  -w, --first-word     Convert only first word\n"
              << "  -n, --line-numbers   Show line numbers\n"
              << "  -s, --strip          Strip leading/trailing whitespace\n"
              << "  -d, --delimiter=STR  Use custom line delimiter\n"
              << "  -h, --help           Show this help message\n"
              << "  -v, --version        Show version information\n\n"
              << "Examples:\n"
              << "  echo 'HELLO WORLD' | " << program_name << "\n"
              << "  " << program_name << " -c FILE.TXT\n"
              << "  " << program_name << " -w -n DOCUMENT.TXT\n"
              << "  cat DATA.TXT | " << program_name << " -s\n\n"
              << "Part of QCO MoreUtils by AnmiTaliDev\n"
              << "Repository: https://github.com/Qainar-Projects/MoreUtils\n";
}

void printVersion() {
    std::cout << "lower 1.0.0\n"
              << "Part of QCO MoreUtils - Advanced System Development More Utilities\n"
              << "Copyright 2025 AnmiTaliDev\n"
              << "Licensed under the Apache License, Version 2.0\n"
              << "Repository: https://github.com/Qainar-Projects/MoreUtils\n";
}

int main(int argc, char* argv[]) {
    LowerConverter converter;
    std::vector<std::string> input_files;
    
    static struct option long_options[] = {
        {"first-char",   no_argument,       0, 'c'},
        {"first-word",   no_argument,       0, 'w'},
        {"line-numbers", no_argument,       0, 'n'},
        {"strip",        no_argument,       0, 's'},
        {"delimiter",    required_argument, 0, 'd'},
        {"help",         no_argument,       0, 'h'},
        {"version",      no_argument,       0, 'v'},
        {0, 0, 0, 0}
    };
    
    int opt;
    int option_index = 0;
    
    while ((opt = getopt_long(argc, argv, "cwnsd:hv", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'c':
                converter.setOnlyFirstChar(true);
                break;
            case 'w':
                converter.setOnlyFirstWord(true);
                break;
            case 'n':
                converter.setLineNumbers(true);
                break;
            case 's':
                converter.setPreserveWhitespace(false);
                break;
            case 'd':
                converter.setDelimiter(optarg);
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
    
    // Collect input files
    for (int i = optind; i < argc; i++) {
        input_files.push_back(argv[i]);
    }
    
    try {
        if (input_files.empty()) {
            // Read from stdin
            if (isatty(STDIN_FILENO)) {
                std::cerr << "lower: reading from stdin (use Ctrl+D to end input)\n";
            }
            converter.processStream(std::cin, std::cout);
        } else {
            // Process files
            for (const auto& filename : input_files) {
                if (filename == "-") {
                    converter.processStream(std::cin, std::cout);
                } else {
                    std::ifstream file(filename);
                    if (!file.is_open()) {
                        std::cerr << "lower: cannot open '" << filename 
                                  << "': " << strerror(errno) << "\n";
                        return 1;
                    }
                    converter.processStream(file, std::cout);
                    file.close();
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "lower: error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}