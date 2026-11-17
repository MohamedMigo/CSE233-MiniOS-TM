# MiniOS-TM v2.0 — Modern C11 CLI | Phase 1: Processes & System Calls

**CSE233 Operating Systems | Galala University | Fall 2026**  
**Approved Project:** Category 2 — Mini OS Task Manager  
**Project:** MiniOS-TM: A Concurrent Process and Thread-Based Task Management System in Linux  
**Group:** 04

## What's new in v2.0: Modern CLI Edition

This is still the **approved MiniOS-TM** Linux C11 project, not a separate demonstration.
The original Phase 1 process calls, foreground/background behavior, TaskPCB, and five original menu choices are preserved.

- **ANSI true-color interface** on supported interactive Linux terminals, automatically colorless when output is redirected or the NO_COLOR environment variable is set.
- **Live overview** with accurate total, RUNNING, COMPLETED and TERMINATED counts derived from the TaskPCB table.
- **Timed monitor** (menu 6): 1-15 snapshots of process states; uses waitpid(WNOHANG) on each refresh, not an asynchronous monitoring thread.
- **System-call inspector** (menu 7) describes fork, execvp, getpid/getppid, waitpid and kill.
- Separate src/ui.c and include/ui.h to keep presentation independent of the OS process-management module.
- Run ./minios_tm --version to identify the version; run make test for seven Linux regression tests.

New source files: src/ui.c and include/ui.h. Original student screenshots under evidence/student_screenshots/ show the earlier v1 interface; generated Linux v2 evidence is included in the submitted ZIP.


## 1. Team

| Name | Student ID | Responsibility in approved proposal |
|---|---|---|
| Mohamed Ashraf (Leader) | 224109680 | Process management, Linux system calls, integration |
| Nour Eldin Mahmoud | 224130194 | IPC, worker pool, synchronization (next phase) |
| Kareem Mohamed | 224114103 | CPU scheduling and performance (later phase) |
| Adham Amr | 226102481 | Memory tracking, page replacement, file I/O (later phase) |

All members should understand the **Phase 1** code and be able to explain it.

## 2. Purpose and connection to the approved project

MiniOS-TM is a **Linux user-space task manager**, not a standalone example of `fork()`.
The **Master Controller** provides an interactive command-line interface (CLI), accepts
Linux commands as tasks, stores their control data in a `TaskPCB` table, creates child
processes for each task, tracks their states, and supports foreground execution,
background execution, and termination.

This is the **process-management foundation** of the approved four-phase MiniOS-TM
project. In later phases, the same application will be extended with IPC, POSIX threads,
scheduling algorithms, memory management, and file persistence.

## 3. Requirements demonstrated in Phase 1

| Requirement | Meaningful implementation |
|---|---|
| Two cooperating processes | The **Master Controller** receives and monitors tasks; each **Child Worker Process** executes a real Linux command. |
| `fork()` | Creates a new Linux child process for a submitted task. |
| `execvp()` | Replaces the child's program with the selected Linux command, such as `ls`, `echo`, or `sleep`. |
| `getpid()`, `getppid()` | Shows the controller's PID and parent PID, and each child's PID and parent PID. |
| `waitpid(pid, ..., 0)` | Blocks the Master Controller until a foreground task finishes and collects its exit status. |
| `waitpid(pid, ..., WNOHANG)` | Checks whether a background child has finished **without blocking** the CLI. Reaps finished children. |
| `kill(pid, SIGTERM)` | Requests termination of a running task; the controller then calls `waitpid()` to reap it. |
| `WIFEXITED`, `WEXITSTATUS`, `WIFSIGNALED`, `WTERMSIG` | Distinguishes normal completion, nonzero exit, and signal-based termination. |
| Zombie prevention | Finished children are reaped with `waitpid()` during menu refresh and shutdown; active children are signaled and reaped on exit. |

**Note:** Background reaping happens when the CLI next refreshes or performs another
menu action. A short-lived zombie is possible while the controller is waiting for
keyboard input; it is reaped at the next refresh/shutdown. This Phase 1 program does
not claim to provide asynchronous `SIGCHLD`-driven reaping.

## 4. Files

```text
MiniOS-TM_Phase1_Submission/
├── include/task.h              # TaskPCB and process state declarations
├── src/main.c                  # Interactive Master Controller CLI
├── src/process_mgr.c           # fork, exec, waitpid, PID, kill, cleanup
├── src/ui.c                    # Modern UI, terminal color and system-call help
├── include/ui.h                # UI declarations
├── Makefile                    # Build/run tests on Linux
├── README.md                   # This documentation
├── DEMO_GUIDE_AR.md            # Step-by-step live demonstration and discussion
├── tests/test_phase1.sh        # Seven automated Linux test scenarios
└── evidence/                   # Genuine Linux terminal captures and test logs
```

## 5. Requirements and compilation

**Environment:** Native Ubuntu Linux or Ubuntu on Windows Subsystem for Linux (WSL2).
**Compiler:** `gcc`, `make`; C11 with POSIX system calls. No special libraries or
`sudo` privileges are required to run the program.

```bash
sudo apt update
sudo apt install build-essential
cd MiniOS-TM_Phase1_Submission
make
./minios_tm
```

To run the automated Linux checks:

```bash
make test
```

