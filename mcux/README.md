# Running on i.MX RT1170 (MCUXpresso)

Target: **MIMXRT1170-EVKB**, 800 × 480, RGB565, double-buffered.

## Read this first

I could verify some of this and not all of it, so here is the split:

**Verified here**
- The whole UI compiles clean against `mcux/lv_conf.h` (`LV_COLOR_DEPTH 16`)
  and renders correctly in RGB565 — see `docs/preview-rgb565.png`, which is a
  real 16-bit framebuffer dump, not the 32-bit one converted.
- No Montserrat dependency: every font is ours, `LV_FONT_DEFAULT` points at
  `ev_font_ui_16`.
- Panel numbers below are read out of NXP's own `display_support.c/.h` for
  this board, not from memory.

**Not verified**
- I do not have the RT1170 SDK or the board, so nothing here has been
  compiled with `arm-none-eabi-gcc` or run on silicon. The board bring-up is
  the SDK's, unchanged — that is deliberate, and it is why this document
  tells you which example to start from instead of shipping you display
  driver code I cannot test.

## One thing to confirm before you order a panel

You picked *800 × 480 parallel RGB*. On the RT1170 EVKB the display output
that is actually wired up is **MIPI DSI** (the FPC connector), and the SDK's
800 × 480 option is a **Raspberry Pi 7" DSI panel**:

```c
#define DEMO_PANEL_RASPI_7INCH 3  /* 800 * 480, Raspberry Pi 7" */
```

So you have three routes:

1. **RPi 7" DSI panel** — 800 × 480, supported by the SDK today, zero driver
   work. Best way to get the cluster on glass this week.
2. **Parallel RGB panel** — RT1170 can drive parallel RGB through eLCDIF
   (`DEMO_DISPLAY_CONTROLLER_ELCDIF`), but you need a carrier that routes
   those pins; the EVKB does not bring them to a display connector. This is
   the right answer for production, more work for a bench bring-up.
3. **MIPI-to-RGB bridge** on your own carrier.

The UI does not care which you pick — it only needs an 800 × 480 framebuffer.

## Panel numbers (from the SDK, for the 800 × 480 option)

| | |
|---|---|
| Driver | `fsl_rpi.h` over MIPI DSI |
| Pixel clock | ~28 MHz (PLL_528, `div = 19`) |
| HSW / HFP / HBP | 20 / 70 / 23 |
| VSW / VFP / VBP | 2 / 7 / 21 |
| Format | `kVIDEO_PixelFormatRGB565`, 2 bytes/px |
| Buffers | 2 |

If you go the parallel-RGB route, these are the values to replace with your
panel's datasheet timings — the structure of `display_support.c` stays the
same, you add a panel case.

## Steps

**1. Get an SDK example that already boots LVGL.**

In the MCUXpresso SDK Builder select **MIMXRT1170-EVKB** and tick the
**LVGL** middleware, download, then import
`boards/evkbmimxrt1170/lvgl_examples/lvgl_demo_widgets_bm` (bare metal) or
`..._freertos`. Build and flash it *unmodified* first. Do not go further
until the stock demo is on the panel — that separates board problems from
UI problems.

> The `lvgl_examples` folder only exists in the SDK download, not in NXP's
> public GitHub mirror. If you cannot find it, `display_examples/fbdev_freertos`
> gets the panel up and you add LVGL on top.

**2. Point it at the 800 × 480 panel.**

In `display_support.h`, or as a preprocessor define in project settings:

```c
#define DEMO_PANEL DEMO_PANEL_RASPI_7INCH
```

Re-flash the stock demo and confirm it fills 800 × 480 correctly.

**3. Add the cluster sources.**

Copy `ui/`, `app/`, `fonts/` and `mcux/app_cluster.[ch]` into the project,
then add to the include paths:

```
ui, app, mcux
```

and to the sources: `ui/*.c`, `app/*.c`, `fonts/*.c`, `mcux/app_cluster.c`.

For the armgcc CMake build, add to `CMakeLists.txt`:

```cmake
target_sources(${MCUX_SDK_PROJECT_NAME} PRIVATE
    ${ProjDirPath}/../ui/ui_dash.c
    ${ProjDirPath}/../ui/ui_events.c
    ${ProjDirPath}/../app/ev_data.c
    ${ProjDirPath}/../mcux/app_cluster.c
)
file(GLOB EV_FONTS ${ProjDirPath}/../fonts/ev_font_*.c)
target_sources(${MCUX_SDK_PROJECT_NAME} PRIVATE ${EV_FONTS})
target_include_directories(${MCUX_SDK_PROJECT_NAME} PRIVATE
    ${ProjDirPath}/../ui ${ProjDirPath}/../app ${ProjDirPath}/../mcux)
```

