/*
 * main_win32.c
 *
 * Windows 11 simulator for the cluster.  Plain Win32 + GDI - no SDL, no
 * external libraries, nothing to install beyond a C compiler.
 *
 * With LV_COLOR_DEPTH 32 an lv_color_t is BGRA in memory, which is exactly
 * the layout of a 32-bit top-down DIB section, so flushing is a row memcpy.
 *
 *   ev_cluster.exe                       run the cluster
 *   ev_cluster.exe --mode 2              start in RUSH
 *   ev_cluster.exe --exit-after 6000     quit after 6 s (used by the tests)
 *   ev_cluster.exe --dump frame.bin      write the final framebuffer out
 *
 * Keys:  1 / 2 / 3  ride mode        Esc  quit
 */

#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lvgl.h"
#include "ui_dash.h"
#include "ev_data.h"

#define SIM_W           EV_W
#define SIM_H           EV_H
#define SIM_TITLE       "EV Cluster - LVGL 8.3"
#define DRAW_BUF_LINES  120

static HWND       g_hwnd;
static HDC        g_memdc;
static HBITMAP    g_dib;
static uint8_t   *g_pixels;          /* BGRA, top-down, SIM_W * SIM_H * 4 */
static bool       g_running = true;

static lv_indev_state_t g_mouse_state = LV_INDEV_STATE_RELEASED;
static lv_point_t       g_mouse_point;

/**********************
 *   LVGL PLUMBING
 **********************/
static void flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *px)
{
    int32_t y;
    int32_t w = area->x2 - area->x1 + 1;

    for (y = area->y1; y <= area->y2; y++) {
        memcpy(g_pixels + ((size_t)y * SIM_W + area->x1) * 4,
               px, (size_t)w * 4);
        px += w;
    }

    /* Push just the dirty rectangle to the window. */
    {
        HDC dc = GetDC(g_hwnd);
        BitBlt(dc, area->x1, area->y1, w, area->y2 - area->y1 + 1,
               g_memdc, area->x1, area->y1, SRCCOPY);
        ReleaseDC(g_hwnd, dc);
    }

    lv_disp_flush_ready(drv);
}

static void mouse_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    LV_UNUSED(drv);
    data->point = g_mouse_point;
    data->state = g_mouse_state;
}

/**********************
 *      WINDOW
 **********************/
static LRESULT CALLBACK wnd_proc(HWND h, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC dc = BeginPaint(h, &ps);
            BitBlt(dc, 0, 0, SIM_W, SIM_H, g_memdc, 0, 0, SRCCOPY);
            EndPaint(h, &ps);
            return 0;
        }
        case WM_MOUSEMOVE:
            g_mouse_point.x = (lv_coord_t)GET_X_LPARAM(lp);
            g_mouse_point.y = (lv_coord_t)GET_Y_LPARAM(lp);
            return 0;
        case WM_LBUTTONDOWN:
            g_mouse_point.x = (lv_coord_t)GET_X_LPARAM(lp);
            g_mouse_point.y = (lv_coord_t)GET_Y_LPARAM(lp);
            g_mouse_state = LV_INDEV_STATE_PRESSED;
            SetCapture(h);
            return 0;
        case WM_LBUTTONUP:
            g_mouse_state = LV_INDEV_STATE_RELEASED;
            ReleaseCapture();
            return 0;
        case WM_KEYDOWN:
            if (wp == VK_ESCAPE) g_running = false;
            if (wp == '1') ev_data_set_mode(EV_MODE_STREET);
            if (wp == '2') ev_data_set_mode(EV_MODE_CRAWL);
            if (wp == '3') ev_data_set_mode(EV_MODE_RUSH);
            return 0;
        case WM_CLOSE:
        case WM_DESTROY:
            g_running = false;
            return 0;
    }
    return DefWindowProc(h, msg, wp, lp);
}

