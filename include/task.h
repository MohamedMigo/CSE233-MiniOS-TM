#ifndef TASK_H
#define TASK_H

#include <sys/types.h>

#define MAX_TASKS 50
#define MAX_CMD_LEN 256
#define MAX_ARGS 32

typedef enum {
    TASK_NEW,
    TASK_READY,
    TASK_RUNNING,
    TASK_WAITING,
    TASK_COMPLETED,
    TASK_TERMINATED
} TaskState;

typedef struct {
    int task_id;
    char command[MAX_CMD_LEN];
    pid_t pid;
    TaskState state;
    int burst_time;
    int memory_size;     // in KB
    int is_background;   // 0 = Foreground, 1 = Background
    int exit_status;
} TaskPCB;

void init_task_manager(void);
int create_and_run_task(const char *cmd_str, int burst_time, int memory_size, int is_background);
void update_background_tasks(void);
int terminate_task(int task_id);
void display_tasks_table(void);
const char* get_state_name(TaskState state);

#endif
