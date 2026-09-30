#include "minemu/platform.h"
#include "minemu/uart.h"

void uart_putc(char c) {
    while ((MINEMU_UART0->status & MINEMU_UART_STATUS_TX_READY) == 0) {
        /* Poll the volatile status register until transmission is possible. */
    }
    MINEMU_UART0->tx_data = (uint8_t)c;
}

void uart_puts(const char *text) {
    while (*text != '\0') {
        uart_putc(*text++);
    }
}
