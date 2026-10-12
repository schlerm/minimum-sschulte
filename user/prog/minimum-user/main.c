#include <stdint.h>

#include "minemu/user_abi.h"

void minemu_user_main(void) {
    static char message[] = "hello world\n";

    /* Request kernel UART output; exclude the string's terminating NUL.
     * The variadic wrapper expects exactly void * and uint32_t arguments.
     */
    (void)ioctl(MINEMU_FD_UART_OUT, MINEMU_IOCTL_UART_WRITE,
                (void *)message, (uint32_t)(sizeof(message) - 1U));

    /* Stay in user mode after printing; no exit syscall is needed here. */
    for (;;) {
        __asm__ volatile("nop");
    }
}
