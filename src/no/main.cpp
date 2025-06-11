/*
 * no - The opposite of yes (Easter egg utility)
 * Part of QCO MoreUtils - Quality Control Operations More Utilities
 * 
 * Copyright 2025 AnmiTaliDev
 * Licensed under the Apache License, Version 2.0
 * 
 * Repository: https://github.com/Qainar-Projects/MoreUtils
 * 
 * A humorous counterpart to the 'yes' utility that outputs 'no' repeatedly.
 * Sometimes you just need to be negative about everything.
 */

#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <algorithm>
#include <signal.h>
#include <getopt.h>
#include <unistd.h>

class NoUtility {
private:
    std::string output_text = "no";
    int delay_ms = 0;
    int count = -1;  // -1 means infinite
    bool uppercase = false;
    bool enthusiastic = false;
    bool polite = false;
    bool sarcastic = false;
    bool quiet = false;
    
    volatile bool running = true;
    
public:
    void setOutputText(const std::string& text) { output_text = text; }
    void setDelay(int delay) { delay_ms = delay; }
    void setCount(int c) { count = c; }
    void setUppercase(bool upper) { uppercase = upper; }
    void setEnthusiastic(bool enthusiastic_flag) { enthusiastic = enthusiastic_flag; }
    void setPolite(bool polite_flag) { polite = polite_flag; }
    void setSarcastic(bool sarcastic_flag) { sarcastic = sarcastic_flag; }
    void setQuiet(bool q) { quiet = q; }
    void setRunning(bool r) { running = r; }
    
    std::string formatOutput() {
        std::string result = output_text;
        
        if (polite) {
            result = "No, thank you";
        } else if (enthusiastic) {
            result = "NO!";
        } else if (sarcastic) {
            result = "no... obviously";
        }
        
        if (uppercase && !enthusiastic && !polite && !sarcastic) {
            std::transform(result.begin(), result.end(), result.begin(), ::toupper);
        }
        
        return result;
    }
    
    int run() {
        if (!quiet) {
            std::string formatted = formatOutput();
            
            if (count == -1) {
                // Infinite loop
                while (running) {
                    std::cout << formatted << std::endl;
                    
                    if (delay_ms > 0) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
                    }
                }
            } else {
                // Limited count
                for (int i = 0; i < count && running; ++i) {
                    std::cout << formatted << std::endl;
                    
                    if (delay_ms > 0 && i < count - 1) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
                    }
                }
            }
        }
        
        return 0;
    }
};

// Global instance for signal handling
NoUtility* global_no = nullptr;

void signalHandler(int signal) {
    if (global_no) {
        global_no->setRunning(false);
    }
}

void printUsage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [OPTIONS] [STRING]\n\n"
              << "The opposite of yes - outputs 'no' repeatedly until terminated.\n\n"
              << "Options:\n"
              << "  -d, --delay=MS           Delay between outputs in milliseconds\n"
              << "  -c, --count=N            Output N times instead of infinitely\n"
              << "  -u, --uppercase          Output in UPPERCASE\n"
              << "  -e, --enthusiastic       Be enthusiastic about saying no (NO!)\n"
              << "  -p, --polite             Be polite about it (No, thank you)\n"
              << "  -s, --sarcastic          Be sarcastic (no... obviously)\n"
              << "  -q, --quiet              Don't output anything (like /dev/null)\n"
              << "  -h, --help               Show this help message\n"
              << "  -v, --version            Show version information\n\n"
              << "Arguments:\n"
              << "  STRING                   Custom string to output instead of 'no'\n\n"
              << "Examples:\n"
              << "  " << program_name << "                    # Infinite 'no'\n"
              << "  " << program_name << " -c 5              # Say 'no' 5 times\n"
              << "  " << program_name << " -e                # Enthusiastic NO!\n"
              << "  " << program_name << " -p                # Polite refusal\n"
              << "  " << program_name << " -s                # Sarcastic response\n"
              << "  " << program_name << " -d 1000 -c 3      # 'no' 3 times with 1s delay\n"
              << "  " << program_name << " \"nope\"            # Custom negative response\n"
              << "  " << program_name << " -q                # Silent treatment\n\n"
              << "Note: This is a humorous counterpart to the 'yes' utility.\n"
              << "Sometimes you just need to be negative about everything.\n\n"
              << "Part of QCO MoreUtils by AnmiTaliDev\n"
              << "Repository: https://github.com/Qainar-Projects/MoreUtils\n";
}

