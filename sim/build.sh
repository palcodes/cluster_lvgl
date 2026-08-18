#!/usr/bin/env bash
# Build the Windows simulator.  Run from anywhere:
#   bash sim/build.sh
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
LVGL_DIR="${LVGL_DIR:-$ROOT/../gui/lvgl}"
OUT="${OUT:-$ROOT/build}"

if [ ! -f "$LVGL_DIR/lvgl.h" ]; then
    echo "LVGL not found at $LVGL_DIR"
    echo "  git clone --depth 1 -b release/v9.4 https://github.com/lvgl/lvgl.git \"$LVGL_DIR\""
    exit 1
fi

mkdir -p "$OUT"

# gcc.exe is a native Windows binary: it parses @response-file contents
# itself, so the automatic POSIX->Windows translation bash applies to
# ordinary command-line arguments never touches this file.  A plain
# `find ... > file` here would leave /c/... paths that gcc.exe cannot open.
#
# Rewriting to C:/... (drive letter, forward slashes) rather than using
# `cygpath -w` is deliberate: cygpath emits backslashes, and gcc's
# response-file parser treats backslash as an escape character, which
# silently eats them (C:\Users\... becomes C:UsersDocuments...).  Forward
# slashes are valid in Win32 paths and gcc reads them correctly.
find "$LVGL_DIR/src" -name '*.c' | sed -E 's#^/([a-zA-Z])/#\1:/#' > "$OUT/lvgl_srcs.txt"

gcc -O2 -Wall -Wextra -DLV_CONF_INCLUDE_SIMPLE \
    -I"$ROOT/sim" -I"$LVGL_DIR" -I"$ROOT/ui" -I"$ROOT/app" \
    "$ROOT/sim/main_win32.c" \
    "$ROOT/ui/cl_screen.c" "$ROOT/ui/cl_fonts.c" "$ROOT/ui/cl_pool.c" \
    "$ROOT/app/cluster_data.c" \
    "$ROOT"/fonts/cl_font_*.c "$ROOT"/icons/cl_*.c \
    "@$OUT/lvgl_srcs.txt" \
    -lgdi32 -luser32 -mwindows \
    -o "$OUT/cluster.exe"

echo "-> $OUT/cluster.exe"
