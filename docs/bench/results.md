# Rendering optimization results

Measured on hardware (Sunton ESP32-4848S040) with the `bench-sweep` harness,
env `esp32-4848s040-perf`, typed render mode. Per-screen CSVs:
`baseline.csv`, `opt1-quantize.csv`, `opt2-direct.csv`.

Stages:
- **baseline** — original render path (partial buffers + per-tile CPU blit).
- **opt1** — rotations quantized to whole degrees (`set_rot_if_changed`).
- **opt2** — DIRECT render into the RGB panel framebuffer (blit eliminated).

## First-paint stall — UI frozen on first show (lower is better)

The dominant "first load" cost: a single blocking `lv_timer_handler` that
paints the whole screen once.

| screen | baseline | opt1 | opt2 | Δ base→opt2 |
|---|--:|--:|--:|--:|
| wind_classic | 2.45 s | 2.40 s | **0.88 s** | **−64%** |
| wind | 1.49 s | 1.50 s | **0.50 s** | **−66%** |
| autopilot | 0.92 s | 0.90 s | 0.94 s | +2% |
| wind_steer | 0.96 s | 0.85 s | 0.95 s | −1% |
| nav | 0.31 s | 0.30 s | **0.19 s** | −38% |
| depth | 0.16 s | 0.15 s | 0.10 s | −34% |
| route | 0.15 s | 0.16 s | 0.10 s | −33% |
| status | 0.19 s | 0.21 s | 0.14 s | −27% |
| trip | 0.14 s | 0.16 s | 0.11 s | −26% |
| dashboard | 0.15 s | 0.15 s | 0.14 s | −10% |
| steering | 0.18 s | 0.18 s | 0.27 s | +49%¹ |

¹ steering/autopilot/wind_steer are *render-bound* (arcs, marker rings, compass
labels) — their first-paint is the SW rasterizer, not the blit, so DIRECT
doesn't move them. Light-screen ±deltas are partly run-to-run noise
(`lvgl_peak` is a single worst-iteration sample over 3 ticks).

## Flush blit cost — avg µs per flushed tile (lower is better)

| screen | baseline | opt1 | opt2 | speedup |
|---|--:|--:|--:|--:|
| wind_steer | 2328 | 2290 | **183** | 12.7× |
| wind_classic | 1388 | 1375 | **253** | 5.5× |
| nav | 2302 | 2123 | **517** | 4.5× |
| dashboard | 2457 | 2355 | **541** | 4.5× |
| wind | 1056 | 1030 | **277** | 3.8× |
| autopilot | 1195 | 2326 | **327** | 3.7× |
| trip | 800 | 746 | **279** | 2.9× |
| status | 444 | 538 | **183** | 2.4× |

## Steady-state refresh — 5 Hz update cost (opt1's target)

| screen | baseline | opt1 | note |
|---|--:|--:|---|
| wind_steer | 17.1 ms | **7.3 ms** | −57%, rotation quantization |
| wind_classic | 5.3 ms | **2.4 ms** | −55% |

## Summary

- **opt2 (DIRECT) is the big win:** eliminates the per-tile CPU blit (flush
  cost 2.4–12.7× lower) and cuts the worst first-paint stalls by ~⅔
  (wind_classic 2.45→0.88 s, wind 1.49→0.50 s).
- **opt1 (quantize)** halves steady refresh on the rotation-heavy compass
  screens; no effect on first-paint (by design).
- **Remaining bottleneck is render-bound:** after the blit is gone, the
  ~0.9 s first-paint on autopilot/wind_steer is pure SW rasterization of arcs +
  marker rings + compass labels. The next lever is reducing that draw work
  (e.g. collapsing the static compass rose into one pre-rendered image).

### Note on opt3 (internal-SRAM draw buffer)

The originally-planned third lever — moving the LVGL draw buffer to internal
SRAM for a faster render-write — is **architecturally mooted by opt2**: DIRECT
mode has no separate draw buffer (LVGL renders into the PSRAM framebuffer
directly). A partial-mode internal-SRAM buffer would also be *slower* than
DIRECT here (it reintroduces the blit: render→SRAM, then SRAM-read→PSRAM-write),
so it is not pursued as a cumulative step.

## Flicker + the double-buffering investigation

DIRECT mode (opt2) was confirmed on hardware to **flicker heavily** on
steering / full-screen repaints: a single scanout framebuffer is rendered
*progressively* (background, then glyphs/arcs), and the LCD DMA scans those
intermediate states. The shipped default reverts to buffered **PARTIAL** mode
(complete tiles blitted atomically → no flicker), keeping the quantize + task
wins. DIRECT remains opt-in behind `-DRENDER_DIRECT_FB`.

The "fast **and** smooth" path is panel **double-buffering** (two framebuffers,
vsync page-flip): LVGL renders the back buffer, the panel swaps to it on vsync,
so the scanout always shows a complete frame — no flicker **and** no blit.

**Blocked on this platform.** It needs `esp_lcd_rgb_panel_config_t.num_fbs = 2`
+ `esp_lcd_rgb_panel_get_frame_buffer()`, which **only exist in esp-idf ≥ 5.0**.
This firmware is on **arduino-esp32 2.x / esp-idf 4.4.7** (the active RGB header
has no `num_fbs`/`double_fb`/`get_frame_buffer`), and the IDF 5.x bump is the
explicitly-parked **spec 21 A** migration (~5-7 days; NimBLE-Arduino 1.4
`esp_bt.h` incompatibility + 36 stricter format-string sites — see
`platformio.ini` header). A working `num_fbs=2` + vsync-flip implementation was
drafted and validated against the idf-5.1 API, then reverted because it cannot
compile on 4.4; fold it into spec 21 A.

LVGL is already a retained-mode compositor doing partial/delta redraws, so a
custom "composer" is **not** the answer — the lever is frame *presentation*
(single vs double buffer), which is the idf-5.x dependency above.

### Achievable now (idf 4.4), no flicker

The remaining first-paint cost is render-bound. The lever that works today is
reducing draw work — e.g. collapsing the static compass rose (36 ticks +
cardinals) into one pre-rendered image so first-paint blits one bitmap instead
of rasterizing 44 vector objects. Cuts first-paint on autopilot/wind_steer/
wind_classic without touching the render-mode/flicker tradeoff.
