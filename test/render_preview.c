/*
 * render_preview.c
 *
 * Headless preview: builds the cluster against a memory framebuffer, runs
 * the timers for a moment so anything scheduled has settled, and dumps the
 * result as raw pixels.  No window, no display driver, no SDL - which makes
 * it the thing to reach for when comparing a build against the Figma frame,
 * or when checking the UI in CI.
 *
 *   render_preview <run_ms> <out.bin> [mode 0=CRAWL 1=STREET 2=RUSH]
 *
 * The dump is CL_PANEL_W x CL_PANEL_H raw pixels - the whole panel canvas,
 * design plus any bezel either side of it.  test/raw_to_png.py works out
 * whether that was 16 or 32 bits from the file length.
 */

#include "lvgl.h"
#include "cl_screen.h"
#include "cluster_data.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define W CL_PANEL_W
#define H CL_PANEL_H

/* Bytes per rendered pixel: LV_COLOR_DEPTH 32 -> XRGB8888, 16 -> RGB565. */
#define BPP (LV_COLOR_DEPTH / 8)

static uint8_t fb[W * H * BPP];
static uint8_t draw_buf_1[W * H * BPP];

static void flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    int32_t y;

    /* RENDER_MODE_FULL hands the whole frame over every time, but copying by
     * area keeps this right if that ever changes. */
    for (y = area->y1; y <= area->y2; y++) {
        int32_t w = area->x2 - area->x1 + 1;
        memcpy(&fb[((size_t)y * W + area->x1) * BPP], px_map, (size_t)w * BPP);
        px_map += (size_t)w * BPP;
    }
    lv_display_flush_ready(disp);
}

int main(int argc, char **argv)
{
    lv_display_t *disp;
    int run_ms = (argc > 1) ? atoi(argv[1]) : 500;
    const char *out = (argc > 2) ? argv[2] : "frame.bin";
    int i;
    FILE *f;

    lv_init();
    disp = lv_display_create(W, H);
    lv_display_set_flush_cb(disp, flush_cb);
    lv_display_set_buffers(disp, draw_buf_1, NULL, sizeof(draw_buf_1),
                           LV_DISPLAY_RENDER_MODE_FULL);

    cl_screen_create(&cl_ui);
    cluster_data_init(&cl_ui);
    if (argc > 3) cluster_set_mode((cl_mode_t)atoi(argv[3]));

    for (i = 0; i < run_ms / 16; i++) {
        lv_tick_inc(16);
        lv_timer_handler();
    }
    lv_obj_invalidate(lv_screen_active());
    lv_tick_inc(16);
    lv_timer_handler();

    f = fopen(out, "wb");
    if (!f) { printf("cannot open %s\n", out); return 1; }
    fwrite(fb, 1, sizeof(fb), f);
    fclose(f);
    printf("wrote %s after %d ms (%dx%d, %d bpp)\n", out, run_ms, W, H, BPP * 8);
    return 0;
}
