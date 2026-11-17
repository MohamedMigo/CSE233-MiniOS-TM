#define _POSIX_C_SOURCE 200809L
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include "task.h"

static TaskPCB tasks[MAX_TASKS];
static size_t task_count = 0;

void init_task_manager(void) { task_count = 0; }

const char *get_state_name(TaskState state) {
    switch (state) {
        case TASK_NEW: return "NEW";
        case TASK_READY: return "READY";
        case TASK_RUNNING: return "RUNNING";
        case TASK_WAITING: return "WAITING";
        case TASK_COMPLETED: return "COMPLETED";
        case TASK_TERMINATED: return "TERMINATED";
        default: return "UNKNOWN";
    }
}

/* No shell is used. Parse whitespace, quoted arguments and backslash escapes. */
static int parse_command(const char *input, char *storage, size_t storage_size,
                         char *argv[MAX_ARGS]) {
    size_t argc = 0;
    char *write = storage;
    char *end = storage + storage_size;
    const char *read = input;

    while (*read) {
        char quote = '\0';
        while (isspace((unsigned char)*read)) read++;
        if (!*read) break;
        if (argc >= MAX_ARGS - 1) return -1;
        argv[argc++] = write;
        while (*read && (quote || !isspace((unsigned char)*read))) {
            if (*read == '\\' && read[1] && quote != '\'') {
                read++;
                if (write >= end - 1) return -1;
                *write++ = *read++;
            } else if (*read == '\'' || *read == '"') {
                if (!quote) quote = *read++;
                else if (quote == *read) { read++; quote = '\0'; }
                else {
                    if (write >= end - 1) return -1;
                    *write++ = *read++;
                }
            } else {
                if (write >= end - 1) return -1;
                *write++ = *read++;
            }
        }
        if (quote || write >= end) return -1;
        *write++ = '\0';
    }
    argv[argc] = NULL;
    return (int)argc;
}

static void set_final_state(TaskPCB *task, int status) {
    if (WIFEXITED(status)) {
        task->exit_status = WEXITSTATUS(status);
        task->term_signal = 0;
        task->state = task->exit_status == 0 ? TASK_COMPLETED : TASK_TERMINATED;
    } else if (WIFSIGNALED(status)) {
        task->term_signal = WTERMSIG(status);
        task->exit_status = 128 + task->term_signal;
        task->state = TASK_TERMINATED;
    }
}

static pid_t wait_for_pid(pid_t pid, int *status, int options) {
    pid_t ret;
    do { ret = waitpid(pid, status, options); }
    while (ret == -1 && errno == EINTR);
    return ret;
}

int create_and_run_task(const char *cmd, int burst, int memory_kb, int background) {
    char storage[MAX_CMD_LEN];
    char *argv[MAX_ARGS];
    int count, status;
    pid_t pid;
    TaskPCB *task;

    if (!cmd || strlen(cmd) >= MAX_CMD_LEN) {
        puts("[Error] Command missing or too long.");
        return -1;
    }
    count = parse_command(cmd, storage, sizeof(storage), argv);
    if (count <= 0) {
        puts("[Error] Empty/malformed command or too many arguments.");
        return -1;
    }
    if (task_count == MAX_TASKS) {
        puts("[Error] Task table full (50 tasks).");
        return -1;
    }
    task = &tasks[task_count];
    memset(task, 0, sizeof(*task));
    task->task_id = (int)task_count + 1;
    snprintf(task->command, sizeof(task->command), "%s", cmd);
    task->parent_pid = getpid();
    task->burst_time = burst;
    task->memory_size = memory_kb;
    task->is_background = background;
    task->exit_status = -1;
    task->state = TASK_READY;

    /* Flush buffered output so fork() will not duplicate it in the child. */
    fflush(NULL);
    pid = fork();
    if (pid < 0) {
        perror("[Error] fork");
        return -1;
    }
    if (pid == 0) {
        printf("[CHILD] PID=%ld PPID=%ld | execvp(%s)\n",
               (long)getpid(), (long)getppid(), argv[0]);
        fflush(stdout);
        execvp(argv[0], argv);
        perror("[CHILD] execvp failed");
        _exit(127); /* No return to the parent's menu if execvp fails. */
    }

    task_count++;
    task->pid = pid;
    task->state = TASK_RUNNING;
    printf("[%s] Task #%d | parent PID=%ld -> child PID=%ld | command: %s\n",
           background ? "BG" : "FG", task->task_id,
           (long)getpid(), (long)pid, task->command);
    if (background) {
        puts("[BG] Controller remains interactive; waitpid(WNOHANG) polls completion.");
        return task->task_id;
    }

    puts("[FG] Master Controller is waiting: waitpid(child, ..., 0)");
    pid = wait_for_pid(task->pid, &status, 0);
    if (pid == task->pid) {
        set_final_state(task, status);
        printf("[FG] Task #%d: %s | exit code %d%s\n", task->task_id,
               get_state_name(task->state), task->exit_status,
               task->term_signal ? " (signal termination)" : "");
    } else {
        perror("[Error] waitpid");
    }
    return task->task_id;
}

