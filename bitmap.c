#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "bitmap.h"
#include "util.h"


bitmap_t *bitmap_new(int width, int height)
{
    size_t bytes = DIV_ROUND_UP(width, 8) * height;
    bitmap_t *bitmap = calloc(1, bytes);
    assert(bitmap != NULL);

    bitmap->width = width;
    bitmap->height = height;
    return bitmap;
}


void bitmap_free(bitmap_t *bitmap)
{
    free(bitmap);
}


void bitmap_set_pixel(uint8_t *byte, uint8_t shift, uint8_t bit)
{
    *byte |= bit << shift;
}


void bitmap_clear_pixel(uint8_t *byte, uint8_t shift, uint8_t bit)
{
    *byte &= ~(bit << shift);
}


void bitmap_xor_pixel(uint8_t *byte, uint8_t shift, uint8_t bit)
{
    *byte ^= bit << shift;
}


void bitmap_plot(bitmap_t *bitmap, bitmap_draw_fn draw_fn, int x, int y)
{
    size_t n = y * DIV_ROUND_UP(bitmap->width, 8) + x / 8; 
    draw_fn(&bitmap->data[n], 7 - (x & 7), 1);
}


void bitmap_line(bitmap_t *bitmap, bitmap_draw_fn draw_fn, int x1, int y1,
        int x2, int y2)
{
    uint16_t stride = DIV_ROUND_UP(bitmap->width, 8);
    int dx = abs(x2 - x1);
    int sx = x1 < x2 ? 1 : -1;
    int dy = -abs(y2 - y1);
    int sy = y1 < y2 ? 1 : -1;
    int err = dx + dy;

    while (true) {
        size_t n = y1 * stride + x1 / 8; 
        draw_fn(&bitmap->data[n], 7 - (x1 & 7), 1);
        if (x1 == x2 && y1 == y2) {
            break;
        }
        int e2 = err * 2;
        if (e2 >= dy) {
            err += dy;
            x1 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y1 += sy;
        }
    }
}


void bitmap_invert(bitmap_t *bitmap)
{
    uint8_t stride = DIV_ROUND_UP(bitmap->width, 8);
    uint8_t numbytes = bitmap->height * stride;
    for (int n = 0; n < numbytes; n++) {
        bitmap->data[n] ^= 0xFF;
    }
}


void bitmap_blit(bitmap_t *dst, const bitmap_t *src, int dst_x, int dst_y,
        int src_x, int src_y, int width, int height)
{
    int src_n, dst_n;
    int src_shift, dst_shift;

    width = width ? width : src->width;
    height = height ? height : src->height;

    if (dst_x < 0) {
        src_x += dst_x;
        width += dst_x;
        dst_x = 0;
    } else if (dst_x > dst->width) {
        width = dst->width - dst_x;
    }

    if (dst_y < 0) {
        src_y += dst_y;
        height += dst_y;
        dst_y = 0;
    } else if (dst_y > dst->height) {
        height = dst->height - dst_y;
    }

    if (width < 0) {
        return;
    }

    if (height < 0) {
        return;
    }

    int src_stride = DIV_ROUND_UP(src->width, 8);
    int dst_stride = DIV_ROUND_UP(dst->width, 8);

    for (int y = 0; y < height; y++) {
        src_n = (src_y + y) * src_stride + src_x / 8;
        src_shift = 7 - (src_x % 8);
        dst_n = (dst_y + y) * dst_stride + dst_x / 8;
        dst_shift = 7 - (dst_x % 8);

        for (int x = 0; x < width; x++) {
            uint8_t bit = (src->data[src_n] >> src_shift) & 1;
            if (bit) {
                dst->data[dst_n] |= bit << dst_shift;
            } else {
                dst->data[dst_n] &= ~(bit << dst_shift);
            }
            if (src_shift-- == 0) {
                src_shift = 7;
                src_n++;
            }
            if (dst_shift-- == 0) {
                dst_shift = 7;
                dst_n++;
            }
        }
    }
}


void bitmap_blit2(bitmap_t *dst, const bitmap_t *src, bitmap_draw_fn draw_fn,
        int dst_x, int dst_y, int src_x, int src_y, int width, int height)
{
    int src_n, dst_n;
    int src_shift, dst_shift;

    width = width ? width : src->width;
    height = height ? height : src->height;

    if (dst_x < 0) {
        src_x += dst_x;
        width += dst_x;
        dst_x = 0;
    } else if (dst_x > dst->width) {
        width = dst->width - dst_x;
    }

    if (dst_y < 0) {
        src_y += dst_y;
        height += dst_y;
        dst_y = 0;
    } else if (dst_y > dst->height) {
        height = dst->height - dst_y;
    }

    if (width < 0) {
        return;
    }

    if (height < 0) {
        return;
    }

    int src_stride = DIV_ROUND_UP(src->width, 8);
    int dst_stride = DIV_ROUND_UP(dst->width, 8);

    for (int y = 0; y < height; y++) {
        src_n = (src_y + y) * src_stride + src_x / 8;
        src_shift = 7 - (src_x % 8);
        dst_n = (dst_y + y) * dst_stride + dst_x / 8;
        dst_shift = 7 - (dst_x % 8);

        for (int x = 0; x < width; x++) {
            uint8_t bit = (src->data[src_n] >> src_shift) & 1;
            draw_fn(&dst->data[dst_n], dst_shift, bit);
            if (src_shift-- == 0) {
                src_shift = 7;
                src_n++;
            }
            if (dst_shift-- == 0) {
                dst_shift = 7;
                dst_n++;
            }
        }
    }
}
