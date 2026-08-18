# Running on i.MX RT1170 (MCUXpresso)

Target: **MIMXRT1176-EVKB**, **ST7701 panel, 480 x 854 over MIPI DSI**, RGB565,
double-buffered, rotated to landscape. **LVGL 9.4**, the version the SDK
project ships.

The integration into `../gui` is done. This document says what was changed
there, what to check on the bench, and what is still unverified.

## Read this first

**Verified here**

- The whole UI compiles clean against LVGL 9.4 and renders correctly - the
  previews in `docs/` are real frames from this code, and
  `test/compare_to_figma.py` puts every element within a pixel of the Figma
  frame (3.29 % of pixels differ by more than 8/255, essentially all of it
  glyph antialiasing).
- It also compiles clean against the **MCUXpresso project's own LVGL
  configuration** - the Kconfig-generated one, VG-Lite and PXP enabled,
  `LV_COLOR_DEPTH 16` - not just against `sim/lv_conf.h`. From the `gui`
  directory:

  ```bash
  gcc -fsyntax-only -DLV_CONF_INCLUDE_SIMPLE=1 -DCL_PANEL_W=854 -DCL_PANEL_H=480 -imacros source/mcux_config.h -I source -I lvgl -I lvgl/src -I source/ui -I source/app -I source/mcux -I vglite/inc source/ui/*.c source/app/*.c source/mcux/app_cluster.c source/fonts/cl_font_*.c source/icons/cl_*.c
  ```

- No Montserrat dependency in the cluster itself: every face is ours and each
  label sets its own. The SDK config still has Montserrat 14 as
  `LV_FONT_DEFAULT`, which costs flash and nothing else.

**Not verified**

- No `arm-none-eabi-gcc` on this machine, so nothing here has been built with
  the ARM toolchain or run on silicon. The board bring-up is the SDK's,
  unchanged, apart from the edits listed below.

## The panel

The display is 480 x 854 **portrait**; the cluster is an 800 x 480
**landscape** design. Two things bridge that:

**Rotation.** `DEMO_USE_ROTATE` makes `lvgl_support.c` hand LVGL a canvas of
the panel on its side - 854 x 480 - and rotate each rendered frame into the
portrait frame buffer through the PXP. That path was already in the SDK port;
it was just switched off.

**The extra 54 px.** 854 is 54 wider than the design. `cl_screen_create()`
puts the whole 800 x 480 frame in a container inset by `CL_ORIGIN_X`, so the
design is centred with 27 px of black either side and every Figma coordinate
stays exactly where it was. `CL_PANEL_W` / `CL_PANEL_H` come from the
project's defined symbols; undefined, the screen is the design frame, which
is why the desktop preview still renders 800 x 480 for comparison against
Figma.

To re-lay-out for the full 854 instead, `CL_W` and the geometry block in
`ui/cl_theme.h` are where to start - and `icons/generate.py` needs its
`POOL_*` constants moved with them, since the backdrop gradient is baked
against the 800 x 480 frame.

## What was changed in `gui`

| File | Change |
|---|---|
| `source/{ui,app,fonts,icons}/`, `source/mcux/app_cluster.[ch]` | the ported UI, copied in |
| `source/mcux/lv_conf.h` | **deleted** - see below |
| `.cproject` | include paths repointed at `source/ui`, `source/app`, `source/mcux`, `source/fonts`, `source/icons` - they pointed at the project root, where those folders do not exist; `CL_PANEL_W=854` and `CL_PANEL_H=480` added to the defined symbols of both configurations |
| `source/mcux_config.h` | `DEMO_USE_ROTATE 1` |
| `source/lvgl_support.c` | `DEMO_LVGL_BUF_SIZE` added and used to size the rotate buffer - see below |
| `source/lvgl_demo_benchmark.c` | `lv_demo_benchmark()` -> `app_cluster_init()` |
| `source/mcux/app_cluster.c` | idle return of `lv_timer_handler()` capped |

### Why `mcux/lv_conf.h` is not shipped into the project

The MCUXpresso build gets its LVGL configuration from Kconfig:
`source/mcux_config.h` (force-included with `-imacros`) carries the
`CONFIG_LV_*` values, and `source/lv_conf.h` only derives `LV_COLOR_DEPTH`
from the frame buffer format. The project also builds with
`LV_CONF_INCLUDE_SIMPLE`, so **a second `lv_conf.h` anywhere on the include
path can shadow the generated one**. `source/mcux` is on the include path for
`app_cluster.h`, so the copy that used to sit there was removed.

