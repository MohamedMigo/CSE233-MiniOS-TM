#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include "../include/task.h"

static TaskPCB task_table[MAX_TASKS];
static int task_count = 0;

void init_task_manager(void) {
    task_count = 0;
}

const char* get_state_name(TaskState state) {
    switch (state) {
        case TASK_NEW:        return "NEW";
        case TASK_READY:      return "READY";
        case TASK_RUNNING:    return "RUNNING";
        case TASK_WAITING:    return "WAITING";
        case TASK_COMPLETED:  return "COMPLETED";
        case TASK_TERMINATED: return "TERMINATED";
        default:              return "UNKNOWN";
    }
}

static void parse_command(char *cmd_copy, char **args) {
    int i = 0;
    char *token = strtok(cmd_copy, " \t\n");
    while (token != NULL && i < MAX_ARGS - 1) {
        args[i++] = token;
        token = strtok(NULL, " \t\n");
    }
    args[i] = NULL;
}

int create_and_run_task(const char *cmd_str, int burst_time, int memory_size, int is_background) {
    if (task_count >= MAX_TASKS) {
        printf("[Error] Maximum task limit (%d) reached.\n", MAX_TASKS);
        return -1;
    }

    char cmd_copy[MAX_CMD_LEN];
    char *args[MAX_ARGS];
    strncpy(cmd_copy, cmd_str, MAX_CMD_LEN - 1);
    cmd_copy[MAX_CMD_LEN - 1] = '\0';

    parse_command(cmd_copy, args);
    if (args[0] == NULL) {
        printf("[Error] Empty command.\n");
        return -1;
    }

    int idx = task_count++;
    task_table[idx].task_id = idx + 1;
    strncpy(task_table[idx].command, cmd_str, MAX_CMD_LEN - 1);
    task_table[idx].command[MAX_CMD_LEN - 1] = '\0';
    task_table[idx].burst_time = burst_time;
    task_table[idx].memory_size = memory_size;
    task_table[idx].is_background = is_background;
    task_table[idx].state = TASK_READY;
    task_table[idx].exit_status = -1;

    pid_t pid = fork();

    if (pid < 0) {
        perror("[Error] fork() failed");
        task_table[idx].state = TASK_TERMINATED;
        return -1;
    }

    if (pid == 0) {
        /* Child Process */
        if (execvp(args[0], args) == -1) {
            perror("[Child Error] execvp() failed");
            exit(EXIT_FAILURE);
        }
    } else {
        /* Parent Process (Master Controller) */
        task_table[idx].pid = pid;
        task_table[idx].state = TASK_RUNNING;

        if (!is_background) {
            printf("[FG] Executing Task #%d (PID: %d): %s\n",
                   task_table[idx].task_id, pid, task_table[idx].command);
            int status;
            if (waitpid(pid, &status, 0) > 0) {
                if (WIFEXITED(status)) {
                    task_table[idx].exit_status = WEXITSTATUS(status);
                    task_table[idx].state = (task_table[idx].exit_status == 0) ? TASK_COMPLETED : TASK_TERMINATED;
                } else if (WIFSIGNALED(status)) {
                    task_table[idx].exit_status = WTERMSIG(status);
                    task_table[idx].state = TASK_TERMINATED;
                }
            }
            printf("[FG] Task #%d finished with status: %s (Code: %d)\n",
                   task_table[idx].task_id, get_state_name(task_table[idx].state), task_table[idx].exit_status);
        } else {
            printf("[BG] Started Task #%d in background (PID: %d): %s\n",
                   task_table[idx].task_id, pid, task_table[idx].command);
        }
    }

    return task_table[idx].task_id;
}

void update_background_tasks(void) {
    for (int i = 0; i < task_count; i++) {
        if (task_table[i].state == TASK_RUNNING && task_table[i].is_background) {
            int status;
            pid_t result = waitpid(task_table[i].pid, &status, WNOHANG);
            if (result > 0) {
                if (WIFEXITED(status)) {
                    task_table[i].exit_status = WEXITSTATUS(status);
                    task_table[i].state = (task_table[i].exit_status == 0) ? TASK_COMPLETED : TASK_TERMINATED;
                } else if (WIFSIGNALED(status)) {
                    task_table[i].exit_status = WTERMSIG(status);
                    task_table[i].state = TASK_TERMINATED;
                }
            }
        }
    }
}

int terminate_task(int task_id) {
    update_background_tasks();
    for (int i = 0; i < task_count; i++) {
        if (task_table[i].task_id == task_id) {
            if (task_table[i].state != TASK_RUNNING) {
                printf("[Info] Task #%d is not currently running (State: %s).\n",
                       task_id, get_state_name(task_table[i].state));
                return -1;
            }
            if (kill(task_table[i].pid, SIGTERM) == 0) {
                int status;
                waitpid(task_table[i].pid, &status, 0);
                task_table[i].state = TASK_TERMINATED;
                task_table[i].exit_status = SIGTERM;
                printf("[Success] Task #%d (PID: %d) terminated via SIGTERM.\n",
                       task_id, task_table[i].pid);
                return 0;
            } else {
                perror("[Error] kill() failed");
                return -1;
            }
        }
    }
    printf("[Error] Task #%d not found.\n", task_id);
    return -1;
}

void display_tasks_table(void) {
    update_background_tasks();
    printf("\n=========================================================================================\n");
    printf("%-5s | %-22s | %-8s | %-5s | %-6s | %-8s | %-11s | %-4s\n",
           "ID", "Command", "PID", "Mode", "Burst", "Mem(KB)", "State", "Exit");
    printf("-----------------------------------------------------------------------------------------\n");

    if (task_count == 0) {
        printf("No tasks submitted yet.\n");
    } else {
        for (int i = 0; i < task_count; i++) {
            printf("%-5d | %-22.22s | %-8d | %-5s | %-6d | %-8d | %-11s | %-4d\n",
                   task_table[i].task_id,
                   task_table[i].command,
                   (int)task_table[i].pid,
                   task_table[i].is_background ? "BG" : "FG",
                   task_table[i].burst_time,
                   task_table[i].memory_size,
                   get_state_name(task_table[i].state),
                   task_table[i].exit_status);
        }
    }
    printf("=========================================================================================\n");
}
