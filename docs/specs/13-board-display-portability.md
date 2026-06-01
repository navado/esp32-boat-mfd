# Board And Display Portability

Status: partially implemented. The firmware now has a board/display geometry
contract, helper names for manager/status reporting, host tests for geometry
classification, and a Tier 1 Waveshare RGB profile matrix. Board-specific
display/touch drivers and hardware validation remain required before declaring
the broader family fully supported.

## Goals

- Support multiple well-known ESP32-S3 display boards without scattering
  `#ifdef` blocks through screen code.
- Preserve the current 480x480 instrument experience.
- Support square, landscape, and portrait displays with predictable layouts.
- Keep hardware initialization, touch calibration, backlight, and optional
  beeper/expander logic out of UI screens.
- Make settings describe capabilities, not one board's controls.

## Target Board Classes

### Square Instrument Class

Examples:

- Sunton/Guition ESP32-4848S040, 4.0 inch, 480x480 RGB ST7701/GT911.
- Waveshare ESP32-S3-Touch-LCD-4, 4.0 inch, 480x480, GT911, integrated CAN on
  some revisions.
- Adafruit Qualia ESP32-S3 with 4 inch 480x480 RGB display.

Use:

- single instrument repeater
- helm display
- compact cockpit MFD

Primary constraints:

- 480x480 or similar square canvas.
- one-handed touch.
- round instruments should remain first-class.

### Medium Rectangular Class

Examples:

- ESP32-S3 4.3 inch 800x480 RGB panels.
- 5.0 inch 800x480 smart display boards.
- 7.0 inch 800x480 RGB panels such as Makerfabs/Waveshare-style boards.

Use:

- wider MFD layout
- split instrument + secondary panels
- route/steering view with side data columns

Primary constraints:

- higher pixel count increases flush cost.
- horizontal layout should not just scale 480x480 coordinates.
- touch targets stay physically large, not merely pixel-large.

### Large Rectangular Class

Examples:

- 7 to 9 inch ESP32-S3 RGB/LVDS-like smart displays where memory bandwidth and
  panel timing are the limiting factors.

Use:

- cabin/nav-station display
- multi-panel overview
- diagnostics/configuration console

Primary constraints:

- ESP32-S3 may struggle with full-frame high-FPS redraw.
- partial redraw and static cached instrument layers become important.
- layouts must support more information without reducing touch target quality.

## What Must Be Abstracted

### Board Identity

Current state:

- `include/board_pins.h` hard-codes one board.
- `platformio.ini` has one firmware environment.
- `main.cpp` directly constructs the RGB panel and touch/backlight peripherals.

Required abstraction:

```cpp
namespace board {

enum class DisplayBus {
  RgbParallel,
  Spi,
  Qspi,
  DsiBridge,
};

enum class TouchKind {
  None,
  GT911,
  FT5x06,
  CST816,
  XPT2046,
  BoardSpecific,
};

struct DisplayTiming {
  uint32_t pclk_hz;
  bool hsync_polarity;
  uint16_t hsync_front_porch;
  uint16_t hsync_pulse_width;
  uint16_t hsync_back_porch;
  bool vsync_polarity;
  uint16_t vsync_front_porch;
  uint16_t vsync_pulse_width;
  uint16_t vsync_back_porch;
  bool pclk_active_neg;
};

struct Geometry {
  uint16_t width_px;
  uint16_t height_px;
  uint16_t diagonal_tenths_in;
  uint8_t rotation;
  bool square;
  DisplayShape shape;
  DensityClass density_class;
  uint16_t usable_x;
  uint16_t usable_y;
  uint16_t usable_width;
  uint16_t usable_height;
};

struct Capabilities {
  bool psram_required;
  bool backlight_pwm;
  bool backlight_expander;
  bool touch_calibration;
  bool beeper;
  bool nmea2000_can;
  bool sd_card;
  DisplayBus display_bus;
  bool touch_interrupt;
};

const char *id();
const char *display_name();
Geometry geometry();
Capabilities capabilities();
const char *shape_name(DisplayShape shape);
const char *density_class_name(DensityClass density);
const char *layout_class_name(LayoutClass layout);
const char *touch_kind_name(TouchKind touch);
const char *display_bus_name(DisplayBus bus);

bool begin();
bool display_begin();
bool touch_begin();
bool backlight_begin();

}  // namespace board
```

