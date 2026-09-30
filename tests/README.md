# Student Tests

`hw1/` contains the initial public black-box tests. They run the packaged student
image and do not link against internal kernel functions. Later homework tests
use sibling directories such as `hw2/` without adding new Just recipes.

Run all released tests from the repository root:

```sh
just test-all hw1
```

Run one public test by homework and manifest name, for example:

```sh
just test hw1 echo
```

Students may add manifests or other tests under `tests/` using any reasonable
layout. Additional student-authored tests are encouraged but are not required
for Assignment 1.

These tests are deliberately small behavioral checks. In particular, the echo
case does not attempt to distinguish command output from every possible input
echo implementation; required command dispatch remains subject to source
review.

## Kernel shell behavior

UART0 input uses a 256-entry ring with 255 usable entries. Main-code reads
mask IRQs and restore the previous mask; the IRQ handler drains the hardware
FIFO before the dispatcher acknowledges the interrupt. On queue overflow,
queued input is abandoned and bytes are discarded through the next newline.
An overflow marker tells the shell to abandon its partial line and print
`input overflow`, then a new prompt.

Command lines accept 20 bytes before the newline, plus a separate NUL
terminator. Backspace removes a byte until the line exceeds the limit. Once
overlong, the entire line is discarded through newline, including backspaces,
and `line too long` is printed. NUL input is ignored; only LF ends a line.
The TUI handles local echo, so the kernel does not echo input characters.

Run the host-side shell and formatter checks (no emulator required):

```sh
cc -std=c11 -Wall -Wextra -Werror -Ikernel/include \
  tests/unit/shell.c kernel/src/core/shell.c kernel/src/core/console.c \
  -o /tmp/minimum-shell-test
/tmp/minimum-shell-test
```
