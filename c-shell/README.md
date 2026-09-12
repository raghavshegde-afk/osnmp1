# c-shell

A custom C shell project with implemented built-ins:
- `activities`: List background jobs
- `fg`: Bring job to foreground
- `bg`: Send job to background
- `ping`: Send signals to jobs/processes
- `spy`: Inspect process file descriptors (syntax: `spy` or `spy PID`)
- `snoop`: Trace system calls

## Snoop

`snoop` traces and counts system calls executed by a process.
- **Command mode**: `snoop command [args...]`
  Runs the command and prints a syscall report after execution completes.
- **Attach mode**: `snoop -p PID`
  Attaches to an existing process. Traces until the process exits naturally or until you press Ctrl-C.

Limitations:
- Only x86-64 Linux syscalls are supported.
