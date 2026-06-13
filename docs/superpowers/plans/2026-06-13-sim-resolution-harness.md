# Host LVGL-SDL Resolution Test Harness — Implementation Plan

> **For agentic workers:** execute task-by-task; each task builds + runs.
> SDL2 (2.32) and LVGL's SDL driver (`lv_sdl_window` / `lv_sdl_sw.c`) are
> present on the dev Mac, so the harness is buildable locally.

**Goal:** Render the real device layout code on the host through LVGL+SDL at
every supported display class, snapshot each to PNG, and assert no-overlap —
so layouts can be validated for all resolutions without per-board hardware.

**Why a host harness (not on-device):** the device builds at a single
compile-time `LCD_W/LCD_H`; the layout geometry (`QG_TILE_W`, wind `CX/CY/R_*`,
`QG_TOP_Y`) is `constexpr`. Rendering other resolutions means building the
layout code with different `LCD_W/H` — cheapest as a host build, one binary
per resolution (or one binary that re-execs per `-D`).

## Supported display classes (from plugin `lib/screen-presets.js`)
- `sunton-480` — 480×480 (square) — the shipped board
- `waveshare-4_3-800x480`, `waveshare-5-800x480`, `waveshare-7-800x480` — 800×480
- `waveshare-5-1024x600` — 1024×600
(800×480 appears 3×; render it once → 3 distinct resolutions total: 480×480,
800×480, 1024×600.)

## Dependency / stub surface (mapped from `src/ui/ui_layouts.cpp`)
`ui_layouts.cpp` must link; at runtime only the numeric/bar/compass/windRose
paint+update paths execute for a dashboard, but ALL referenced symbols must
resolve. External symbols it pulls in:

| Symbol | Host strategy |
|---|---|
| `heap_caps_calloc`, `MALLOC_CAP_INTERNAL/SPIRAM/DMA` | shim → `calloc`; caps macros → `0` (host header `sim/esp_heap_caps_shim.h`) |
| `sk::copyData(sk::Data&)` | stub fills a fixed demo snapshot (so widgets show real-ish values) |
| `net::logf(...)` / `net::logf_at(...)` | stub → `vprintf` or no-op |
| `ui::show_by_id(const char*)` | stub → no-op (tap nav not exercised) |
| `autopilot::*` (Mode/State/Result, copy/adjust/set/silence/backend/mode) | compile real `src/autopilot_pure.cpp` if host-safe; else stub the small API |
| `beeper::audible/set` | stub no-op |
| `app::*` (post/Command) | stub no-op (only `tile_zoom_cb` path) |
| `storage::Namespace` | stub if referenced at link |
| `board::chipTempC` etc. | stub returns NAN |
| `set_text_if_changed` (`ui_dirty.h`) | header-inline → no link needed (verify) |

TUs to compile: `ui_layouts.cpp`, `ui/ui_theme.cpp`, `fonts/font_xl_64.c`,
`ui/layout_context.cpp` (already host-built in `native`), plus `sim/stubs.cpp`,
`sim/sim_main.cpp`. Do NOT compile `screen_*.cpp` — the harness defines its own
`ScreenVariantSpec` tables (one per template) to avoid their dep web.

## Tasks

### Task 1 — sim lv_conf with SDL
- Create `sim/lv_conf.h`: copy `include/lv_conf.h`, set `LV_USE_SDL 1`,
  `LV_COLOR_DEPTH 16`, enable the montserrat fonts already used (14/20/28/38/48),
  `LV_FONT_MONTSERRAT_* = 1`, `LV_USE_SNAPSHOT 1`, software renderer.
- Verify: a trivial `sim_main` that creates an `lv_sdl_window` display at
  480×480, fills bg, runs 5 `lv_timer_handler` ticks, exits cleanly.

### Task 2 — esp/host shims
- `sim/esp_heap_caps_shim.h`: `#define heap_caps_calloc(n,s,c) calloc(n,s)`,
  `heap_caps_malloc`, `heap_caps_get_*` → fixed large values, `MALLOC_CAP_* 0`.
  Inject via `-include sim/esp_heap_caps_shim.h` so `ui_layouts.cpp` compiles
  unchanged.
- `sim/stubs.cpp`: define `sk::copyData` (demo data: SOG 6.2, depth 8.4,
  AWS 12.4, AWA 42°, heading 56°, battery 0.82, position, current set/drift),
  `net::logf`, `ui::show_by_id`, `beeper::*`, `app::*`, `board::*`, and any
  `autopilot::*` not satisfied by `autopilot_pure.cpp`.

### Task 3 — sim_main: render + snapshot
- `sim/sim_main.cpp`:
  - Read target `LCD_W/LCD_H` from compile defines (`-D LCD_W=... -D LCD_H=...`).
  - Init LVGL + SDL display at that size.
  - Build a dashboard `ScreenVariantSpec` (QuadGrid, 4 tiles: wind/SOG/depth/
    batt) and call `ui::layouts::create(spec, nullptr)` (or `create_quad_grid`).
  - Pump `lv_timer_handler` + call the template `update` a few times so values
    populate from the demo `sk::Data`.
  - `lv_snapshot_take(lv_screen_active(), ...)` → write PNG to
    `docs/sim-shots/dash-<W>x<H>.png` (reuse the PNG encode from
    `src/screenshot.cpp` or LVGL's, or dump raw + convert with a tiny helper).
  - Exit 0.

### Task 4 — platformio env + per-resolution runner
- `[env:sim]` in `platformio.ini`: `platform = native`,
  `build_flags = -DLV_CONF_PATH=sim/lv_conf.h -Isim -include sim/esp_heap_caps_shim.h`
  `$(pkg-config --cflags --libs sdl2)` (or explicit `-I/opt/homebrew/include
  -L/opt/homebrew/lib -lSDL2`), `build_src_filter` listing the TUs above.
- `tools/sim_render.sh`: loop resolutions `480x480 800x480 1024x600`, for each
  `pio run -e sim` with `-DLCD_W -DLCD_H`, run the binary, collect PNGs.
- Headless note: SDL needs a video driver. Use `SDL_VIDEODRIVER=offscreen`
  (SDL ≥2.0.something supports the `offscreen` driver) so it runs without a
  window/display in CI.

### Task 5 — no-overlap assertion
- Extend `sim_main` (or a sibling) to walk the built tile rects and assert no
  pairwise overlap and all within bounds, at each resolution — fail nonzero.
- Wire into `make` as `make sim` / add to CI matrix.

### Task 6 — wire screenshots into docs
- Add `docs/sim-shots/*.png` to the README/guide resolution gallery.

## Acceptance
- `tools/sim_render.sh` produces `dash-480x480.png`, `dash-800x480.png`,
  `dash-1024x600.png` with the glass-cockpit dashboard rendered correctly.
- No-overlap assertion passes for all three.
- Runs headless (offscreen SDL) for CI.

## Risks / notes
- The biggest cost is the stub surface; keep widening `sim/stubs.cpp` until the
  link is clean (iterate on undefined-symbol errors).
- `font_xl_64.c` includes `lvgl.h` — fine on host with the sim lv_conf.
- If `autopilot_pure.cpp` doesn't cover the `autopilot::` API ui_layouts needs,
  stub the remainder rather than pulling `autopilot.cpp` (Arduino deps).
- Keep the harness's spec tables in sync with the real screens, or (better,
  later) factor the built-in specs into a host-includable header both use.
