#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "unicode.h"


static const uint8_t LEN[] = {1, 1, 1, 1, 2, 2, 3, 0};
static const uint8_t MSK[] = {
    0xFF >> 0, 0xFF >> 0, 0xFF >> 3, 0xFF >> 4,
    0xFF >> 5, 0xFF >> 0, 0xFF >> 0, 0xFF >> 0
};


int utf8_cp(const char * restrict s, uint32_t *ch)
{
    int len = 0;
    int32_t val = 0;
    unsigned first = (uint8_t)(*s);

    len = (first > 0) * (1 + ((first & 0xC0) == 0xC0) * LEN[(first >> 3) & 7]);
    val = first & MSK[len];

    for (int k = len; k > 1; k--) {
        if ((*++s & 0xC0) != 0x80) {
            val = first;
            len = 1;
            break;
        }
        val = (val << 6) | (*s & 0x3F);
    }
    if (ch) {
        *ch = val;
    }
    return len;
}

size_t utf8_len(const char *s)
{
    size_t len = 0;
    size_t offset = 0;
    while (s[offset]) {
        offset += utf8_cp(&s[offset], NULL);
        len++;
    }
    return len;
}