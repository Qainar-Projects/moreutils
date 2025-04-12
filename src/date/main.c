/**
 * date - Flexible date/time utility (ASD MoreUtils)
 * Author: AnmiTaliDev
 * License: Apache 2.0
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <time.h>
 #include <string.h>
 #include <getopt.h>
 #include <unistd.h>
 
 #define VERSION "1.0.0"
 #define AUTHOR "AnmiTaliDev"
 #define LICENSE "Apache 2.0"
 
 typedef struct {
     const char *format;
     int utc;
     int show_help;
     int show_version;
 } DateOptions;
 
 DateOptions parse_args(int argc, char *argv[]) {
     DateOptions options = {
         .format = "%Y-%m-%d %H:%M:%S",
         .utc = 0,
         .show_help = 0,
         .show_version = 0,
     };
 
     struct option long_options[] = {
         {"format", required_argument, 0, 'f'},
         {"utc", no_argument, 0, 'u'},
         {"help", no_argument, 0, 'h'},
         {"version", no_argument, 0, 'v'},
         {0, 0, 0, 0}
     };
 
     int opt;
     while ((opt = getopt_long(argc, argv, "f:uhv", long_options, NULL)) != -1) {
         switch (opt) {
             case 'f':
                 options.format = optarg;
                 break;
             case 'u':
                 options.utc = 1;
                 break;
             case 'h':
                 options.show_help = 1;
                 break;
             case 'v':
                 options.show_version = 1;
                 break;
             default:
                 fprintf(stderr, "Usage: %s [-f format] [-u] [-h] [-v]\n", argv[0]);
                 exit(EXIT_FAILURE);
         }
     }
 
     return options;
 }
 
 void print_help(const char *prog_name) {
     printf("Usage: %s [OPTIONS]\n\n", prog_name);
     printf("Options:\n");
     printf("  -f, --format=FORMAT  Set output format (default: \"%%Y-%%m-%%d %%H:%%M:%%S\")\n");
     printf("  -u, --utc            Use UTC time\n");
     printf("  -h, --help           Show this help\n");
     printf("  -v, --version        Show version\n");
     printf("\nExamples:\n");
     printf("  %s -f \"%%d/%%m/%%Y\"      # 12/04/2025\n", prog_name);
     printf("  %s -u -f \"%%H:%%M UTC\"   # 15:30 UTC\n", prog_name);
 }
 
 void print_version() {
     printf("date (ASD MoreUtils) %s\n", VERSION);
     printf("Author: %s\n", AUTHOR);
     printf("License: %s\n", LICENSE);
 }
 
 int main(int argc, char *argv[]) {
     DateOptions options = parse_args(argc, argv);
 
     if (options.show_help) {
         print_help(argv[0]);
         return EXIT_SUCCESS;
     }
 
     if (options.show_version) {
         print_version();
         return EXIT_SUCCESS;
     }
 
     time_t raw_time;
     struct tm *time_info;
     char buffer[256];
 
     time(&raw_time);
     time_info = (options.utc) ? gmtime(&raw_time) : localtime(&raw_time);
 
     if (!time_info) {
         fprintf(stderr, "Error: Failed to get time\n");
         return EXIT_FAILURE;
     }
 
     if (strftime(buffer, sizeof(buffer), options.format, time_info) == 0) {
         fprintf(stderr, "Error: Invalid format or buffer overflow\n");
         return EXIT_FAILURE;
     }
 
     printf("%s\n", buffer);
     return EXIT_SUCCESS;
 }