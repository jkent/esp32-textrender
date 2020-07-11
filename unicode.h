#pragma once

#include <stddef.h>
#include <stdint.h>


int utf8_cp(const char * restrict s, uint32_t *ch);
size_t utf8_len(const char *s);
