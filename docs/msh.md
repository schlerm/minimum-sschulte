# User-mode msh

The kernel loads the existing `minimum-user` module. Its entry function now calls
`msh_run`, which owns line editing and command interpretation in user mode.
There are no MMIO operations or privileged instructions in the shell.

## Files and control flow

- `user/prog/minimum-user/main.c` enters the shell.
- `console.c` implements `msh_write`, `msh_puts`, and `msh_getc` through the shared
  user `ioctl` wrapper. Writes split transfers at 256 bytes, handle partial
  progress, and fail on errors or zero progress. Read requests one byte and
  distinguishes no input from a driver error.
- `shell.c` collects a line, edits it, executes it, and prints the next prompt.
  Output errors stop the user program in an idle loop instead of repeatedly
  attempting a failed syscall.
- `shell.h` sets the line capacity to 128 bytes, plus a separate NUL terminator.

The existing kernel still handles UART interrupts, buffers incoming bytes,
validates syscall pointers, and touches device registers. Input reads are
nonblocking, so the user shell retries when no input is ready. It busy-waits;
there is no scheduler or sleeping-input facility yet.

## Input and commands

Only LF terminates a line. Both 0x08 and 0x7f remove the previous byte; backspace
on an empty line does nothing. NUL bytes are ignored. Input is not echoed by the
shell because the emulator TUI already provides local echo.

Once a line exceeds 128 bytes, the remainder (including backspaces) is discarded
through the next LF. A fresh prompt appears and the truncated command is never
executed. A UART overflow error likewise abandons the partial line; the kernel
has already resynchronized through a newline before reporting its loss marker.

Leading ASCII spaces are ignored. Empty/all-space lines just produce another
prompt. `echo` prints its arguments separated by one ASCII space, with trailing
spaces removed, followed by LF. Bare `echo` prints LF. Unknown commands report
`command not found: COMMAND`, using only the first word. There is no quoting,
expansion, or special treatment of tabs or CR.

## Assignment/fixture differences

The HW2 fixtures conflict with the earlier webpage's 20-byte limit and the old
kernel shell's whitespace behavior. The implementation follows the checked-in
fixtures without changing them:

- `msh-echo` requires a 25-byte command to succeed.
- `msh-editing` expects repeated argument spaces collapsed and trailing spaces
  trimmed.
- `msh-overlong` expects a 136-byte line to be discarded with only a new prompt.

A 128-byte capacity satisfies these cases. This is an explicit interpretation of
the conflicting materials; use instructor guidance if a strict 20-byte policy is
required instead.

## Startup and testing

The existing user build automatically includes the new C files, and the module
name/image manifest stay the same. `kernel/linker/kernel.ld` reserves the user
page pool outside boot-loaded segments to avoid clearing 64 KiB of unused pool
storage during boot. `user_page_index` still zeros every allocated page before
user access. The loader retains its 16-page capacity.

Inside the course container:

```sh
make
just test-all hw2
```

Host-side behavior tests, with a mocked ioctl boundary:

```sh
cc -std=c11 -Wall -Wextra -Werror -Ikernel/include -Iuser/prog/minimum-user \
  tests/unit/user-shell.c user/prog/minimum-user/shell.c \
  user/prog/minimum-user/console.c -o /tmp/minimum-user-shell-test
/tmp/minimum-user-shell-test
```

These cover empty input, spacing, both backspaces, unknown commands, the exact
capacity, overlong-line recovery, input-loss recovery, partial writes, chunking,
and failed/no-progress writes. The old kernel shell source remains as an HW1
reference; it is not linked into the running kernel.