static bool create_window(void)
{
    WNDCLASSA wc;
    RECT r;
    BITMAPINFO bmi;
    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc   = wnd_proc;
    wc.hInstance     = GetModuleHandle(NULL);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = "EvClusterWnd";
    if (!RegisterClassA(&wc)) return false;

    /* Ask for a client area of exactly SIM_W x SIM_H. */
    r.left = 0; r.top = 0; r.right = SIM_W; r.bottom = SIM_H;
    AdjustWindowRect(&r, style, FALSE);

    g_hwnd = CreateWindowA("EvClusterWnd", SIM_TITLE, style,
                           CW_USEDEFAULT, CW_USEDEFAULT,
                           r.right - r.left, r.bottom - r.top,
                           NULL, NULL, wc.hInstance, NULL);
    if (!g_hwnd) return false;

    /* Top-down 32-bit DIB: negative height, BGRA byte order. */
    memset(&bmi, 0, sizeof(bmi));
    bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth       = SIM_W;
    bmi.bmiHeader.biHeight      = -SIM_H;
    bmi.bmiHeader.biPlanes      = 1;
    bmi.bmiHeader.biBitCount    = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    {
        HDC screen = GetDC(NULL);
        g_memdc = CreateCompatibleDC(screen);
        ReleaseDC(NULL, screen);
    }
    g_dib = CreateDIBSection(g_memdc, &bmi, DIB_RGB_COLORS,
                             (void **)&g_pixels, NULL, 0);
    if (!g_dib || !g_pixels) return false;
    SelectObject(g_memdc, g_dib);
    memset(g_pixels, 0, (size_t)SIM_W * SIM_H * 4);

    ShowWindow(g_hwnd, SW_SHOW);
    UpdateWindow(g_hwnd);
    return true;
}

/**********************
 *        MAIN
 **********************/
int main(int argc, char **argv)
{
    static lv_disp_draw_buf_t draw_buf;
    static lv_color_t buf1[SIM_W * DRAW_BUF_LINES];
    static lv_color_t buf2[SIM_W * DRAW_BUF_LINES];
    static lv_disp_drv_t  disp_drv;
    static lv_indev_drv_t indev_drv;

    int         mode       = 0;
    int         exit_after = 0;         /* ms; 0 = run until closed */
    const char *dump_path  = NULL;
    ULONGLONG   start, last;
    int         i;

    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--mode")       && i + 1 < argc) mode       = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--exit-after") && i + 1 < argc) exit_after = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--dump")  && i + 1 < argc) dump_path  = argv[++i];
    }

    /* Without this Windows 11 scales the window on high-DPI displays and the
     * 1 px hairlines turn to mush. */
    SetProcessDPIAware();

    if (!create_window()) {
        MessageBoxA(NULL, "Could not create the window.", SIM_TITLE, MB_ICONERROR);
        return 1;
    }

    lv_init();

    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, SIM_W * DRAW_BUF_LINES);
    lv_disp_drv_init(&disp_drv);
    disp_drv.draw_buf = &draw_buf;
    disp_drv.flush_cb = flush_cb;
    disp_drv.hor_res  = SIM_W;
    disp_drv.ver_res  = SIM_H;
    lv_disp_drv_register(&disp_drv);

    lv_indev_drv_init(&indev_drv);
    indev_drv.type    = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = mouse_read_cb;
    lv_indev_drv_register(&indev_drv);

    ev_dash_create(&ev_ui);
    ev_data_init(&ev_ui);
    if (mode) ev_data_set_mode((ev_mode_t)mode);

    start = GetTickCount64();
    last  = start;

    while (g_running) {
        MSG msg;
        ULONGLONG now;

        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        now = GetTickCount64();
        if (now != last) {
            lv_tick_inc((uint32_t)(now - last));
            last = now;
        }
        lv_timer_handler();

        if (exit_after && (int)(now - start) >= exit_after) g_running = false;

        Sleep(1);
    }

    if (dump_path) {
        FILE *f = fopen(dump_path, "wb");
        if (f) {
            fwrite(g_pixels, 1, (size_t)SIM_W * SIM_H * 4, f);
            fclose(f);
        }
    }
    return 0;
}