The one in this repo is for the RGB565 *preview* only. If a setting has to
change on the target, change it in `source/mcux_config.h`.

### The rotated buffer was undersized

`DEMO_FB_SIZE` is `stride(480) x 854 = 819,840` bytes. Rotated, LVGL renders
854 px wide, and `854 x 2 = 1708` rounds up to a 1728-byte stride - so it
writes `1728 x 480 = 829,440` bytes into a buffer declared 9,600 bytes
smaller. `DEMO_LVGL_BUF_SIZE` sizes that buffer from the rotated geometry
instead. Worth knowing if the panel size ever changes: the two orientations
do not have the same stride.

### Why the poll return is capped

LVGL 9's `lv_timer_handler()` returns `LV_NO_TIMER_READY` (0xFFFFFFFF) when
nothing is due, which this screen reaches as soon as it settles - nothing
here animates. The service loop does `vTaskDelay(app_cluster_poll())`, which
would then park the task for 49 days and the panel would never pick up the
next `cluster_set_*()`. `app_cluster_poll()` caps it at 30 ms.

## Memory

| | |
|---|---|
| Frame buffers | 2 x 480 x 854 x 2 B = **1.56 MB** -> `NCACHE_REGION` (16 MB) |
| LVGL rotate buffer | 1728 x 480 = **810 KB** -> `NCACHE_REGION` |
| Backdrop buffer | **384 KB** `.bss` -> `BOARD_SDRAM` (48 MB, the default data region) |
| LVGL heap | 384 KB (`CONFIG_LV_MEM_SIZE_KILOBYTES`); the screen's largest single allocation is a glow dot's shadow at about 20 KB |
| Type | **79 KB** flash, all ten faces |
| Images | **52 KB** flash - icons, two baked shadows, the gradient |

Nothing here is tight, and no linker changes were needed.

## On the bench, in this order

**1. Flash it.** The stock demo is gone - `AppTask()` now calls
`app_cluster_init()` and services `app_cluster_poll()`. If the panel comes up
black, the display half is unchanged from the example that worked, so suspect
the UI half.

**2. Check which way up it is.** `DEMO_FlushDisplay()` in
`source/lvgl_support.c` rotates with `LV_DISPLAY_ROTATION_90`. If the cluster
is upside down, that is `LV_DISPLAY_ROTATION_270` - the one thing here that
cannot be worked out without seeing the panel. Both branches, PXP and the
software fallback, carry the constant; change both.

**3. Check the icons and the backdrop.** `CONFIG_LV_USE_DRAW_VG_LITE 1` puts
the GPU in the draw path. The icons are A8 alpha maps recoloured at draw
time, and the backdrop is one full-screen A8 blit; LVGL falls back to
software for anything the VG-Lite unit declines, so this *should* be
invisible. If tell-tales come out the wrong colour or the gradient is wrong,
set `CONFIG_LV_USE_DRAW_VG_LITE 0` in `source/mcux_config.h` to confirm it is
the GPU path before chasing anything else.

**4. Touch.** The GT911 is initialised for the MIPI panel and failure is not
fatal - `DEMO_InitTouch()` prints and returns false, and no input device is
registered. The cluster has nothing clickable, so it does not care either
way.

## Live data

`app/cluster_data.h`. Integer only, no floating point in the update path.

```c
cluster_set_speed(56);
cluster_set_mode(CL_MODE_STREET);
cluster_set_soc(89);
cluster_set_nav("CARTER ROAD", 200, CL_TURN_LEFT);
```

`cluster_data_init()` seeds every value with what the frame shows, which is
what makes a freshly flashed board comparable against the design. There is no
demo loop to remove. Call the setters from whatever task owns CAN or the BMS;
they touch LVGL objects directly, so keep them on the LVGL task or add the
usual lock if they have to run on another one.

## If the frame rate disappoints

Turn on `CONFIG_LV_USE_PERF_MONITOR` and get a number before changing
anything. This screen is cheap to draw - the backdrop is an untransformed
alpha blit and there is no animation - so the order is:

1. **The two glow dots and the warning halo**, the only real shadows left.
   `shadow_width` to 0 on those three in `ui/cl_screen.c` is style-only.
2. **The two 290 px rings and the AUTO BALANCE curve**, thin antialiased
   strokes. Cheap, but they cover a lot of area.
3. **The rotation itself.** Every frame is rotated by the PXP into the frame
   buffer. If that dominates, the answer is a panel scanned in landscape, not
   a cheaper UI.
