# Autopilot + Wind screen redesign — reference glass-cockpit style

**Date:** 2026-06-14
**Status:** Approved (interactive brainstorm)
**Scope:** `src/ui/screen_autopilot.cpp`, `src/ui/screen_wind.cpp`, new shared
`ui_compass` primitives, sim harness for the AP screen, and docs/README/manual
refresh. The round Waveshare-knob views (`ui::ap_hud`, `knob_*`) are **out of
scope** this pass.

## Motivation

Two reference renders (a wide landscape HUD and a square "Style 2 / HOME" HUD)
define a clean glass-cockpit language: a **semicircular compass arc** (white
band, green outer rail, red cardinals + needle, amber target bug), a centered
**HDG** readout with a **COG/SOG** sub-line, an **XTE deviation strip**, and a
row of **rounded numeric tiles**.

The current wind screen has a **rendering defect**: too many independently
rotating in-dial labels (bezel cardinals + 30/60/90/120/150 angle scale + A/T
wind markers + corner readouts) share one circular face with no collision
budget, so their bounding boxes overdraw each other — e.g. the amber A/T markers
overprint the "E" cardinal and the "30" scale, and the tide marker box
overprints "TWS 10.7". Confirmed in `docs/sim-shots/wind-480x480.png` and
`docs/widget-previews/device-wind-rework-2026-06-13.png`.

## Decisions (locked)

1. **Wind geometry:** keep the true 360° rose (wind can blow from dead astern),
   but move **all numerics out of the dial into a bottom tile row** and reduce
   in-dial labels. This both adopts the reference aesthetic and fixes the
   overlap bug at its root.
2. **Autopilot screen** becomes the reference monitor HUD. Controls are **touch
   + external network knob**:
   - Primary touch button **`ON` / `STBY`** toggles engage ↔ standby.
   - **Long-press** that button opens a **mode picker** (AUTO / WIND / ROUTE) so
     mode buttons stay hidden for the clean look.
   - **Tap the port / starboard half of the dial** = ∓1°; **long-press** a half
     = ∓10°.
   - The existing network AP bridge (knob / plugin) keeps working unchanged.
3. **Scope:** all three RGB panels — 480×480 (square, image-2 layout) and
   800×480 / 1024×600 (wide, image-1 layout). Layout switches on aspect ratio.

## Shared primitives (new: `include/ui_compass.h` + `src/ui/ui_compass.cpp`)

All metrics/colors come from `ui::theme` / `ui::chrome` (no inline magic).

- **`Compass`** — a semicircular heading gauge centered at `(cx, cy)` radius `r`:
  - fixed white **band arc** (LVGL 180→360, thick), green **rail arc** just
    outside it, tick marks every 15° across the top semicircle;
  - a **rotating scale group** (degree numbers every 30°, cardinals N/E/S/W in
    `theme.alarm` red) rotated by `-heading` so the heading sits under a fixed
    red **lubber** triangle at top-center; the lower half is occluded by the
    opaque center panel + lower content;
  - an **amber target bug** (`theme.warn`) placed on the rail at
    `(target − heading)`;
  - refs returned so `refresh()` can rotate the scale + bug and set the center
    text. Reusable by AP now; available to any heading view later.
- **`numeric_tile(parent, caption, unit, …)`** — rounded card (uses
  `style_panel`): dim caption top-left, unit top-right, big value bottom.
  Returns the value label.
- **`xte_strip(parent, …)`** — horizontal PORT…STBD scale, −1.0…1.0 ticks, red
  center deviation needle; returns the needle for `refresh()` to translate.
- New palette token **`arc_band`** (near-white) added to `Palette` (night +
  day), wired through `ui_theme.cpp`.

## Autopilot screen (`ui::autopilot`)

- **480×480:** compass arc fills the top ~55%; center shows `HDG nnn°` (xl font)
  + `COG nnn° | SOG n.n kn`; XTE strip under the arc; bottom row = 4 tiles
  `DEPTH · SPEED · AWS · AWA`. Top-left `ON/STBY` button, top-right `HOME`.
- **800×480 / 1024×600:** compass + XTE centered; the four tiles split into a
  left column (DEPTH/SPEED) and right column (AWS/AWA) flanking the arc, matching
  the wide reference.
- **State coloring:** badge/needle use `theme.good` when engaged, `theme.fg_dim`
  when standby. Target bug hidden unless a target heading exists.
- **Input:** invisible left/right tap-zone objects over the dial half-circles
  (`LV_EVENT_CLICKED` = ∓1°, `LV_EVENT_LONG_PRESSED` = ∓10°); `ON/STBY` button
  (`CLICKED` = toggle engage/standby, `LONG_PRESSED` = mode picker overlay).
  Commands route through the existing `app::post_net` PUT path (unchanged).

## Wind screen (`ui::wind`)

- Keep the rose, bezel rotation, close-hauled arcs, boat outline, A/T markers,
  current arrow, waypoint pip.
- **Remove** the three `inner_readout` blocks (AWS/TWS/HDG) and the
  `build_wind_scale` 30…150 numbers (the chief clutter sources). Cardinals stay
  but lighter.
- Restyle the face ring as the white **band** + green close-hauled rail accent.
- **Bottom tile row** holds the numerics: `AWS · AWA · TWS · TWA` on 480; wide
  panels add `SOG · SOW · TWD` (and HDG). Rose shrinks to sit above the tiles.

## Sim / verification

- New `sim/sim_ap.cpp` + `[env:sim-ap]` (build filter: `screen_autopilot.cpp`,
  `ui_compass.cpp`, `ui_theme.cpp`, fonts, `sim_ap.cpp`, `stubs.cpp`).
- Extend `sim/stubs.cpp`: add `sk::connectionStatus()` and set `apState="auto"`,
  `apTargetHdg` in the demo snapshot so the AP render shows AUTO + a target bug.
- `tools/sim_render.sh` renders `ap-{w}x{h}` and the restyled `wind-{w}x{h}` at
  480/800/1024, plus the existing `knob-ap_hud` round control.
- Gate: `make pre-commit` + `pio test -e native` + `pio run -e esp32-4848s040`.

## Docs

Regenerate sim shots; update `README.md` image gallery and the user manual under
`docs/` to show every supported size (480/800/1024) and the round AP control.

## Risks

- Semicircle occlusion of the lower scale ring relies on opaque lower content —
  verified visually per resolution via the sim before shipping.
- Tap-zones must not steal the global horizontal swipe (screen cycle): zones are
  click/long-press only and sit inside the dial, leaving screen edges free for
  swipe; verified on device.
- Stack discipline: `refresh()` keeps the existing `sk::Data` snapshot copy
  pattern; no new large stack temporaries (per CLAUDE.md memory traps).