Board-specific files:

- `include/boards/board_sunton_4848s040.h`
- `src/boards/board_sunton_4848s040.cpp`
- `include/boards/board_waveshare_touch_lcd_4.h`
- `src/boards/board_waveshare_touch_lcd_4.cpp`
- later: `board_makerfabs_7_parallel.cpp`, `board_qualia_rgb666.cpp`

Implemented Waveshare Tier 1 build profiles:

| PlatformIO env | Resolution | Shape | Density | Layout class |
|----------------|------------|-------|---------|--------------|
| `waveshare-touch-lcd-4` | 480x480 | square | `mdpi` | `square-480` |
| `waveshare-touch-lcd-4_3` | 800x480 | rectangle | `mdpi` | `landscape-800x480` |
| `waveshare-touch-lcd-4_3b` | 800x480 | rectangle | `mdpi` | `landscape-800x480` |
| `waveshare-touch-lcd-5_800x480` | 800x480 | rectangle | `mdpi` | `landscape-800x480` |
| `waveshare-touch-lcd-5_1024x600` | 1024x600 | rectangle | `hdpi` | `landscape-1024x600` |
| `waveshare-touch-lcd-7_800x480` | 800x480 | rectangle | `mdpi` | `landscape-800x480` |
| `waveshare-touch-lcd-7b_1024x600` | 1024x600 | rectangle | `hdpi` | `landscape-1024x600` |

Derived Waveshare environments extend the production ESP32-S3 RGB baseline and
reuse the 4 inch Waveshare build flags, then override board identity and panel
size:

```ini
[env:waveshare-touch-lcd-7b_1024x600]
extends = env:esp32-4848s040
build_flags =
  ${env:waveshare-touch-lcd-4.build_flags}
  -DBOARD_ID_WAVESHARE_TOUCH_LCD_7B_1024X600
  -DLCD_W=1024
  -DLCD_H=600
```

### Display Driver

Current state:

- ST7701 init sequence lives in `main.cpp`.
- RGB timing and pin map are macros.
- LVGL buffer height is hard-coded as `LCD_W * 40`.

Required abstraction:

```cpp
namespace display {

struct Config {
  uint16_t width_px;
  uint16_t height_px;
  uint8_t rotation;
  uint16_t preferred_buffer_rows;
  bool partial_refresh;
  bool full_refresh_required;
};

bool begin(const Config &cfg);
void flush(const lv_area_t *area, uint8_t *px_map);
uint32_t last_flush_px();
uint32_t frame_px();
float last_flush_ratio();

}  // namespace display
```

Rules:

- Display driver owns Arduino_GFX or alternate panel backend.
- `main.cpp` should not know ST7701/ST7262/GC9503 command arrays.
- Buffer rows are board/display dependent:
  - 480x480: 40 rows is acceptable.
  - 800x480: start with 20-40 rows.
  - large panels: choose rows based on PSRAM and flush timing.
- Prefer LVGL partial render mode unless a specific board requires full refresh.
- Track flush pixels and timing for performance diagnostics.

### Touch Driver

Current state:

- GT911 I2C polling and endian behavior are embedded in `main.cpp`.
- Gesture thresholds assume 480x480.
- Calibration assumes raw GT911 points.

Required abstraction:

```cpp
namespace input {

struct TouchPoint {
  int16_t x;
  int16_t y;
  int16_t raw_x;
  int16_t raw_y;
  bool pressed;
  uint32_t ms;
};

bool begin_touch();
bool latest_touch(TouchPoint &out);
bool latest_raw_touch(TouchPoint &out);
void set_geometry(uint16_t width, uint16_t height, uint8_t rotation);

}  // namespace input
```

Rules:

- Touch hardware task owns I2C/SPI touch reads.
- LVGL indev callback only copies a snapshot.
- Touch calibration stores one record per board id and rotation.
- Gesture thresholds scale with the smaller screen dimension.

