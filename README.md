# Cluster — Dashboard v2

An 800 × 480 instrument cluster for an electric two-wheeler, built in LVGL
9.4 from the Figma frame **Dashboard - v2**
([node 1:2](https://www.figma.com/design/u7aoLFxLjCtp1zq2fgMUdD/randoms?node-id=1-2&m=dev)).

![STREET](docs/preview-street.png)

That is a real frame rendered from this code, not the design. It differs
from the Figma export on **3.29 %** of pixels by more than 8/255, and
essentially all of that is glyph antialiasing — every element's ink lands
within one pixel of where the frame puts it. `test/compare_to_figma.py`
prints the table.

| | |
|---|---|
| **Target** | LVGL 9.4, 800 × 480 design on a 854 × 480 canvas |
| **Panel** | ST7701, 480 × 854 over MIPI DSI, RGB565, rotated to landscape |
| **Type** | Bai Jamjuree Medium + Jura SemiBold/Bold (both SIL OFL) |
| **Flash** | 131 KB of assets — 79 KB type, 52 KB images |
| **RAM** | 384 KB for the backdrop, plus LVGL's own heap |

No animations yet, by request.

---

## What's on screen

| Region | Shows |
|---|---|
| Status strip | Clock, state of charge, battery, navigation and Bluetooth lamps, network |
| Top left | Warning lamp with its halo, low-beam headlamp, the node 1:6 tell-tale |
| Centre | Turn-by-turn disc, street and distance; the speed numeral; both turn indicators |
| Left column | Ride-mode selector — the active mode in a glowing pill, the one either side of it above and below |
| Right column | Odometer and the range panel |
| Bottom | The AUTO BALANCE curve, its marker and the hint line |

---

## Files

```
ui/
  cl_theme.h      geometry and colour, every constant tagged with its node id
  cl_fonts.h/.c   the ten faces, and the leading each one needs
  cl_icons.h      declarations for the generated image data
  cl_pool.h/.c    the backdrop gradient, expanded at start-up
  cl_screen.h/.c  the screen
app/
  cluster_data.h/.c   the vehicle boundary: cluster_set_*()
fonts/
  cl_font_*.c     generated faces
  generate.sh     refetch and rebuild them
icons/
  svg/            the assets Figma exported — the source of truth
  cl_icon_*.c     rasterised alpha maps
  cl_glow_pill.c  cl_shade_panel.c  cl_pool_src.c   computed alpha maps
  generate.py     rasteriser and gradient baker
sim/
  main_win32.c    Win32 + GDI runner, no SDL and no dependencies
  lv_conf.h       PC config, 32-bit colour
  build.sh        build via git bash / MSYS2
  build.bat       build via cmd
test/
  render_preview.c      headless render to a raw framebuffer
  build_preview.sh      build and run it, then convert to PNG
  raw_to_png.py         framebuffer to PNG, 16 or 32 bit
  compare_to_figma.py   measure a render against the design
mcux/
  lv_conf.h       RGB565 preview config - NOT the one the board builds with
  app_cluster.h/.c    the three-call seam into an SDK project
  README.md       hardware bring-up
archive/
  the previous cluster, kept whole
```

`ui/` knows nothing about where numbers come from and `app/` knows nothing
about how they are drawn. Swapping the seeded values for a real CAN feed
touches one file.

---

## Running it

### Headless — the one to reach for

Opens no window, renders to memory, writes a PNG. This is what the design
was checked against.

```bash
bash test/build_preview.sh
```

LVGL comes from `../gui/lvgl` by default - the copy inside the MCUXpresso
project, so the preview is built against the exact LVGL the board runs. Set
`LVGL_DIR` to point elsewhere.

Then:

```bash
python test/compare_to_figma.py
```

which prints every element's offset from the frame, samples the soft
passages value by value, and writes `build/compare.png` and `build/diff.png`.

LVGL is compiled once into `build/liblvgl-sim.a` and reused, so after the
first run the edit-render-compare loop is a few seconds.

Arguments are milliseconds to run and ride mode (`0` CRAWL, `1` STREET,
`2` RUSH). To render what the panel will actually show:

```bash
CONF_DIR=mcux OUT=build565 CL_SCREEN=854x480 bash test/build_preview.sh
```

`CL_SCREEN=WxH` renders the panel canvas rather than the design frame on its
own: the design stays 800 × 480 and is centred in it. Without it the preview
is 800 × 480, which is what `compare_to_figma.py` expects.

`test/raw_to_png.py` detects RGB565 from the dump size.

![RGB565](docs/preview-rgb565.png)

A real 16-bit framebuffer dump at the panel's full 854 × 480 canvas - the
design centred, 27 px of bezel either side. The pool banding is 16-bit
colour on a smooth dark gradient and is inherent to the panel format.

### On Windows

Plain Win32 + GDI — nothing to install but a compiler. Get MinGW-w64
(MSYS2: `pacman -S mingw-w64-x86_64-gcc`, then put `C:\msys64\mingw64\bin`
on PATH), then:

```bash
bash sim/build.sh && ./build/cluster.exe
```

(LVGL again comes from `../gui/lvgl`; without that tree,
`git clone --depth 1 -b release/v9.4 https://github.com/lvgl/lvgl.git ../lvgl_src`
and set `LVGL_DIR`.)

or `sim\build.bat` from cmd. A real 800 × 480 window opens.

| | |
|---|---|
| `1` `2` `3` | CRAWL / STREET / RUSH |
| `Esc` | quit |
| `--mode 2` | start in RUSH |
| `--exit-after 6000` | quit after 6 s |
| `--dump frame.bin` | write the final framebuffer |

> The simulator compiles clean but was never executed here — this machine's
> Application Control policy blocks running a freshly built GUI binary. The
> headless preview, which is a console binary and exercises the same UI
> code, runs fine and is what every figure in this README comes from.

### On i.MX RT1170

Done — the UI is integrated into the MCUXpresso project in `../gui`, which
drives an **ST7701 panel, 480 × 854 over MIPI DSI**, rotated to landscape.
See **[mcux/README.md](mcux/README.md)** for what was changed there and what
to check on the bench.

The seam into any SDK project is still three calls:

```c
#include "app_cluster.h"

app_cluster_init();   /* instead of lv_demo_widgets()  */
app_cluster_poll();   /* instead of lv_timer_handler() */
```

---

## Live data

All in `app/cluster_data.h`. Integer only — no floating point in the update
path, so it works unchanged on a part without an FPU.

```c
cluster_set_speed(56);                            /* km/h                  */
cluster_set_mode(CL_MODE_STREET);
cluster_set_odometer(8999999u);
cluster_set_range(72);                            /* km                    */
cluster_set_soc(89);                              /* percent               */
cluster_set_nav("CARTER ROAD", 200, CL_TURN_LEFT);
cluster_set_turn_signals(true, true);
cluster_set_warning(true);
cluster_set_beam(true);
cluster_set_clock(17, 35);
cluster_set_network("4G");
cluster_set_bluetooth(true);
cluster_set_auto_balance(false);
```

`cluster_data_init()` seeds every one of these with the value the frame
shows, which is what makes a freshly built screen comparable against the
design pixel for pixel. There is no demo loop to remove.

Guidance and tell-tales hide rather than blank, so nothing reflows around
them. `cluster_set_nav()` switches from metres to `x.y km` past a
kilometre, and a NULL street clears the whole guidance group.

The setters deliberately do **not** invent state colours — no amber battery
below 20 %, no red below 10 %. The frame specifies one appearance and this
reproduces it; conditional styling belongs wherever the thresholds are
decided, hooked in through these same functions.

---

## Ride modes

![CRAWL](docs/preview-crawl.png)
![RUSH](docs/preview-rush.png)

The selector is a rotary: the active mode sits in the pill with its
neighbours above and below, wrapping at the ends. `cl_screen_set_mode()`
rewrites the three labels; the pill itself never moves.

---

## Three things LVGL cannot do directly

The frame asks for three effects with no LVGL equivalent. Each is handled
where it appears in the code, and each is worth knowing about before
changing it.

**The backdrop gradient** (node 1:3) is radial, and LVGL has no radial
gradient that matches it. Stacked shadows were the obvious workaround and
they do not work: LVGL's shadow is a box blur bounded by `shadow_width` that
allocates `(shadow_width + radius)² × 2` bytes to draw, so a feather wide
enough to cross the frame costs megabytes and every affordable version leaves
its own rectangle showing as a hard step. A small alpha map drawn zoomed is
not the answer either — LVGL 8.3 refused outright, and while LVGL 9 will
scale an A8 source, a 4× transform of a full-screen image every refresh is a
lot of work to save 384 KB, and it resamples the gradient differently from
the frame.

So Figma's gradient is *evaluated* rather than approximated. `generate.py`
inverts the node's own `gradientTransform`, interpolates its two stops, and
writes a quarter-scale alpha map that `cl_pool.c` expands once at start-up.
That is 24 KB of flash, 384 KB of RAM, no per-frame cost, and within three
counts per channel of the frame. If RAM is the tighter budget, have
`generate.py` emit the map at full scale and point `cl_pool` straight at it
— 384 KB of flash instead.

**The two inset shadows** — the mode pill's blue inner glow (node 1:32) and
the shade across the top of the range panel (node 1:45) — cast inwards, and
LVGL's shadows only cast out. Both are baked to alpha maps and recoloured at
draw time. Their parameters are *not* the ones the node exports: Figma's
inner shadow is not the CSS formula its export writes down, so both were
fitted by least squares against the rendered frame. The panel's fitted
amplitude came out at exactly the 0.50 the node states, which is a good sign
the fit is finding the real thing.

**The stadium outline** (node 1:3's top stroke) runs all the way round both
end caps. `LV_BORDER_SIDE_TOP` cannot draw that — it pushes the inner
rectangle outwards on the three unset sides, which leaves the curved part
outside the border band and squares the shape off. Two arcs and a hairline
give the real geometry.

---

## Regenerating the assets

```bash
bash fonts/generate.sh
```

Fetches Bai Jamjuree and Jura from Google Fonts and re-runs `lv_font_conv`
via `npx`, so nothing is installed globally. Faces are subsetted to what
they actually render — captions carry only their own letters, anything
showing live data carries printable ASCII.

```bash
python icons/generate.py
```

Rasterises `icons/svg/` and recomputes the three baked maps. No
dependencies beyond the standard library: it parses the SVG paths, flattens
the curves and scanline-fills them at 4× supersampling with the nonzero
winding rule. Re-export a node from Figma, drop the file in `icons/svg/`,
re-run it.

If you regenerate a font at a different size, re-measure its leading —
`cl_fonts.c` carries the gap between where Figma puts a text node and where
LVGL puts a label, and `compare_to_figma.py` reports it.

---

## Tuning

**Colour and geometry** are all in `ui/cl_theme.h`, every constant tagged
with the node it came from. Coordinates are the node's own x/y; the two
places where Figma's numbers and its render disagree are called out in
comments, both caused by centre-aligned strokes.

**Memory.** The largest single allocation is the glow dots' shadow at about
20 KB — `(shadow_width + radius)² × 2`. Undersize `LV_MEM_SIZE` and LVGL
hangs rather than reporting an error, so change it with the headless preview
in hand; the board's is 384 KB and has room to spare. The 384 KB backdrop
buffer is a static array and does not come out of `LV_MEM_SIZE`.

**Redraw cost.** Nothing here is expensive: the backdrop is an untransformed
alpha blit, the two rings and the AUTO BALANCE curve are thin strokes, and
the only shadows left are the two glow dots and the warning halo. Setting
`shadow_width` to 0 on those three is the one lever, and it is style-only.

**Another resolution.** Positions are absolute, inside a container the size
of the design. A panel *larger* than the frame needs nothing but
`CL_PANEL_W`/`CL_PANEL_H`, which centre the design in it — that is how the
854-wide ST7701 is handled. To actually redraw at another size, `CL_W`/`CL_H`
and the geometry block in `cl_theme.h` are where to start — and `generate.py` needs
its `POOL_*` constants moved with them, since the gradient is baked against
the 800 × 480 frame.
