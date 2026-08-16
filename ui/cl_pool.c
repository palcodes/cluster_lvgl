/*
 * cl_pool.c
 *
 * The backdrop gradient, expanded to full size once at start-up.
 *
 * Node 1:3's radial gradient is the largest thing on the screen and the one
 * LVGL has the least help for.  Three approaches were tried:
 *
 *   Stacked shadows.  LVGL's shadow is a box blur bounded by shadow_width
 *   and it allocates (shadow_width + radius)^2 * 2 bytes to draw one, so a
 *   feather wide enough to cross the frame is unaffordable and every
 *   affordable version leaves its rectangle showing as a hard step.
 *
 *   A small alpha map drawn zoomed.  LVGL 8.3 refuses: lv_draw_img.c turns
 *   an LV_IMG_CF_ALPHA_8BIT source into TRUE_COLOR_ALPHA the moment a zoom
 *   or an angle is set, drops the decoded pointer, and the line-reading
 *   fallback it lands in cannot transform - so nothing is drawn at all.
 *
 *   A full-size alpha map in flash.  Correct and free at runtime, but 384 KB
 *   of generated C for one background.
 *
 * So the map ships at a quarter scale - 24 KB, evaluated from the node's own
 * gradient by icons/generate.py - and is expanded here into a full-size
 * buffer once, at start-up.  After that it is an ordinary un-zoomed
 * ALPHA_8BIT image taking LVGL's fast path: no per-frame transform, no
 * shadow buffer, and the colour still comes from img_recolor.
 *
 * The cost is CL_W * CL_H bytes of RAM.  If that is the wrong trade for a
 * given board, the alternative is to have generate.py emit the map at full
 * scale and point cl_pool at it directly; see the README.
 */

#include "cl_pool.h"

static uint8_t pool_buf[CL_W * CL_H];

lv_img_dsc_t cl_pool = {
    .header.cf          = LV_IMG_CF_ALPHA_8BIT,
    .header.always_zero = 0,
    .header.reserved    = 0,
    .header.w           = CL_W,
    .header.h           = CL_H,
    .data_size          = CL_W * CL_H,
    .data               = pool_buf,
};

void cl_pool_build(void)
{
    const uint8_t *src = cl_pool_src.data;
    const int32_t  sw  = (int32_t)cl_pool_src.header.w;
    const int32_t  sh  = (int32_t)cl_pool_src.header.h;
    int32_t x, y;

    /* Bilinear, in 8.8 fixed point.  Sample centres are half a source pixel
     * in from the edge, which is what keeps the expansion from shifting the
     * whole gradient half a scale step to one side. */
    for (y = 0; y < CL_H; y++) {
        int32_t fy = ((y * 2 + 1) * 256) / (2 * CL_POOL_SCALE) - 128;
        int32_t y0, y1, wy;

        if (fy < 0) fy = 0;
        y0 = fy >> 8;
        y1 = (y0 + 1 < sh) ? y0 + 1 : y0;
        wy = fy & 0xFF;

        for (x = 0; x < CL_W; x++) {
            int32_t fx = ((x * 2 + 1) * 256) / (2 * CL_POOL_SCALE) - 128;
            int32_t x0, x1, wx, a, b;

            if (fx < 0) fx = 0;
            x0 = fx >> 8;
            x1 = (x0 + 1 < sw) ? x0 + 1 : x0;
            wx = fx & 0xFF;

            a = src[y0 * sw + x0] * (256 - wx) + src[y0 * sw + x1] * wx;
            b = src[y1 * sw + x0] * (256 - wx) + src[y1 * sw + x1] * wx;
            pool_buf[y * CL_W + x] = (uint8_t)((a * (256 - wy) + b * wy) >> 16);
        }
    }
}