**4. Replace `lv_conf.h`.**

Use `mcux/lv_conf.h`. It is the stock LVGL 8.3 template with these changes:

| Setting | Value | Why |
|---|---|---|
| `LV_COLOR_DEPTH` | 16 | matches the RGB565 framebuffer |
| `LV_MEM_SIZE` | 256 KB | **do not lower this** — see below |
| `LV_DISP_DEF_REFR_PERIOD` | 16 ms | the halo drift wants 60 Hz, not 33 |
| `LV_MEMCPY_MEMSET_STD` | 1 | newlib's are faster than LVGL's fallbacks |
| `LV_FONT_MONTSERRAT_*` | 0 | not used; saves the flash |
| `LV_FONT_CUSTOM_DECLARE` | `LV_FONT_DECLARE(ev_font_ui_16)` | |
| `LV_FONT_DEFAULT` | `&ev_font_ui_16` | |
| `LV_DRAW_COMPLEX` | 1 | **required** — shadows and arc anti-aliasing |

Keep whatever the SDK example set for `LV_TICK_CUSTOM` and any PXP/VGLite
options; those are board-side decisions and they were already correct.

### Why `LV_MEM_SIZE` is 256 KB

This one bit me while verifying, and it fails in a way that is genuinely
nasty to debug on hardware, so it is worth knowing.

LVGL 8.3 draws a box shadow by allocating a mask of
`(radius + shadow_width)²` **uint16** from the LVGL heap. The outer bloom is
a 292 px circle with a 96 px blur:

```
(146 + 96)² × 2 bytes = 114 KB   in a single allocation
```

I first set the heap to 96 KB. The allocation fails, `LV_USE_ASSERT_MALLOC`
is on, and LVGL's default assert handler is `while(1)` — so the board does
not log an error or draw a broken frame, it simply **hangs during the first
paint**, looking exactly like a display driver problem.

So: shadow blur costs heap *quadratically*. If you change `shadow_width` on
`glow_out` / `glow_in`, recompute. If you need the heap back, shrink the
blur rather than the heap — dropping `glow_out` to a 48 px blur takes the
allocation from 114 KB to 75 KB.

**5. Swap the demo for the cluster.**

In the example's `main()`, replace the demo call:

```c
- lv_demo_widgets();
+ app_cluster_init();
```

and in the service loop:

```c
- lv_timer_handler();
+ app_cluster_poll();
```

If the example does not already have a 1 kHz LVGL tick (i.e. `LV_TICK_CUSTOM`
is 0), call `app_cluster_tick_1ms()` from `SysTick_Handler()`.

That is the whole integration — three lines.

## Memory

| | |
|---|---|
| Framebuffers | 800 × 480 × 2 B × 2 = **1.5 MB** → SDRAM (EVKB has 64 MB) |
| LVGL heap | 96 KB (`LV_MEM_SIZE`) |
| Fonts | **62.6 KB** flash, all eight faces, measured from compiled objects |
| UI + data code | ~15 KB flash |

The SDK's `lv_port_disp.c` already puts the framebuffers in SDRAM and does
the D-cache clean before handing a buffer to the display controller. Do not
re-implement that — if you see tearing or stale rows, it is a cache
maintenance bug, and the SDK version is the known-good reference.

## If the frame rate disappoints

Turn on `LV_USE_PERF_MONITOR 1` first and get a number before changing
anything. The expensive parts, in order:

1. **Seven overlapping ~300 px arcs.** Drop `EV_ARC_SEGMENTS` from 6 to 1 in
   `ui/ui_theme.h` for a flat single-colour ring — that removes five of them.
2. **Two shadow blooms**, redrawn as the halo rotates under them. Set
   `shadow_width` to 0 on `glow_out` / `glow_in` in `ui/ui_dash.c`.
3. **Halo drift**, which invalidates the dial area continuously. Raise the
   periods in `ev_dash_halo_drift()`, or drop to one ring.

All three are style-only; no layout moves and nothing else has to change.

Note that PXP acceleration will not help much here — it accelerates blend and
fill, and this screen's cost is arc mask generation, which stays on the CPU
in LVGL 8.3.
