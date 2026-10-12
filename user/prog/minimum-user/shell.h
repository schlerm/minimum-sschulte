#ifndef MSH_SHELL_H
#define MSH_SHELL_H

/* HW2 fixtures accept >20-byte lines and reject their 136-byte test line. */
#define MSH_LINE_CAPACITY 128u

void msh_run(void) __attribute__((noreturn));

#endif
