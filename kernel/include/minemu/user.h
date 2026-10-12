#ifndef MINEMU_USER_H
#define MINEMU_USER_H

#include <stdbool.h>
#include <stdint.h>
#include "minemu/boot.h"

bool user_range_valid(uint32_t address, uint32_t length, uint32_t permissions);
bool user_copy_from(void *destination, uint32_t address, uint32_t length);
bool user_copy_to(uint32_t address, const void *source, uint32_t length);
void user_start(const struct minemu_boot_info *boot_info) __attribute__((noreturn));

#endif
