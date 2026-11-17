#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "task.h"

/* Read a full menu line; discard overlong input to keep future reads aligned. */
static int read_line(const char *prompt, char *buffer, size_t size) {
    int c;
    size_t len;

    fputs(prompt, stdout);
    fflush(stdout);
    if (fgets(buffer, (int)size, stdin) == NULL) return 0;
    len = strlen(buffer);
    if (len && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
    } else {
        while ((c = getchar()) != '\n' && c != EOF) { }
        puts("[Error] Input was too long.");
        return -1;
    }
    return 1;
}

static int read_int(const char *prompt, int min, int max, int *value) {
    char line[64];
    char *end;
    long parsed;
    int r = read_line(prompt, line, sizeof(line));
    if (r <= 0) return r;
    errno = 0;
    parsed = strtol(line, &end, 10);
    while (*end == ' ' || *end == '\t') end++;
    if (errno || end == line || *end != '\0' || parsed < min || parsed > max) {
        printf("[Error] Enter an integer from %d to %d.\n", min, max);
        return -1;
    }
    *value = (int)parsed;
    return 1;
}

int main(void) {
    int choice, burst, memory_kb, task_id, r;
    char command[MAX_CMD_LEN];

    setvbuf(stdout, NULL, _IOLBF, 0);
    init_task_manager();
    printf("MiniOS-TM | CSE233 Group 4 | Phase 1\n");
    printf("Master Controller: PID=%ld, PPID=%ld\n",
           (long)getpid(), (long)getppid());

    for (;;) {
        update_background_tasks();
        puts("\n========== MINI OS TASK MANAGER ==========");
        puts("1. Submit & run FOREGROUND task");
        puts("2. Submit & run BACKGROUND task");
        puts("3. Display tasks / PID / states");
        puts("4. Terminate a running task (SIGTERM)");
        puts("5. Exit (clean up child processes)");
        r = read_int("Select [1-5]: ", 1, 5, &choice);
        if (r == 0) break;        /* Ctrl+D / closed input */
        if (r < 0) continue;

        switch (choice) {
            case 1:
            case 2:
                r = read_line("Linux command (e.g. ls -l, sleep 20): ",
                              command, sizeof(command));
                if (r == 0) goto done;
                if (r < 0) break;
                r = read_int("Estimated burst time (seconds, 0-3600): ",
                             0, 3600, &burst);
                if (r == 0) goto done;
                if (r < 0) break;
                r = read_int("Estimated memory (KB, 1-1048576): ",
                             1, 1048576, &memory_kb);
                if (r == 0) goto done;
                if (r < 0) break;
                create_and_run_task(command, burst, memory_kb, choice == 2);
                break;
            case 3:
                display_tasks_table();
                break;
            case 4:
                display_tasks_table();
                r = read_int("Task ID to terminate: ", 1, MAX_TASKS, &task_id);
                if (r == 0) goto done;
                if (r > 0) terminate_task(task_id);
                break;
            case 5:
                goto done;
            default:
                break;
        }
    }
done:
    shutdown_task_manager();
    puts("MiniOS-TM exited cleanly. Goodbye!");
    return 0;
}
