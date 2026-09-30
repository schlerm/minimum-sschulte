/* Host-side shell/formatter tests. UART input/output is replaced by memory. */
#include <assert.h>
#include <limits.h>
#include <setjmp.h>
#include <stdio.h>
#include <string.h>

#include "minemu/console.h"
#include "minemu/shell.h"
#include "minemu/uart.h"

static jmp_buf finished;
static const unsigned char *input;
static char output[4096];
static size_t output_length;
static int inject_overflow;

int uart_getc(void) {
    if (inject_overflow) {
        inject_overflow = 0;
        return UART_INPUT_OVERFLOW;
    }
    if (*input == '\0') {
        longjmp(finished, 1);
    }
    return *input++;
}

void uart_putc(char c) {
    assert(output_length + 1 < sizeof(output));
    output[output_length++] = c;
    output[output_length] = '\0';
}

void uart_puts(const char *text) {
    while (*text != '\0') {
        uart_putc(*text++);
    }
}

static void check(const char *keys, const char *expected) {
    input = (const unsigned char *)keys;
    output_length = 0;
    output[0] = '\0';
    if (setjmp(finished) == 0) {
        shell_run();
    }
    if (strcmp(output, expected) != 0) {
        fprintf(stderr, "expected: [%s]\nactual: [%s]\n", expected, output);
        assert(0);
    }
}

int main(void) {
    check("", "msh> ");
    check("\n   \necho\necho   \n", "msh> msh> msh> \nmsh> \nmsh> ");
    check("   echo   hello  you\n", "msh> hello  you\nmsh> ");
    check("mystery extra\nechoes\n", "msh> command not found: mystery\nmsh> command not found: echoes\nmsh> ");
    check("\b\177echo abz\b\177cd\n", "msh> acd\nmsh> ");
    check("echo 123456789012345\n", "msh> 123456789012345\nmsh> ");
    check("echo 1234567890123456\b\necho ok\n", "msh> line too long\nmsh> ok\nmsh> ");
    check("echo a\rb\n", "msh> a\rb\nmsh> ");
    inject_overflow = 1;
    check("echo recovered\n", "msh> input overflow\nmsh> recovered\nmsh> ");

    char expected[256];
    output_length = 0;
    output[0] = '\0';
    console_printf("%s %c %u %d %x %%", "test", 'A', UINT_MAX, INT_MIN, UINT_MAX);
    snprintf(expected, sizeof(expected), "%s %c %u %d %x %%", "test", 'A', UINT_MAX, INT_MIN, UINT_MAX);
    assert(strcmp(output, expected) == 0);
    puts("shell and console tests passed");
    return 0;
}
