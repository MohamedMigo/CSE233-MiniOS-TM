#ifndef MINIOS_UI_H
#define MINIOS_UI_H

#include <sys/types.h>
#include "task.h"

/* Presentation-only functions. No OS process state is stored here. */
void ui_init(void);
void ui_banner(pid_t controller_pid, pid_t controller_ppid);
void ui_menu(const TaskStats *stats);
void ui_section(const char *title);
void ui_system_calls_guide(void);
void ui_watch_notice(int seconds);
const char *ui_state_color(TaskState state);
const char *ui_reset_color(void);

#endif
