# Waveshare ESP32-S3 Knob — Remote Controller & Dedicated Views

**Date:** 2026-06-13
**Status:** Design (approved interaction model; pending spec review)
**Board:** Waveshare ESP32-S3-Knob-Touch-LCD-1.8 (module JC3636K518)

## 1. Goal

Add a new board variant — the Waveshare 1.8″ round knob display — to the
Yey Boats Instruments firmware. The knob is primarily a **remote controller** with a
small set of **dedicated round views**. A rotary-encoder-driven multilevel menu
lets the operator:

1. Control the autopilot (engage/standby, mode, course/wind-angle) — *home*.
2. Select any display device on the SignalK network.
3. Select the active view shown on that display (including the knob itself).

The knob reuses the existing networking/SignalK/manager stack; the autopilot
control reuses the exact command path the on-screen autopilot screen already
uses; cross-device view switching reuses the manager's `screen.set` command and
push-live (`configPush`) machinery.

## 2. Hardware

ESP32-S3 (8 MB PSRAM, 16 MB flash). Verified pin map (sources:
[Tasmota #23737](https://github.com/arendst/Tasmota/discussions/23737),
[ESPHome #3253](https://github.com/orgs/esphome/discussions/3253)):

| Function | Pin(s) |
|---|---|
| ST77916 QSPI display, 360×360, **round** | CLK 13, D0 15, D1 16, D2 17, D3 18, CS 14, RST 21 |
| Backlight | 47 (LEDC PWM) |
| CST816S touch (I²C addr `0x15`) | SDA 11, SCL 12, INT 9, RST 10 |
| Rotary encoder (quadrature A/B) | A 8, B 7 |
| Encoder push button | GPIO 0 (boot-strap pin; input after boot, `INPUT_PULLUP`) |
| DRV2605 haptic | on the I²C bus (0x5A) |

Notes / non-goals:
- The board carries a **second ESP32** for audio (DAC over UART). We ignore it
  entirely; only the S3 runs our firmware. No audio support.
- SD-card pins overlap the display QSPI bus — SD unused.
- Touch is **secondary**: the menu is encoder-first. CST816 read is wired for
  completeness/future use; v1 navigation does not depend on it.

## 3. Firmware architecture

### 3.1 Board abstraction
- New compile id `BOARD_ID_WAVESHARE_KNOB_1_8` (+ `[env:waveshare-knob-1_8]` and
  `[env:release-waveshare-knob-1_8]` in `platformio.ini`).
- `include/board_pins_waveshare_knob.h` — full GPIO map above, `LCD_W=LCD_H=360`.
- `src/boards/board_waveshare_knob.cpp` implements the `board::` API:
  - `geometry()`: 360×360, `DisplayShape::Round`, density Hdpi, a round-appropriate
    `LayoutClass`. **Decision:** add `LayoutClass::Round` rather than overloading
    `SquareCompact`, so round-only layout logic is explicit. `usable_*` describes
    the inscribed square (the safe rectangle inside the circle,
    side ≈ 360/√2 ≈ 254 px) for any rectangular content.
  - `capabilities()`: `touch=CST816`, `backlight=LedcPwm`, `display_bus=Qspi`,
    `beeper=true` (routed to DRV2605 haptic), `nmea2000_can=false`,
    `psram_required=true`, `touch_interrupt=true` (INT on GPIO 9).
  - `set_backlight()` → LEDC PWM on GPIO 47. `chipTempC()` as on other S3 boards.

### 3.2 Display bring-up (the real lift)
`main.cpp` currently hardcodes `Arduino_RGB_Display` + the ST7701 init table.
Board-gate the `gfx` construction:
- **Knob path:** `Arduino_ESP32QSPI` databus (CS 14, CLK 13, D0–D3 15/16/17/18) →
  `Arduino_ST77916(bus, RST 21, rotation, ips=true, 360, 360)`.
- **Sunton/Waveshare-RGB path:** unchanged.

The LVGL flush callback (`gfx->draw16bitRGBBitmap`) and the **PSRAM draw buffers**
(memory trap: LVGL draw buffers in PSRAM, never internal DMA SRAM) are unchanged —
only the `gfx` object differs. Wrap the divergence in a single
`#if defined(BOARD_ID_WAVESHARE_KNOB_1_8)` block near display init; keep the ST77916
init/QSPI specifics minimal and documented.

**Risk:** confirm `moononournation/GFX Library for Arduino@~1.4.7` ships
`Arduino_ST77916` + `Arduino_ESP32QSPI`. If absent, bump to the first `1.5.x` that
has them (isolated to the knob env's `lib_deps`).

### 3.3 Input pipeline (mirrors `touch_task` → `app::Command` → `app::pump`)
- **`knob_input` (new, device-only).** A dedicated FreeRTOS task on core 0:
  - **ESP32Encoder** (`madhephaestus/ESP32Encoder`) on A 8 / B 7 — PCNT hardware
    counting, no ISR jitter. Detents → `DetentCW` / `DetentCCW`.
  - **OneButton** (`mathertel/OneButton`) on GPIO 0 — classifies `Click`,
    `DoubleClick`, `LongPressStart`. Track a `button_held` flag so
    *rotation while held* = ±5 step.
  - Emits semantic `KnobEvent`s and posts them via the app command queue (new
    `app::CommandType::Knob` carrying the event enum), drained on the LVGL task.
  - Optional haptic ack (DRV2605) on detent/select — short pulse; routed through
    the existing `beeper` abstraction so it stays one call site.
- **No `LV_INDEV_TYPE_ENCODER`.** The bespoke gesture vocabulary (hold+rotate ±5,
  double-click-back, click=engage vs long-press=mode) does not map onto LVGL's
  encoder model, and this project already bypasses LVGL gestures for the same
  "exact control" reason. Custom classification keeps the state machine testable.

### 3.4 Menu state machine — `knob_menu` (pure, host-tested)
A pure C++ module (no LVGL, no Arduino), unit-tested on the `native` env like
`autopilot_pure.cpp` / `layout.cpp`. Inputs: `KnobEvent` + a read-only snapshot
(autopilot state from `sk::data`, device/view lists). Outputs: a `MenuModel`
(current level, highlighted index, list contents, pending action) plus an
optional `MenuAction` (a side effect to perform — a SignalK PUT or a view-switch
command). `knob_ui` renders `MenuModel` on the LVGL task; the device loop turns
`MenuAction` into the existing `app::Command`s.

```
HOME = Autopilot HUD  (default dedicated view)
  scroll            -> adjust target heading ±1°  (apparent wind angle in Wind mode)
  hold + scroll     -> adjust ±5°
  click             -> engage / disengage (toggle Standby <-> last active mode)
  long-press        -> open MODE PICKER
  double-click      -> open MENU (Select Display)
  (scroll in Standby pre-sets the target so engaging Compass holds it)

MODE PICKER  (overlay)
  scroll            -> highlight Standby / Compass / Wind / Route
  click             -> engage highlighted mode, return HOME
  double-click      -> cancel, return HOME

SELECT DISPLAY  (level 2)
  scroll            -> move; shows "#N name", highlights currently-active display.
                       List = the knob itself + remote MFDs from the manager.
  click             -> enter SELECT VIEW for that display
  double-click      -> back to HOME

SELECT VIEW  (level 3)
  scroll            -> move through that display's views; highlights its current
  click             -> switch that display to the view; brief confirm; stay
  double-click      -> back to SELECT DISPLAY
```

Autopilot side effects reuse the on-screen autopilot path exactly:
- engage/mode → `SignalKPut steering/autopilot/state` (`standby|auto|wind|route`)
- ±N course → `SignalKPut steering/autopilot/actions/adjustHeading` (degrees)
- absolute target (pre-set) → `SignalKPut steering/autopilot/target/headingTrue` (rad)

### 3.5 Dedicated views (knob acts as a display)
The knob registers a small curated set of round views in its own screen registry,
so it appears in "Select Display" like any device and "Select View" lists these:
- **(a) Autopilot HUD** — home/default (mode badge, big target, current HDG + Δ).
- **(b) Compass / Heading** — round heading ring with HDG/COG.
- **(c) Wind angle** — apparent wind angle on the round dial.
- **(d) Big number** — one large value (depth or SOG), tap/select to choose which.

All four are round-first designs that fit inside the 360 circle. Selecting one
sets what the knob shows when idle (HOME autopilot control gestures are active
only when the Autopilot HUD is the active knob view).

## 4. Cross-device control (manager-first, SignalK fallback)

### 4.1 Manager present (v1 primary)
- **Enumerate displays:** knob (already a manager client, `src/manager.cpp`) GETs
  the device list → populates Select Display (id, number, name, current screen,
  online).
- **Per-device views:** GET each device's available views.
- **Switch:** reuse `POST /devices/:id/command {type:"screen.set", screen:<id>}`
  → target device runs `screen.set` → `ui::show_by_id()`; instant-apply via the
  existing `network.espdisp.configPush` subscription. No device-side changes.
- **Plugin additions (small, read-only):** `GET /devices` (summary list) and
  `GET /devices/:id/views` (view ids+titles derived from the device's
  config/profile). The plugin already holds a per-device registry + config, so
  these are thin projections.

### 4.2 Manager absent (fallback, firmware phase 2)
- **Switch:** knob publishes `network.espdisp.remoteControl {target:<id>, screen:<id>}`;
  target devices subscribe (small addition to `src/signalk.cpp`) and act through the
  same `screen.set` path.
- **Enumerate:** devices announce `network.espdisp.devices.<id>` (name, current
  screen, view list) on the SignalK tree; the knob reads it from `sk::data`.
- Scoped after the manager path so v1 ships on the proven mechanism first.

## 5. Documentation & assets (added scope)

- **README:** add the knob to the supported-boards table; new **"Remote Knob"**
  section (what it is, the menu map, gesture cheat-sheet); round-view screenshots
  in the gallery.
- **`make sim` harness:** extend to render the round 360×360 knob views headlessly
  for the README/gallery (harness already renders real layouts at arbitrary
  resolutions — add a 360×360 round profile + the knob screens).
- **User guide (`docs/user-guide-signalk.md`):** knob usage walkthrough.
- **New section "Install the SignalK plugin":** install `signalk-espdisp-manager`
  (from `signalk/plugins/`), enable in SignalK, first-run admin.
- **New section "Deploy & use the remote knob":** flash `waveshare-knob-1_8` →
  provision (`manager-token`) → pair to displays → menu usage. Cross-link from
  README and `docs/signalk-espdisp-manager.md`.

## 6. Testing

- **Host (`native`):** `knob_menu` state-machine tests — every transition, the
  ±1/±5 logic, double-click-back, mode picker, action emission for each level.
  Add `test_knob_menu` to the `native` env filter + `build_src_filter`.
- **Sim (`sim`):** render the four round knob views at 360×360; no-overlap/bounds
  assertions (the harness already does this) adapted to the inscribed circle.
- **Device:** build `waveshare-knob-1_8`; bench-verify encoder detents (±1/±5),
  click/double-click/long-press, autopilot PUTs reach the sim via the bridge, and
  a remote view-switch lands on a second display. `espdisp soak` before any
  stability claim.

## 7. Memory-trap compliance

- LVGL draw buffers in PSRAM (not internal DMA SRAM).
- Live `layout::Config` PSRAM-allocated; `parse()` uses in-place `memset`.
- `NimBLECharacteristic::setValue` uses the `(uint8_t*, len)` overload.
- New `knob_*` modules: large PODs (`MenuModel`) zeroed in place, never
  stack-constructed as big temporaries; LVGL mutations only on the UI task
  (state machine posts `app::Command`s, never touches `lv_obj_*`).

## 8. Open risks / to verify during implementation

1. `Arduino_GFX@~1.4.7` ST77916 + ESP32QSPI class availability (else bump env lib).
2. GPIO 0 as encoder button vs boot strapping — confirm clean boot with the
   encoder at rest (pull-up, not held low at reset).
3. CST816 INT polarity / reset timing on this panel (driver/lib choice).
4. Manager per-device "available views" source — confirm the plugin can derive a
   view list from stored config/profile; if not, define where the list comes from.
5. DRV2605 haptic is optional for v1 — can land as a follow-up without blocking.

## 9. File-level change summary

New:
- `include/board_pins_waveshare_knob.h`, `src/boards/board_waveshare_knob.cpp`
- `include/knob_input.h` / `src/knob_input.cpp` (device-only)
- `include/knob_menu.h` / `src/knob_menu.cpp` (pure, host-tested)
- `include/knob_ui.h` / `src/ui/knob_ui.cpp` + round view screens
- `test/test_knob_menu/test_knob_menu.cpp`
- plugin: `GET /devices`, `GET /devices/:id/views` handlers

Modified:
- `platformio.ini` (knob envs, lib_deps, native/sim filters), `.github/workflows/ci.yml`
- `include/board.h` (`LayoutClass::Round`), `include/app_events.h` (`CommandType::Knob`)
- `src/main.cpp` (board-gated QSPI/ST77916 display init; knob input/UI wiring)
- `src/signalk.cpp` (phase-2 `remoteControl` subscribe + device announce)
- README, `docs/user-guide-signalk.md`, `docs/signalk-espdisp-manager.md`, sim harness