Default thresholds:

```text
swipe_min_px = max(50, min(width, height) * 0.14)
tap_drift_px = swipe_min_px - 1
edge_zone_px = max(36, min(width, height) * 0.08)
```

### Backlight And Power

Current state:

- `ledcSetup`, `ledcAttachPin`, and `ledcWrite` are called directly.
- Settings write raw brightness values.

Required abstraction:

```cpp
namespace board {

enum class BacklightKind {
  None,
  LedcPwm,
  IoExpanderPwm,
  PanelCommand,
};

bool set_backlight(uint8_t value_0_255);
uint8_t backlight();
bool set_power(bool on);

}  // namespace board
```

Rules:

- Settings call `ui::set_brightness`, which calls `board::set_backlight`.
- Board implementation decides LEDC pin, expander register, or no-op.
- Brightness buckets store normalized 0-255 values, not pin-specific duty.
- Add optional auto-dim later using the same API.

### Optional Peripherals

Abstract:

- beeper/audio alert
- CAN/NMEA2000 pins and transceiver presence
- SD/FFAT storage
- RTC/battery monitor
- external buttons/rotary encoders
- IO expanders and reset sequencing
- USB serial chip quirks

Every optional capability must be discoverable through `board::capabilities()`.
Screens and settings hide unsupported controls.

## Layout Generalization

### Geometry Classes

```cpp
enum class LayoutClass {
  SquareCompact,      // 480x480, 4 inch
  LandscapeCompact,   // 800x480, 4.3-5 inch
  LandscapeWide,      // 800x480+, 7-9 inch
  PortraitCompact,
  PortraitTall,
};
```

Compute from geometry:

```text
aspect = width / height
square if 0.90 <= aspect <= 1.10
landscape if aspect > 1.10
portrait if aspect < 0.90
wide if diagonal >= 7.0 inch or width >= 800
```

### Screen Safe Area

Create a runtime layout context:

```cpp
namespace ui {

struct LayoutContext {
  uint16_t w;
  uint16_t h;
  uint16_t short_side;
  uint16_t long_side;
  bool square;
  bool landscape;
  bool wide;
  uint16_t margin;
  uint16_t gap;
  uint16_t touch_min;
};

LayoutContext layout_context();

}  // namespace ui
```

Rules:

- Replace direct `LCD_W/LCD_H` use in screens with `layout_context()` during
  migration.
- Keep `LCD_W/LCD_H` compatibility macros only as wrappers around board
  geometry until migration completes.
- Touch targets use physical intent:
  - minimum 44 px on 480x480
  - minimum 56-64 px on 800x480 if the panel is 7 inch
  - never below 9 mm physical target when diagonal is known

### Template Behavior By Geometry

| Template | SquareCompact | LandscapeCompact/Wide |
| --- | --- | --- |
| `quad_grid` | 2x2 grid | 4-6 tile strip/grid with optional hero |
| `hero_plus` | hero top, 1-4 values below | hero left, secondary column/right rail |
| `round_instrument` | centered dial, corner values | dial left/center, data/control rail right |
| `trend_chart` | value top, chart bottom | value left/top, wider chart with side stats |
| `status_list` | compact rows | grouped columns |
| `control_console` | mode row + button grid | status/dial left, controls right |
| `setup_form` | paged/compact rows | two-column settings form |
| `route_progress` | compact XTE bar/spatial view | spatial track view plus waypoint panel |

### Font Scaling

Do not scale font size directly with viewport width.

Use named roles:

- `caption`
- `body`
- `value`
- `hero`
- `instrument_label`
- `button`

Each board/display class maps roles to fonts. Example:

| Role | 480x480 | 800x480 |
| --- | --- | --- |
| caption | Montserrat 14 | Montserrat 16 |
| body | Montserrat 20 | Montserrat 22 |
| value | Montserrat 28 | Montserrat 32 |
| hero | Montserrat 48 | Montserrat 60/72 if available |
| button | Montserrat 14/20 | Montserrat 20/24 |

Add fonts only when needed; large font assets affect flash.

## Settings Generalization

