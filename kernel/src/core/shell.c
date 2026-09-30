#include <stddef.h>

#include "minemu/console.h"
#include "minemu/shell.h"
#include "minemu/uart.h"

#define LINE_CAPACITY 20u

static void shell_execute(char *line) {
    while (*line == ' ') {
        ++line;
    }
    if (*line == '\0') {
        return;
    }

    char *command = line;
    while (*line != '\0' && *line != ' ') {
        ++line;
    }
    size_t command_length = (size_t)(line - command);
    if (*line == ' ') {
        *line++ = '\0';
    }
    while (*line == ' ') {
        ++line;
    }

    if (command_length == 4 && command[0] == 'e' && command[1] == 'c'
        && command[2] == 'h' && command[3] == 'o') {
        console_printf("%s\n", line);
    } else {
        console_printf("command not found: %s\n", command);
    }
}

void shell_run(void) {
    char line[LINE_CAPACITY + 1];
    size_t length = 0;
    int overlong = 0;
    console_printf("msh> ");

    for (;;) {
        int byte = uart_getc();
        if (byte == UART_NO_INPUT) {
            continue;
        }
        if (byte == UART_INPUT_OVERFLOW) {
            /* The driver has already discarded through a newline. */
            length = 0;
            overlong = 0;
            console_printf("input overflow\nmsh> ");
        } else if (byte == '\n') {
            if (overlong) {
                console_printf("line too long\n");
            } else {
                line[length] = '\0';
                shell_execute(line);
            }
            length = 0;
            overlong = 0;
            console_printf("msh> ");
        } else if (overlong) {
            /* Reject the whole overlong line; never execute a truncated command. */
            continue;
        } else if (byte == 0x08 || byte == 0x7f) {
            if (length != 0) {
                --length;
            }
        } else if (byte != 0) {
            /* Ignore NUL because commands are represented as C strings. */
            if (length == LINE_CAPACITY) {
                overlong = 1;
            } else {
                line[length++] = (char)byte;
            }
        }
    }
}
