# Gesture Subsystem Specification

Goal: make gestures reliable, scalable to any screen, independent of rendering,
and compatible with the existing LVGL + FreeRTOS architecture.

## Current State

The firmware already moved GT911 I2C polling out of the LVGL input callback:

- A FreeRTOS touch task samples GT911 at about 60 Hz.
- The task writes the latest touch point into a mutex-guarded snapshot.
- LVGL `touch_read_cb` copies that snapshot and never touches I2C.

This is the right low-level foundation.

The remaining weakness is that high-level gestures are still recognized through
LVGL `LV_EVENT_GESTURE` callbacks attached per screen. That couples navigation
gestures to the active LVGL tree and makes it harder to support global gestures,
screen-specific gestures, edge gestures, long presses, multi-finger gestures, or
input modalities beyond touch.

## Design Goals

- Keep GT911 I2C polling asynchronous and off the LVGL/rendering task.
- Recognize gestures from raw touch samples, not LVGL screen callbacks.
- Route gestures through the app event queue.
- Allow global gestures and screen-local gestures.
- Allow overlays such as MOB/alarm/settings to consume or block gestures.
- Preserve normal LVGL touch/click behavior for buttons, sliders, lists, and
  keyboards.
- Make gestures configurable per screen without editing `main.cpp`.
- Keep recognition bounded, deterministic, and allocation-free during runtime.

## Non-Goals

- Complex multi-touch UI in the first implementation.
- Arbitrary user-defined gesture scripts.
- Gesture recognition that depends on screen redraw or frame rate.
- Replacing LVGL pointer input for ordinary widgets.

## Architecture

```text
GT911 I2C task, core 0
  -> samples raw touch at ~60 Hz
  -> writes TouchSample into ring buffer / queue

Gesture recognizer task, core 0
  -> reads raw samples
  -> updates gesture state machine
  -> emits GestureEvent objects
  -> posts app::Command to UI queue

LVGL/UI task, core 1
  -> still receives pointer input for normal widgets
  -> drains app queue
  -> applies navigation/screen actions
```

LVGL remains the owner of UI mutation. The gesture recognizer never calls LVGL.

## Modules

Add:

- `include/input_touch.h`
- `src/input_touch.cpp`
- `include/gesture.h`
- `src/gesture.cpp`

Optional later:

- `include/gesture_map.h`
- `src/gesture_map.cpp`

### `input_touch`

Owns hardware sampling and raw touch data.

```cpp
namespace input {

enum class TouchPhase : uint8_t {
    Up,
    Down,
    Move,
};

struct TouchSample {
    int16_t x = -1;
    int16_t y = -1;
    bool pressed = false;
    uint8_t contacts = 0;
    uint32_t t_ms = 0;
};

void setup();
bool latest(TouchSample &out);
bool read_sample(TouchSample &out, uint32_t timeout_ms);

}  // namespace input
```

Implementation notes:

- Keep the existing GT911 endian behavior.
- Keep sample period near 16 ms.
- Use a small FreeRTOS queue or ring buffer for samples.
- Keep a latest snapshot for LVGL pointer reads.
- If the queue overflows, drop oldest samples and increment a diagnostic
  counter.

### `gesture`

Owns recognition and routing.

```cpp
namespace gesture {

enum class Type : uint8_t {
    None,
    Tap,
    LongPress,
    Swipe,
    EdgeSwipe,
    Drag,
};

enum class Direction : uint8_t {
    None,
    Left,
    Right,
    Up,
    Down,
};

enum class Zone : uint8_t {
    Any,
    LeftEdge,
    RightEdge,
    TopEdge,
    BottomEdge,
    Center,
};

struct Event {
    Type type = Type::None;
    Direction dir = Direction::None;
    Zone zone = Zone::Any;
    int16_t start_x = -1;
    int16_t start_y = -1;
    int16_t end_x = -1;
    int16_t end_y = -1;
    int16_t dx = 0;
    int16_t dy = 0;
    uint32_t duration_ms = 0;
    uint32_t t_ms = 0;
};

void setup();
bool latest(Event &out);
void set_enabled(bool enabled);

}  // namespace gesture
```

## Gesture State Machine

The recognizer tracks one active pointer sequence:

```text
Idle
  -> Pressed      first down sample
Pressed
  -> Dragging     movement exceeds drag threshold
  -> LongPressed  hold exceeds long-press threshold
  -> Idle         release before thresholds = tap
Dragging
  -> Idle         release = swipe/drag classification
LongPressed
  -> Idle         release = long press complete
```

Threshold defaults for 480x480:

- Tap max duration: 250 ms.
- Tap max movement: 12 px.
- Long press duration: 700 ms.
- Drag start movement: 14 px.
- Swipe min distance: 80 px.
- Swipe max off-axis ratio: 0.55.
- Edge width: 36 px.
- Gesture timeout: 1200 ms.

Thresholds should live in constants first. Later they may be layout settings.

## Gesture Routing

Gesture recognition and gesture action are separate.

The recognizer emits `gesture::Event`. A router maps events to app commands.

Global default map:

| Gesture | Action |
|---|---|
| Swipe left | Next visible screen |
| Swipe right | Previous visible screen |
| Swipe up from any zone | Show Settings |
| Swipe down from any zone | Show Dashboard |
| Long press MOB button zone | Trigger MOB |
| Long press active MOB overlay clear zone | Clear MOB |