Settings should be generated from descriptors, filtered by board capability and
display class.

Descriptor:

```cpp
enum class SettingKind {
  Segmented,
  Bucket,
  Action,
  Toggle,
  List,
  Numeric,
};

struct SettingSpec {
  const char *id;
  const char *label;
  SettingKind kind;
  const char *capability_required;
  const char *group;
  uint8_t option_count;
  const char *const *option_labels;
};
```

Groups:

- Display: brightness, theme, day/night mode, orientation.
- Data: source priority, Signal K target, NMEA2000 enable.
- Instruments: units, damping/response, alarm thresholds.
- Input: touch calibration, gesture sensitivity, external controls.
- Network: WiFi, BLE, OTA.
- Hardware: beeper, CAN, SD diagnostics.

Rules:

- Unsupported hardware controls are hidden.
- Dangerous controls require hold/confirm.
- Board-specific settings live in a Hardware group.
- Wide screens may show a two-column settings form.
- Square screens keep one row/control per setting.

## Configuration Schema

Add a hardware/display section to config exports:

```json
{
  "hardware": {
    "board_id": "sunton_4848s040",
    "display": {
      "class": "square_compact",
      "width": 480,
      "height": 480,
      "rotation": 0,
      "diagonal_tenths_in": 40
    },
    "capabilities": {
      "backlight": true,
      "touch_calibration": true,
      "beeper": false,
      "nmea2000_can": false,
      "sd_card": true
    }
  }
}
```

Rules:

- `hardware` is reported by the device; external config may request but not
  blindly overwrite board identity.
- Layout JSON may include geometry preferences but must validate against
  actual device geometry.
- Settings sync must not push unsupported hardware settings from one board to
  another.

## Build System

PlatformIO environments should select board implementation:

```ini
[env:esp32-4848s040]
build_flags =
  -D BOARD_ID_SUNTON_4848S040

[env:waveshare-touch-lcd-4]
build_flags =
  -D BOARD_ID_WAVESHARE_TOUCH_LCD_4

[env:makerfabs-7-parallel]
build_flags =
  -D BOARD_ID_MAKERFABS_7_PARALLEL
```

Rules:

- Only one board id macro may be defined.
- Common code includes `board.h`, not board-specific headers.
- Board implementations can be excluded with `src_filter` when needed.
- Native tests use a fake board implementation.

## Migration Plan

1. Introduce `board::geometry()` and compatibility `LCD_W/LCD_H` wrappers.
2. Move backlight setup/write into `board::set_backlight`.
3. Move display construction/init/flush into `display` module.
4. Move GT911 touch polling from `main.cpp` into `input_touch`.
5. Add `ui::LayoutContext`.
6. Convert settings to descriptor-driven groups.
7. Convert dashboard/nav/depth/status to `LayoutContext`.
8. Convert round instruments to geometry-aware `round_instrument`.
9. Add a fake board for native tests.
10. Add second real board environment, preferably another 480x480 square board.
11. Add first rectangular 800x480 board environment.

## Acceptance

- Existing `esp32-4848s040` build remains visually equivalent.
- No screen module directly initializes display, touch, or backlight hardware.
- Settings hide unsupported hardware features.
- Touch calibration persists per board id + rotation.
- `pio test -e native` covers layout class calculation and settings filtering.
- A square board can use the current screen stack.
- A rectangular board can run without overlapping text, even before every screen
  is visually optimized.
- Render metrics show flush area and timing per board/display class.

## Sources Checked

- Waveshare ESP32-S3-Touch-LCD-4 product page: 4 inch 480x480 ESP32-S3 touch
  board.
- Makerfabs MaTouch ESP32-S3 7 inch parallel TFT wiki: 7 inch parallel TFT with
  touch and Arduino RGB display examples.
- Espressif ESP32-S3 LCD EV Board documentation: examples of 3.95 inch
  480x480 RGB/FT5x06 and 4.3 inch 800x480 RGB/GT1151 classes.
- Adafruit Qualia ESP32-S3 for RGB-666 displays: ESP32-S3 RGB display carrier
  that can drive 480x480 square panels.
