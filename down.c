#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <pwd.h>
#include <sys/types.h>

int main() {
    struct dirent *entry;
    DIR *dp = opendir("/proc");

    if (dp == NULL) {
        perror("opendir");
        return 1;
    }

    FILE *pager = popen("less -R", "w");
    if (!pager) {
        perror("popen");
        closedir(dp);
        return 1;
    }

    fprintf(pager, "\033[H\033[J");

    fprintf(pager,
        "%-6s %-8s %-6s %-8s %-8s %s\n",
        "PID", "USER", "STATE", "MEM(KB)", "THREADS", "NAME"
    );
    fprintf(pager,
        "-------------------------------------------------------------\n"
    );

    while ((entry = readdir(dp))) {
        int pid = atoi(entry->d_name);
        if (pid <= 0)
            continue;

        char path[256];
        snprintf(path, sizeof(path), "/proc/%d/status", pid);

        FILE *f = fopen(path, "r");
        if (!f)
            continue;

        char line[256];
        char name[64] = "?";
        char user[32] = "?";
        char state = '?';
        long mem_kb = 0;
        int threads = 0;
        uid_t uid = -1;

        while (fgets(line, sizeof(line), f)) {
            if (sscanf(line, "Name: %63s", name) == 1)
                continue;

            if (sscanf(line, "State: %c", &state) == 1)
                continue;

            if (sscanf(line, "Uid: %u", &uid) == 1) {
                struct passwd *pw = getpwuid(uid);
                if (pw)
                    snprintf(user, sizeof(user), "%s", pw->pw_name);
                else
                    snprintf(user, sizeof(user), "%u", uid);
                continue;
            }

            if (sscanf(line, "VmRSS: %ld", &mem_kb) == 1)
                continue;

            if (sscanf(line, "Threads: %d", &threads) == 1)
                continue;
        }

        fclose(f);

        fprintf(pager,
            "%-6d %-8s %-5c %-8ld %-8d %s\n",
            pid,
            user,
            state,
            mem_kb,
            threads,
            name
        );
    }

    closedir(dp);
    pclose(pager);
    return 0;
}