To remove the generated executable:

```bash
make clean
```

**Do not use a Windows `.exe` compiler** to demonstrate these Linux/POSIX process calls.
Run the demo in Ubuntu/WSL2.

## 6. Live demo: suggested inputs

### A. Foreground process (`fork`, `execvp`, `getpid`, `getppid`, blocking `waitpid`)

```text
Choose: 1
Linux command: echo "MiniOS Phase1 Verification"
Estimated burst time: 1
Estimated memory: 64
Choose: 3 (display table)
```

Expected: the parent displays its PID, the child prints its PID and PPID, the
command output appears, and Task #1 becomes **COMPLETED** with exit code **0**.

### B. Concurrent background task and nonblocking monitoring

```text
Choose: 2
Linux command: sleep 30
Estimated burst time: 30
Estimated memory: 256
Choose: 3 (display while it is still running)
```

Expected: Task #2 has its own PID and is **RUNNING**, while the Master Controller
remains responsive. The table shows the child's parent PID.

### C. Termination and proper reaping

```text
Choose: 4
Task ID: 2
Choose: 3 (display updated table)
```

Expected: `kill(PID, SIGTERM)` followed by `waitpid`; Task #2 shows
**TERMINATED** and **SIG15**, with no unreaped terminated child remaining.

### D. Observe background *normal* completion

```text
Choose: 2
Linux command: sleep 2
Estimated burst time: 2
Estimated memory: 128
(wait about 3 seconds)
Choose: 3
```

Expected: `[REAP]` log and **COMPLETED** with exit code **0**.

### E. Observe failed execution

```text
Choose: 1
Linux command: command_that_does_not_exist_12345
Estimated burst time: 0
Estimated memory: 64
```

Expected: `execvp failed`, the child exits **127**, and the task is **TERMINATED**.

Choose **6** to monitor task states or **7** to inspect the system calls. Choose **5** to exit. All still-running child tasks receive SIGTERM (or SIGKILL
if unresponsive), and the Master Controller reaps them before exiting.

## 7. Task states and roles

- **Master Controller (parent):** Reads tasks from the CLI, stores `TaskPCB` records,
  launches child processes, polls task completion and displays task states.
- **Child Worker (child):** Prints `getpid()` and `getppid()`, executes the requested
  Linux command using `execvp()`, and returns its exit status to the parent.
- **NEW/READY:** Declared states for the extensible TaskPCB model; Phase 1
  transitions tasks to `READY` immediately before creation and to `RUNNING`
  after a successful `fork()`.
- **RUNNING:** A child has been launched and has not yet been reaped.
- **COMPLETED:** The child exited normally with code 0.
- **TERMINATED:** The child exited with a nonzero code, failed to execute, or
  was terminated by a signal.
- **WAITING:** Reserved for later task scheduling and synchronization phases.

`burst_time` and `memory_size` are **user-entered metadata only** in Phase 1.
The program does not yet allocate task memory or implement real CPU scheduling.
Those features belong to later phases described in the approved proposal.

## 8. Process workflow

```text
User --> Master Controller (CLI) --> TaskPCB record
                   |
                   +--> fork() --> Child PID
                                       |
                                       +--> getpid()/getppid()
                                       +--> execvp(command, args)

Foreground: Master ----waitpid(pid, 0)----> exit status + final task state
Background: Master ----waitpid(WNOHANG)---> updates completed child later
Cancellation: Master ----kill(SIGTERM)----> child ----waitpid()----> reaped
Shutdown: Master ----SIGTERM/SIGKILL-----> remaining children reaped
```

## 9. Evidence and testing

The `evidence/` folder contains **actual Ubuntu/Linux xterm screenshots** taken
from a run of this C program under a virtual X display, plus saved console output
from automated Linux tests. PID values change between runs.

| Evidence | Shows |
|---|---|
| `01_foreground.png` | Master/child PID, PPID, `execvp`, blocking `waitpid`, completion |
| `02_background_running.png` | Multiple cooperating processes, responsive CLI, RUNNING task |
| `03_termination.png` | `kill(SIGTERM)`, wait/reap and TERMINATED task |
| `04_background_completed.png` | Nonblocking `waitpid(WNOHANG)` and final COMPLETED state |
| `linux_tests.txt` | Output of `make test` executed on Linux |

For the class demonstration, rerun on **your own Ubuntu/WSL2 machine**, take
screenshots of the live terminal and confirm the same behavior. Console-generated
screenshots in this submission are genuine test-run evidence, but they are not a
substitute for your group's live explanation.

## 10. Scope limits / design choices

- Commands are executed **directly** through `execvp()` (no shell by default).
  Basic quotes and escaped spaces are accepted. Shell operators (`|`, `>`, `&&`)
  require an explicit shell command, e.g. `sh -c "echo hi"`.
- Phase 1 tracks up to `MAX_TASKS` (50) tasks in a fixed-size table.
- Foreground tasks block the CLI until they exit; use background mode for
  long-running commands that need cancellation from the menu.
- This is a learning project to demonstrate Linux process control, not a
  privileged task manager or full operating system.

---
**Repository:** https://github.com/MohamedMigo/CSE233-MiniOS-TM  
**Original proposal:** `CSE233_Group04_ProjectProposal.pdf` (approved Category 2).
