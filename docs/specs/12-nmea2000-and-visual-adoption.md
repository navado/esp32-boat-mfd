# NMEA2000 And Visual Design Adoption

Status: proposed implementation spec after comparison with
`navado/NMEA2000-DISPLAY-on-ESP32S3`.

## Goals

- Add direct NMEA2000 support without weakening the current Signal K path.
- Bring over proven marine-instrument behavior: freshness, smoothing, damping,
  autopilot state, alarms, and audible alerts.
- Adopt the strongest visual concepts while keeping our native LVGL 9,
  PlatformIO, tested-parser architecture.
- Avoid copying GPL-3.0 implementation code into this PolyForm-NC project.

## Source Architecture Findings

The reference project is a standalone NMEA2000 instrument:

- Arduino IDE sketch.
- LVGL 8.4 with SquareLine-generated screens.
- Direct NMEA2000/CAN ingestion using Timo Lappalainen's library.
- Raymarine EVO autopilot support through proprietary PGNs.
- FFAT bitmap assets loaded into PSRAM at startup.
- Board-specific Waveshare Rev4 driver for GT911, display timing, expander,
  backlight, reset, and beeper.
- Preferences-backed unit and damping settings.

Our project should keep:

- PlatformIO and CI.
- LVGL 9 native screen modules.
- Signal K WebSocket/REST as the primary network data path.
- Event-driven command queues.
- Native host tests for parsers and layout/config logic.
- Data-driven layout templates.

## Licensing Boundary

The reference repository is GPL-3.0. Do not copy source code, generated UI
files, or assets into this repository unless the licensing model is explicitly
changed or the code is isolated as a separate GPL-compatible component.

Allowed adoption:

- Reimplement behavior from public protocol knowledge and observed behavior.
- Use the architectural pattern: direct NMEA2000 source, smoothing/freshness
  accessors, board abstraction, beeper task.
- Record PGN numbers and externally documented message meanings.

Do not adopt:

- SquareLine generated C files.
- Bitmap assets.
- Raymarine helper implementation code.
- Exact UI image compositions.

## Feature 1: Data Source Abstraction

Add a source-neutral internal data model so Signal K and NMEA2000 can feed the
same screen/rendering code.

Proposed modules:

- `include/boat_data.h`
- `src/boat_data.cpp`
- `include/data_sources.h`
- `src/source_signalk.cpp`
- `src/source_nmea2000.cpp`

Model:

```cpp
namespace boat {

enum class SourceKind {
  SignalK,
  NMEA2000,
  Demo,
};

struct Field {
  double value;
  uint32_t updated_ms;
  SourceKind source;
};

struct Snapshot {
  Field sog_mps;
  Field stw_mps;
  Field cog_true_rad;
  Field heading_true_rad;
  Field awa_rad;
  Field aws_mps;
  Field twa_rad;
  Field tws_mps;
  Field depth_m;
  Field battery_v;
  Field xte_m;
  Field btw_rad;
  Field dtw_m;
  Field autopilot_target_rad;
  char autopilot_state[16];
};

bool copy_snapshot(Snapshot &out);

}  // namespace boat
```

Rules:

- Signal K continues to parse into `sk::Data` during migration.
- A bridge copies `sk::Data` into `boat::Snapshot`.
- NMEA2000 later writes the same `boat::Snapshot` fields.
- Screens read `boat::Snapshot` once they migrate.
- Source priority is configurable: `signalk_first`, `nmea2000_first`,
  `freshest`, or per-field override.

## Feature 2: Direct NMEA2000 Source

Add optional NMEA2000/CAN support as a separate source module.

Configuration:

```json
{
  "sources": {
    "nmea2000": {
      "enabled": false,
      "rx_pin": 0,
      "tx_pin": 6,
      "mode": "listen",
      "timeout_ms": 2000
    }
  }
}
```

Initial PGNs:

| PGN | Purpose |
| --- | --- |
| 127250 | Vessel heading |
| 127245 | Rudder angle |
| 128259 | Speed through water |
| 128267 | Depth |
| 129025 | Position rapid update |
| 129026 | COG/SOG rapid update |
| 129283 | Cross-track error |
| 129284 | Navigation data to waypoint |
| 130306 | Wind data |
| 127508 | Battery status |
| 127505 | Fluid level |

