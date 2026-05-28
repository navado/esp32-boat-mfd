# Event-Driven MVC Redesign

Status: proposed architecture direction. This document describes the target
shape and migration path; it is not a claim that the current code already
fully implements it.

## Goal

Make the firmware behave like a small event-driven MVC system:

- inputs emit typed events
- controllers handle events and side effects
- models hold authoritative state
- views render from models and do not perform slow I/O

The goal is not a desktop-style MVC framework. The goal is a clear ownership
model that keeps LVGL responsive, keeps callbacks short, and makes web, BLE,
serial, touch, and timers use the same command path.

## Target Shape

```text
Inputs
  touch / gestures / BLE / web / serial / timers / Signal K
        |
        v
app event queues
        |
        v
controllers / reducers
  update state, call services, enqueue effects
        |
        v
models
  Signal K data, UI state, layout config, WiFi state
        |
        v
views
  LVGL screen modules render/refresh from model snapshots
```

## Terms

### Model

Authoritative state owned by a module, with explicit read/write contracts.

Current and future models:

- `sk::data` or a Signal K snapshot model
- `layout::Config`
- configuration domains and sync metadata described in
  `08-configuration-storage-sync.md`
- UI state: active screen, theme, brightness, overlays, alarms
- WiFi/network state: `net::WifiState`, IP, RSSI, saved networks
- outbound operation status: last Signal K PUT, last layout fetch, last WiFi
  action

Models should not call LVGL, web server APIs, BLE APIs, or blocking network
operations.

### View

LVGL-backed screen modules under `src/ui/`.

View rules:

- A view may create, own, and refresh LVGL objects for its screen.
- A view reads model snapshots or receives refresh data.
- A view should avoid LVGL setters when the displayed value has not changed;
  rendering performance guidance lives in `09-rendering-performance-interactivity.md`.
- A view does not perform HTTP, WiFi scans, BLE notifications, NVS writes, or
  reboots directly.
- User interactions inside a view emit events instead of mutating global state
  through ad hoc calls.

Expected per-screen interface over time:

```cpp
struct Screen {
    const char *id;
    lv_obj_t *(*build)();
    void (*refresh)(const AppSnapshot &state);
    bool (*set_attr)(const char *widget, const char *attr, JsonVariant value);
};
```

The exact type names can differ. The important point is that screen modules
become explicit view adapters.

### Controller

Event handlers that translate events into model updates and effects.

Examples:

- UI controller: screen navigation, theme, brightness, overlay state
- Network controller: WiFi save/forget/scan, reboot, device identity
- Signal K controller: outbound PUT requests, connection state
- Layout controller: apply layout, reload defaults, fetch layout
- Widget controller: OpenHASP-style runtime widget addressing

Controllers may enqueue slow effects to worker tasks. Controllers must respect
the LVGL single-owner rule.

### Event

A typed request or notification crossing module/task boundaries.

Current code uses `app::Command` as the first event bus. The redesign should
evolve this toward explicit event categories rather than introducing a second
parallel dispatcher.

Candidate categories:

```cpp
enum class EventType : uint8_t {
    UiShowScreen,
    UiSetTheme,
    UiSetBrightness,
    UiGesture,

    NetSaveWifi,
    NetForgetWifi,
    NetScanWifi,
    NetSetDeviceId,
    NetReboot,

    SignalKPut,
    SignalKSetTarget,

    LayoutApply,
    LayoutFetch,
    LayoutReloadDefault,

    WidgetSetAttr,
};
```

Names are illustrative. Prefer the repo's existing naming style when
implementing.

## Task Ownership

Hard rule: only the LVGL/UI owner task may call LVGL APIs or mutate
LVGL-backed objects.

Recommended ownership:

- Core 1 / UI owner:
  - `lv_timer_handler()`
  - touch and gesture interpretation
  - screen changes
  - screen refresh
  - screenshots
  - layout application to LVGL-backed views
- Core 0 / I/O workers:
  - WebServer and DNSServer
  - WiFi manager
  - BLE callbacks and advertising watchdog
  - Signal K WebSocket and HTTP writes
  - layout fetch over HTTP

Cross-task communication should be through FreeRTOS queues, semaphores, event
groups, or small protected snapshots. Do not add direct cross-task LVGL calls.

