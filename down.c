#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h> 
#include <sys/ioctl.h>
#include <ncurses.h>
#define CLR_RESET   "\033[0m"
#define CLR_RED     "\033[31m"
#define CLR_GREEN   "\033[32m"
#define CLR_YELLOW  "\033[33m"
#define CLR_BLUE    "\033[34m"
#define CLR_MAGENTA "\033[35m"
#define CLR_CYAN    "\033[36m"
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

int main() {
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  nodelay(stdscr, TRUE);

  while(1) {
    process_count = 0;

    DIR *dp = opendir("/proc");
    if (!dp) break; 

    struct dirent *entry;

    while ((entry = readdir(dp))) {
      int pid = atoi(entry->d_name);
      if (pid <= 0) continue;

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
        }
        if (sscanf(line, "VmRSS: %ld", &mem_kb) == 1) continue;
        if (sscanf(line, "Threads: %d", &threads) ==1 ) continue;
      }
      fclose(f);

      if (process_count < MAX_PROCESSES) {
        process_list[process_count].pid = pid;
        strcpy(process_list[process_count].user, user);
        process_list[process_count].state = state;
        process_list[process_count].mem_kb = mem_kb;
        process_list[process_count].threads = threads;
        strcpy(process_list[process_count].name, name);
        process_count++;
      }
    }
    closedir(dp);

    int max_rows, max_cols;
    getmaxyx(stdscr, max_rows, max_cols);
    int available_rows = max_rows - 3;

    clear();

    mvprintw(0, 0, "%-6s %-8s %-6s %-8s %-8s %s",
             "PID", "USER", "STATE", "MEM{KB}", "THREADS", "NAME");
    
    for (int i = 0; i < available_rows && (i + offset) < process_count; i++) {
      Process *p = &process_list[i + offset];

      mvprintw(i + 2, 0, 
               "%-6d %-8s %-5c %-8ld %-8d %s",
               p->pid,
               p->user,
               p->state,
               p->mem_kb,
               p->threads,
               p->name);
    }
    mvprintw(max_rows - 1, 0, "Use UP/DOWN arrows to scroll, q to quit");

    refresh();

    int ch = getch();

    if(ch == KEY_DOWN && offset + available_rows < process_count)
      offset++;

    if(ch == KEY_UP && offset > 0)
      offset--;
    
    if (ch == 'q')
      break;
    
    usleep(200000);
  }
  endwin();
  return 0;
}

  
