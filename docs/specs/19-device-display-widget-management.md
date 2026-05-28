# Device Display And Widget Management Spec

Status: draft.

## Goal

Let the SignalK `espdisp-manager` plugin centrally configure devices with
different display sizes, resolutions, orientations, and supported widget
features.

The firmware must report what it can render, then apply only the layout,
widget, and typography configuration selected for that device.

## Device Capability Contract

Registration must include display and widget capabilities.

```json
{
  "id": "espdisp-aabbccddeeff",
  "board": "esp32-4848s040",
  "model": "ESP32-4848S040",
  "display": {
    "width": 480,
    "height": 480,
    "rotation": 0,
    "colorDepth": 16,
    "density": "mdpi",
    "shape": "square",
    "safeArea": {
      "x": 0,
      "y": 0,
      "width": 480,
      "height": 480
    }
  },
  "touch": {
    "enabled": true,
    "width": 480,
    "height": 480,
    "controller": "GT911",
    "interrupt": true
  },
  "capabilities": {
    "widgets": {
      "numeric": true,
      "text": true,
      "gauge": true,
      "compass": true,
      "windRose": true,
      "trend": true,
      "bar": true,
      "button": true,
      "autopilot": true,
      "map": false
    },
    "fonts": {
      "scalable": false,
      "sizes": [10, 12, 14, 16, 18, 20, 24, 28, 32, 36, 42, 48, 56],
      "families": ["default"]
    },
    "layout": {
      "grid": true,
      "absolute": false,
      "variants": true
    }
  }
}
```

Firmware source of truth:

```text
board profile       -> physical display/touch geometry
compiled font table -> supported font sizes
widget registry     -> supported widget types
feature flags       -> optional features such as map/autopilot/NMEA
```

The device must not invent capabilities from server config. It reports only
what is actually compiled in and available at runtime.

The status heartbeat should repeat current display state when it changes:

```json
{
  "display": {
    "width": 480,
    "height": 480,
    "rotation": 0,
    "brightness": 0.8
  },
  "ui": {
    "screen": "dashboard",
    "theme": "day",
    "layoutVariant": "square-480",
    "widgetConfigHash": "sha256:..."
  }
}
```

## Firmware Data Model

Recommended internal structs:

```cpp
struct DisplayCaps {
    uint16_t width;
    uint16_t height;
    uint16_t rotation;
    uint8_t color_depth;
    const char *density;
    const char *shape;
    Rect safe_area;
};

struct FontCaps {
    bool scalable;
    uint8_t count;
    uint16_t sizes[16];
};

struct WidgetCaps {
    bool numeric;
    bool text;
    bool gauge;
    bool compass;
    bool wind_rose;
    bool trend;
    bool bar;
    bool button;
    bool autopilot;
    bool map;
};

struct WidgetStyle {
    uint16_t font_size;
    uint16_t label_font_size;
    uint16_t value_font_size;
    uint16_t unit_font_size;
};

struct WidgetDef {
    char id[32];
    WidgetType type;
    char title[24];
    char path[96];
    char unit[12];
    uint8_t precision;
    WidgetStyle style;
};

struct LayoutTile {
    char widget_id[32];
    uint8_t col;
    uint8_t row;
    uint8_t col_span;
    uint8_t row_span;
};

struct RenderPlan {
    char config_hash[72];
    char layout_variant[32];
    char widget_variant[32];
    WidgetDef widgets[MAX_WIDGETS];
    ScreenPlan screens[MAX_SCREENS];
};
```

Keep the parsed JSON document separate from the active render plan. The active
plan should contain only validated, clamped, firmware-native values.

## Firmware Module Boundaries

Suggested modules:

```text
include/device_caps.h
src/device_caps.cpp

include/manager_config.h
src/manager_config.cpp

include/widget_registry.h
src/widget_registry.cpp

include/font_resolver.h
src/font_resolver.cpp

include/render_plan.h
src/render_plan.cpp
```

Responsibilities:

- `device_caps`: build registration/status capability JSON.
- `manager_config`: parse and validate manager config payloads.
- `widget_registry`: map server widget type names to firmware handlers.
- `font_resolver`: clamp requested font sizes to compiled LVGL fonts.
- `render_plan`: convert validated config to screen/widget render state.

