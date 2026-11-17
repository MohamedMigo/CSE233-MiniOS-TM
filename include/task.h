#ifndef MINIOS_TASK_H
#define MINIOS_TASK_H

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

/* Phase 1 process control block. Later phases can extend this structure. */
typedef struct {
    int task_id;
    char command[MAX_CMD_LEN];
    pid_t pid;
    pid_t parent_pid;
    TaskState state;
    int burst_time;          /* User-estimated seconds, not CPU time */
    int memory_size;         /* User-estimated KB, not allocated yet */
    int is_background;       /* 0 = foreground, 1 = background */
    int exit_status;         /* Unix exit code; -1 while running */
    int term_signal;         /* Signal number when killed; 0 otherwise */
} TaskPCB;

void init_task_manager(void);
int create_and_run_task(const char *cmd_str, int burst_time, int memory_size,
                        int is_background);
void update_background_tasks(void);
int terminate_task(int task_id);
void display_tasks_table(void);
void shutdown_task_manager(void);
const char *get_state_name(TaskState state);

#endif
