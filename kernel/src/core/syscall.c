/* Request semantics follow the supplied user_abi.h and process-basics example. */
#include "minemu/mmu.h"
#include "minemu/platform.h"
#include "minemu/syscall.h"
#include "minemu/uart.h"
#include "minemu/user.h"
#include "minemu/user_abi.h"

static int32_t ioctl_dispatch(uint32_t fd, uint32_t request,
                              uint32_t argument0, uint32_t argument1) {
    switch (request) {
    case MINEMU_IOCTL_UART_READ: {
        uint8_t bytes[MINEMU_IOCTL_MAX_TRANSFER];
        if (fd != MINEMU_FD_UART_IN || argument1 > sizeof(bytes) ||
            !user_range_valid(argument0, argument1, MINEMU_PTE_WRITABLE)) {
            return MINEMU_IOCTL_ERROR;
        }
        /* Validate the whole destination before consuming any input. */
        int32_t count = uart_read(bytes, argument1);
        if (count < 0) {
            return MINEMU_IOCTL_ERROR;
        }
        return user_copy_to(argument0, bytes, (uint32_t)count)
            ? count : MINEMU_IOCTL_ERROR;
    }
    case MINEMU_IOCTL_UART_WRITE: {
        uint8_t bytes[MINEMU_IOCTL_MAX_TRANSFER];
        if ((fd != MINEMU_FD_UART_OUT && fd != MINEMU_FD_UART_DIAG) ||
            argument1 > sizeof(bytes) ||
            !user_copy_from(bytes, argument0, argument1)) {
            return MINEMU_IOCTL_ERROR;
        }
        for (uint32_t index = 0; index < argument1; ++index) {
            uart_putc((char)bytes[index]);
        }
        return (int32_t)argument1;
    }
    case MINEMU_IOCTL_UART_STATUS: {
        if (fd != MINEMU_FD_UART_IN && fd != MINEMU_FD_UART_OUT &&
            fd != MINEMU_FD_UART_DIAG) {
            return MINEMU_IOCTL_ERROR;
        }
        uint32_t status = uart_status();
        uint32_t result = 0;
        if ((status & MINEMU_UART_STATUS_RX_READY) != 0) {
            result |= MINEMU_IOCTL_UART_RX_READY;
        }
        if ((status & MINEMU_UART_STATUS_TX_READY) != 0) {
            result |= MINEMU_IOCTL_UART_TX_READY;
        }
        return (int32_t)result;
    }
    case MINEMU_IOCTL_RNG_NEXT:
    case MINEMU_IOCTL_RNG_STATE: {
        if (fd != MINEMU_FD_RNG ||
            !user_range_valid(argument0, sizeof(uint32_t), MINEMU_PTE_WRITABLE)) {
            return MINEMU_IOCTL_ERROR;
        }
        uint32_t value = request == MINEMU_IOCTL_RNG_NEXT
            ? MINEMU_RNG->data : MINEMU_RNG->state;
        return user_copy_to(argument0, &value, sizeof(value)) ? 0 : MINEMU_IOCTL_ERROR;
    }
    case MINEMU_IOCTL_RNG_SEED:
        if (fd != MINEMU_FD_RNG) {
            return MINEMU_IOCTL_ERROR;
        }
        MINEMU_RNG->seed = argument0;
        return 0;
    case MINEMU_IOCTL_TRACE_EVENT:
        if (fd != MINEMU_FD_TRACE) {
            return MINEMU_IOCTL_ERROR;
        }
        MINEMU_TRACE->event = argument0;
        return 0;
    default:
        return MINEMU_IOCTL_ERROR;
    }
}

struct minemu_trap_frame *minemu_svc_dispatch(struct minemu_trap_frame *frame) {
    uint32_t instruction;
    /* Accept only A32 user calls through an aligned, readable executable SVC. */
    if (frame->exception_id != MINEMU_EXCEPTION_SVC ||
        (frame->spsr & UINT32_C(0x3f)) != UINT32_C(0x10) ||
        (frame->fault_pc & 3U) != 0 ||
        !user_range_valid(frame->fault_pc, sizeof(instruction),
                          MINEMU_PTE_READABLE | MINEMU_PTE_EXECUTABLE) ||
        !user_copy_from(&instruction, frame->fault_pc, sizeof(instruction)) ||
        instruction != UINT32_C(0xef000000) ||
        frame->r[7] != MINEMU_SYSCALL_IOCTL) {
        frame->r[0] = (uint32_t)MINEMU_IOCTL_ERROR;
        return frame;
    }
    frame->r[0] = (uint32_t)ioctl_dispatch(frame->r[0], frame->r[1],
                                         frame->r[2], frame->r[3]);
    return frame;
}
