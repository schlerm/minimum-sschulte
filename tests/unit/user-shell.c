/* Compile with the real user shell/console and a memory-backed ioctl stub. */
#include <assert.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "console.h"
#include "shell.h"
#include "minemu/user_abi.h"

static jmp_buf done;
static const unsigned char *input;
static char output[4096];
static size_t length;
static uint32_t write_limit = 3;
static int32_t write_failure;
static int pause_read;

int32_t ioctl(int fd, uint32_t request, ...) {
    va_list args;
    va_start(args, request);
    void *buffer = va_arg(args, void *);
    uint32_t count = va_arg(args, uint32_t);
    va_end(args);
    if (request == MINEMU_IOCTL_UART_WRITE) {
        assert(fd == MINEMU_FD_UART_OUT);
        assert(count <= MINEMU_IOCTL_MAX_TRANSFER);
        if (write_failure) return write_failure == 1 ? 0 : -1;
        uint32_t copied = count < write_limit ? count : write_limit;
        assert(length + copied < sizeof(output));
        memcpy(output + length, buffer, copied);
        length += copied;
        output[length] = '\0';
        return (int32_t)copied;
    }
    assert(fd == MINEMU_FD_UART_IN && request == MINEMU_IOCTL_UART_READ && count == 1);
    if (!pause_read) {
        pause_read = 1;
        return 0;
    }
    pause_read = 0;
    if (*input == '\0') longjmp(done, 1);
    if (*input == 1) {
        ++input;
        return -1;
    }
    *(unsigned char *)buffer = *input++;
    return 1;
}

static void check(const char *keys, const char *expected) {
    input = (const unsigned char *)keys;
    length = 0;
    output[0] = '\0';
    pause_read = 0;
    if (setjmp(done) == 0) msh_run();
    if (strcmp(output, expected) != 0) {
        fprintf(stderr, "expected [%s], got [%s]\n", expected, output);
        assert(0);
    }
}

int main(void) {
    check("", "msh> ");
    check("\n  \necho\necho   \n", "msh> msh> msh> \nmsh> \nmsh> ");
    check("   echo   worked   fine  \n", "msh> worked fine\nmsh> ");
    check("\b\177echx\bo abx\177c\n", "msh> abc\nmsh> ");
    check("bogus argument\nechoes\n", "msh> command not found: bogus\nmsh> command not found: echoes\nmsh> ");
    check("echo broken\1echo recovered\n", "msh> msh> recovered\nmsh> ");
    check("echo a\rb\n", "msh> a\rb\nmsh> ");

    char keys[MSH_LINE_CAPACITY + 32];
    char expected[MSH_LINE_CAPACITY + 32];
    memcpy(keys, "echo ", 5);
    memset(keys + 5, 'x', MSH_LINE_CAPACITY - 5);
    keys[MSH_LINE_CAPACITY] = '\n';
    keys[MSH_LINE_CAPACITY + 1] = '\0';
    memcpy(expected, "msh> ", 5);
    memset(expected + 5, 'x', MSH_LINE_CAPACITY - 5);
    strcpy(expected + MSH_LINE_CAPACITY, "\nmsh> ");
    check(keys, expected);
    strcpy(keys + MSH_LINE_CAPACITY, "x\b\necho ok\n");
    check(keys, "msh> msh> ok\nmsh> ");

    char bytes[600];
    memset(bytes, 'z', sizeof(bytes));
    length = 0;
    write_limit = 1000;
    assert(msh_write(bytes, sizeof(bytes)));
    assert(length == sizeof(bytes) && memcmp(output, bytes, sizeof(bytes)) == 0);
    write_failure = 1;
    assert(!msh_write(bytes, 1));
    write_failure = 2;
    assert(!msh_write(bytes, 1));
    assert(msh_write(bytes, 0));
    puts("user shell and console tests passed");
}
