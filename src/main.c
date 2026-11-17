#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "task.h"
#include "ui.h"

/* Parse entire input lines so malformed input cannot desynchronize the menu. */
static int read_line(const char *prompt, char *buffer, size_t size) {
    int c;
    size_t len;
    fputs(prompt, stdout);
    fflush(stdout);
    if (fgets(buffer, (int)size, stdin) == NULL) return 0;
    len = strlen(buffer);
    if (len && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
    } else if (!feof(stdin)) {
        c = getchar();
        if (c != EOF) {
            while (c != '\n' && c != EOF) c = getchar();
            puts("[Error] Input was too long.");
            return -1;
        }
    }
    return 1;
}

static int read_int(const char *prompt, int minimum, int maximum, int *value) {
    char line[64], *end;
    long parsed;
    int result = read_line(prompt, line, sizeof(line));
    if (result <= 0) return result;
    errno = 0;
    parsed = strtol(line, &end, 10);
    while (*end == ' ' || *end == '\t') ++end;
    if (errno || end == line || *end || parsed < minimum || parsed > maximum) {
        printf("[Error] Enter an integer from %d to %d.\n", minimum, maximum);
        return -1;
    }
    *value = (int)parsed;
    return 1;
}

static void watch_tasks(int seconds) {
    const struct timespec one_second = {.tv_sec = 1, .tv_nsec = 0};
    ui_watch_notice(seconds);
    for (int i = 0; i < seconds; ++i) {
        printf("\n[MONITOR] Snapshot %d/%d\n", i + 1, seconds);
        display_tasks_table();
        if (i + 1 < seconds) {
            struct timespec remaining = one_second;
            while (nanosleep(&remaining, &remaining) == -1 && errno == EINTR) { }
        }
    }
}

int main(int argc, char **argv) {
    int choice, burst, memory_kb, task_id, seconds, result;
    char command[MAX_CMD_LEN];
    TaskStats stats;

    if (argc == 2 && strcmp(argv[1], "--version") == 0) {
        puts("MiniOS-TM v2.0 (C11, POSIX Linux) - CSE233 Group 04");
        return 0;
    }
    if (argc != 1) {
        fprintf(stderr, "Usage: %s [--version]\n", argv[0]);
        return 2;
    }

    setvbuf(stdout, NULL, _IOLBF, 0);
    ui_init();
    init_task_manager();
    ui_banner(getpid(), getppid());

    for (;;) {
        update_background_tasks();
        get_task_stats(&stats);
        ui_menu(&stats);
        result = read_int("minios> Select [1-7]: ", 1, 7, &choice);
        if (result == 0) break;
        if (result < 0) continue;

        switch (choice) {
            case 1:
            case 2:
                ui_section(choice == 1 ? "FOREGROUND DISPATCH" : "BACKGROUND DISPATCH");
                result = read_line("Linux command (e.g. ls -l, sleep 30): ",
                                   command, sizeof(command));
                if (result == 0) goto done;
                if (result < 0) break;
                result = read_int("Estimated burst (seconds, 0-3600): ", 0, 3600, &burst);
                if (result == 0) goto done;
                if (result < 0) break;
                result = read_int("Estimated memory (KB, 1-1048576): ", 1, 1048576, &memory_kb);
                if (result == 0) goto done;
                if (result < 0) break;
                create_and_run_task(command, burst, memory_kb, choice == 2);
                break;
            case 3:
                ui_section("TASK REGISTRY / PROCESS STATUS");
                display_tasks_table();
                break;
            case 4:
                ui_section("PROCESS TERMINATION");
                display_tasks_table();
                result = read_int("Task ID to terminate (NOT Linux PID): ", 1, MAX_TASKS, &task_id);
                if (result == 0) goto done;
                if (result > 0) terminate_task(task_id);
                break;
            case 5:
                goto done;
            case 6:
                result = read_int("Monitor snapshots [1-15]: ", 1, 15, &seconds);
                if (result == 0) goto done;
                if (result > 0) watch_tasks(seconds);
                break;
            case 7:
                ui_system_calls_guide();
                break;
            default:
                break;
        }
    }
done:
    shutdown_task_manager();
    puts("MiniOS-TM exited cleanly. Goodbye!");
    return 0;
}
