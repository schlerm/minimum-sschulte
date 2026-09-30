#include <limits.h>
#include <stdarg.h>

#include "minemu/console.h"
#include "minemu/uart.h"

static void console_unsigned(unsigned int value, unsigned int base) {
    const char digits[] = "0123456789abcdef";
    char buffer[sizeof(value) * CHAR_BIT];
    unsigned int length = 0;

    /* Collect digits in reverse order, including one digit for zero. */
    do {
        buffer[length++] = digits[value % base];
        value /= base;
    } while (value != 0);

    while (length != 0) {
        uart_putc(buffer[--length]);
    }
}

void console_printf(const char *format, ...) {
    va_list args;
    va_start(args, format);

    while (*format != '\0') {
        if (*format != '%') {
            uart_putc(*format++);
            continue;
        }

        ++format;
        if (*format == '\0') {
            uart_putc('%');
            break;
        }

        switch (*format) {
        case 's': {
            const char *text = va_arg(args, const char *);
            uart_puts(text != 0 ? text : "(null)");
            break;
        }
        case 'c':
            /* Variadic char arguments are promoted to int. */
            uart_putc((char)va_arg(args, int));
            break;
        case 'u':
            console_unsigned(va_arg(args, unsigned int), 10);
            break;
        case 'x':
            console_unsigned(va_arg(args, unsigned int), 16);
            break;
        case 'd': {
            int value = va_arg(args, int);
            unsigned int magnitude = (unsigned int)value;
            if (value < 0) {
                uart_putc('-');
                /* Unsigned subtraction also handles INT_MIN safely. */
                magnitude = 0u - magnitude;
            }
            console_unsigned(magnitude, 10);
            break;
        }
        case '%':
            uart_putc('%');
            break;
        default:
            uart_putc('%');
            uart_putc(*format);
            break;
        }
        ++format;
    }

    va_end(args);
}
