#include "console.h"
#include "minemu/user_abi.h"

bool msh_write(const char *bytes, size_t count) {
    while (count != 0) {
        uint32_t chunk = count > MINEMU_IOCTL_MAX_TRANSFER
            ? MINEMU_IOCTL_MAX_TRANSFER : (uint32_t)count;
        int32_t written = ioctl(MINEMU_FD_UART_OUT, MINEMU_IOCTL_UART_WRITE,
                                (void *)bytes, chunk);
        /* Avoid an endless retry if the kernel reports an error/no progress. */
        if (written <= 0 || (uint32_t)written > chunk) {
            return false;
        }
        bytes += written;
        count -= (size_t)written;
    }
    return true;
}

bool msh_puts(const char *text) {
    size_t length = 0;
    while (text[length] != '\0') {
        ++length;
    }
    return msh_write(text, length);
}

int msh_getc(void) {
    unsigned char byte;
    int32_t count = ioctl(MINEMU_FD_UART_IN, MINEMU_IOCTL_UART_READ,
                          (void *)&byte, UINT32_C(1));
    if (count == 0) {
        return MSH_NO_INPUT;
    }
    return count == 1 ? byte : MSH_INPUT_ERROR;
}
