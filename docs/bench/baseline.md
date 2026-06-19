# Rendering benchmark — baseline (pre-optimization)

Captured on hardware (Sunton ESP32-4848S040) via `bench-sweep` over USB serial,
firmware env `esp32-4848s040-perf` (`-DDBG_PERF_COUNTERS=1`). 11 non-hidden
screens, 4 s dwell/cell (tick 1 discarded), typed render mode. Raw CSV (both
modes): `baseline.csv`. Re-run with `tools/bench_capture.py --cmd bench-sweep`.

## Metric meanings (this harness)

| Column | Meaning | "Good" direction |
|---|---|---|
| `build_us` | cold `build_fn()` — LVGL tree construction, once per screen | lower |
| `lvgl_peak_us` / `loop_peak_us` | **worst single `lv_timer_handler` / loop iteration in the cell. On a freshly-shown screen this is the first full-screen paint — the UI is frozen this long.** | lower |
| `first_frame_us` | screen-show → first flushed tile (time to first pixel) | lower |
| `refresh_us` | steady-state 5 Hz `ui_refresh` duration (in-place updates) | lower |
| `flush_avg_us` / `flush_peak_us` | per-flush CPU blit cost (`draw16bitRGBBitmap`) | lower |
| `fps` | flush events/s. **Low ≈ good here** (dirty-cache means static screens emit ~0 flushes); not a smoothness score. | n/a |

## Baseline — typed mode (the "first load, latency, FPS, update" table)

| screen | cold build (ms) | **first paint stall (ms)** | first pixel (ms) | steady refresh (µs) | flush avg/peak (µs) |
|---|--:|--:|--:|--:|--:|
| dashboard¹ | 0 (pre-built) | 150 | (900¹) | 850 | 2457 / 4083 |
| wind | 52 | **1486** | 122 | 1235 | 1056 / 2491 |
| wind_classic | 56 | **2449** | 357 | 5302 | 1388 / 3234 |
| wind_steer | 92 | 962 | 244 | **17090** | 2328 / 4464 |
| nav | 44 | 306 | 70 | 1914 | 2302 / 3570 |
| depth | 33 | 157 | 46 | 277 | 0 / 0² |
| steering | 47 | 179 | 54 | 652 | 0 / 0² |
| route | 31 | 150 | 42 | 182 | 0 / 0² |
| autopilot | 114 | 925 | 283 | 4342 | 1195 / 2493 |
| trip | 17 | 144 | 38 | 777 | 800 / 1111 |
| status | 25 | 189 | 53 | 1650 | 444 / 552 |

¹ dashboard is the active screen when the sweep starts, so its `show()` is a
no-op and `first_frame` (900 ms) is an artifact — its real warm first-pixel is
~5 ms (see `dashboard,store` row). Build is 0 because it was built at boot.

² fps/flush = 0 means the screen is fully static at steady state (nothing
redraws) — the dirty-cache working as intended. Good, not a stall.

## Findings

1. **First-load cost is dominated by the first full-screen paint, not
   construction.** `build_fn()` is cheap everywhere (17–114 ms). The expensive
   part is the first render: **wind_classic freezes the UI for ~2.45 s, wind
   ~1.49 s, autopilot ~0.93 s** in a single blocking `lv_timer_handler`. This is
   the number to beat for "first load."

2. **The first-paint stall is a rendering cost** (software-rasterize the whole
   480×480 once + CPU-blit every tile through the all-PSRAM pixel path:
   render→PSRAM, read→PSRAM, write→framebuffer-PSRAM). The compass/wind screens
   (arcs, 36 ticks, rotated ARGB marker canvases, boat-hull polyline) are the
   heaviest.

3. **Steady-state is mostly healthy** thanks to dirty-caching — except
   **wind_steer's 17 ms refresh** (heaviest live update; compass-scale rotation
   + marker ring) and wind_classic's 5.3 ms.

4. **flush peak ≈ 2.5–4.5 ms** per partial tile across active screens — the CPU
   blit through PSRAM, paid on every changed region.

## Targets these numbers will measure

- Removing the flush copy (DIRECT render into the panel framebuffer) → should
  cut `first paint stall`, `flush_avg/peak`.
- Internal-SRAM draw buffer → faster render-write (watch the memory-trap:
  internal SRAM starves WiFi/BLE).
- Rotation quantization / single-image bezel → `refresh_us` (esp. wind_steer)
  and steady flush volume.
