# Mini Project 1 — OSN (CS3.301)

## How to Run

### C-Shell
```bash
cd c-shell
make clean && make all
./shell.out
```

### xv6 — Default (Round Robin)
```bash
cd xv6
make clean && make qemu
```

### xv6 — MLFQ Scheduler
```bash
cd xv6
make clean && make qemu SCHEDULER=MLFQ
```

### xv6 — FIFO (FCFS) Scheduler
```bash
cd xv6
make clean && make qemu SCHEDULER=FIFO
```

Inside xv6, run `schedulertest` to test the scheduler. Press `Ctrl+P` for procdump output.

---

## Folder Structure

```
mini-project1/
├── README.md              # This file
├── AI-usage.md            # AI usage disclosure
├── c-shell/
│   ├── Makefile
│   ├── README.md          # C-Shell specific docs (builtins, snoop)
│   ├── Report.pdf
│   ├── include/           # Header files
│   │   ├── builtins.h
│   │   ├── execution.h
│   │   ├── jobs.h
│   │   ├── parser.h
│   │   ├── shell.h
│   │   └── syscalls.h
│   └── src/               # Source files
│       ├── main.c         # Main loop, input parsing, semicolon/pipe/bg dispatch
│       ├── lexer.c        # Tokenizer
│       ├── parser.c       # Command parser
│       ├── execution.c    # fork/exec, pipelines, redirections
│       ├── builtins.c     # hop, reveal, peek, locate, spy, snoop
│       ├── jobs.c         # Job control, SIGCHLD handler, fg/bg/resume
│       └── shell.c        # Shell state init, prompt
└── xv6/
    ├── Makefile
    ├── report.pdf         # MLFQ analysis report
    ├── report_gen/        # Report generation scripts and images
    ├── kernel/
    │   ├── proc.c         # MLFQ scheduler, yield, procdump (modified)
    │   ├── proc.h         # struct proc with MLFQ fields (modified)
    │   ├── trap.c         # Timer interrupt handler
    │   └── ...
    └── user/
        ├── schedulertest.c  # MLFQ test program
        └── ...
```

---

## Design Choices

### C-Shell

- **Modular architecture**: Separated into lexer → parser → execution pipeline. Each stage is independently testable.
- **Job control**: Uses process groups (`setpgid`) and terminal control (`tcsetpgrp`) for proper fg/bg management. SIGCHLD handler with `waitpid(-1, ..., WNOHANG)` reaps background jobs.
- **Signal safety**: `sigprocmask` blocks SIGCHLD during foreground `waitpid` to prevent race conditions. EINTR retry loops on `waitpid` calls.
- **Pipeline implementation**: Creates all pipes before forking, each child closes unused ends. Pipeline command-not-found does NOT break sequential (`;`) execution per spec C4 req 8.
- **Redirection**: Validates all redirect targets before forking (checks file existence for `<`, writability for `>`). Supports multiple input/output redirections per command.
- **`peek -n` multi-file**: Line numbering continues across files (e.g., `peek -n file1 file2` numbers lines 1-N continuously).
- **`snoop`**: Uses `ptrace` to trace syscalls. Supports both command mode and attach-to-PID mode.

### xv6 MLFQ

- **4 priority queues** (0 highest → 3 lowest) with time slices {1, 4, 8, 16} ticks.
- **Preemption at tick boundaries**: `yield()` is called every timer tick. The scheduler then picks the highest-priority runnable process. This naturally handles preemption — if a higher-priority process arrives, it gets selected on the next tick.
- **Demotion**: `yield()` increments `ticks_used`; when it reaches the queue's quantum, the process is moved to the next lower queue.
- **Priority boost**: Every 48 ticks, all processes are moved to queue 0 and `ticks_used` is reset to prevent starvation.
- **Voluntary yield (sleep)**: Preserves the process's current queue level — sleeping processes are not penalized.
- **`procdump`**: Prints `q=`, `ticks_used=`, and `ticks_since_boost=` for each process.

### xv6 FIFO (FCFS)

- **Non-preemptive**: Selects the RUNNABLE process with the lowest PID (earliest created), which approximates first-come-first-served arrival order.
- **Runs to completion**: Once scheduled, a process runs until it voluntarily yields (e.g., sleeps, exits, or performs I/O). Timer interrupts still call `yield()`, but the scheduler will re-select the same process if it's still the lowest-PID runnable process.
- **Simple selection**: Scans the entire process table each scheduling decision to find the minimum PID.

---

## Assumptions

- `hop` requires an absolute path to reach directories outside the current working directory directly.
- `reveal` sorts entries alphabetically (default `scandir` ordering).
- `peek -rn` preserves original line numbers in reverse (e.g., a 3-line file prints lines 3, 2, 1).
- Background processes receive SIGHUP when the shell exits.
- `snoop` only supports x86-64 Linux syscalls.
- xv6 MLFQ: 1 timer tick = 1 time slice unit. Queue 0 effectively behaves as 1-tick round-robin.

---

## Key Changes to xv6

### `kernel/proc.h`
- Added `queue_level`, `ticks_used` fields to `struct proc` for MLFQ tracking.
- Added `first_run_time`, `rtime`, `has_started` for scheduler test metrics.

### `kernel/proc.c`
- **`allocproc()`**: Initializes MLFQ fields (`queue_level=0`, `ticks_used=0`).
- **`scheduler()`**: Three scheduling policies compiled via `#ifdef`:
  - **RR**: Default round-robin, cycles through process table.
  - **MLFQ**: 4-level priority queue scheduling with demotion and periodic boost.
  - **FIFO**: Picks the lowest-PID runnable process (first-come-first-served).
- **`yield()`**: Increments `ticks_used`, demotes process when quantum exhausted.
- **`procdump()`**: Prints `q=`, `ticks_used=`, `ticks_since_boost=` per process.
- **`mlfq_boost()`**: Moves all processes to queue 0, resets `ticks_used`.

### `kernel/trap.c`
- Timer interrupt calls `yield()` on every tick for both user and kernel traps.

### `user/schedulertest.c`
- Test program that forks multiple children with varying workloads to demonstrate MLFQ behavior. Reports creation time, first-run time, end time, runtime, turnaround, response, and waiting times.

### `Makefile`
- Added `SCHEDULER` variable support: `make qemu SCHEDULER=MLFQ` or `SCHEDULER=FIFO`. Default is `SCHEDULER=RR` (round-robin).
