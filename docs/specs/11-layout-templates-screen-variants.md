# Layout Templates And Screen Variants

Status: proposed implementation spec. This defines reusable screen layouts so
individual screens become variants of a small set of native LVGL templates.

## Goals

- Keep the UI visually consistent across screens.
- Avoid hand-built layout drift in every `screen_*.cpp` file.
- Make layout JSON expressive without becoming arbitrary CSS.
- Support tap-to-detail navigation from dashboard tiles and secondary values.
- Preserve fast native rendering and bounded embedded parsing.

## Model

A screen is a variant of a layout template:

```text
screen = template + variant settings + metric bindings + commands
```

Examples:

- Dashboard is a `quad_grid` variant with four metric tiles.
- Nav is a `hero_plus` variant with SOG as the hero and COG/heading as
  secondary values.
- Wind is a `round_instrument` variant with wind angle markers.
- Status is a `status_list` variant with rows and bars.

`type` remains the stable native renderer id for compatibility. Add
`template` when the screen can be rendered by the shared layout system.

## JSON Shape

```json
{
  "id": "dashboard",
  "title": "Dashboard",
  "type": "dashboard",
  "template": "quad_grid",
  "variant": "cruise_scan",
  "tiles": [
    {
      "id": "wind",
      "label": "Wind",
      "primary_path": "environment.wind.speedApparent",
      "secondary_path": "environment.wind.angleApparent",
      "unit": "kn",
      "accent": "amber",
      "tap": { "command": "show_screen", "screen": "wind" }
    }
  ],
  "settings": {}
}
```

Rules:

- `id` values are stable widget ids for API addressing.
- `tap` commands are declarative and are posted to the app command queue.
- Missing `template` falls back to the current native `type` renderer.
- Fixed-size arrays should be capped per template during parsing.

## Template Catalog

### `quad_grid`

Purpose: quick scan dashboard and drill-down entry point.

Layout:

- 2x2 square grid on 480x480.
- Optional compact footer for Signal K/WiFi/status.
- Each tile has caption, primary value, unit, secondary text or icon, and
  semantic accent rail.

Use for:

- Dashboard
- Diagnostics overview
- Shallow-water quick scan
- Passage summary

Interaction:

- Tap tile: navigate to the screen named by the tile binding.
- Long press tile, later: open widget configuration or pin/swap metric.
- Tile degraded state: tap still navigates to the detail screen.

Implementation notes:

- Use fixed 2-column/2-row LVGL grid or explicit coordinates.
- Tile dimensions must not change when values update.
- Store each tile's command in `lv_obj_set_user_data`.

### `hero_plus`

Purpose: one primary value or instrument with 1-4 supporting values.

Layout variants:

- `hero_plus_1`: hero with one secondary strip.
- `hero_plus_2`: hero with two secondary panels.
- `hero_plus_3`: hero with three compact secondary panels.
- `hero_plus_4`: hero with four compact secondary panels around or below the
  hero.

Use for:

- Nav: SOG hero, COG/heading/position secondary.
- Route: DTW or BTW hero, XTE/TTG/ETA/VMG secondary.
- Trip: elapsed or distance hero, average/max/distance secondary.
- Depth simple mode: depth hero, temperature/min/max secondary.

Interaction:

- Tap hero: open the primary metric detail or cycle an approved submode.
- Tap secondary value: navigate to its metric detail screen if one exists.

Implementation notes:

- Create a `HeroPlusSpec` with fixed `secondary_count <= 4`.
- Use semantic alignment: navigation values right-aligned, status values
  two-column aligned, position text two-line and fixed-width.
- Avoid relayout on value changes.

### `round_instrument`

Purpose: spatial instrument for angle, direction, and steering tasks.

Variants:

- `wind_round`: bow-up wind dial with apparent/true markers.
- `steering_round`: heading arc, heading bug, COG/CTS marker, XTE bar.
- `compass_round`: heading/COG compass repeater.
- `rudder_round`: rudder angle with centered zero and port/starboard zones.
- `autopilot_round`: target heading/wind angle with large mode and heading-step
  controls.

Use for:

- Wind
- Steering
- Autopilot heading view
- Future compass repeater

Interaction:

- Tap center: cycle apparent/true/combined mode where relevant.
- Tap side metric: navigate to wind, route, or autopilot detail.
- Hold on autopilot target controls must require explicit confirmation before
  state-changing commands.

Implementation notes:

- Prefer static arcs, cached tick geometry, and marker updates over rebuilding
  the dial.
- If profiling shows LVGL object rotation is expensive, move the dial face to a
  cached canvas or image and redraw only markers.
- Keep port/starboard color semantics stable.
- In day mode, support a white instrument face with gray tick marks and dark
  labels.

### `trend_chart`

Purpose: primary value plus recent history.

Layout:

- Hero value at top or upper-left.
- Fixed chart area with threshold line and min/max labels.
- Optional two secondary values below or beside the chart.

Use for:

- Depth trend
- Speed trend
- Battery voltage/SOC trend
- Wind shift trend

Interaction:

- Tap chart: cycle time window.
- Tap threshold label: open the related setting when safe.

Implementation notes:

- Use ring buffers and append-only updates.
- Do not autoscale every frame; scale on window/threshold changes or bounded
  intervals.

### `split_pair`

Purpose: compare two equally important values.

Layout:

- Two large panels side-by-side or stacked depending on orientation.
- Optional shared center indicator.

Use for:

- Heading vs COG.
- Apparent vs true wind.
- Battery house vs starter.
- Port vs starboard tank/engine in future variants.

Interaction:

- Tap either side: navigate to that metric's detail.

### `status_list`

Purpose: dense operational status without pretending every value is a dashboard
tile.

Layout:

