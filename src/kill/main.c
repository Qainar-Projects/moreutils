/*
 * QCO MoreUtils - Kill
 * Flexible process signal management utility
 * Author: AnmiTaliDev
 * License: Apache 2.0
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <unistd.h>
 #include <dirent.h>
 #include <signal.h>
 #include <sys/types.h>
 #include <pwd.h>
 #include <errno.h>
 
 #define MAX_SIGNALS 32
 
 typedef struct {
     pid_t pid;
     unsigned long start_time;
 } ProcessInfo;
 
 typedef struct {
     char *signal;
     char *exact_name;
     char *contains_str;
     char *username;
     uid_t uid;
     int newest;
     int oldest;
 } Criteria;
 
 const char *signals[MAX_SIGNALS] = {
     "HUP", "INT", "QUIT", "ILL", "TRAP", "ABRT", "BUS", "FPE",
     "KILL", "USR1", "SEGV", "USR2", "PIPE", "ALRM", "TERM", "STKFLT",
     "CHLD", "CONT", "STOP", "TSTP", "TTIN", "TTOU", "URG", "XCPU",
     "XFSZ", "VTALRM", "PROF", "WINCH", "POLL", "PWR", "SYS"
 };
 
 void list_signals() {
     printf("Available signals:\n");
     for (int i = 0; i < MAX_SIGNALS; i++) {
         printf("%2d) SIG%-8s\n", i + 1, signals[i]);
     }
 }
 
 int parse_signal(const char *sig) {
     for (int i = 0; i < MAX_SIGNALS; i++) {
         if (strcasecmp(sig, signals[i]) == 0 || 
             (sig[0] == '-' && strcasecmp(sig + 1, signals[i]) == 0)) {
             return i + 1;
         }
     }
     char *end;
     long num = strtol(sig, &end, 10);
     if (*end == '\0' && num > 0 && num <= MAX_SIGNALS) return num;
     return -1;
 }
 
 int get_uid(const char *username, uid_t *uid) {
     struct passwd *pwd = getpwnam(username);
     if (!pwd) return -1;
     *uid = pwd->pw_uid;
     return 0;
 }
 
 int read_proc_file(pid_t pid, const char *filename, char *buf, size_t size) {
     char path[256];
     snprintf(path, sizeof(path), "/proc/%d/%s", pid, filename);
     FILE *f = fopen(path, "r");
     if (!f) return -1;
     size_t read = fread(buf, 1, size - 1, f);
     fclose(f);
     buf[read] = '\0';
     return 0;
 }
 
 int get_process_info(pid_t pid, Criteria *crit, ProcessInfo *info) {
     char cmdline[4096];
     if (read_proc_file(pid, "cmdline", cmdline, sizeof(cmdline))) return 0;
 
     // Check process name
     if (crit->exact_name) {
         char *first = strtok(cmdline, "\0");
         char *slash = strrchr(first, '/');
         char *name = slash ? slash + 1 : first;
         if (strcmp(name, crit->exact_name) != 0) return 0;
     }
 
     // Check command line substring
     if (crit->contains_str && !strstr(cmdline, crit->contains_str)) return 0;
 
     // Check user ownership
     if (crit->username) {
         char status[4096];
         if (read_proc_file(pid, "status", status, sizeof(status))) return 0;
         char *uid_line = strstr(status, "Uid:");
         if (!uid_line) return 0;
         uid_t uid;
         sscanf(uid_line + 5, "%u", &uid);
         if (uid != crit->uid) return 0;
     }
 
     // Get process start time
     char stat[1024];
     if (read_proc_file(pid, "stat", stat, sizeof(stat))) return 0;
     unsigned long start_time;
     sscanf(strrchr(stat, ')') + 2, "%*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %lu", &start_time);
 
     info->pid = pid;
     info->start_time = start_time;
     return 1;
 }
 
 int find_processes(Criteria *crit, ProcessInfo **result) {
     DIR *dir = opendir("/proc");
     if (!dir) return -1;
 
     ProcessInfo *list = NULL;
     int count = 0;
 
     struct dirent *entry;
     while ((entry = readdir(dir))) {
         if (entry->d_type != DT_DIR) continue;
         pid_t pid = atoi(entry->d_name);
         if (pid <= 0) continue;
 
         ProcessInfo info;
         if (get_process_info(pid, crit, &info)) {
             list = realloc(list, (count + 1) * sizeof(ProcessInfo));
             list[count++] = info;
         }
     }
 
     closedir(dir);
     *result = list;
     return count;
 }
 
 int main(int argc, char *argv[]) {
     Criteria crit = {0};
     int opt;
     int list_mode = 0;
     char *signum = "TERM";
 
     // Parse command-line arguments
     while ((opt = getopt(argc, argv, "ls:n:o:e:u:c:")) != -1) {
         switch (opt) {
             case 'l': list_mode = 1; break;
             case 's': signum = optarg; break;
             case 'n': crit.exact_name = optarg; break;
             case 'o': crit.contains_str = optarg; break;
             case 'e': crit.exact_name = optarg; break;
             case 'u': 
                 if (get_uid(optarg, &crit.uid) != 0) {
                     fprintf(stderr, "Unknown user: %s\n", optarg);
                     exit(EXIT_FAILURE);
                 }
                 break;
             case 'c': crit.contains_str = optarg; break;
             default: exit(EXIT_FAILURE);
         }
     }
 
     if (list_mode) {
         list_signals();
         return 0;
     }
 
     // Validate signal
     int sig = parse_signal(signum);
     if (sig <= 0) {
         fprintf(stderr, "Invalid signal: %s\n", signum);
         exit(EXIT_FAILURE);
     }
 
     // Find matching processes
     ProcessInfo *processes;
     int count = find_processes(&crit, &processes);
     if (count < 0) {
         fprintf(stderr, "Error searching processes\n");
         exit(EXIT_FAILURE);
     }
 
     // Time-based filtering
     if (crit.newest || crit.oldest) {
         ProcessInfo selected = processes[0];
         for (int i = 1; i < count; i++) {
             if ((crit.newest && processes[i].start_time > selected.start_time) ||
                 (crit.oldest && processes[i].start_time < selected.start_time)) {
                 selected = processes[i];
             }
         }
         count = 1;
         processes[0] = selected;
     }
 
     // Send signals
     for (int i = 0; i < count; i++) {
         if (kill(processes[i].pid, sig) == -1) {
             fprintf(stderr, "Error sending signal to PID %d: %s\n", 
                     processes[i].pid, strerror(errno));
         }
     }
 
     free(processes);
     return 0;
 }