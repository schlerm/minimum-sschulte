#include "minemu/irq.h"
#include "minemu/platform.h"
#include "minemu/uart.h"

struct minemu_trap_frame *minemu_irq_dispatch(struct minemu_trap_frame *frame) {
    uint32_t source = (uint32_t)frame->exception_id;
    switch (source) {
    case MINEMU_IRQ_UART0:
        uart_handle_irq();
        break;
    case MINEMU_IRQ_UART1:
        while ((MINEMU_UART1->status & MINEMU_UART_STATUS_RX_READY) != 0) {
            (void)MINEMU_UART1->rx_data;
        }
        break;
    case MINEMU_IRQ_SYSTICK:
        MINEMU_SYSTICK->ack = MINEMU_SYSTICK_ACK;
        break;
    case MINEMU_IRQ_BLOCK:
        MINEMU_BLOCK->ack = MINEMU_BLOCK_ACK;
        break;
    default:
        /* No claimed source means there is nothing to acknowledge. */
        return frame;
    }
    MINEMU_INTERRUPT->eoi = source;
    return frame;
}