## Plugin Generated Config

The device config returned by:

```text
GET /plugins/espdisp-manager/devices/:id/config
```

must include selected display, layout, and widget sections.

```json
{
  "display": {
    "width": 480,
    "height": 480,
    "rotation": 0,
    "selectedVariant": "square-480"
  },
  "layout": {
    "version": 1,
    "variant": "square-480",
    "screens": []
  },
  "widgets": {
    "version": 1,
    "variant": "square-480",
    "defaults": {
      "fontSize": 18,
      "labelFontSize": 12,
      "valueFontSize": 34,
      "unitFontSize": 14
    },
    "items": {
      "sog": {
        "type": "numeric",
        "title": "SOG",
        "path": "navigation.speedOverGround",
        "unit": "kn",
        "fontSize": 42,
        "precision": 1
      }
    }
  }
}
```

## Variant Matching

The plugin may define multiple variants:

```json
{
  "layout": {
    "variants": [
      {
        "id": "square-480",
        "match": {
          "display": {
            "width": 480,
            "height": 480
          }
        },
        "screens": []
      },
      {
        "id": "wide-800x480",
        "match": {
          "display": {
            "width": 800,
            "height": 480
          }
        },
        "screens": []
      }
    ]
  }
}
```

Firmware does not choose between variants. It reports capabilities; the plugin
selects one and sends the selected variant only.

If the plugin sends multiple variants by mistake, firmware should:

1. Prefer `layout.variant`.
2. Fall back to exact width/height match.
3. Reject config if no variant matches.

## Widget Model

Widgets are named reusable render definitions. Screens may reference widgets by
id instead of duplicating display logic.

```json
{
  "widgets": {
    "defaults": {
      "fontSize": 18,
      "labelFontSize": 12,
      "valueFontSize": 34,
      "unitFontSize": 14
    },
    "items": {
      "batteryVoltage": {
        "type": "numeric",
        "title": "BAT",
        "path": "electrical.batteries.house.voltage",
        "unit": "V",
        "precision": 1,
        "fontSize": 30
      },
      "autopilotState": {
        "type": "text",
        "title": "AP",
        "path": "steering.autopilot.state",
        "fontSize": 24,
        "requires": {
          "capability": "autopilotControls"
        }
      }
    }
  }
}
```

Supported v1 widget types:

```text
numeric
text
gauge
compass
windRose
trend
bar
button
autopilot
```

Firmware must ignore unknown widget ids that are not referenced by screens.
Firmware must reject a screen that references an unsupported widget type.

## Widget Type Semantics

### `numeric`

Required fields:

```text
path
title
```

Optional fields:

```text
unit
precision
min
max
warningBelow
warningAbove
alarmBelow
alarmAbove
```

Renders a numeric value from the normalized boat data model or raw SignalK path.

### `text`

Required fields:

```text
path
title
```

Optional fields:

```text
transform
placeholder
```

Renders string/enumerated values such as autopilot state.

### `gauge`

Required fields:

```text
path
min
max
```

Optional fields:

```text
zones
unit
precision
```

Renders radial or linear gauge depending on layout tile shape.

### `compass`

Required fields:

```text
path
```

Expected value is radians unless `unit` says otherwise. Firmware should prefer
normalized fields for heading/course where available.

### `windRose`

Required fields:

```text
anglePath
speedPath
```

Renders apparent or true wind according to paths.

### `trend`

Required fields:

```text
path
windowSec
```

Uses existing history buffers where available. If no buffer exists, firmware
may render the current value and report a warning in `config.error`.

### `bar`

Required fields:

```text
path
min
max
```

Renders compact horizontal or vertical bar.

### `button`

Required fields:

```text
action
title
```

Allowed actions in v1:

```text
screen.set
theme.set
config.reload
layout.reload
autopilot.standby
```

Potentially dangerous actions such as autopilot engage must require explicit
firmware-side permission flags.

### `autopilot`

Composite widget for autopilot state/target. It must obey managed autopilot
permissions:

