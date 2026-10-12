/* User-mode adaptation of kernel/src/core/shell.c. */
#include "console.h"
#include "shell.h"

static void output(const char *text) {
    if (!msh_puts(text)) {
        /* Output failure is fatal; remain in user mode without retrying I/O. */
        for (;;) {}
    }
}

static void execute(char *line) {
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

    if (command_length == 4 && command[0] == 'e' && command[1] == 'c'
        && command[2] == 'h' && command[3] == 'o') {
        /* Match HW2's editing fixture: one space between arguments, no trailing
         * spaces. Compact in place; the destination never overtakes the source.
         */
        char *arguments = line;
        char *destination = arguments;
        while (*line != '\0') {
            while (*line == ' ') {
                ++line;
            }
            if (*line == '\0') {
                break;
            }
            if (destination != arguments) {
                *destination++ = ' ';
            }
            while (*line != '\0' && *line != ' ') {
                *destination++ = *line++;
            }
        }
        *destination = '\0';
        output(arguments);
    } else {
        output("command not found: ");
        output(command);
    }
    output("\n");
}

void msh_run(void) {
    char line[MSH_LINE_CAPACITY + 1];
    size_t length = 0;
    bool overlong = false;
    output("msh> ");

    for (;;) {
        int byte = msh_getc();
        if (byte == MSH_NO_INPUT) {
            continue;
        }
        if (byte == MSH_INPUT_ERROR) {
            /* The driver's loss marker follows discard-through-newline. */
            length = 0;
            overlong = false;
            output("msh> ");
        } else if (byte == '\n') {
            if (!overlong) {
                line[length] = '\0';
                execute(line);
            }
            length = 0;
            overlong = false;
            output("msh> ");
        } else if (overlong) {
            /* Discard through LF, never executing a truncated command. */
            continue;
        } else if (byte == 0x08 || byte == 0x7f) {
            if (length != 0) {
                --length;
            }
        } else if (byte != 0) {
            if (length == MSH_LINE_CAPACITY) {
                overlong = true;
            } else {
                line[length++] = (char)byte;
            }
        }
    }
}