Implementation rules:

- NMEA2000 processing runs on core 0.
- It must not call LVGL.
- It updates only the source-neutral data model through a short locked swap.
- Enable direct NMEA2000 at compile time first; runtime config can follow.
- Keep Signal K enabled by default.

## Feature 3: Freshness And Smoothing

Add smoothed/fresh accessors so screens do not decide this independently.

API sketch:

```cpp
namespace boat {

bool fresh(const Field &field, uint32_t timeout_ms);
double value_or_nan(const Field &field, uint32_t timeout_ms);

double heading_smoothed_deg();
double cog_smoothed_deg();
double awa_smoothed_deg();
double aws_smoothed_kn();
double sog_smoothed_kn();
double depth_smoothed_m();

bool has_fresh_wind();
bool has_fresh_heading();
bool has_fresh_position();
bool has_fresh_route();

}  // namespace boat
```

Settings:

- `wind_response`: `fast`, `normal`, `smooth`
- `heading_response`: `fast`, `normal`, `smooth`
- `speed_response`: `fast`, `normal`, `smooth`
- `source_timeout_ms`: default 2000 for NMEA2000, 10000 for Signal K stalled
  status

Rules:

- Use exponential moving average for scalar values.
- Use sine/cosine smoothing for angles.
- Preserve last-good values only while fresh.
- Render stale values with missing-data state, not zero.

## Feature 4: Raymarine EVO Autopilot Support

Add a dedicated autopilot backend interface. Signal K remains backend 1;
direct NMEA2000/Raymarine is backend 2.

API:

```cpp
namespace autopilot {

enum class Backend {
  SignalK,
  NMEA2000Raymarine,
};

enum class Mode {
  Unknown,
  Standby,
  Auto,
  Wind,
  PreTrack,
  Track,
};

struct State {
  Backend backend;
  Mode mode;
  double current_heading_rad;
  double target_heading_rad;
  double target_wind_angle_rad;
  bool alarm_active;
  bool warning_active;
  char alarm_text[48];
};

bool copy_state(State &out);
bool set_mode(Mode mode);
bool adjust_heading_deg(int delta);
bool silence_alarm();

}  // namespace autopilot
```

Direct NMEA2000/Raymarine PGNs to investigate and reimplement:

| PGN | Use |
| --- | --- |
| 126208 | Group function command, pilot mode/heading commands |
| 126720 | Manufacturer proprietary key commands |
| 127237 | Heading/track control |
| 65288 | Pilot alarm state |
| 65345 | Pilot wind angle |
| 65360 | Pilot locked heading |
| 65361 | Alarm silence command |
| 65379 | Pilot mode/submode |

Safety:

- Commands are posted to the network/NMEA task, never executed from LVGL
  callbacks.
- Every command has visible queued/pending/result state.
- Track, wind, tack/gybe, and alarm-silence actions require explicit controls.
- Add a dry-run/sniffer mode before enabling transmit.

## Feature 5: Audible Alarm/Beeper Task

Add optional board-level audible feedback.

API:

```cpp
namespace board {

bool beeper_available();
void beep_short();
void alarm_pattern(uint32_t on_ms, uint32_t off_ms, uint16_t repeat);
void alarm_stop();

}  // namespace board
```

Rules:

- Beeper runs in its own low-priority task.
- UI and alarm checks enqueue beeper requests.
- Critical alarms can repeat; normal button taps use short nonblocking beep.
- Settings include `audible_alarms`: off/on.

## Feature 6: Board Support Layer

Expand `board_pins.h` into a board support abstraction.

See `13-board-display-portability.md` for the full hardware/display abstraction
and multi-board migration plan.

Proposed files:

- `include/board.h`
- `src/board_sunton_4848s040.cpp`
- `src/board_waveshare_rev4.cpp` later, if that hardware becomes a target.

Responsibilities:

- Backlight set/get.
- Touch initialization and calibration.
- Display reset and RGB timing.
- Optional beeper.
- Optional IO expander.
- Optional startup reset workaround.

This keeps hardware-specific recovery logic out of `main.cpp` and screen code.

