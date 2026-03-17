#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h> 
#include <ctype.h>
#include <ncurses.h>
#include <time.h>

#define MAX_PROCESSES 4096

typedef struct {
    int pid;
    char user[32];
    char state;
    long mem_kb;
    int threads;
    char name[64];
} Process;

Process process_list[MAX_PROCESSES];
int process_count = 0;
int offset = 0;
time_t last_scan = 0;

// Return color pair for state
short state_color_pair(char state) {
    switch(state) {
        case 'R': return 1; // green
        case 'S': return 2; // yellow
        case 'D': return 3; // cyan
        case 'T': return 4; // magenta
        case 'Z': return 5; // red
        default:  return 0; // default
    }
}

int main() {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    curs_set(0);

    if (has_colors()) {
        start_color();
        init_pair(1, COLOR_GREEN,   COLOR_BLACK); // Running
        init_pair(2, COLOR_YELLOW,  COLOR_BLACK); // Sleeping
        init_pair(3, COLOR_CYAN,    COLOR_BLACK); // Disk sleep
        init_pair(4, COLOR_MAGENTA, COLOR_BLACK); // Stopped
        init_pair(5, COLOR_RED,     COLOR_BLACK); // Zombie
    }

    while (1) {
        time_t now = time(NULL);
        if (now != last_scan) {
            process_count = 0;
            DIR *dp = opendir("/proc");
            if (!dp) break;

            struct dirent *entry;
            while ((entry = readdir(dp))) {
                if (!isdigit(entry->d_name[0])) continue;

                int pid = atoi(entry->d_name);
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
                        if (pw) snprintf(user, sizeof(user), "%s", pw->pw_name);
                    }
                    if (sscanf(line, "VmRSS: %ld kB", &mem_kb) == 1) continue;
                    if (sscanf(line, "Threads: %d", &threads) == 1) continue;
                }
                fclose(f);

                if (process_count < MAX_PROCESSES) {
                    process_list[process_count].pid = pid;
                    strncpy(process_list[process_count].user, user, sizeof(process_list[0].user)-1);
                    process_list[process_count].user[sizeof(process_list[0].user)-1] = '\0';
                    process_list[process_count].state = state;
                    process_list[process_count].mem_kb = mem_kb;
                    process_list[process_count].threads = threads;
                    strncpy(process_list[process_count].name, name, sizeof(process_list[0].name)-1);
                    process_list[process_count].name[sizeof(process_list[0].name)-1] = '\0';
                    process_count++;
                }
            }
            closedir(dp);
            last_scan = now;
        }

        int max_rows = getmaxy(stdscr);
        int available_rows = max_rows - 4; // leave space for header/footer

        erase();

        mvprintw(0, 0, "%-6s %-8s %-6s %-8s %-8s %s",
                 "PID", "USER", "STATE", "MEM{KB}", "THREADS", "NAME");

        for (int i = 0; i < available_rows && (i + offset) < process_count; i++) {
            Process *p = &process_list[i + offset];

            short color = state_color_pair(p->state);
            if (color != 0) attron(COLOR_PAIR(color));
            mvprintw(i + 2, 0,
                     "%-6d %-8s %-6c %-8ld %-8d %s",
                     p->pid,
                     p->user,
                     p->state,
                     p->mem_kb,
                     p->threads,
                     p->name);
            if (color != 0) attroff(COLOR_PAIR(color));
        }

        mvprintw(max_rows - 2, 0, "Use UP/DOWN arrows to scroll, q to quit");
        mvprintw(max_rows - 1, 0, "R:Running S:Sleeping D:Disk T:Stopped Z:Zombie");

        refresh();

        int ch = getch();
        if (ch == 'q') break;
        else if (ch == KEY_UP) offset--;
        else if (ch == KEY_DOWN) offset++;

        if (offset > process_count - available_rows) offset = process_count - available_rows;
        if (offset < 0) offset = 0;

        napms(50);
    }

    endwin();
    return 0;
}

  
