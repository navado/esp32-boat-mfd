#!/usr/bin/env bash
# Render the dashboard at every supported display class via the headless host
# LVGL harness (env:sim), writing BMP + PNG to docs/sim-shots/. LCD_W/LCD_H are
# compile-time, so each resolution is a separate build (PLATFORMIO_BUILD_FLAGS
# injects -DSIM_LCD_W/-DSIM_LCD_H).
#
# Usage: tools/sim_render.sh
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p docs/sim-shots

# "WxH" per supported display class (800x480 covers the 4.3/5/7" Waveshare).
RES=("480x480" "800x480" "1024x600")

for r in "${RES[@]}"; do
  w="${r%x*}"; h="${r#*x}"
  echo "=== rendering ${w}x${h} ==="
  PLATFORMIO_BUILD_FLAGS="-DSIM_LCD_W=${w} -DSIM_LCD_H=${h}" pio run -e sim >/dev/null
  bin=".pio/build/sim/program"
  "$bin" "docs/sim-shots/dash-${w}x${h}.bmp"
  if command -v sips >/dev/null 2>&1; then
    sips -s format png "docs/sim-shots/dash-${w}x${h}.bmp" \
      --out "docs/sim-shots/dash-${w}x${h}.png" >/dev/null
    rm -f "docs/sim-shots/dash-${w}x${h}.bmp"
  fi
done
echo "done -> docs/sim-shots/"
