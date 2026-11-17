#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "ui.h"

static int use_color = 0;

#define C_CYAN   "\033[38;2;56;189;248m"
#define C_PURPLE "\033[38;2;167;139;250m"
#define C_GREEN  "\033[38;2;74;222;128m"
#define C_AMBER  "\033[38;2;251;191;36m"
#define C_RED    "\033[38;2;248;113;113m"
#define C_MUTED  "\033[38;2;148;163;184m"
#define C_BOLD   "\033[1m"
#define C_RESET  "\033[0m"

static const char *color(const char *value) { return use_color ? value : ""; }

void ui_init(void) {
    const char *term = getenv("TERM");
    const char *no_color = getenv("NO_COLOR");
    use_color = isatty(STDOUT_FILENO) && term && strcmp(term, "dumb") != 0 &&
                !(no_color && *no_color);
}

const char *ui_reset_color(void) { return color(C_RESET); }

const char *ui_state_color(TaskState state) {
    switch (state) {
        case TASK_RUNNING: return color(C_GREEN);
        case TASK_COMPLETED: return color(C_CYAN);
        case TASK_TERMINATED: return color(C_RED);
        case TASK_WAITING: return color(C_AMBER);
        case TASK_READY: return color(C_PURPLE);
        default: return color(C_MUTED);
    }
}

void ui_banner(pid_t pid, pid_t ppid) {
    printf("\n%s", color(C_CYAN));
    puts("+--------------------------------------------------------------+");
    printf("|%s  MINI OS  /  TASK MANAGER%s                         %sv2.0 C11%s  |\n",
           color(C_BOLD), color(C_CYAN), color(C_PURPLE), color(C_CYAN));
    puts("+--------------------------------------------------------------+");
    printf("%s  CSE233 - Group 04  |  Linux Process Control  |  Phase 01%s\n",
           color(C_MUTED), color(C_RESET));
    printf("  Master PID: %ld    Parent PID: %ld\n", (long)pid, (long)ppid);
}

void ui_menu(const TaskStats *stats) {
    printf("\n%s+-------------------------- LIVE OVERVIEW ---------------------+%s\n",
           color(C_CYAN), color(C_RESET));
    printf("  %sTOTAL %-3zu%s     %sRUNNING %-3zu%s     %sCOMPLETED %-3zu%s     %sSTOPPED %-3zu%s\n",
           color(C_PURPLE), stats->total, color(C_RESET),
           color(C_GREEN), stats->running, color(C_RESET),
           color(C_CYAN), stats->completed, color(C_RESET),
           color(C_RED), stats->terminated, color(C_RESET));
    printf("%s+---------------------------- ACTIONS ------------------------+%s\n",
           color(C_CYAN), color(C_RESET));
    printf("  %s[1]%s Run foreground task          %s[2]%s Run background task\n",
           color(C_CYAN), color(C_RESET), color(C_CYAN), color(C_RESET));
    printf("  %s[3]%s View process table          %s[4]%s Terminate task (SIGTERM)\n",
           color(C_CYAN), color(C_RESET), color(C_CYAN), color(C_RESET));
    printf("  %s[5]%s Exit and clean up           %s[6]%s Live monitor (timed)\n",
           color(C_CYAN), color(C_RESET), color(C_CYAN), color(C_RESET));
    printf("  %s[7]%s System-call inspector       %sC11 / POSIX%s\n",
           color(C_CYAN), color(C_RESET), color(C_MUTED), color(C_RESET));
    printf("%s+--------------------------------------------------------------+%s\n",
           color(C_CYAN), color(C_RESET));
}

void ui_section(const char *title) {
    printf("\n%s%s>> %s%s\n", color(C_BOLD), color(C_CYAN), title, color(C_RESET));
}

void ui_system_calls_guide(void) {
    ui_section("SYSTEM CALL INSPECTOR - PHASE 1");
    puts("  fork()             Create a new cooperating child process.");
    puts("  getpid()/getppid() Display the parent-child PID relationship.");
    puts("  execvp()           Replace the child with the requested command.");
    puts("  waitpid(..., 0)    Wait for a foreground task and collect its status.");
    puts("  waitpid(WNOHANG)   Check background tasks without blocking the CLI.");
    puts("  kill(SIGTERM)      Request termination of a running child.");
    puts("  WIFEXITED/...      Distinguish exit codes from signal termination.");
    puts("  Note: memory size and burst time are metadata in Phase 1.");
}

void ui_watch_notice(int seconds) {
    ui_section("LIVE PROCESS MONITOR");
    printf("  Capturing %d snapshot(s), approximately 1 second apart.\n", seconds);
    puts("  Each refresh polls child completion using waitpid(WNOHANG).");
    puts("  This is a timed console monitor, not a background monitoring thread.");
}