void update_background_tasks(void) {
    size_t i;
    for (i = 0; i < task_count; i++) {
        TaskPCB *task = &tasks[i];
        int status;
        pid_t pid;
        if (!task->is_background || task->state != TASK_RUNNING) continue;
        pid = wait_for_pid(task->pid, &status, WNOHANG);
        if (pid == task->pid) {
            set_final_state(task, status);
            printf("[REAP] Task #%d PID=%ld -> %s (exit=%d, signal=%d)\n",
                   task->task_id, (long)task->pid,
                   get_state_name(task->state), task->exit_status,
                   task->term_signal);
        } else if (pid < 0) {
            perror("[Error] waitpid(WNOHANG)");
        }
    }
}

int terminate_task(int task_id) {
    size_t i;
    update_background_tasks();
    for (i = 0; i < task_count; i++) {
        TaskPCB *task = &tasks[i];
        int status;
        pid_t pid;
        if (task->task_id != task_id) continue;
        if (task->state != TASK_RUNNING) {
            printf("[Info] Task #%d is %s, not running.\n",
                   task_id, get_state_name(task->state));
            return -1;
        }
        if (kill(task->pid, SIGTERM) < 0) {
            perror("[Error] kill(SIGTERM)");
            update_background_tasks();
            return -1;
        }
        printf("[SIGNAL] kill(PID=%ld, SIGTERM) sent by Master PID=%ld\n",
               (long)task->pid, (long)getpid());
        pid = wait_for_pid(task->pid, &status, 0);
        if (pid == task->pid) {
            set_final_state(task, status);
            printf("[REAP] waitpid PID=%ld -> %s (exit=%d, signal=%d)\n",
                   (long)task->pid, get_state_name(task->state),
                   task->exit_status, task->term_signal);
            return 0;
        }
        perror("[Error] waitpid after kill");
        return -1;
    }
    printf("[Error] Task #%d not found.\n", task_id);
    return -1;
}

void display_tasks_table(void) {
    size_t i;
    update_background_tasks();
    puts("\n=======================================================================================================");
    printf("%-4s | %-24s | %-7s | %-7s | %-4s | %-5s | %-7s | %-11s | %-5s\n",
           "ID", "Command", "PID", "PPID", "Mode", "Burst", "Mem(KB)", "State", "Exit");
    puts("-------------------------------------------------------------------------------------------------------");
    if (!task_count) puts("No tasks submitted yet.");
    for (i = 0; i < task_count; i++) {
        const TaskPCB *t = &tasks[i];
        char exit_display[16];
        if (t->state == TASK_RUNNING) snprintf(exit_display, sizeof(exit_display), "-");
        else if (t->term_signal) snprintf(exit_display, sizeof(exit_display), "SIG%d", t->term_signal);
        else snprintf(exit_display, sizeof(exit_display), "%d", t->exit_status);
        printf("%-4d | %-24.24s | %-7ld | %-7ld | %-4s | %-5d | %-7d | %-11s | %-5s\n",
               t->task_id, t->command, (long)t->pid, (long)t->parent_pid,
               t->is_background ? "BG" : "FG", t->burst_time, t->memory_size,
               get_state_name(t->state), exit_display);
    }
    puts("=======================================================================================================");
}

void shutdown_task_manager(void) {
    size_t i;
    struct timespec pause_time = {0, 50000000L}; /* 50 ms */
    int round;

    update_background_tasks();
    for (i = 0; i < task_count; i++) {
        if (tasks[i].state == TASK_RUNNING) {
            printf("[CLEANUP] SIGTERM to task #%d PID=%ld\n",
                   tasks[i].task_id, (long)tasks[i].pid);
            if (kill(tasks[i].pid, SIGTERM) == -1 && errno != ESRCH)
                perror("[CLEANUP] kill(SIGTERM)");
        }
    }
    /* Reap each child without leaving zombies; escalate if SIGTERM is ignored. */
    for (round = 0; round < 20; round++) {
        int any_running = 0;
        update_background_tasks();
        for (i = 0; i < task_count; i++)
            if (tasks[i].state == TASK_RUNNING) any_running = 1;
        if (!any_running) return;
        nanosleep(&pause_time, NULL);
    }
    for (i = 0; i < task_count; i++) {
        int status;
        pid_t result;
        if (tasks[i].state != TASK_RUNNING) continue;
        printf("[CLEANUP] SIGKILL to unresponsive PID=%ld\n", (long)tasks[i].pid);
        if (kill(tasks[i].pid, SIGKILL) == -1 && errno != ESRCH)
            perror("[CLEANUP] kill(SIGKILL)");
        result = wait_for_pid(tasks[i].pid, &status, 0);
        if (result == tasks[i].pid) set_final_state(&tasks[i], status);
    }
}
