/*
 * stat.c - File system statistics utility
 * Part of QCO MoreUtils package
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
 #include <sys/sysmacros.h>
 #include <stdlib.h>
 #include <string.h>
 #include <time.h>
 #include <sys/stat.h>
 #include <sys/types.h>
 #include <unistd.h>
 #include <pwd.h>
 #include <grp.h>
 #include <errno.h>
 #include <getopt.h>
 
 #define PROG_NAME "stat"
 #define PROG_VERSION "1.0.0"
 
 // Format flags
 #define FORMAT_DEFAULT 0
 #define FORMAT_TERSE   1
 #define FORMAT_CUSTOM  2
 
 // Time display formats
 #define TIME_NORMAL    0
 #define TIME_ISO       1
 #define TIME_LOCALE    2
 
 // Structure to hold command-line options
 typedef struct {
     int format_type;
     int time_format;
     int dereference;
     int file_system;
     char *custom_format;
 } options_t;
 
 // Function prototypes
 void print_version(void);
 void print_help(void);
 void print_stat(const char *path, options_t *opts);
 void print_file_stat(const char *path, const struct stat *sb, options_t *opts);
 void print_fs_stat(const char *path, options_t *opts);
 char *format_time(time_t t, int format);
 char *format_mode(mode_t mode);
 char *format_permissions(mode_t mode);
 const char *file_type(mode_t mode);
 
 int main(int argc, char *argv[]) {
     options_t opts = {
         .format_type = FORMAT_DEFAULT,
         .time_format = TIME_NORMAL,
         .dereference = 0,
         .file_system = 0,
         .custom_format = NULL
     };
 
     // Define command-line options
     static struct option long_options[] = {
         {"dereference", no_argument, NULL, 'L'},
         {"file-system", no_argument, NULL, 'f'},
         {"format", required_argument, NULL, 'c'},
         {"terse", no_argument, NULL, 't'},
         {"help", no_argument, NULL, 'h'},
         {"version", no_argument, NULL, 'v'},
         {NULL, 0, NULL, 0}
     };
 
     int opt;
     while ((opt = getopt_long(argc, argv, "Lfc:thv", long_options, NULL)) != -1) {
         switch (opt) {
             case 'L':
                 opts.dereference = 1;
                 break;
             case 'f':
                 opts.file_system = 1;
                 break;
             case 'c':
                 opts.format_type = FORMAT_CUSTOM;
                 opts.custom_format = optarg;
                 break;
             case 't':
                 opts.format_type = FORMAT_TERSE;
                 break;
             case 'h':
                 print_help();
                 return 0;
             case 'v':
                 print_version();
                 return 0;
             default:
                 fprintf(stderr, "Try '%s --help' for more information.\n", PROG_NAME);
                 return 1;
         }
     }
 
     // Check if at least one file path was provided
     if (optind >= argc) {
         fprintf(stderr, "%s: missing operand\n", PROG_NAME);
         fprintf(stderr, "Try '%s --help' for more information.\n", PROG_NAME);
         return 1;
     }
 
     // Process each file path
     int exit_status = 0;
     for (int i = optind; i < argc; i++) {
         if (argc - optind > 1) {
             printf("File: %s\n", argv[i]);
         }
         
         print_stat(argv[i], &opts);
         
         if (i < argc - 1) {
             printf("\n");
         }
     }
 
     return exit_status;
 }
 
 void print_version(void) {
     printf("%s %s\n", PROG_NAME, PROG_VERSION);
     printf("Part of QCO MoreUtils package\n");
     printf("Copyright 2025 AnmiTaliDev\n");
     printf("License Apache-2.0: Apache License 2.0 <http://www.apache.org/licenses/LICENSE-2.0>\n");
     printf("This is free software: you are free to change and redistribute it.\n");
     printf("There is NO WARRANTY, to the extent permitted by law.\n");
 }
 
 void print_help(void) {
     printf("Usage: %s [OPTION]... FILE...\n", PROG_NAME);
     printf("Display file or file system status.\n\n");
     printf("Options:\n");
     printf("  -L, --dereference     follow links\n");
     printf("  -f, --file-system     display file system status instead of file status\n");
     printf("  -c, --format=FORMAT   use the specified FORMAT instead of the default\n");
     printf("  -t, --terse           print the information in terse form\n");
     printf("  -h, --help            display this help and exit\n");
     printf("  -v, --version         output version information and exit\n\n");
     printf("Part of QCO MoreUtils package\n");
 }
 
 void print_stat(const char *path, options_t *opts) {
     struct stat sb;
     int status;
 
     if (opts->dereference) {
         status = stat(path, &sb);
     } else {
         status = lstat(path, &sb);
     }
 
     if (status == -1) {
         fprintf(stderr, "%s: cannot stat '%s': %s\n", PROG_NAME, path, strerror(errno));
         return;
     }
 
     if (opts->file_system) {
         print_fs_stat(path, opts);
     } else {
         print_file_stat(path, &sb, opts);
     }
 }
 
 void print_file_stat(const char *path, const struct stat *sb, options_t *opts) {
     struct passwd *pw;
     struct group *gr;
     char *access_time, *mod_time, *change_time;
 
     switch (opts->format_type) {
         case FORMAT_TERSE:
             printf("%s %lu %u %u %llu %lu %lu %lu %lu %lu %lu\n",
                    path,
                    (unsigned long)sb->st_size,
                    (unsigned)sb->st_uid,
                    (unsigned)sb->st_gid,
                    (unsigned long long)sb->st_blocks,
                    (unsigned long)sb->st_ino,
                    (unsigned long)sb->st_mode,
                    (unsigned long)sb->st_nlink,
                    (unsigned long)sb->st_atime,
                    (unsigned long)sb->st_mtime,
                    (unsigned long)sb->st_ctime);
             break;
 
         case FORMAT_CUSTOM:
             // A more complex custom formatter would go here
             // This is a simplified version
             if (opts->custom_format) {
                 printf("Custom format: %s\n", opts->custom_format);
                 // Would implement custom format parsing here
             }
             break;
 
         default:
             // Default full format
             pw = getpwuid(sb->st_uid);
             gr = getgrgid(sb->st_gid);
 
             access_time = format_time(sb->st_atime, opts->time_format);
             mod_time = format_time(sb->st_mtime, opts->time_format);
             change_time = format_time(sb->st_ctime, opts->time_format);
 
             printf("  File: %s\n", path);
             printf("  Size: %lu       Blocks: %llu     %s\n", 
                    (unsigned long)sb->st_size, 
                    (unsigned long long)sb->st_blocks, 
                    file_type(sb->st_mode));
             printf("Device: %xh/%ud   Inode: %-10lu  Links: %lu\n", 
                    (unsigned)major(sb->st_dev), 
                    (unsigned)minor(sb->st_dev), 
                    (unsigned long)sb->st_ino, 
                    (unsigned long)sb->st_nlink);
             printf("Access: (%04o/%s)  Uid: (%5u/%8s)   Gid: (%5u/%8s)\n", 
                    (unsigned)sb->st_mode & 07777, 
                    format_permissions(sb->st_mode), 
                    (unsigned)sb->st_uid, 
                    pw ? pw->pw_name : "unknown", 
                    (unsigned)sb->st_gid, 
                    gr ? gr->gr_name : "unknown");
             printf("Access: %s\n", access_time);
             printf("Modify: %s\n", mod_time);
             printf("Change: %s\n", change_time);
 
             free(access_time);
             free(mod_time);
             free(change_time);
             break;
     }
 }
 
 void print_fs_stat(const char *path, options_t *opts) {
     printf("File system statistics for %s not yet implemented.\n", path);
     // Would implement file system statistics using statfs/statvfs here
 }
 
 char *format_time(time_t t, int format) {
     char *buf = malloc(100);
     if (!buf) return NULL;
 
     switch (format) {
         case TIME_ISO:
             strftime(buf, 100, "%Y-%m-%d %H:%M:%S %z", localtime(&t));
             break;
         case TIME_LOCALE:
             strftime(buf, 100, "%c", localtime(&t));
             break;
         default:
             strftime(buf, 100, "%Y-%m-%d %H:%M:%S.000000000 %z", localtime(&t));
             break;
     }
 
     return buf;
 }
 
 const char *file_type(mode_t mode) {
     if (S_ISREG(mode))  return "regular file";
     if (S_ISDIR(mode))  return "directory";
     if (S_ISCHR(mode))  return "character special file";
     if (S_ISBLK(mode))  return "block special file";
     if (S_ISFIFO(mode)) return "fifo";
     if (S_ISLNK(mode))  return "symbolic link";
     if (S_ISSOCK(mode)) return "socket";
     return "unknown";
 }
 
 char *format_permissions(mode_t mode) {
     static char perms[11];
     
     perms[0] = (S_ISDIR(mode)) ? 'd' : (S_ISLNK(mode)) ? 'l' : '-';
     perms[1] = (mode & S_IRUSR) ? 'r' : '-';
     perms[2] = (mode & S_IWUSR) ? 'w' : '-';
     perms[3] = (mode & S_IXUSR) ? 'x' : '-';
     perms[4] = (mode & S_IRGRP) ? 'r' : '-';
     perms[5] = (mode & S_IWGRP) ? 'w' : '-';
     perms[6] = (mode & S_IXGRP) ? 'x' : '-';
     perms[7] = (mode & S_IROTH) ? 'r' : '-';
     perms[8] = (mode & S_IWOTH) ? 'w' : '-';
     perms[9] = (mode & S_IXOTH) ? 'x' : '-';
     perms[10] = '\0';
     
     // Handle special bits
     if (mode & S_ISUID) perms[3] = (perms[3] == 'x') ? 's' : 'S';
     if (mode & S_ISGID) perms[6] = (perms[6] == 'x') ? 's' : 'S';
     if (mode & S_ISVTX) perms[9] = (perms[9] == 'x') ? 't' : 'T';
     
     return perms;
 }