Suggested edge gestures:

| Gesture | Action |
|---|---|
| Bottom edge swipe up | Show Settings or quick drawer |
| Top edge swipe down | Show status/alerts drawer |
| Left edge swipe right | Previous screen |
| Right edge swipe left | Next screen |

Screen-local maps:

- Wind: optional long press center toggles apparent/true emphasis.
- Depth: optional tap chart cycles time scale.
- Autopilot: no global swipe over action button hitboxes during press.
- WiFi Setup: swipes disabled while keyboard or password field is active.
- Settings: down swipe returns to previous screen or dashboard.

## Consumption And Blocking

Gestures must respect UI context:

- If MOB overlay is active, only MOB overlay gestures are accepted.
- If a modal, keyboard, or text input is active, navigation gestures are
  suppressed unless they start from a configured edge.
- If a touch begins on an interactive LVGL widget, global gestures are
  suppressed unless movement exceeds the swipe threshold and the widget opts in.
- Screen modules may register local gesture handlers.

Suggested API:

```cpp
namespace gesture {

using Handler = bool (*)(const Event &ev);  // true = consumed

void set_global_handler(Handler h);
void set_screen_handler(const char *screen_id, Handler h);
void set_blocked(bool blocked);

}  // namespace gesture
```

First implementation can keep routing internal and add handler registration
later.

## App Event Integration

Extend `app::CommandType`:

```cpp
GestureAction,
MobTrigger,
MobClear,
```

Or translate gestures directly to existing commands:

- `ShowScreen` with `a = "next"`, `"prev"`, `"settings"`, `"dashboard"`.
- `RunCommand` with `a = "mob"` or `"mob-clear"`.

Preferred first implementation:

- Gesture router emits existing `ShowScreen` and `RunCommand` commands.
- Add explicit command types only when needed.

## LVGL Pointer Integration

Keep LVGL pointer input for all normal UI controls:

- Buttons
- Sliders
- Lists
- Text areas
- Keyboard

The touch task should feed both:

- latest snapshot for LVGL pointer callback
- sample queue for gesture recognizer

Avoid double-executing interactions:

- A tap should normally be left to LVGL, not converted to a global gesture.
- Navigation should use swipes/edge swipes, not generic taps.
- Long press should only become a global action in explicit zones.

## Screen Registration

Current screens are registered by `ui::register_screen()`. The gesture subsystem
should use screen metadata instead of requiring per-screen LVGL event callbacks.

Recommended addition:

```cpp
struct ScreenGesturePolicy {
    bool allow_nav_swipe = true;
    bool allow_settings_swipe = true;
    bool allow_dashboard_swipe = true;
    bool suppress_when_text_input = true;
};

void ui::set_gesture_policy(const char *screen_id, const ScreenGesturePolicy &policy);
```

First implementation may use hardcoded policy by screen id:

- `wifi`: suppress swipes while keyboard active.
- `settings`: allow down-to-dashboard, left/right disabled or optional.
- `autopilot`: suppress gestures beginning on control buttons.

## Diagnostics

Expose gesture metrics through `bench`, `/api/state`, and BLE connection JSON:

- raw touch sample rate
- dropped touch samples
- last gesture type/direction
- gesture count
- suppressed gesture count
- average gesture recognition latency
- max recognizer loop duration

Example `/api/state` shape:

```json
{
  "input": {
    "touch_hz": 60,
    "touch_dropped": 0,
    "last_gesture": "swipe_left",
    "gestures": 123,
    "suppressed": 4
  }
}
```

## Failure Behavior

- If gesture task fails to start, LVGL touch still works.
- If gesture queue overflows, drop samples and keep latest snapshot.
- If app queue is full, log and drop gesture action.
- Never block I2C sampling waiting for UI.
- Never block LVGL rendering waiting for gesture recognition.

## Implementation Plan

1. Extract current touch task and snapshot code from `main.cpp` into
   `input_touch`.
2. Add a sample queue while preserving `input::latest()` for LVGL pointer input.
3. Update `touch_read_cb` to use `input::latest()`.
4. Add `gesture` recognizer task reading from the sample queue.
5. Implement swipe left/right/up/down and route to `app::post(ShowScreen)`.
6. Remove per-screen `LV_EVENT_GESTURE` callback attachment.
7. Add basic suppression:
   - MOB active blocks navigation gestures.
   - WiFi keyboard/text input blocks global gestures.
8. Add diagnostics counters.
9. Add manual hardware tests.

## Manual Test Matrix

1. Swipe left/right on every screen: moves through visible screens.
2. Swipe up on every screen: opens Settings.
3. Swipe down from Settings: returns to Dashboard.
4. Swipe down from any normal screen: returns to Dashboard.
5. Press buttons on Autopilot: no accidental screen change.
6. Drag brightness slider: no accidental screen change.
7. Type WiFi password: no accidental screen change.
8. MOB active: normal navigation swipes ignored.
9. Rapid swipes for 30 seconds: no crash and no render stall.
10. Signal K disconnected and WiFi scanning: gestures remain responsive.

## Acceptance Criteria

- No screen-specific LVGL gesture callbacks are required for global navigation.
- Gesture recognition continues when rendering is slow.
- Rendering continues when touch input is noisy.
- Normal LVGL controls continue to work.
- Global gestures are consistent on every screen.
- Screen-specific gesture policies can be added without editing the recognizer.
- `make test` and `make build` pass.

