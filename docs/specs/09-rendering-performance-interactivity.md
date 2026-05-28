# Rendering Performance And Interactivity

Status: proposed implementation plan. This document captures the current
performance risks, profiling plan, and rendering/interactivity improvements to
apply incrementally.

## Goal

Keep the UI responsive while preserving the marine-instrument visual quality:

- touch and gestures should be accepted even while a screen is expensive to
  render
- rendering should avoid unnecessary invalidation and redraw
- slow network/BLE/web work should not block the LVGL owner task
- performance decisions should be based on `/bench` and screen-specific
  measurements, not assumptions

## Current Rendering Model

The firmware uses LVGL partial render mode:

```text
LVGL invalidates changed areas
  -> renders invalidated regions into DMA-capable draw buffers
  -> disp_flush_cb sends those regions to Arduino_GFX / RGB panel
```

The display buffers are currently partial buffers, not full-screen
framebuffers. This is the right default for internal SRAM pressure.

Important detail: the app currently has a runtime `force-invalidate` path that
can call `lv_obj_invalidate(lv_screen_active())` every UI refresh cycle. That
forces a full active-screen redraw and defeats most of LVGL's partial
invalidation benefits. It was introduced because some animations/time-based
updates appeared to stop when LVGL saw no dirty objects.

## Can We Render Only Changed Parts?

Yes, LVGL is designed to render only invalidated regions, and the project is
already configured for partial render mode. The blocker is not LVGL capability;
the blocker is application behavior:

- full-screen `lv_obj_invalidate(lv_screen_active())` marks everything dirty
- repeated setters with unchanged values can mark objects dirty
- transforms on large object trees can create large invalidated bounding boxes
- rotating a parent group, such as the Wind bezel, can dirty a broad area
- animated "alive" placeholders force continuous redraw even without data

Therefore the plan is:

1. keep partial render mode
2. remove or scope forced full-screen invalidation
3. update only objects whose displayed value changed
4. avoid transforms on large containers where possible
5. measure after each change

## Metrics To Add

Current `/bench` reports FPS, flush average/peak, loop peak, LVGL peak, queue
depths, and slowest section. Extend it with render-specific counters:

- invalidated area count per second
- total invalidated pixels per second
- largest invalidated area in the last window
- flush count per second
- total flushed pixels per second
- largest flushed rectangle
- active screen id
- current `force-invalidate` mode
- current screen refresh duration
- worst per-screen refresh duration since last reset

If LVGL exposes invalid area details cleanly through the display event/callback
path, use that. Otherwise record flush rectangle sizes inside
`disp_flush_cb()`, which already receives `lv_area_t`.

Minimum derived metrics:

```text
flush_px = width * height
flush_pct = flush_px / (480 * 480)
flush_px_per_sec += flush_px
max_flush_px = max(max_flush_px, flush_px)
```

Add a command to reset render counters:

```text
bench-reset
```

## Profiling Plan

### Phase 1: Baseline

Record these on hardware for each important screen:

- Dashboard
- Wind
- Nav
- Steering
- WiFi keyboard
- Settings

Commands:

```text
bench
force-invalidate on
bench
force-invalidate off
bench
```

For Wind also test:

```text
wind-refresh on
bench
wind-refresh off
bench
```

Record:

- FPS
- flush average/peak
- flushed pixels/sec
- loop peak
- LVGL peak
- slowest section
- touch/gesture responsiveness

### Phase 2: Isolate Expensive Objects

On Wind, add temporary compile-time or console toggles:

- disable bezel rotation
- disable apparent wind marker rotation
- disable true wind marker rotation
- disable tide/waypoint marker rotation
- disable placeholder sweep animation
- disable labels only

The goal is to identify whether the cost comes from:

- large transformed bezel tree
- marker transforms
- labels
- forced full-screen invalidation
- screenshot/overlay/breadcrumb work

### Phase 3: Buffer Experiments

Test partial draw buffer heights:

- 20 rows
- 40 rows
- 60 rows
- 80 rows

Keep buffers in `MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL` if possible. Falling
back to PSRAM draw buffers should be treated as a performance risk and measured
separately.

Record memory low-water and flush peaks. Bigger buffers may reduce flush count
but increase internal SRAM pressure.

### Phase 4: Full-Frame / Double-Buffer Experiment

Only run this if tearing or full-screen redraw cost remains a real problem.

480 x 480 x 16 bpp is about 450 KB per frame. Two frames are about 900 KB.
That fits in PSRAM, but RGB panel DMA and PSRAM bandwidth can make it slower or
less stable than partial DMA buffers.

Experiment behind a compile-time flag:

```text
RENDER_EXPERIMENT_FULL_FRAME=1
RENDER_EXPERIMENT_DOUBLE_BUFFER=1
```

Acceptance for keeping it:

- no visible tearing regression
- touch remains responsive
- loop and LVGL peaks improve or remain acceptable
- internal SRAM low-water remains safe

Default should remain partial DMA buffers until data proves otherwise.

## Implementation Improvements

### 1. Dirty-Value Caches

Every screen refresh should cache displayed values and skip setters when the
displayed value is unchanged.