## Feature 7: Asset Strategy

Adopt the performance idea, not the pipeline.

Use PSRAM-cached assets only when profiling proves they help:

- static round dial faces
- compass tick rings
- boat silhouette
- route perspective grid
- warning/alarm symbols

Do not adopt:

- mandatory FFAT upload flow
- SquareLine asset export dependency
- bitmap-only UI

Preferred implementation:

- Native LVGL objects for text, panels, buttons, and bars.
- Optional generated/cached bitmaps for static decorative instrument faces.
- Regenerate cached canvas only when theme/scale changes.

## Adopted Visual Design Concepts

The reference UI is strongest where it looks like a marine instrument rather
than a phone app. Adopt these concepts in our visual system:

### White Daylight Instrument Face

Day mode should support a white or very light instrument-face surface for wind,
steering, route, and settings screens.

Rules:

- Use high-contrast black/dark gray labels.
- Keep red/port and green/starboard sectors saturated but not neon.
- Use gray tick marks and bezels.
- Reserve colored fills for domain state.

### Round Instrument Bezel

Wind, true wind, steering, compass, and autopilot heading screens should use a
shared round instrument vocabulary:

- outer tick ring
- 30/60/90/120/150 labels where relevant
- top lubber marker
- colored port/starboard sectors
- center value block
- small corner secondary values

The reusable `round_instrument` template owns this vocabulary.

### Large Marine Control Buttons

Autopilot and settings controls should use large, finger-safe segmented or
button-grid controls.

Rules:

- One stateful control for a mode group: `Auto / Track / Wind`,
  `Day / Night`, `Off / On`.
- Heading adjustments use fixed buttons: `-10`, `-1`, `+1`, `+10`.
- Active state is visible through fill/border/text, not only text.

### Dense Framed Data Panels

For status/navigation panels, adopt the dense framed-instrument feel:

- visible panel boundaries
- aligned labels and values
- small caption in a corner
- large numeric value centered or right-aligned
- no marketing-style cards or decorative empty space

Our panels keep the flatter 6 px radius and semantic accent rails from
`10-ui-appearance-redesign-session.md`; the reference project's heavy shadows
are not adopted.

### Route Spatial View

Route and steering can adopt a simplified perspective/track view:

- boat at bottom-center
- centerline or grid forward
- XTE indicator left/right of track
- waypoint/bearing context

Implement first as native LVGL line/canvas drawing, not as a large bitmap.

### Settings As Instrument Console

Settings should remain a functional console:

- one coherent control per setting
- clear active state
- no duplicate on/off buttons
- discrete operational buckets before free-form input
- large confirm/cancel only when settings are staged instead of immediate

## Visual Concepts Not Adopted

- Script logo/footer branding inside operational screens.
- Heavy beveled shadows as the main visual system.
- Sea/sky photographic background on primary instrument screens.
- SquareLine-generated global object UI architecture.
- Full bitmap dependency for core controls.

## Migration Plan

1. Add `boat::Snapshot` and bridge Signal K into it.
2. Move screen reads from `sk::Data` to `boat::Snapshot` gradually.
3. Add freshness/smoothing helpers and migrate wind/nav/steering first.
4. Add `board::` backlight and beeper abstraction.
5. Add optional beeper task and audible alarm setting.
6. Add NMEA2000 listen-only source behind compile-time flag.
7. Add NMEA2000 sniffer diagnostics.
8. Rework `round_instrument` visual template using white day face, colored
   sectors, tick ring, lubber marker, and corner secondary values.
9. Add autopilot backend interface.
10. Add Raymarine NMEA2000 autopilot backend in dry-run/sniffer mode.
11. Enable controlled transmit only after hardware validation.

## Acceptance

- Signal K-only builds behave exactly as before unless the source model is
  explicitly enabled.
- Native tests cover source priority, freshness, smoothing, and stale behavior.
- Direct NMEA2000 can run listen-only without UI stalls.
- NMEA2000 transmit is impossible until explicitly enabled.
- Autopilot commands never run from LVGL callbacks.
- Day-mode wind/steering/autopilot screens read like marine instruments.
- No GPL source or assets are copied into this repository.
