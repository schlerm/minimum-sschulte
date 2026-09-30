#ifndef MINEMU_CONSOLE_H
#define MINEMU_CONSOLE_H

/* Supports %s, %c, %u, %d, %x and %%; no widths or length modifiers.
 * Unknown specifiers and a trailing % are printed literally.
 * Output is blocking and is not synchronized between execution contexts.
 */
void console_printf(const char *format, ...);

#endif