Example pattern:

```cpp
static char s_last_aws[16] = "";
if (strcmp(s_last_aws, buf) != 0) {
    strcpy(s_last_aws, buf);
    lv_label_set_text(lbl_aws_value, buf);
}
```

For rotations:

```cpp
static int16_t s_last_awa_rot = INT16_MIN;
int16_t rot = deg_to_lvgl(deg);
if (rot != s_last_awa_rot) {
    s_last_awa_rot = rot;
    lv_obj_set_style_transform_rotation(awa_marker, rot, 0);
}
```

Acceptance:

- no repeated LVGL setters for unchanged text/rotation/hidden state
- `/bench` shows reduced invalidated/flushed pixels

### 2. Screen-Specific Force Invalidate

Replace one global `force-invalidate` behavior with a policy:

```text
force-invalidate off
force-invalidate active
force-invalidate all
```

Then move toward per-screen declarations:

```cpp
enum class InvalidatePolicy {
    OnlyDirty,
    ActiveScreen,
    TimedAnimation,
};
```

Screens with no continuous animation should use `OnlyDirty`.

### 3. Remove Placeholder Sweep When Not Needed

Wind currently animates the apparent wind marker when no wind data exists so
the screen looks alive. This forces redraw even when the boat is offline.

Options:

- disable placeholder animation after the first few seconds
- update it at 1 Hz instead of every refresh
- replace it with static "no data" state

### 4. Avoid Rotating Large Object Trees

The Wind bezel is visually rich but expensive if the whole container rotates.
Options, in increasing implementation cost:

- quantize heading rotation to 2-5 degree steps
- update heading rotation at lower frequency than numeric labels
- keep the bezel static and rotate a small heading pointer instead
- pre-render the bezel as a bitmap/canvas and rotate only a small layer
- draw the bezel procedurally into a cached canvas only when heading changes
  past a threshold

Preferred first step: quantize and cache. Preferred second step: static bezel
plus small heading/current markers if profiling shows group rotation dominates.

### 5. Reduce Widget Count On Hot Screens

Wind contains many LVGL objects: rings, ticks, labels, arcs, markers, and data
boxes. If profiling shows object traversal is expensive:

- merge static ticks/rings into one canvas or bitmap
- remove invisible/covered decorative layers
- replace repeated small `lv_obj_create` ticks with `lv_line` or a canvas
- keep labels as LVGL objects for readability and localization

### 6. Prioritize Input Events

Navigation/touch events should not wait behind long-running UI work.

Options:

- add a small high-priority UI queue for input/navigation events
- drain navigation events before layout/apply/screenshot work
- coalesce repeated navigation gestures while one screen transition is pending

Do not process LVGL mutations outside the UI task.

### 7. Move Gesture Policy To UI Task

The touch task should classify raw gestures but not read UI state directly.

Recommended flow:

```text
touch task
  -> detects raw swipe
  -> posts UiGesture(direction, start/end/duration)
UI task
  -> checks active screen, MOB overlay, WiFi keyboard, settings state
  -> decides action
```

This avoids cross-core reads of UI state and aligns with the event-driven MVC
spec.

## Interactivity Improvements

### Touch Feedback

Buttons and controls should show immediate feedback before slow work starts:

- pressed visual state
- "queued" or "working" state
- disabled state while an operation is pending
- last result/error status

High-value targets:

- WiFi scan/connect
- Settings brightness/theme controls
- Autopilot controls
- Layout fetch/apply

### Queue Drop Visibility

If a UI or net event cannot be queued:

- log the drop
- increment a dropped counter
- expose the counter in `/api/state` or `/bench`

Gestures must only increment "handled" counters after queueing succeeds.

### Input During Render Spikes

During known render spikes:

- touch sampling should continue
- gesture events should queue
- UI task should process input before lower-priority refresh work once it
  becomes available

This means `app::pump()` should remain early in `ui_refresh()`, and any future
long UI-side operations should be chunked.

## Acceptance Criteria

Rendering:

- `force-invalidate off` or screen-specific invalidation works on screens that
  do not need continuous redraw.
- Wind no longer repeats setters for unchanged values.
- `/bench` shows per-screen render cost and flushed pixel counts.
- Wind screen LVGL peak is reduced versus baseline, or the expensive object is
  identified with measurements.

Interactivity:

- swipe/tap behavior is implemented as UI-task policy over raw gesture events.
- settings close/back behavior uses a distinct event, not a generic
  `ShowScreen("dashboard")` intercept.
- gestures report dropped events accurately.
- WiFi keyboard and settings controls do not lose taps to gesture recognition.

Regression:

```sh
pio run -e esp32-4848s040
pio test -e native
```

Hardware:

- no visible tearing regression
- touch remains responsive on Wind while data updates
- web `/api/state` remains responsive
- BLE console remains responsive during WiFi/Signal K operations

## Non-Goals

- Do not switch to full-screen double buffering by default without hardware
  measurements.
- Do not move LVGL calls off the UI task.
- Do not optimize by removing useful cockpit information before profiling.
- Do not add large persistent bitmap assets unless they clearly reduce runtime
  cost and fit flash/memory budgets.