## Event Flow Examples

### Web Requests Screen Change

```text
POST /api/screen/wind
  -> web handler posts UiShowScreen("wind")
  -> UI controller handles event on UI task
  -> screen manager loads Wind view
```

The web task never calls `ui::show()` directly.

### BLE Requests WiFi Scan

```text
BLE NUS RX "scan"
  -> BLE callback posts NetScanWifi
  -> net worker runs WiFi.scanNetworks()
  -> logs/status model updated
  -> UI refresh reads status if needed
```

The BLE callback stays short and does not block on WiFi.

### Autopilot Button Sends Signal K PUT

```text
LVGL button event
  -> view emits SignalKPut(path, value)
  -> controller queues net-side effect
  -> net worker performs HTTP PUT
  -> operation status is logged/stored
```

Touch/rendering cannot freeze while HTTP is slow.

### Runtime Widget Attribute Set

```text
POST /api/widget/wind/awa_marker/color
  -> web handler posts WidgetSetAttr
  -> UI task dispatches to Wind view set_attr()
  -> view updates LVGL object
```

This is the bridge from the event-driven architecture to the OpenHASP-style
runtime addressing work.

## Migration Plan

### Phase 1: Document Existing Event Bus

Treat `app::Command` as the current event bus. Keep `app::post()`,
`app::post_net()`, and `app::pump()` as the initial boundary.

Acceptance:

- Web/BLE/serial paths that can block or mutate UI route through the queue.
- Queue ownership for heap blobs is explicit.
- `/bench` exposes queue and slow-section diagnostics.

### Phase 2: Add UI State Model

Introduce a small `UiState` or `AppState` model for:

- active screen id/index
- theme
- brightness
- overlay flags
- last gesture/event counters

Acceptance:

- Screen refresh code can read a coherent UI state snapshot.
- Theme/brightness/screen changes are represented as state transitions, not
  only direct side effects.

### Phase 3: Split Events by Domain

Replace broad `RunCommand` use for mutating actions with typed events where
practical.

Keep `net::dispatchCommand()` as the legacy console funnel, but make it parse
into typed events for mutating operations.

Acceptance:

- `theme`, `bright`, `view`, `wifi`, `wifi-forget`, `layout-fetch`, and
  `sk ...` have typed event paths.
- Read-only commands may remain synchronous if they are fast.

### Phase 4: Formalize Screen View Interface

Give each screen module an explicit build/refresh/set-attribute interface.

Acceptance:

- `screen_manager` can enumerate screens by id.
- Runtime widget addressing can start with one screen without changing the
  whole renderer.
- Views do not invoke network or BLE work directly.

### Phase 5: Add Controllers/Reducers

Move event handling out of ad hoc switch sites into domain-specific handlers:

- `ui_controller`
- `net_controller`
- `signalk_controller`
- `layout_controller`

Acceptance:

- `app_events.cpp` remains the queue boundary and dispatcher, but detailed
  behavior lives in domain controllers.
- Each controller has a narrow dependency set.

### Phase 6: Snapshot Shared Models

Protect shared state crossing tasks:

- Signal K data should be copied or double-buffered for UI reads.
- layout JSON/config reads should not race layout replacement.
- WiFi saved network list should not race manager reads.

Acceptance:

- No free/replace races.
- UI reads coherent snapshots.
- Web/BLE can expose status without locking for long periods.

## Non-Goals

- Do not introduce dynamic allocation-heavy MVC objects for every widget.
- Do not create a second event bus alongside `app::Command` unless the old one
  is being retired.
- Do not make LVGL objects model objects. LVGL objects are view-owned.
- Do not block BLE/web callbacks while waiting for UI work except for explicit
  request/response operations such as screenshots, where a semaphore contract
  already exists.
- Do not refactor all screens at once. The migration should work one domain or
  one screen at a time.

## Verification

Minimum checks after each migration step:

```sh
pio run -e esp32-4848s040
pio test -e native
```

Hardware checks:

- `/bench` loop peak and slow-section peak do not regress.
- BLE console remains responsive during WiFi and Signal K operations.
- Web UI remains reachable in STA and AP/captive modes.
- Touch gestures and screen switching remain responsive during network work.
