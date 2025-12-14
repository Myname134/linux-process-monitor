#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>

int main() {
    struct dirent *entry;
    DIR *dp = opendir("/proc");

    if (dp == NULL) {
        perror("opendir");
        return 1;
    }

    printf("%-10s %-20s\n", "PID", "Process Name");
    while ((entry = readdir(dp))) {
        // Skip non-numeric entries
        int pid = atoi(entry->d_name);
        if (pid > 0) {
            char path[256], name[256];
            snprintf(path, sizeof(path), "/proc/%d/comm", pid);
            FILE *f = fopen(path, "r");
            if (f) {
                fgets(name, sizeof(name), f);
                name[strcspn(name, "\n")] = 0; // Remove newline
                printf("%-10d %-20s\n", pid, name);
                fclose(f);
            }
        }
    }

    closedir(dp);
    return 0;
}

