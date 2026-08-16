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
 * The dump is lv_color_t-sized pixels; test/raw_to_png.py works out whether
 * that was 16 or 32 bits from the file length.
 */

#include "lvgl.h"
#include "cl_screen.h"
#include "cluster_data.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define W CL_W
#define H CL_H

static lv_color_t fb[W * H];
static lv_color_t draw_buf_1[W * H];
static lv_disp_draw_buf_t draw_buf;

static void flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p)
{
    int32_t y;
    for (y = area->y1; y <= area->y2; y++) {
        int32_t w = area->x2 - area->x1 + 1;
        memcpy(&fb[y * W + area->x1], color_p, (size_t)w * sizeof(lv_color_t));
        color_p += w;
    }
    lv_disp_flush_ready(drv);
}

int main(int argc, char **argv)
{
    static lv_disp_drv_t disp_drv;
    int run_ms = (argc > 1) ? atoi(argv[1]) : 500;
    const char *out = (argc > 2) ? argv[2] : "frame.bin";
    int i;
    FILE *f;

    lv_init();
    lv_disp_draw_buf_init(&draw_buf, draw_buf_1, NULL, W * H);
    lv_disp_drv_init(&disp_drv);
    disp_drv.draw_buf     = &draw_buf;
    disp_drv.flush_cb     = flush_cb;
    disp_drv.hor_res      = W;
    disp_drv.ver_res      = H;
    disp_drv.full_refresh = 1;
    lv_disp_drv_register(&disp_drv);

    cl_screen_create(&cl_ui);
    cluster_data_init(&cl_ui);
    if (argc > 3) cluster_set_mode((cl_mode_t)atoi(argv[3]));

    for (i = 0; i < run_ms / 16; i++) {
        lv_tick_inc(16);
        lv_timer_handler();
    }
    lv_obj_invalidate(lv_scr_act());
    lv_tick_inc(16);
    lv_timer_handler();

    f = fopen(out, "wb");
    if (!f) { printf("cannot open %s\n", out); return 1; }
    fwrite(fb, sizeof(lv_color_t), (size_t)W * H, f);
    fclose(f);
    printf("wrote %s after %d ms\n", out, run_ms);
    return 0;
}