```text
autopilot.allowEngage
autopilot.allowStandby
autopilot.allowHeadingAdjust
```

## Data Path Resolution

Widget paths may be either:

```text
raw SignalK path: navigation.speedOverGround
local alias:      boat.sog
```

Firmware should resolve paths in this order:

1. local normalized alias
2. known SignalK parser field
3. raw SignalK dynamic value if stored
4. missing value

Recommended aliases:

```text
boat.sog
boat.cogTrue
boat.headingTrue
boat.depth
boat.aws
boat.awa
boat.tws
boat.twa
boat.batteryVoltage
boat.batterySoc
boat.autopilotState
boat.autopilotTarget
```

Missing values render as placeholder text such as `--`, not as zero.

## Font Size Rules

Firmware font handling must be deterministic.

Input font values:

```text
fontSize
labelFontSize
valueFontSize
unitFontSize
titleFontSize
buttonFontSize
```

Rules:

1. Widget-specific font size overrides widget defaults.
2. Widget defaults override theme defaults.
3. Firmware must clamp to supported sizes.
4. If fonts are bitmap-only, choose nearest available size.
5. Text must not overflow fixed controls; reduce to next supported size if
   needed.
6. Font size is in CSS-like logical pixels before display scaling.

Example:

```json
{
  "widgets": {
    "defaults": {
      "valueFontSize": 34
    },
    "items": {
      "sog": {
        "valueFontSize": 42
      }
    }
  }
}
```

The `sog` widget renders value text at `42` if supported; otherwise nearest
lower supported size.

## Font Resolver Details

The firmware should maintain a static table:

```cpp
struct FontEntry {
    uint16_t size;
    const lv_font_t *font;
};
```

Resolution algorithm:

```text
requested <= smallest supported -> smallest
requested exactly supported     -> requested
requested between sizes         -> nearest lower
requested > largest supported   -> largest
```

Use nearest lower rather than nearest absolute to avoid accidental overflow.

If a string does not fit:

1. Try abbreviated label if configured.
2. Reduce font to next lower supported size.
3. If still too wide, truncate with ellipsis for labels.
4. For numeric values, reduce precision before truncating.

The layout engine must never resize a tile because of text size.

## Screen References

Layouts may reference widget ids:

```json
{
  "layout": {
    "screens": [
      {
        "id": "dashboard",
        "type": "grid",
        "tiles": [
          {
            "widget": "sog",
            "area": {
              "col": 0,
              "row": 0,
              "colSpan": 2,
              "rowSpan": 1
            }
          }
        ]
      }
    ]
  }
}
```

Firmware should resolve `widget` references during config validation, not at
draw time.

## Render Plan Generation

Validation should produce a render plan:

```text
manager JSON -> validate -> clamp fonts -> resolve widgets -> resolve screens -> render plan
```

The render plan should contain:

- selected layout variant
- selected widget variant
- screen list
- tile geometry in pixels or grid units
- resolved widget definitions
- resolved LVGL font pointers
- required data paths

The UI layer should render from the render plan only. It should not traverse
the original JSON during normal draw/refresh.

## Persistence

Persist:

```text
last accepted config JSON
last accepted config hash
last accepted config version
last selected layout variant
last selected widget variant
```

Do not persist:

```text
server-provided generatedAt as an apply decision
failed config as active config
device tokens inside layout/widget config
```

Boot behavior:

1. Load last accepted render plan/config.
2. Start UI with baked fallback if no accepted config exists.
3. Fetch manager config after network is available.
4. Apply new config only after full validation.

## Config Apply Rules

On config fetch:

1. Verify `protocol`.
2. Verify `deviceId`.
3. Verify selected display variant is compatible.
4. Verify all referenced widget ids exist.
5. Verify all referenced widget types are supported.
6. Clamp supported font sizes.
7. Build an internal render plan.
8. Persist only after validation succeeds.
9. Report `config.applied=true` and `widgetConfigHash` in heartbeat.

If validation fails:

```json
{
  "config": {
    "version": 17,
    "hash": "sha256:...",
    "applied": false,
    "error": {
      "code": "unsupported_widget_type",
      "message": "widget map requires unsupported type map",
      "path": "widgets.items.chart"
    }
  }
}
```

