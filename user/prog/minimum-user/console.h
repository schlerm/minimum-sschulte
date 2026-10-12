#ifndef MSH_CONSOLE_H
#define MSH_CONSOLE_H

#include <stdbool.h>
#include <stddef.h>

#define MSH_NO_INPUT (-1)
#define MSH_INPUT_ERROR (-2)

bool msh_write(const char *bytes, size_t count);
bool msh_puts(const char *text);
/* Returns a byte, MSH_NO_INPUT, or MSH_INPUT_ERROR. */
int msh_getc(void);

#endif
