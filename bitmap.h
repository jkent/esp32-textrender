#pragma once

#include <stdint.h>

typedef struct bitmap_t {
    uint16_t width;
    uint16_t height;
    uint8_t data[];
} bitmap_t;

typedef void (*bitmap_draw_fn)(uint8_t *byte, uint8_t shift, uint8_t bit);

bitmap_t *bitmap_new(int width, int height);
void bitmap_free(bitmap_t *image);
void bitmap_set_pixel(uint8_t *byte, uint8_t shift, uint8_t bit);
void bitmap_clear_pixel(uint8_t *byte, uint8_t shift, uint8_t bit);
void bitmap_xor_pixel(uint8_t *byte, uint8_t shift, uint8_t bit);
void bitmap_plot(bitmap_t *bitmap, bitmap_draw_fn draw_fn, int x, int y);
void bitmap_line(bitmap_t *bitmap, bitmap_draw_fn draw_fn, int x1, int y1,
        int x2, int y2);
void bitmap_invert(bitmap_t *bitmap);
void bitmap_blit(bitmap_t *dst, const bitmap_t *src, int dst_x, int dst_y,
        int src_x, int src_y, int width, int height);
void bitmap_blit2(bitmap_t *dst, const bitmap_t *src, bitmap_draw_fn draw_fn,
        int dst_x, int dst_y, int src_x, int src_y, int width, int height);