Validation error codes:

```text
invalid_protocol
wrong_device
display_mismatch
unsupported_layout_type
unsupported_widget_type
missing_widget
invalid_font_size
too_many_widgets
too_many_screens
too_many_tiles
invalid_path
invalid_action
out_of_memory
```

Limits for v1:

```text
MAX_WIDGETS = 32
MAX_SCREENS = 16
MAX_TILES_PER_SCREEN = 16
MAX_WIDGET_ID = 31 chars + NUL
MAX_PATH = 95 chars + NUL
MAX_TITLE = 23 chars + NUL
```

If the config exceeds limits, reject the whole config and continue running the
previous accepted plan.

## Commands Related To Widgets

The firmware should support these management commands:

```text
config.reload
layout.reload
screen.set
theme.set
brightness.set
```

Additional widget-specific command:

```json
{
  "type": "widget.refresh",
  "payload": {
    "widget": "sog"
  }
}
```

`widget.refresh` is optional in v1. If unsupported, firmware must ack with:

```json
{
  "ok": false,
  "code": "unsupported_command",
  "message": "widget.refresh is not supported"
}
```

## Status Reporting

Heartbeat should include:

```json
{
  "display": {
    "width": 480,
    "height": 480,
    "rotation": 0,
    "brightness": 0.8
  },
  "ui": {
    "screen": "dashboard",
    "theme": "day",
    "layoutVariant": "square-480",
    "widgetVariant": "square-480",
    "widgetConfigHash": "sha256:..."
  },
  "config": {
    "version": 17,
    "hash": "sha256:...",
    "applied": true,
    "layoutVariant": "square-480",
    "widgetVariant": "square-480"
  }
}
```

If config failed:

```json
{
  "config": {
    "version": 18,
    "hash": "sha256:...",
    "applied": false,
    "activeHash": "sha256:previous",
    "error": {
      "code": "missing_widget",
      "message": "screen dashboard references missing widget sog",
      "path": "layout.screens[0].tiles[0].widget"
    }
  }
}
```

## Firmware Implementation Plan

### Phase D1: Capability Reporting

- Add display geometry to device identity.
- Add widget capability flags.
- Add supported font size list.
- Add heartbeat display/ui fields.

### Phase D2: Config Parser

- Parse `display`, `layout`, and `widgets`.
- Preserve unknown fields for forward compatibility where possible.
- Reject unsupported widget references.
- Enforce fixed upper bounds before allocating render objects.

### Phase D3: Font Resolver

- Add font size resolver:

```text
requested size -> supported size -> LVGL font pointer
```

- Add overflow fallback for fixed-size labels.
- Report resolved sizes in debug logs when applying config.

### Phase D4: Widget Registry

- Create firmware-side widget registry:

```text
numeric
text
gauge
compass
windRose
trend
bar
button
autopilot
```

- Map widget config to existing LVGL screen/widget builders.
- Add data-path resolver from widget path to normalized boat fields.

### Phase D5: Layout Variant Application

- Replace baked screen layout with selected server-provided render plan where
  available.
- Keep baked default as fallback.
- Add `layout.reload` command path that rebuilds render plan from persisted
  config.

### Phase D6: Tests

Add host/unit tests for:

- exact 480x480 match
- 800x480 wide match
- unsupported widget rejection
- font clamping
- missing widget reference rejection
- stable config hash reporting

Add system tests for:

- plugin assigns square layout to 480x480 device
- plugin assigns wide layout to mock 800x480 firmware
- command reload applies changed widget font size

### Phase D7: Diagnostics

Add console commands:

```text
manager-layout
manager-widgets
manager-config-dump
font-dump
```

Expected output:

```text
[mgr] layout variant=square-480 screens=5 widgets=12 hash=sha256:...
[font] requested=42 resolved=42 family=default
```

## Open Questions

- Should firmware support absolute positioning, or only grid layouts in v1?
- Which LVGL font sizes are compiled into each target?
- Should font sizes be global per screen or per widget?
- Should unsupported widgets be hidden or reject the whole config?
- Should widget paths use raw SignalK paths or local normalized field names?
