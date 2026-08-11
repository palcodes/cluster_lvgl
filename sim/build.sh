#!/usr/bin/env bash
# Build the Windows simulator.  Run from anywhere:
#   LVGL_DIR=../lvgl_src bash sim/build.sh
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
LVGL_DIR="${LVGL_DIR:-$ROOT/../lvgl_src}"
OUT="${OUT:-$ROOT/build}"

if [ ! -f "$LVGL_DIR/lvgl.h" ]; then
    echo "LVGL not found at $LVGL_DIR"
    echo "  git clone --depth 1 -b release/v8.3 https://github.com/lvgl/lvgl.git \"$LVGL_DIR\""
    exit 1
fi

mkdir -p "$OUT"
find "$LVGL_DIR/src" -name '*.c' > "$OUT/lvgl_srcs.txt"

gcc -O2 -Wall -Wextra -DLV_CONF_INCLUDE_SIMPLE \
    -I"$ROOT/sim" -I"$LVGL_DIR" -I"$ROOT/ui" -I"$ROOT/app" \
    "$ROOT/sim/main_win32.c" \
    "$ROOT/ui/ui_dash.c" "$ROOT/ui/ui_events.c" "$ROOT/app/ev_data.c" \
    "$ROOT"/fonts/*.c \
    "@$OUT/lvgl_srcs.txt" \
    -lgdi32 -luser32 -mwindows \
    -o "$OUT/ev_cluster.exe"

echo "-> $OUT/ev_cluster.exe"
