/*
 * tee.c - utility from ASD MoreUtils package
 * Version 1.0.0
 * 
 * Copyright 2025 AnmiTaliDev
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
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
 #include <fcntl.h>
 #include <signal.h>
 #include <errno.h>
 #include <sys/types.h>
 #include <getopt.h>
 
 #define BUFFER_SIZE 4096
 #define VERSION "1.0.0"
 #define PACKAGE "ASD MoreUtils"
 
 /* Command line option flags */
 static int append_flag = 0;        /* -a, --append */
 static int ignore_interrupts = 0;  /* -i, --ignore-interrupts */
 static int output_linebuffered = 0; /* -l, --line-buffered */
 static int verbose_mode = 0;       /* -v, --verbose */
 
 /* Help information */
 static void print_help(void) {
     printf("Usage: tee [OPTION]... [FILE]...\n");
     printf("Read from standard input and write to both standard output and files.\n\n");
     printf("  -a, --append              append to the given FILEs, do not overwrite\n");
     printf("  -i, --ignore-interrupts   ignore interrupt signals\n");
     printf("  -l, --line-buffered       use line buffering for output\n");
     printf("  -v, --verbose             print diagnostic messages\n");
     printf("      --help                display this help and exit\n");
     printf("      --version             output version information and exit\n");
     printf("\nPart of %s package, version %s\n", PACKAGE, VERSION);
     printf("Author: AnmiTaliDev\n");
     printf("License: Apache 2.0\n");
 }
 
 /* Version information */
 static void print_version(void) {
     printf("tee (ASD MoreUtils) %s\n", VERSION);
     printf("Copyright (C) 2025 AnmiTaliDev\n");
     printf("License: Apache 2.0\n");
 }
 
 /* Signal handler */
 static void signal_handler(int sig) {
     if (verbose_mode) {
         fprintf(stderr, "Received signal %d\n", sig);
     }
     exit(EXIT_SUCCESS);
 }
 
 int main(int argc, char *argv[]) {
     char buffer[BUFFER_SIZE];
     size_t bytes_read;
     int *file_descriptors = NULL;
     int num_files = 0;
     int option;
     
     /* Command line options */
     static struct option long_options[] = {
         {"append", no_argument, NULL, 'a'},
         {"ignore-interrupts", no_argument, NULL, 'i'},
         {"line-buffered", no_argument, NULL, 'l'},
         {"verbose", no_argument, NULL, 'v'},
         {"help", no_argument, NULL, 'h'},
         {"version", no_argument, NULL, 'V'},
         {NULL, 0, NULL, 0}
     };
 
     /* Parse command line options */
     while ((option = getopt_long(argc, argv, "ailv", long_options, NULL)) != -1) {
         switch (option) {
             case 'a':
                 append_flag = 1;
                 break;
             case 'i':
                 ignore_interrupts = 1;
                 break;
             case 'l':
                 output_linebuffered = 1;
                 break;
             case 'v':
                 verbose_mode = 1;
                 break;
             case 'h':
                 print_help();
                 exit(EXIT_SUCCESS);
             case 'V':
                 print_version();
                 exit(EXIT_SUCCESS);
             default:
                 fprintf(stderr, "Try 'tee --help' for more information.\n");
                 exit(EXIT_FAILURE);
         }
     }
 
     /* Set up signal handling */
     if (ignore_interrupts) {
         signal(SIGINT, SIG_IGN);
     } else {
         signal(SIGINT, signal_handler);
     }
 
     /* Prepare file descriptors */
     num_files = argc - optind;
     if (num_files > 0) {
         file_descriptors = malloc(num_files * sizeof(int));
         if (!file_descriptors) {
             perror("Memory allocation error");
             exit(EXIT_FAILURE);
         }
 
         /* Open files */
         for (int i = 0; i < num_files; i++) {
             int flags = O_WRONLY | O_CREAT;
             
             flags |= append_flag ? O_APPEND : O_TRUNC;
             
             file_descriptors[i] = open(argv[optind + i], flags, 0666);
             if (file_descriptors[i] == -1) {
                 fprintf(stderr, "tee: %s: %s\n", argv[optind + i], strerror(errno));
                 file_descriptors[i] = -1;  // Mark as invalid
                 continue;
             }
             
             if (verbose_mode) {
                 fprintf(stderr, "Opened file: %s (fd: %d, mode: %s)\n", 
                         argv[optind + i], 
                         file_descriptors[i], 
                         append_flag ? "append" : "overwrite");
             }
         }
     }
 
     /* Set output buffering */
     if (output_linebuffered) {
         setvbuf(stdout, NULL, _IOLBF, 0);
     }
 
     /* Main read/write loop */
     while ((bytes_read = read(STDIN_FILENO, buffer, BUFFER_SIZE)) > 0) {
         /* Write to standard output */
         if (write(STDOUT_FILENO, buffer, bytes_read) != bytes_read) {
             perror("Error writing to standard output");
             break;
         }
 
         /* Write to all files */
         for (int i = 0; i < num_files; i++) {
             if (file_descriptors[i] != -1) {
                 if (write(file_descriptors[i], buffer, bytes_read) != bytes_read) {
                     fprintf(stderr, "tee: %s: %s\n", argv[optind + i], strerror(errno));
                     close(file_descriptors[i]);
                     file_descriptors[i] = -1;  // Mark as invalid
                 }
             }
         }
     }
 
     /* Check for read error */
     if (bytes_read == -1) {
         perror("Read error");
     }
 
     /* Close files */
     for (int i = 0; i < num_files; i++) {
         if (file_descriptors[i] != -1) {
             if (close(file_descriptors[i]) == -1) {
                 fprintf(stderr, "Error closing file: %s\n", strerror(errno));
             }
         }
     }
 
     /* Free memory */
     free(file_descriptors);
 
     return EXIT_SUCCESS;
 }