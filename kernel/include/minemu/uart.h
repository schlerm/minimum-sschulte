#ifndef MINEMU_UART_H
#define MINEMU_UART_H

/* Blocking output to the UART0 console. No newline is added automatically. */
void uart_putc(char c);
void uart_puts(const char *text);

#define UART_NO_INPUT (-1)
#define UART_INPUT_OVERFLOW (-2)

/* Enable UART0 receive interrupts; the caller enables CPU IRQs separately. */
void uart_init(void);
void uart_handle_irq(void);
/* Nonblocking: a byte, UART_NO_INPUT, or UART_INPUT_OVERFLOW. */
int uart_getc(void);

#endif
