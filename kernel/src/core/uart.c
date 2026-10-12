#include "minemu/platform.h"
#include "minemu/uart.h"

#define RX_SIZE 256u
#define RX_LOST 256u

static uint16_t rx_buffer[RX_SIZE];
static unsigned int rx_head;
static unsigned int rx_tail;
static int rx_discarding;

/* Preserve the caller's IRQ mask instead of unconditionally enabling IRQs. */
static uint32_t irq_save(void) {
    uint32_t status;
    __asm__ volatile("mrs %0, cpsr\n\tcpsid i" : "=r"(status) : : "memory");
    return status;
}

static void irq_restore(uint32_t status) {
    if ((status & UINT32_C(0x80)) == 0) {
        __asm__ volatile("cpsie i" : : : "memory");
    }
}

void uart_init(void) {
    uint32_t status = irq_save();
    rx_head = 0;
    rx_tail = 0;
    rx_discarding = 0;
    MINEMU_UART0->control = MINEMU_UART_CONTROL_RX_IRQ_ENABLE;
    MINEMU_INTERRUPT->enable |= UINT32_C(1) << MINEMU_IRQ_UART0;
    irq_restore(status);
}

void uart_handle_irq(void) {
    /* IRQ entry already masks IRQs. Drain even when the software queue fills. */
    while ((MINEMU_UART0->status & MINEMU_UART_STATUS_RX_READY) != 0) {
        uint8_t byte = (uint8_t)MINEMU_UART0->rx_data;
        unsigned int next = (rx_head + 1u) % RX_SIZE;
        if (!rx_discarding && next == rx_tail) {
            /* Abandon queued input and resynchronize at a complete line. */
            rx_head = 0;
            rx_tail = 0;
            rx_discarding = 1;
        }
        if (rx_discarding) {
            if (byte == '\n') {
                rx_buffer[rx_head] = RX_LOST;
                rx_head = (rx_head + 1u) % RX_SIZE;
                rx_discarding = 0;
            }
        } else {
            rx_buffer[rx_head] = byte;
            rx_head = next;
        }
    }
}

int uart_getc(void) {
    uint32_t status = irq_save();
    int result = UART_NO_INPUT;
    if (rx_tail != rx_head) {
        unsigned int byte = rx_buffer[rx_tail];
        rx_tail = (rx_tail + 1u) % RX_SIZE;
        result = byte == RX_LOST ? UART_INPUT_OVERFLOW : (int)byte;
    }
    irq_restore(status);
    return result;
}

void uart_putc(char c) {
    while ((MINEMU_UART0->status & MINEMU_UART_STATUS_TX_READY) == 0) {
        /* Poll the volatile status register until transmission is possible. */
    }
    MINEMU_UART0->tx_data = (uint8_t)c;
}

int32_t uart_read(uint8_t *destination, uint32_t count) {
    if (count == 0) {
        return 0;
    }
    uint32_t status = irq_save();
    /* SVC masks IRQs, so collect any bytes whose IRQ has not yet run. */
    uart_handle_irq();
    uint32_t copied = 0;
    while (copied < count && rx_tail != rx_head) {
        unsigned int byte = rx_buffer[rx_tail];
        if (byte == RX_LOST) {
            /* Return existing bytes first; report the loss on the next read. */
            if (copied != 0) {
                break;
            }
            rx_tail = (rx_tail + 1u) % RX_SIZE;
            irq_restore(status);
            return -1;
        }
        rx_tail = (rx_tail + 1u) % RX_SIZE;
        destination[copied++] = (uint8_t)byte;
    }
    irq_restore(status);
    return (int32_t)copied;
}

uint32_t uart_status(void) {
    uint32_t saved = irq_save();
    uint32_t status = MINEMU_UART0->status;
    if (rx_tail != rx_head) {
        status |= MINEMU_UART_STATUS_RX_READY;
    }
    irq_restore(saved);
    return status & (MINEMU_UART_STATUS_RX_READY | MINEMU_UART_STATUS_TX_READY);
}

void uart_puts(const char *text) {
    while (*text != '\0') {
        uart_putc(*text++);
    }
}
