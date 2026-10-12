#include "minemu/user_abi.h"

static uint8_t buffer[8192] __attribute__((aligned(4096)));
static const char readonly[] = "ioctl validation passed\n";
extern char __test_gap[];

static void check(int condition, uint32_t code) {
    if (!condition) {
        (void)ioctl(MINEMU_FD_TRACE, MINEMU_IOCTL_TRACE_EVENT, code);
        __asm__ volatile("bkpt #0");
        for (;;) {}
    }
}

static int32_t bad_svc(uint32_t number, int nonzero_immediate) {
    register uint32_t r0 __asm__("r0") = MINEMU_FD_TRACE;
    register uint32_t r1 __asm__("r1") = MINEMU_IOCTL_TRACE_EVENT;
    register uint32_t r2 __asm__("r2") = 0xbad;
    register uint32_t r7 __asm__("r7") = number;
    if (nonzero_immediate) {
        __asm__ volatile("svc #1" : "+r"(r0) : "r"(r1), "r"(r2), "r"(r7) : "memory", "cc");
    } else {
        __asm__ volatile("svc #0" : "+r"(r0) : "r"(r1), "r"(r2), "r"(r7) : "memory", "cc");
    }
    return (int32_t)r0;
}

void minemu_user_main(void) {
    uint32_t state;
    check(bad_svc(99, 0) == -1, 1);
    check(bad_svc(MINEMU_SYSCALL_IOCTL, 1) == -1, 2);
    check(ioctl(99, MINEMU_IOCTL_TRACE_EVENT, UINT32_C(123)) == -1, 3);
    check(ioctl(MINEMU_FD_TRACE, UINT32_C(0xffffffff)) == -1, 4);
    check(ioctl(MINEMU_FD_UART_IN, MINEMU_IOCTL_UART_WRITE, (void *)buffer, UINT32_C(1)) == -1, 5);
    check(ioctl(MINEMU_FD_UART_OUT, MINEMU_IOCTL_UART_READ, (void *)buffer, UINT32_C(1)) == -1, 6);
    check(ioctl(MINEMU_FD_UART_OUT, MINEMU_IOCTL_UART_WRITE, (void *)0, UINT32_C(0)) == 0, 7);
    check(ioctl(MINEMU_FD_UART_IN, MINEMU_IOCTL_UART_READ, (void *)0, UINT32_C(0)) == 0, 8);
    check(ioctl(MINEMU_FD_UART_OUT, MINEMU_IOCTL_UART_WRITE, (void *)buffer, UINT32_C(257)) == -1, 9);
    check(ioctl(MINEMU_FD_UART_OUT, MINEMU_IOCTL_UART_WRITE, (void *)buffer, UINT32_MAX) == -1, 10);
    check(ioctl(MINEMU_FD_UART_OUT, MINEMU_IOCTL_UART_WRITE, (void *)0xc0030000, UINT32_C(4)) == -1, 11);
    check(ioctl(MINEMU_FD_UART_OUT, MINEMU_IOCTL_UART_WRITE, (void *)0x10004000, UINT32_C(4)) == -1, 12);
    check(ioctl(MINEMU_FD_UART_OUT, MINEMU_IOCTL_UART_WRITE, (void *)0xfffffffe, UINT32_C(4)) == -1, 13);
    check(ioctl(MINEMU_FD_UART_OUT, MINEMU_IOCTL_UART_WRITE, (void *)0x00600000, UINT32_C(1)) == -1, 14);
    check(ioctl(MINEMU_FD_UART_OUT, MINEMU_IOCTL_UART_WRITE,
                (void *)((uintptr_t)__test_gap - 1U), UINT32_C(2)) == -1, 15);

    check(ioctl(MINEMU_FD_RNG, MINEMU_IOCTL_RNG_SEED, UINT32_C(0x12345678)) == 0, 16);
    check(ioctl(MINEMU_FD_RNG, MINEMU_IOCTL_RNG_NEXT, (uint32_t *)(uintptr_t)readonly) == -1, 17);
    check(ioctl(MINEMU_FD_RNG, MINEMU_IOCTL_RNG_NEXT, (uint32_t *)0x007ffffe) == -1, 18);
    check(ioctl(MINEMU_FD_RNG, MINEMU_IOCTL_RNG_STATE, &state) == 0 && state == 0x12345678, 19);
    /* A valid output may straddle two mapped writable pages. */
    check(ioctl(MINEMU_FD_RNG, MINEMU_IOCTL_RNG_NEXT, (uint32_t *)(void *)(buffer + 4094)) == 0, 20);
    check(buffer[4094] == 0xa5 && buffer[4095] == 0x5a && buffer[4096] == 0x98 && buffer[4097] == 0x87, 21);
    check(ioctl(MINEMU_FD_RNG, MINEMU_IOCTL_RNG_STATE, &state) == 0 && state == 0x87985aa5, 22);
    check(ioctl(MINEMU_FD_TRACE, MINEMU_IOCTL_RNG_SEED, UINT32_C(123)) == -1, 23);
    check((ioctl(MINEMU_FD_UART_DIAG, MINEMU_IOCTL_UART_STATUS) & MINEMU_IOCTL_UART_TX_READY) != 0, 24);
    check(ioctl(MINEMU_FD_RNG, MINEMU_IOCTL_UART_STATUS) == -1, 25);
    check(ioctl(MINEMU_FD_UART_IN, MINEMU_IOCTL_UART_READ, (void *)buffer, UINT32_C(1)) == 0, 26);

    while ((ioctl(MINEMU_FD_UART_IN, MINEMU_IOCTL_UART_STATUS) & MINEMU_IOCTL_UART_RX_READY) == 0) {}
    /* Invalid destinations must not remove the scheduled "abc" input. */
    check(ioctl(MINEMU_FD_UART_IN, MINEMU_IOCTL_UART_READ, (void *)readonly, UINT32_C(1)) == -1, 27);
    check(ioctl(MINEMU_FD_UART_IN, MINEMU_IOCTL_UART_READ, (void *)0x007fffff, UINT32_C(2)) == -1, 28);
    check(ioctl(MINEMU_FD_UART_IN, MINEMU_IOCTL_UART_READ, (void *)buffer, UINT32_C(257)) == -1, 29);
    check(ioctl(MINEMU_FD_UART_IN, MINEMU_IOCTL_UART_READ, (void *)buffer, UINT32_C(2)) == 2, 30);
    check(buffer[0] == 'a' && buffer[1] == 'b', 31);
    check(ioctl(MINEMU_FD_UART_IN, MINEMU_IOCTL_UART_READ, (void *)buffer, UINT32_C(2)) == 1 && buffer[0] == 'c', 32);
    check(ioctl(MINEMU_FD_UART_IN, MINEMU_IOCTL_UART_READ, (void *)buffer, UINT32_C(1)) == 0, 33);

    for (uint32_t i = 0; i < 256; ++i) buffer[4000 + i] = 'x';
    check(ioctl(MINEMU_FD_UART_OUT, MINEMU_IOCTL_UART_WRITE, (void *)(buffer + 4000), UINT32_C(256)) == 256, 34);
    check(ioctl(MINEMU_FD_UART_DIAG, MINEMU_IOCTL_UART_WRITE, (void *)readonly, (uint32_t)(sizeof(readonly) - 1)) == (int32_t)sizeof(readonly) - 1, 35);
    (void)ioctl(MINEMU_FD_TRACE, MINEMU_IOCTL_TRACE_EVENT, UINT32_C(0x600d));
    __asm__ volatile("bkpt #0");
    for (;;) {}
}