- Title/header area.
- Rows with label, value, state color, optional bar.
- Optional action row at bottom.

Use for:

- Device status
- Signal K status
- Tank/battery systems
- Settings summaries

Interaction:

- Tap row: show detail or edit related setting.
- Tap action row: post command such as scan WiFi or reconnect Signal K.

### `control_console`

Purpose: safe touch controls with visible command state.

Layout:

- State panel at top.
- Target/current value block.
- Large command buttons in fixed grid.
- Pending/result message area.

Use for:

- Autopilot
- WiFi connect/forget
- Trip reset
- Future alarm acknowledgement

Interaction:

- All actions post commands asynchronously.
- State-changing actions show queued, pending, success, and failure feedback.
- Destructive actions use hold or confirmation.

### `route_progress`

Purpose: active waypoint/route progress with cross-track context.

Layout:

- Waypoint header.
- DTW/BTW or ETA hero.
- XTE centerline bar.
- Secondary values for TTG, VMG, ETA, route name.
- Optional spatial track view with boat at bottom-center, centerline/grid, and
  XTE offset.

Use for:

- Route
- Steering route submode

Interaction:

- Tap XTE bar: open steering screen.
- Tap waypoint header: open route detail.
- Tap spatial track view: toggle compact/details view.

### `setup_form`

Purpose: touch-safe configuration and provisioning.

Layout:

- Scrollable rows or paged groups.
- Large controls: toggles, sliders, segmented choices, text entry.
- Fixed action bar for apply/cancel when needed.

Use for:

- WiFi setup
- Settings
- Layout/widget configuration

Interaction:

- Controls update local draft state first.
- Apply persists through the configuration storage/sync subsystem.

### `alert_focus`

Purpose: urgent full-screen state.

Layout:

- Full-screen severity color frame.
- Large alarm title.
- Primary bearing/distance/value if relevant.
- Acknowledge/silence/clear controls.

Use for:

- MOB
- Shallow alarm
- Signal K data loss alarm
- Autopilot command failure escalation

Interaction:

- Acknowledge is explicit.
- Clear for emergency states requires hold or confirmation.

## Screen Variant Map

Default mapping:

| Screen | Template | Variant |
| --- | --- | --- |
| Dashboard | `quad_grid` | `cruise_scan` |
| Wind | `round_instrument` | `wind_round` |
| Nav | `hero_plus` | `sog_navigation` |
| Depth | `trend_chart` | `depth_sounder` |
| Steering | `round_instrument` | `steering_round` |
| Route | `route_progress` | `active_waypoint` |
| Autopilot | `control_console` | `heading_control` |
| Trip | `hero_plus` | `trip_computer` |
| Status | `status_list` | `device_and_vessel_status` |
| WiFi Setup | `setup_form` | `wifi_provisioning` |
| Settings | `setup_form` | `device_preferences` |

## C++ Implementation Plan

Add a small layout factory:

```cpp
// include/ui_layouts.h
namespace ui::layouts {

enum class TemplateId {
  QuadGrid,
  HeroPlus,
  RoundInstrument,
  TrendChart,
  SplitPair,
  StatusList,
  ControlConsole,
  RouteProgress,
  SetupForm,
  AlertFocus,
};

struct MetricBinding {
  const char *id;
  const char *label;
  const char *unit;
  const char *primary_path;
  const char *secondary_path;
  const char *target_screen;
  uint32_t accent;
};

struct ScreenVariantSpec {
  const char *screen_id;
  const char *title;
  TemplateId template_id;
  const MetricBinding *metrics;
  uint8_t metric_count;
  uint8_t variant_flags;
};

lv_obj_t *create(lv_obj_t *parent, const ScreenVariantSpec &spec);
void update(lv_obj_t *root, const ScreenVariantSpec &spec, const sk::Data &data);

}  // namespace ui::layouts
```

Implementation files:

- `include/ui_layouts.h`
- `src/ui/ui_layouts.cpp`
- Optional template-specific files once the factory grows:
  - `src/ui/layout_quad_grid.cpp`
  - `src/ui/layout_hero_plus.cpp`
  - `src/ui/layout_round_instrument.cpp`

Rules:

- Layout creation happens on the LVGL/UI task only.
- Update functions mutate existing LVGL objects instead of recreating them.
- Data lookup initially uses `sk::Data`; later variants can bind directly to
  parsed Signal K paths.
- Template roots own their child widget handles in a compact struct stored as
  user data or in the screen module.
- Tap handlers post `app::Command` values; they must not directly call screen
  transition logic from the LVGL callback.

## Tap-To-Detail Command Flow

```text
LV_EVENT_CLICKED on tile
  -> read MetricBinding.target_screen
  -> post Command::ShowScreen(target_screen)
  -> app controller validates target
  -> UI task performs transition
```

This keeps touch callbacks thin and makes the behavior compatible with the
event-driven MVC/MVU-lite proposal.

## Migration Order

1. Extract the dashboard 2x2 tile code into `quad_grid`.
2. Convert Nav and Trip to `hero_plus`.
3. Convert Depth to `trend_chart`.
4. Convert Status to `status_list`.
5. Convert Steering and Wind to `round_instrument` after profiling the current
   dial rendering.
6. Convert Autopilot to `control_console`.
7. Convert WiFi and Settings to `setup_form`.
8. Add JSON parsing for `template`, `variant`, `tiles`, and declarative `tap`
   commands.

## Acceptance

- Dashboard tiles tap through to the correct detail screen.
- Shared templates preserve the visual language from the appearance redesign.
- Value updates do not resize tiles, panels, or round instruments.
- Existing screen `type` renderers continue to work while templates migrate.
- `pio run -e esp32-4848s040` and `pio test -e native` pass after code changes.
