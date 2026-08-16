#!/usr/bin/env bash
# Build and run the headless preview on a PC.
#
#   LVGL_DIR=/path/to/lvgl ./test/build_preview.sh [run_ms] [mode]
#
# mode: 0 = STREET, 1 = CRAWL, 2 = RUSH
# Needs: gcc (MSYS2 mingw64 works), python3, and an LVGL 8.3 checkout.

set -e

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
LVGL_DIR="${LVGL_DIR:-$ROOT/../lvgl_src}"
CONF_DIR="${CONF_DIR:-$ROOT/sim}"
OUT="${OUT:-$ROOT/build}"
RUN_MS="${1:-6000}"
MODE="${2:-0}"

if [ ! -f "$LVGL_DIR/lvgl.h" ]; then
    echo "LVGL not found at $LVGL_DIR"
    echo "  git clone --depth 1 -b release/v8.3 https://github.com/lvgl/lvgl.git \"$LVGL_DIR\""
    exit 1
fi

mkdir -p "$OUT"

# gcc.exe is a native Windows binary: it parses @response-file contents
# itself, so bash's automatic /c/... -> C:\... argument translation never
# touches this file. Rewriting to C:/... (forward slashes) rather than
# `cygpath -w` is deliberate: cygpath emits backslashes, and gcc's
# response-file parser treats backslash as an escape character, silently
# eating them. Forward slashes are valid in Win32 paths and gcc reads
# them correctly.
find "$LVGL_DIR/src" -name '*.c' | sed -E 's#^/([a-zA-Z])/#\1:/#' > "$OUT/lvgl_srcs.txt"

gcc -O1 -Wall -Wextra -DLV_CONF_INCLUDE_SIMPLE \
    -I"$CONF_DIR" -I"$LVGL_DIR" -I"$ROOT/ui" -I"$ROOT/app" \
    "$ROOT/test/render_preview.c" \
    "$ROOT/ui/ui_dash.c" \
    "$ROOT/ui/ui_events.c" \
    "$ROOT/app/ev_data.c" \
    "$ROOT"/fonts/*.c \
    "@$OUT/lvgl_srcs.txt" \
    -o "$OUT/render_preview"

"$OUT/render_preview" "$RUN_MS" "$OUT/frame.bin" "$MODE"
python "$ROOT/test/raw_to_png.py" "$OUT/frame.bin" "$OUT/preview.png"
echo "-> $OUT/preview.png"
