#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h> 
#include <sys/ioctl.h>
#define CLR_RESET   "\033[0m"
#define CLR_RED     "\033[31m"
#define CLR_GREEN   "\033[32m"
#define CLR_YELLOW  "\033[33m"
#define CLR_BLUE    "\033[34m"
#define CLR_MAGENTA "\033[35m"
#define CLR_CYAN    "\033[36m"

void print_header(FILE *pager) {
    fprintf(pager, "%-6s %-8s %-6s %-8s %-8s %s\n",
        "PID", "USER", "STATE", "MEM(KB)", "THREADS", "NAME");
    fprintf(pager, "-------------------------------------------------------------\n");
}

int main() {
    FILE *pager = stdout;
    if (!pager) {
        perror("popen");
        return 1;
    }

    while (1) {
        struct winsize w;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
        int max_rows = w.ws_row;
        int available_rows = max_rows - 3;
        fprintf(pager, "\033[H\033[J"); 
        print_header(pager);

        DIR *dp = opendir("/proc");
        if (!dp) {
            perror("opendir");
            break;
        }

        struct dirent *entry;
        
        int printed = 0;
        
        while ((entry = readdir(dp))) {
            int pid = atoi(entry->d_name);
            if (pid <= 0)
                continue;

            char path[256];
            snprintf(path, sizeof(path), "/proc/%d/status", pid);

            FILE *f = fopen(path, "r");
            if (!f) continue;

            char line[256];
            char name[64] = "?";
            char user[32] = "?";
            char state = '?';
            long mem_kb = 0;
            int threads = 0;
            uid_t uid = -1;

            while (fgets(line, sizeof(line), f)) {
                if (sscanf(line, "Name: %63s", name) == 1) continue;
                if (sscanf(line, "State: %c", &state) == 1) continue;
                if (sscanf(line, "Uid: %u", &uid) == 1) {
                    struct passwd *pw = getpwuid(uid);
                    if (pw)
                        snprintf(user, sizeof(user), "%s", pw->pw_name);
                    else
                        snprintf(user, sizeof(user), "%u", uid);
                    continue;
                }
                if (sscanf(line, "VmRSS: %ld", &mem_kb) == 1) continue;
                if (sscanf(line, "Threads: %d", &threads) == 1) continue;
            }

            fclose(f);

            const char *state_color = CLR_RESET;
            switch (state) {
                case 'R': state_color = CLR_GREEN;   break;
                case 'S': state_color = CLR_BLUE;    break;
                case 'D': state_color = CLR_YELLOW;  break;
                case 'T': state_color = CLR_MAGENTA; break;
                case 'Z': state_color = CLR_RED;     break;
                case 'I': state_color = CLR_CYAN;    break;
                default:  state_color = CLR_RESET;   break;
            }
            
            if (printed >= available_rows)
                break; 

            fprintf(pager,
                "%-6d %-8s %s%-5c%s %-8ld %-8d %s\n",
                pid,
                user,
                state_color, state, CLR_RESET,
                mem_kb,
                threads,
                name
            );
            printed++;
        }

        closedir(dp);

        fflush(pager);
        sleep(1);     
    }
    return 0;
}

