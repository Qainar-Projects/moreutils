/*
 * QCO MoreUtils - Tree Utility
 * 
 * Author: AnmiTaliDev
 * License: Apache License 2.0
 * 
 * Tree utility displays directory structure in a tree-like format
 * Supports custom indentation, coloring, and filtering options
 */

 #include <iostream>
 #include <filesystem>
 #include <vector>
 #include <string>
 #include <algorithm>
 #include <cstring>
 #include <iomanip>
 
 namespace fs = std::filesystem;
 
 namespace QCO {
 namespace MoreUtils {
 
 class TreeUtil {
 private:
     struct Options {
         bool showHidden = false;
         bool showPermissions = false;
         bool showFileSize = false;
         bool colorOutput = true;
         bool onlyDirs = false;
         bool onlyFiles = false;
         int maxDepth = -1;  // -1 means no limit
         std::string indentChars = "│   ";
         std::string branchChars = "├── ";
         std::string lastBranchChars = "└── ";
         std::vector<std::string> patterns;
     };
 
     Options options;
     int dirCount = 0;
     int fileCount = 0;
 
     // Terminal colors
     const std::string COLOR_RESET = "\033[0m";
     const std::string COLOR_BLUE = "\033[1;34m";
     const std::string COLOR_GREEN = "\033[1;32m";
     const std::string COLOR_YELLOW = "\033[1;33m";
     const std::string COLOR_RED = "\033[1;31m";
 
     bool matchesPattern(const fs::path& path) {
         if (options.patterns.empty()) {
             return true;
         }
 
         std::string filename = path.filename().string();
         for (const auto& pattern : options.patterns) {
             // Very simple pattern matching, could be extended with regex
             if (pattern[0] == '*') {
                 std::string suffix = pattern.substr(1);
                 if (filename.size() >= suffix.size() &&
                     filename.substr(filename.size() - suffix.size()) == suffix) {
                     return true;
                 }
             } else if (pattern[pattern.size() - 1] == '*') {
                 std::string prefix = pattern.substr(0, pattern.size() - 1);
                 if (filename.substr(0, prefix.size()) == prefix) {
                     return true;
                 }
             } else if (filename == pattern) {
                 return true;
             }
         }
         return false;
     }
 
     void printWithColor(const std::string& text, const std::string& color) {
         if (options.colorOutput) {
             std::cout << color << text << COLOR_RESET;
         } else {
             std::cout << text;
         }
     }
 
     std::string getHumanReadableSize(uintmax_t size) {
         const char* units[] = {"B", "KB", "MB", "GB", "TB", "PB", "EB"};
         int unitIndex = 0;
         double fileSize = static_cast<double>(size);
 
         while (fileSize >= 1024.0 && unitIndex < 6) {
             fileSize /= 1024.0;
             unitIndex++;
         }
 
         char buffer[32];
         if (unitIndex == 0) {
             sprintf(buffer, "%ju%s", size, units[unitIndex]);
         } else {
             sprintf(buffer, "%.1f%s", fileSize, units[unitIndex]);
         }
         return std::string(buffer);
     }
 
     void printTree(const fs::path& path, const std::string& prefix, bool isLast, int depth = 0) {
         if (options.maxDepth != -1 && depth > options.maxDepth) {
             return;
         }
 
         std::string filename = path.filename().string();
         
         // Skip hidden files/directories if not showing hidden
         if (!options.showHidden && !filename.empty() && filename[0] == '.') {
             return;
         }
 
         bool isDirectory = fs::is_directory(path);
 
         // Skip based on file/directory filters
         if ((options.onlyDirs && !isDirectory) || (options.onlyFiles && isDirectory)) {
             return;
         }
 
         // Skip if doesn't match pattern
         if (!matchesPattern(path)) {
             return;
         }
 
         // Print current item
         std::cout << prefix;
         std::cout << (isLast ? options.lastBranchChars : options.branchChars);
 
         // Print size if needed
         if (options.showFileSize && !isDirectory) {
             try {
                 uintmax_t size = fs::file_size(path);
                 std::cout << "[" << getHumanReadableSize(size) << "] ";
             } catch (const std::exception&) {
                 std::cout << "[???] ";
             }
         }
 
         // Print permissions if needed
         if (options.showPermissions) {
             fs::perms p = fs::status(path).permissions();
             std::cout << "[";
             std::cout << ((p & fs::perms::owner_read) != fs::perms::none ? "r" : "-");
             std::cout << ((p & fs::perms::owner_write) != fs::perms::none ? "w" : "-");
             std::cout << ((p & fs::perms::owner_exec) != fs::perms::none ? "x" : "-");
             std::cout << "] ";
         }
 
         // Print name with appropriate color
         if (isDirectory) {
             printWithColor(filename, COLOR_BLUE);
             dirCount++;
         } else if (fs::is_symlink(path)) {
             printWithColor(filename, COLOR_YELLOW);
             fileCount++;
         } else if ((fs::status(path).permissions() & fs::perms::owner_exec) != fs::perms::none) {
             printWithColor(filename, COLOR_GREEN);
             fileCount++;
         } else {
             printWithColor(filename, COLOR_RESET);
             fileCount++;
         }
         std::cout << std::endl;
 
         // If directory, process contents
         if (isDirectory) {
             try {
                 std::vector<fs::path> paths;
                 for (const auto& entry : fs::directory_iterator(path)) {
                     paths.push_back(entry.path());
                 }
 
                 std::sort(paths.begin(), paths.end());
 
                 for (size_t i = 0; i < paths.size(); i++) {
                     printTree(
                         paths[i],
                         prefix + (isLast ? "    " : options.indentChars),
                         i == paths.size() - 1,
                         depth + 1
                     );
                 }
             } catch (const std::exception& e) {
                 std::cout << prefix + (isLast ? "    " : options.indentChars);
                 std::cout << options.lastBranchChars;
                 printWithColor("Error: " + std::string(e.what()), COLOR_RED);
                 std::cout << std::endl;
             }
         }
     }
 
 public:
     TreeUtil() = default;
 
     void setShowHidden(bool value) { options.showHidden = value; }
     void setShowPermissions(bool value) { options.showPermissions = value; }
     void setShowFileSize(bool value) { options.showFileSize = value; }
     void setColorOutput(bool value) { options.colorOutput = value; }
     void setOnlyDirs(bool value) { options.onlyDirs = value; }
     void setOnlyFiles(bool value) { options.onlyFiles = value; }
     void setMaxDepth(int value) { options.maxDepth = value; }
     void setIndentChars(const std::string& value) { options.indentChars = value; }
     void setBranchChars(const std::string& value) { options.branchChars = value; }
     void setLastBranchChars(const std::string& value) { options.lastBranchChars = value; }
     void addPattern(const std::string& pattern) { options.patterns.push_back(pattern); }
     void clearPatterns() { options.patterns.clear(); }
 
     void run(const std::string& path = ".") {
         dirCount = 0;
         fileCount = 0;
 
         fs::path rootPath = fs::absolute(path);
         if (!fs::exists(rootPath)) {
             std::cerr << "Error: Path does not exist: " << rootPath << std::endl;
             return;
         }
 
         std::cout << rootPath.string() << std::endl;
 
         if (fs::is_directory(rootPath)) {
             std::vector<fs::path> paths;
             for (const auto& entry : fs::directory_iterator(rootPath)) {
                 paths.push_back(entry.path());
             }
 
             std::sort(paths.begin(), paths.end());
 
             for (size_t i = 0; i < paths.size(); i++) {
                 printTree(paths[i], "", i == paths.size() - 1);
             }
 
             // Print summary
             std::cout << std::endl;
             std::cout << dirCount << " directories, " << fileCount << " files" << std::endl;
         } else {
             std::cerr << "Error: Path is not a directory: " << rootPath << std::endl;
         }
     }
 };
 
 } // namespace MoreUtils
 } // namespace QCO
 
 void printUsage(const char* programName) {
     std::cout << "QCO MoreUtils - Tree Utility" << std::endl;
     std::cout << "Author: AnmiTaliDev" << std::endl;
     std::cout << "License: Apache License 2.0" << std::endl << std::endl;
     std::cout << "Usage: " << programName << " [OPTIONS] [DIRECTORY]" << std::endl;
     std::cout << "Options:" << std::endl;
     std::cout << "  -a             Show all files (including hidden)" << std::endl;
     std::cout << "  -d             Show only directories" << std::endl;
     std::cout << "  -f             Show only files" << std::endl;
     std::cout << "  -l             Show file permissions" << std::endl;
     std::cout << "  -s             Show file sizes" << std::endl;
     std::cout << "  -L LEVEL       Limit display to LEVEL levels deep" << std::endl;
     std::cout << "  -P PATTERN     List only files that match the pattern" << std::endl;
     std::cout << "  -n             No color output" << std::endl;
     std::cout << "  -h, --help     Display this help and exit" << std::endl;
 }
 
 int main(int argc, char* argv[]) {
     QCO::MoreUtils::TreeUtil tree;
     std::string directory = ".";
     
     for (int i = 1; i < argc; i++) {
         if (strcmp(argv[i], "-a") == 0) {
             tree.setShowHidden(true);
         } else if (strcmp(argv[i], "-d") == 0) {
             tree.setOnlyDirs(true);
         } else if (strcmp(argv[i], "-f") == 0) {
             tree.setOnlyFiles(true);
         } else if (strcmp(argv[i], "-l") == 0) {
             tree.setShowPermissions(true);
         } else if (strcmp(argv[i], "-s") == 0) {
             tree.setShowFileSize(true);
         } else if (strcmp(argv[i], "-n") == 0) {
             tree.setColorOutput(false);
         } else if (strcmp(argv[i], "-L") == 0 && i + 1 < argc) {
             i++;
             try {
                 tree.setMaxDepth(std::stoi(argv[i]));
             } catch (const std::exception&) {
                 std::cerr << "Error: Invalid depth value: " << argv[i] << std::endl;
                 return 1;
             }
         } else if (strcmp(argv[i], "-P") == 0 && i + 1 < argc) {
             i++;
             tree.addPattern(argv[i]);
         } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
             printUsage(argv[0]);
             return 0;
         } else if (argv[i][0] != '-') {
             directory = argv[i];
         } else {
             std::cerr << "Unknown option: " << argv[i] << std::endl;
             printUsage(argv[0]);
             return 1;
         }
     }
     
     tree.run(directory);
     return 0;
 }