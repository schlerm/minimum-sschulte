# User-mode ioctl implementation

The kernel now boots the packaged `minimum-user` module in A32 user mode.
The current user program runs `msh` using `ioctl` for all input and output.
The independent hello fixture still exercises user-mode hello world.
The old kernel shell source is retained but is no longer linked or called.
See [msh.md](msh.md) for the user-shell implementation and behavior.

## ABI and request behavior

The implementation follows `kernel/include/minemu/user_abi.h`, the existing
user wrapper, and the supplied `examples/process-basics/kernel/syscall.c`
request semantics. It does not implement the conflicting raw-device-address
fd/op numbering from the assignment webpage. UART input (fd 0), output (fd 1),
and diagnostics (fd 2) all use UART0. RNG is fd 3; TRACE is fd 4.
UART1 is not exposed by this shared ABI.

| Request | Allowed fd | Arguments after request | Result |
| --- | --- | --- | --- |
| UART_READ | UART_IN | `void *destination`, `uint32_t count` | Available byte count; does not wait for input |
| UART_WRITE | UART_OUT, UART_DIAG | `void *source`, `uint32_t count` | Count after transmitting all bytes |
| UART_STATUS | Any UART fd | None | RX_READY / TX_READY bit mask |
| RNG_NEXT | RNG | `uint32_t *destination` | 0 after generating and storing a value |
| RNG_STATE | RNG | `uint32_t *destination` | 0 after storing state without advancing it |
| RNG_SEED | RNG | `uint32_t seed` | 0 after setting seed |
| TRACE_EVENT | TRACE | `uint32_t value` | 0 after recording event |

All failures return `MINEMU_IOCTL_ERROR` (-1), with no separate errno.
UART transfers are limited to 256 bytes per call; larger requests are rejected,
not truncated. Zero-length transfers return 0 and do not inspect the pointer
or consume input. String terminators have no special meaning to a transfer.
The variadic wrapper requires exactly the pointer/integer types shown above.

## Call path

1. `user/lib/src/ioctl.c` unpacks variadic arguments into r0 (fd), r1 (request),
   r2/r3 (arguments), and r7 (syscall number), then executes `svc #0`.
2. `kernel/src/startup/svc.S` masks IRQs and saves an 88-byte trap frame. The
   saved SPSR retains the user's previous interrupt mask. The trampoline records
   LR - 4 as the SVC instruction address and calls `minemu_svc_dispatch`.
3. `kernel/src/core/syscall.c` checks A32 user origin, instruction alignment and
   user read/execute permission, the exact `svc #0` encoding, and syscall number.
   It checks the request/device pairing and any memory ranges before device I/O.
4. The dispatcher writes the result to saved r0 and returns the same frame.
   The trampoline restores registers and returns with `movs pc, lr`, restoring
   the caller's mode and interrupt mask and resuming after SVC.

Unsupported syscalls, including the later process/thread calls declared in the
shared header, return -1. They are outside this implementation.

## Loading and memory protection

`kernel/src/core/user.c` adapts the supplied user-mode-hello loader. It maps
System ROM for supervisor reads, finds the `minimum-user` module, copies its
segments into statically reserved RAM, clears BSS, and creates user mappings
with the segment permissions. Kernel and MMIO mappings stay supervisor-only.
There is one address space, up to 16 image pages (64 KiB), and a 4 KiB user
stack at 0x007ff000-0x00800000. Image mappings lie in 0x00400000-0x007ff000.
`startup/user.S` initializes the banked user SP/LR and enters user mode.
The image-page pool is reserved in a non-loaded linker section. The loader
zeros each allocated page before mapping it; Boot ROM need not also zero all
unused pool pages, keeping startup within the prompt test tick budget.

`user_range_valid` checks bounds without overflowing address arithmetic, then
checks every covered page for validity, user access, and requested permissions.
`user_copy_from` requires readable pages; `user_copy_to` requires writable pages.
The instruction check additionally requires executable memory.

UART_WRITE copies the entire source into a kernel buffer before any output.
UART_READ validates the full destination before consuming bytes. RNG_NEXT checks
its destination before advancing the generator. Copies use byte accesses, so
valid unaligned user buffers and buffers crossing mapped pages work. MMIO itself
still uses aligned 32-bit register accesses.

These checks use the loader's page table. Mappings are fixed after user entry;
there is no concurrent address-space replacement or scheduler in this version.

## UART integration and synchronization

Existing IRQ reception and its protected ring buffer remain in use. The new
`uart_read` copies bytes into a kernel buffer, preserving the caller's IRQ mask.
It also drains pending hardware bytes because IRQs are masked during syscall
handling. `uart_status` reports hardware flags plus bytes already queued in
software, without consuming them.

If the ring overflow marker is next, read consumes that marker and returns -1.
If some bytes precede the marker, it returns those bytes first and reports the
loss on the next call. The existing discard-through-newline policy is retained.

SVC handlers keep IRQs masked until returning to user mode. This makes validation
and device access straightforward but increases interrupt latency during UART
writes. The 256-byte limit bounds each transfer; TX still uses blocking polling.

## Verification

Run these in the course environment:

```sh
make
just test hw2 hello
just test hw2 devices
just test hw2 ioctl-validation
```

The additional fixture tests all seven requests, wrong descriptors and requests,
unknown syscall numbers and nonzero SVC immediates, zero/max/oversized lengths,
kernel/MMIO/unmapped pointers, address wraparound, read-only destinations,
valid page-crossing buffers, invalid page-crossing ranges, UART short/empty reads,
and preservation of RNG state and queued input on rejected requests.
It uses separate RX and RW ELF segments to exercise real page permissions.
The trace contains a single success marker; any failed check records its number.

Run `just test-all hw2` to include all shell tests as well as these syscall tests.

## Supplied-code attribution

The module loader and user-entry/SVC assembly are adapted from
`examples/user-mode-hello/kernel/{main.c,user.S,svc.S}`. Device request semantics
follow `examples/process-basics/kernel/syscall.c`. Both are supplied repository
examples; the syscall validation and buffered UART integration are implemented
in the canonical kernel here.
