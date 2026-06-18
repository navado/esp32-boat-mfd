# SignalK Screens — Manager Preview Fidelity Rework

**Status:** Queued (not yet started)
**Date:** 2026-06-18
**Implementation repo:** `yey-boats/Instruments-manager` (cloned locally as `../signalk-espdisp-manager`)
**Source of truth:** the firmware `src/ui/screen_*.cpp` renderers — the device display is canonical; the manager preview must mirror it.

## Problem

The manager's browser layout-editor "Preview" pane is supposed to be a faithful mirror
of what the device actually draws. Today it diverges from the device on several screens.

Evidence (device on right, browser preview on left):

- `assets/2026-06-18-device-vs-preview-wind-steer.jpg` — the device renders the full
  **Wind Steer** HUD (semicircle port/stbd compass band, `AUTO` annunciator, `HDG 167.1°`
  center, `TWA 166°S | TWD 333°` subline, and four right-rail tiles TWA / TWS / AWA / AWS,
  with `STBY` / `HOME` buttons). The browser **Preview "Wind Steer" pane is blank.**
- `assets/2026-06-18-device-vs-preview-depth.jpg` — the **Depth** screen roughly matches
  (4-tile grid: BELOW K / DEPTH 5m / DEPTH / H2O TEMP), but small differences remain
  (caption casing `5m` vs `5M`, tile spacing/typography).

So the preview is **blank for at least one HUD screen** and **generic for the non-HUD data
screens**, instead of mirroring the device exactly.

## Goal

For every user-facing device data screen, the manager preview reproduces the device's
**exact** layout, widgets, value formatting, and styling — bound to live SignalK values —
so a user editing a layout sees precisely what the device will show.

Setup/built-in screens that are not data dashboards are **out of scope** (touch_cal,
touch_grid, wifi_setup, settings, demo_grid, manager).

## Current state (manager `public/`)

- `live-preview.js` dispatches: System panel (`status`/`system`), fullscreen HUD
  (`DeviceHud.fullscreenForScreen` → `autopilot` / `wind` / `wind_classic` / `wind_steer`),
  marker-ring compass/windRose tiles, then a generic `cfg.screens[].tiles` grid.
- `device-hud.js` exports faithful SVG renderers: `autopilotHud`, `windDial`, `windSteer`,
  `systemPanel`, `compassTileSVG`. `SCREEN_FULLSCREEN` maps device ids → renderer
  (exact + `<base>_` prefix match).
- **Gap A — Wind Steer blank:** the `windSteer` renderer and `wind_steer` id mapping both
  exist, yet the pane renders empty. Root cause to confirm first (one of): the device's
  reported screen `id` does not match the `wind_steer` key; `windSteer(a)` throws/returns
  empty when the bound values are absent; or the preview WS feed isn't delivering values
  so the dial draws nothing visible. **First task is to reproduce + isolate.**
- **Gap B — non-HUD data screens generic:** `dashboard`, `nav`, `route`, `steering`,
  `trip` fall through to the generic preset tile grid rather than the device's real
  per-screen layout/widgets (e.g. the steering compass marker ring, the route screen's
  XTE strip + waypoint readouts).
- **Gap C — tile-grid fidelity:** caption casing, units, precision, and spacing drift from
  the device (Depth screenshot).

## Device screen inventory (firmware = truth)

Data screens that MUST be mirrored: `autopilot`, `wind`, `wind_classic`, `wind_steer`,
`depth`, `dashboard`, `nav`, `route`, `steering`, `trip`, `status`.

Each device renderer lives in `src/ui/screen_<name>.cpp`; the preview's fidelity is
judged against that file (and against on-device screenshots where available via the
firmware `/api/screenshot.png` endpoint).

## Approach

1. **Reproduce & isolate Gap A** (Wind Steer blank). Confirm whether it is id-mapping,
   a renderer exception, or missing live data. Fix the actual cause; add a regression
   guard so a renderer that throws degrades to a visible stub, never a blank pane.
2. **Per-screen fidelity audit.** For each data screen in the inventory, compare the
   device renderer (`screen_<name>.cpp` + a device screenshot) against the preview and
   record concrete deltas (layout, widgets, captions, units, precision, colors, fonts).
3. **Close non-HUD gaps (Gap B).** Promote `steering` (marker-ring compass), `route`
   (waypoint/XTE strip), `nav`, `trip`, and `dashboard` from the generic grid to
   device-faithful renderers in `device-hud.js`, dispatched from `live-preview.js`.
4. **Tighten tile-grid fidelity (Gap C).** Align caption casing, unit strings, precision,
   and tile metrics with the device for the remaining grid screens.
5. **Lock it with visual tests.** Extend the manager's Playwright/visual-test harness so
   each mirrored screen has a screenshot baseline; a renderer regressing to blank or
   diverging from baseline fails CI.

## Acceptance criteria

- Selecting **any** data screen in the editor Preview shows a faithful render — never a
  blank pane — for both populated and absent live data.
- Wind Steer preview matches `assets/2026-06-18-device-vs-preview-wind-steer.jpg` (device side):
  semicircle band, AUTO/STBY annunciator, HDG center, TWA|TWD subline, four right-rail tiles.
- `steering` and `route` previews render their device-specific layouts (marker-ring compass;
  waypoint + XTE strip), not a generic tile grid.
- Depth (and other grid screens) match device caption casing, units, and precision.
- A visual-regression baseline exists per mirrored screen and runs in the manager's CI.

## Out of scope

- Changing the device firmware screen designs (device is canonical).
- Setup/built-in screens (touch_cal, touch_grid, wifi_setup, settings, demo_grid, manager).
- The data-simulator enrichment (tracked separately; that work makes route/AIS/AP data
  available to *exercise* these previews but is not part of this rework).

## Notes for the implementer

- This is **manager-repo** work (`../signalk-espdisp-manager/public/{live-preview,device-hud}.js`
  + `test/`), not firmware. Cross-check each screen against `espdisp/src/ui/screen_*.cpp`.
- Prior context: `2026-06-17-device-mirrored-layout-editor-design.md` and the marker-ring
  design (`2026-06-18-compass-marker-rings-design.md`) — the preview already mirrors marker
  rings for grid compass tiles; extend the same fidelity discipline to the fullscreen and
  remaining data screens.
