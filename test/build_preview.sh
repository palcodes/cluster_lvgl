#!/usr/bin/env bash
# Build and run the headless preview on a PC.
#
#   LVGL_DIR=../lvgl_src bash test/build_preview.sh [run_ms] [mode]
#
# mode: 0 = CRAWL, 1 = STREET, 2 = RUSH
#
# LVGL itself is compiled once into build/liblvgl-<conf>.a and reused, so
# only the cluster's own handful of files are rebuilt on a normal run - the
# difference between a four minute edit-render loop and a five second one.
# Delete the archive, or touch lv_conf.h, to force it to be rebuilt.
#
# To render what the panel will actually show rather than the 32-bit desktop
# version, point it at the RT1170 config - raw_to_png.py works out the depth
# from the dump size:
#
#   CONF_DIR=mcux OUT=build565 LVGL_DIR=../lvgl_src bash test/build_preview.sh
#
# Needs: gcc (MSYS2 mingw64 works), python3, and an LVGL 8.3 checkout.

set -e

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
LVGL_DIR="${LVGL_DIR:-$ROOT/../lvgl_src}"
CONF_DIR="${CONF_DIR:-$ROOT/sim}"
OUT="${OUT:-$ROOT/build}"
RUN_MS="${1:-500}"
MODE="${2:-1}"

if [ ! -f "$LVGL_DIR/lvgl.h" ]; then
    echo "LVGL not found at $LVGL_DIR"
    echo "  git clone --depth 1 -b release/v8.3 https://github.com/lvgl/lvgl.git \"$LVGL_DIR\""
    exit 1
fi

mkdir -p "$OUT"
LIB="$OUT/liblvgl-$(basename "$CONF_DIR").a"

if [ ! -f "$LIB" ] || [ "$CONF_DIR/lv_conf.h" -nt "$LIB" ]; then
    echo "building LVGL into $(basename "$LIB") (once)"
    OBJ="$OUT/lvgl-obj-$(basename "$CONF_DIR")"
    rm -rf "$OBJ"; mkdir -p "$OBJ"
    n=0
    for c in $(find "$LVGL_DIR/src" -name '*.c'); do
        gcc -O1 -DLV_CONF_INCLUDE_SIMPLE -I"$CONF_DIR" -I"$LVGL_DIR" \
            -c "$c" -o "$OBJ/$(printf '%04d' $n).o"
        n=$((n + 1))
    done
    ar rcs "$LIB" "$OBJ"/*.o
    rm -rf "$OBJ"
fi

gcc -O1 -Wall -Wextra -DLV_CONF_INCLUDE_SIMPLE \
    -I"$CONF_DIR" -I"$LVGL_DIR" -I"$ROOT/ui" -I"$ROOT/app" \
    "$ROOT/test/render_preview.c" \
    "$ROOT/ui/cl_screen.c" \
    "$ROOT/ui/cl_fonts.c" \
    "$ROOT/ui/cl_pool.c" \
    "$ROOT/app/cluster_data.c" \
    "$ROOT"/fonts/cl_font_*.c \
    "$ROOT"/icons/cl_*.c \
    "$LIB" \
    -o "$OUT/render_preview"

"$OUT/render_preview" "$RUN_MS" "$OUT/frame.bin" "$MODE"
python "$ROOT/test/raw_to_png.py" "$OUT/frame.bin" "$OUT/preview.png"
echo "-> $OUT/preview.png"