void printVersion() {
    std::cout << "no 1.0.0 (Easter Egg Edition)\n"
              << "Part of QCO MoreUtils - Quality Control Operations More Utilities\n"
              << "Copyright 2025 AnmiTaliDev\n"
              << "Licensed under the Apache License, Version 2.0\n"
              << "Repository: https://github.com/Qainar-Projects/MoreUtils\n\n"
              << "\"Sometimes the most powerful word is 'no'\" - AnmiTaliDev\n";
}

void printEasterEgg() {
    std::cout << "\n"
              << "    ███╗   ██╗ ██████╗ \n"
              << "    ████╗  ██║██╔═══██╗\n"
              << "    ██╔██╗ ██║██║   ██║\n"
              << "    ██║╚██╗██║██║   ██║\n"
              << "    ██║ ╚████║╚██████╔╝\n"
              << "    ╚═╝  ╚═══╝ ╚═════╝ \n\n"
              << "The art of saying no, perfected in CLI form.\n"
              << "Resistance is not futile - it's a feature!\n\n";
}

int main(int argc, char* argv[]) {
    NoUtility no_util;
    global_no = &no_util;
    
    // Set up signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    static struct option long_options[] = {
        {"delay",        required_argument, 0, 'd'},
        {"count",        required_argument, 0, 'c'},
        {"uppercase",    no_argument,       0, 'u'},
        {"enthusiastic", no_argument,       0, 'e'},
        {"polite",       no_argument,       0, 'p'},
        {"sarcastic",    no_argument,       0, 's'},
        {"quiet",        no_argument,       0, 'q'},
        {"help",         no_argument,       0, 'h'},
        {"version",      no_argument,       0, 'v'},
        {"easter-egg",   no_argument,       0, 6001}, // Hidden option
        {0, 0, 0, 0}
    };
    
    int opt;
    int option_index = 0;
    
    while ((opt = getopt_long(argc, argv, "d:c:uepsqhv", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'd':
                try {
                    no_util.setDelay(std::stoi(optarg));
                } catch (...) {
                    std::cerr << "no: invalid delay value: " << optarg << std::endl;
                    return 1;
                }
                break;
            case 'c':
                try {
                    int count = std::stoi(optarg);
                    if (count < 0) {
                        std::cerr << "no: count cannot be negative" << std::endl;
                        return 1;
                    }
                    no_util.setCount(count);
                } catch (...) {
                    std::cerr << "no: invalid count value: " << optarg << std::endl;
                    return 1;
                }
                break;
            case 'u':
                no_util.setUppercase(true);
                break;
            case 'e':
                no_util.setEnthusiastic(true);
                break;
            case 'p':
                no_util.setPolite(true);
                break;
            case 's':
                no_util.setSarcastic(true);
                break;
            case 'q':
                no_util.setQuiet(true);
                break;
            case 'h':
                printUsage(argv[0]);
                return 0;
            case 'v':
                printVersion();
                return 0;
            case 6001: // easter-egg
                printEasterEgg();
                return 0;
            case '?':
                std::cerr << "Try '" << argv[0] << " --help' for more information.\n";
                return 1;
            default:
                return 1;
        }
    }
    
    // Handle custom string argument
    if (optind < argc) {
        no_util.setOutputText(argv[optind]);
    }
    
    try {
        return no_util.run();
    } catch (const std::exception& e) {
        std::cerr << "no: error: " << e.what() << std::endl;
        return 1;
    }
}