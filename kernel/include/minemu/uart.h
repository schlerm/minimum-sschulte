#ifndef MINEMU_UART_H
#define MINEMU_UART_H

/* Blocking output to the UART0 console. No newline is added automatically. */
void uart_putc(char c);
void uart_puts(const char *text);

#endif
