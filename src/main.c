#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/task.h"

static void clear_input_buffer(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

int main(void) {
    int choice;
    char cmd[MAX_CMD_LEN];
    int burst, mem, task_id;

    init_task_manager();

    while (1) {
        update_background_tasks();
        printf("\n--- MiniOS-TM (Phase 1: Process & System Calls Manager) ---\n");
        printf("1. Submit & Run Foreground Task (Blocking)\n");
        printf("2. Submit & Run Background Task (Non-Blocking)\n");
        printf("3. Display All Tasks & Process States\n");
        printf("4. Terminate a Running Task (kill)\n");
        printf("5. Exit\n");
        printf("Select an option: ");

        if (scanf("%d", &choice) != 1) {
            printf("[Error] Invalid input. Please enter a number.\n");
            clear_input_buffer();
            continue;
        }
        clear_input_buffer();

        switch (choice) {
            case 1:
            case 2:
                printf("Enter Linux command (e.g., ls -l, sleep 15, ps): ");
                if (fgets(cmd, sizeof(cmd), stdin) == NULL) break;
                cmd[strcspn(cmd, "\n")] = '\0';

                printf("Enter estimated Burst Time (seconds): ");
                if (scanf("%d", &burst) != 1 || burst < 0) {
                    printf("[Error] Invalid burst time.\n");
                    clear_input_buffer();
                    break;
                }

                printf("Enter Memory Footprint (KB): ");
                if (scanf("%d", &mem) != 1 || mem <= 0) {
                    printf("[Error] Invalid memory size.\n");
                    clear_input_buffer();
                    break;
                }
                clear_input_buffer();

                create_and_run_task(cmd, burst, mem, (choice == 2) ? 1 : 0);
                break;

            case 3:
                display_tasks_table();
                break;

            case 4:
                display_tasks_table();
                printf("Enter Task ID to terminate: ");
                if (scanf("%d", &task_id) != 1) {
                    printf("[Error] Invalid Task ID.\n");
                    clear_input_buffer();
                    break;
                }
                clear_input_buffer();
                terminate_task(task_id);
                break;

            case 5:
                printf("Exiting MiniOS-TM Phase 1. Goodbye!\n");
                return 0;

            default:
                printf("[Error] Invalid choice. Select 1-5.\n");
        }
    }
    return 0;
}
