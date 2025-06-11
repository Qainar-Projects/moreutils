/**
 * yes.c - Simple utility to output a string repeatedly until interrupted
 * 
 * Part of QCO MoreUtils package
 * 
 * Copyright 2025 AnmiTaliDev
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *    http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <unistd.h>
 #include <stdbool.h>
 #include <getopt.h>
 #include <signal.h>
 
 #define VERSION "1.0.0"
 #define BUFFER_SIZE 8192
 
 // Flag to control program execution
 volatile sig_atomic_t keep_running = 1;
 
 // Signal handler for clean termination
 void handle_signal(int sig) {
     keep_running = 0;
 }
 
 // Print help message
 void print_help(const char* program_name) {
     printf("Usage: %s [OPTION]... [STRING]...\n", program_name);
     printf("Repeatedly output a line with all specified STRING(s), or 'y'.\n\n");
     printf("  -h, --help       display this help and exit\n");
     printf("  -v, --version    output version information and exit\n");
     printf("  -n, --newline    don't output the trailing newline\n");
     printf("  -l N, --limit=N  stop after N iterations\n");
     printf("\nPart of QCO MoreUtils by AnmiTaliDev.\n");
     printf("Licensed under Apache License 2.0.\n");
 }
 
 int main(int argc, char *argv[]) {
     char buffer[BUFFER_SIZE];
     const char* output = "y";
     bool add_newline = true;
     long long limit = -1; // Negative means infinite
     int c;
     
     // Define long options
     static struct option long_options[] = {
         {"help", no_argument, 0, 'h'},
         {"version", no_argument, 0, 'v'},
         {"newline", no_argument, 0, 'n'},
         {"limit", required_argument, 0, 'l'},
         {0, 0, 0, 0}
     };
     
     // Set up signal handlers
     signal(SIGINT, handle_signal);
     signal(SIGTERM, handle_signal);
 
     // Process command line options
     while ((c = getopt_long(argc, argv, "hvnl:", long_options, NULL)) != -1) {
         switch (c) {
             case 'h':
                 print_help(argv[0]);
                 return 0;
             case 'v':
                 printf("yes (QCO MoreUtils) %s\n", VERSION);
                 printf("Copyright (C) 2025 AnmiTaliDev\n");
                 printf("License Apache 2.0\n");
                 return 0;
             case 'n':
                 add_newline = false;
                 break;
             case 'l':
                 limit = atoll(optarg);
                 if (limit < 0) {
                     fprintf(stderr, "Error: Limit must be a non-negative number\n");
                     return 1;
                 }
                 break;
             case '?':
                 return 1;
             default:
                 abort();
         }
     }
 
     // Check if we have arguments to output instead of default "y"
     if (optind < argc) {
         // Combine all remaining arguments with spaces between them
         buffer[0] = '\0';
         for (int i = optind; i < argc; i++) {
             if (i > optind) {
                 strcat(buffer, " ");
             }
             strcat(buffer, argv[i]);
             
             // Check if buffer is getting too full
             if (strlen(buffer) > BUFFER_SIZE - 100) {
                 break;
             }
         }
         output = buffer;
     }
 
     // Main output loop
     long long count = 0;
     while (keep_running && (limit < 0 || count < limit)) {
         if (add_newline) {
             printf("%s\n", output);
         } else {
             printf("%s", output);
             fflush(stdout);
         }
         count++;
     }
 
     return 0;
 }