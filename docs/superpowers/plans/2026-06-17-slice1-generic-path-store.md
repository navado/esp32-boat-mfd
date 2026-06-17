# Slice 1 — Firmware generic path→value store + render-by-path — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Give the firmware a generic dynamic `path → latest value` store so any SignalK path an authored field binds to can be rendered, without a firmware recompile per path — keeping the existing typed `sk::Data` / `MetricSource` fast-path for built-in screens.

**Architecture:** A pure-C++, fixed-capacity `sk::PathStore` (host-testable, PSRAM-allocated on device) holds `path → double`. The existing host-tested `widget_data::resolve_numeric()` already documents a "raw SignalK dynamic value" resolution step that is currently unimplemented (returns NaN); this slice implements that step by consulting the store. The SignalK WS frame handler feeds every numeric delta into the store alongside the typed parser. No LVGL or renderer changes are required for this slice — it is the data foundation the later slices consume.

**Tech Stack:** C++17, PlatformIO `native` env with Unity, ArduinoJson v7, ESP-IDF `heap_caps` (device only).

This is **Slice 1 of 6** from `docs/superpowers/specs/2026-06-17-device-mirrored-layout-editor-design.md`. Slices 2–6 (manifest, per-screen subscriptions, flash persistence, editor CRUD, zoom) get their own plans after this lands.

---

## File Structure

- `include/path_store.h` (create) — `sk::PathStore` class: fixed-capacity upsert map of `path → double`. One responsibility: store/lookup dynamic path values. Pure C++, no Arduino/LVGL deps.
- `src/path_store.cpp` (create) — implementation.
- `test/test_path_store/test_path_store.cpp` (create) — Unity host tests for the store.
- `include/widget_data_resolver.h` (modify) — add a `resolve_numeric` overload taking a `const PathStore*` for the step-3 dynamic lookup.
- `src/widget_data_resolver.cpp` (modify) — implement step-3 dynamic lookup via the store; add `captureDynamic()` helper that upserts a numeric SignalK value into a store.
- `test/test_widget_data/test_widget_data.cpp` (modify) — extend with store-backed resolution + capture tests.
- `include/signalk.h` (modify) — export `sk::dynamicStore()` accessor.
- `src/signalk.cpp` (modify) — allocate the store (PSRAM) and feed it from the WS frame handler.
- `platformio.ini` (modify) — add `path_store.cpp` to the `native` `build_src_filter` and `test_path_store` to `test_filter`.

---

### Task 1: `sk::PathStore` data structure (host-tested)

**Files:**
- Create: `include/path_store.h`
- Create: `src/path_store.cpp`
- Create: `test/test_path_store/test_path_store.cpp`
- Modify: `platformio.ini` (native `build_src_filter` + `test_filter`)

- [ ] **Step 1: Create the header**

Create `include/path_store.h`:

```cpp
#pragma once

// Generic dynamic SignalK path -> latest-value store. Lets the renderer
// resolve ANY configured path string (not just the curated MetricSource
// set) so authored layout fields render without a firmware recompile.
//
// Fixed capacity (no heap churn): one entry per (view * tile * named-path)
// the device can show at once, bounded by the capability manifest caps.
// Pure C++ / host-testable; the live device instance is PSRAM-allocated.

#include <math.h>
#include <stddef.h>

namespace sk {

class PathStore {
public:
    static constexpr int CAP = 64;       // >= maxViews(8) * maxTiles(4) * 2 named paths
    static constexpr int PATH_LEN = 80;  // longest SignalK path we accept

    void clear();

    // Upsert path -> value. Returns false if `path` is new and the store is
    // full (existing paths always update). NaN values are stored (a path that
    // went invalid reads back NaN, distinct from "absent").
    bool set(const char *path, double value);

    // Latest value for `path`, or NaN if the path was never stored.
    double get(const char *path) const;

    bool has(const char *path) const;
    int size() const { return count_; }

private:
    struct Entry {
        char path[PATH_LEN];
        double value;
        bool used;
    };
    Entry entries_[CAP] = {};
    int count_ = 0;
    int find_(const char *path) const;  // index or -1
};

}  // namespace sk
```

- [ ] **Step 2: Create the implementation**

Create `src/path_store.cpp`:

```cpp
#include "path_store.h"

#include <string.h>

namespace sk {

int PathStore::find_(const char *path) const {
    if (!path) return -1;
    for (int i = 0; i < CAP; ++i) {
        if (entries_[i].used && strncmp(entries_[i].path, path, PATH_LEN) == 0) return i;
    }
    return -1;
}

void PathStore::clear() {
    for (int i = 0; i < CAP; ++i) entries_[i].used = false;
    count_ = 0;
}

bool PathStore::set(const char *path, double value) {
    if (!path || !path[0]) return false;
    int i = find_(path);
    if (i < 0) {
        for (int j = 0; j < CAP; ++j) {
            if (!entries_[j].used) { i = j; break; }
        }
        if (i < 0) return false;  // full
        entries_[i].used = true;
        strncpy(entries_[i].path, path, PATH_LEN - 1);
        entries_[i].path[PATH_LEN - 1] = 0;
        ++count_;
    }
    entries_[i].value = value;
    return true;
}

double PathStore::get(const char *path) const {
    int i = find_(path);
    return i < 0 ? NAN : entries_[i].value;
}

bool PathStore::has(const char *path) const { return find_(path) >= 0; }

}  // namespace sk
```

- [ ] **Step 3: Wire the native build** — edit `platformio.ini`, in `[env:native]`, append `+<path_store.cpp>` to `build_src_filter` and add `test_path_store` to `test_filter`.

`build_src_filter` (add the token at the end, before the closing of the line):
```
build_src_filter = -<*> +<signalk_parser.cpp> +<layout.cpp> +<boat_data.cpp> +<nmea0183_parser.cpp> +<autopilot_pure.cpp> +<manager_config.cpp> +<font_resolver.cpp> +<widget_data_resolver.cpp> +<boards/board_common.cpp> +<boards/board_native_fake.cpp> +<ui/layout_context.cpp> +<manager_url.cpp> +<manager_endpoint.cpp> +<error_log.cpp> +<hostname_check.cpp> +<ui_config_check.cpp> +<sources_check.cpp> +<log_level_check.cpp> +<device_discovery.cpp> +<net_health.cpp> +<knob_menu.cpp> +<proto/proto.cpp> +<path_store.cpp>
```

`test_filter` — add a line `    test_path_store` to the list.

- [ ] **Step 4: Write the failing test**

Create `test/test_path_store/test_path_store.cpp`:

```cpp
#include "path_store.h"
#include <math.h>
#include <unity.h>

void setUp() {}
void tearDown() {}

static void test_absent_is_nan() {
    sk::PathStore s;
    TEST_ASSERT_TRUE(isnan(s.get("environment.depth.belowTransducer")));
    TEST_ASSERT_FALSE(s.has("anything"));
    TEST_ASSERT_EQUAL_INT(0, s.size());
}

static void test_set_then_get() {
    sk::PathStore s;
    TEST_ASSERT_TRUE(s.set("navigation.speedOverGround", 3.5));
    TEST_ASSERT_TRUE(s.has("navigation.speedOverGround"));
    TEST_ASSERT_EQUAL_DOUBLE(3.5, s.get("navigation.speedOverGround"));
    TEST_ASSERT_EQUAL_INT(1, s.size());
}

static void test_upsert_updates_in_place() {
    sk::PathStore s;
    s.set("p", 1.0);
    s.set("p", 2.0);
    TEST_ASSERT_EQUAL_DOUBLE(2.0, s.get("p"));
    TEST_ASSERT_EQUAL_INT(1, s.size());  // not duplicated
}

static void test_nan_value_is_stored_distinct_from_absent() {
    sk::PathStore s;
    s.set("p", NAN);
    TEST_ASSERT_TRUE(s.has("p"));            // present...
    TEST_ASSERT_TRUE(isnan(s.get("p")));     // ...but NaN
}

static void test_full_rejects_new_keeps_updating_existing() {
    sk::PathStore s;
    char buf[16];
    for (int i = 0; i < sk::PathStore::CAP; ++i) {
        snprintf(buf, sizeof(buf), "p%d", i);
        TEST_ASSERT_TRUE(s.set(buf, i));
    }
    TEST_ASSERT_FALSE(s.set("overflow", 9));   // new key rejected when full
    TEST_ASSERT_TRUE(s.set("p0", 100));        // existing key still updates
    TEST_ASSERT_EQUAL_DOUBLE(100, s.get("p0"));
}

static void test_clear_empties() {
    sk::PathStore s;
    s.set("p", 1.0);
    s.clear();
    TEST_ASSERT_EQUAL_INT(0, s.size());
    TEST_ASSERT_FALSE(s.has("p"));
}

int main(int, char **) {
    UNITY_BEGIN();
    RUN_TEST(test_absent_is_nan);
    RUN_TEST(test_set_then_get);
    RUN_TEST(test_upsert_updates_in_place);
    RUN_TEST(test_nan_value_is_stored_distinct_from_absent);
    RUN_TEST(test_full_rejects_new_keeps_updating_existing);
    RUN_TEST(test_clear_empties);
    return UNITY_END();
}
```

- [ ] **Step 5: Run the test**

Run: `pio test -e native -f test_path_store`
Expected: PASS (6 tests). If the env can't find `path_store.cpp`, recheck the `build_src_filter` edit in Step 3.

- [ ] **Step 6: Commit**

```bash
git add include/path_store.h src/path_store.cpp test/test_path_store/test_path_store.cpp platformio.ini
git commit -m "feat(fw): generic dynamic SignalK path->value store (PathStore)

Co-Authored-By: Claude Opus 4.8 (1M context) <noreply@anthropic.com>"
```

---

### Task 2: Resolve unknown paths via the store + `captureDynamic`

**Files:**
- Modify: `include/widget_data_resolver.h`
- Modify: `src/widget_data_resolver.cpp`
- Test: `test/test_widget_data/test_widget_data.cpp`

- [ ] **Step 1: Add the overload + capture declaration to the header**

In `include/widget_data_resolver.h`, add `#include "path_store.h"` near the existing `#include "signalk_parser.h"`, then add below the existing `resolve_numeric` declaration:

```cpp
// Same as resolve_numeric(path, d) but, when the path is NOT a known typed
// field, falls back to the dynamic store (raw SignalK value). This is the
// "step 3" the original resolution order documented but did not implement.
double resolve_numeric(const char *path, const sk::Data &d, const sk::PathStore *store);

// Upsert a numeric SignalK delta value into `store`. Used by the WS frame
// handler so every incoming numeric path is renderable by string. Non-finite
// JSON values are stored as NaN (path present, value invalid). Returns true
// if a numeric value was captured.
bool captureDynamic(const char *path, double value, sk::PathStore &store);
```

- [ ] **Step 2: Write the failing test**

In `test/test_widget_data/test_widget_data.cpp`, add these tests and register them in the file's `main()` `RUN_TEST` list (mirror the existing registration style in that file):

```cpp
static void test_resolve_falls_back_to_store_for_unknown_path() {
    sk::Data d;            // typed fields default to NaN/0 per sk::Data
    sk::PathStore store;
    store.set("propulsion.0.revolutions", 27.5);
    // Unknown to the typed resolver -> uses the store.
    TEST_ASSERT_EQUAL_DOUBLE(27.5,
        widget_data::resolve_numeric("propulsion.0.revolutions", d, &store));
}

static void test_known_path_prefers_typed_field_over_store() {
    sk::Data d;
    d.sog = 4.0;                              // typed field present
    sk::PathStore store;
    store.set("navigation.speedOverGround", 99.0);  // stale store value
    // Known typed field wins; store is only the fallback.
    TEST_ASSERT_EQUAL_DOUBLE(4.0,
        widget_data::resolve_numeric("navigation.speedOverGround", d, &store));
}

static void test_unknown_path_without_store_is_nan() {
    sk::Data d;
    TEST_ASSERT_TRUE(isnan(widget_data::resolve_numeric("x.y.z", d, nullptr)));
}

static void test_capture_dynamic_round_trip() {
    sk::PathStore store;
    TEST_ASSERT_TRUE(widget_data::captureDynamic("foo.bar", 1.25, store));
    TEST_ASSERT_EQUAL_DOUBLE(1.25, store.get("foo.bar"));
}
```

- [ ] **Step 3: Run to verify it fails**

Run: `pio test -e native -f test_widget_data`
Expected: FAIL — `resolve_numeric` 3-arg overload and `captureDynamic` are not defined.

- [ ] **Step 4: Implement**

In `src/widget_data_resolver.cpp`, add `#include "path_store.h"` at the top with the other includes. Find the existing `double resolve_numeric(const char *path, const sk::Data &d)` definition. Add the overload and capture below it (the original 2-arg version stays unchanged):

```cpp
double resolve_numeric(const char *path, const sk::Data &d, const sk::PathStore *store) {
    double v = resolve_numeric(path, d);   // typed alias/known-field resolution
    if (!isnan(v)) return v;               // typed field wins
    if (store && store->has(path)) return store->get(path);  // step 3: dynamic
    return NAN;
}

bool captureDynamic(const char *path, double value, sk::PathStore &store) {
    return store.set(path, value);
}
```

- [ ] **Step 5: Run to verify it passes**

Run: `pio test -e native -f test_widget_data`
Expected: PASS (existing tests + the 4 new ones).

- [ ] **Step 6: Commit**

```bash
git add include/widget_data_resolver.h src/widget_data_resolver.cpp test/test_widget_data/test_widget_data.cpp
git commit -m "feat(fw): resolve unknown paths via dynamic store (widget_data step 3)

Co-Authored-By: Claude Opus 4.8 (1M context) <noreply@anthropic.com>"
```

---

### Task 3: Device feed — global PSRAM store fed from the WS frame handler

**Files:**
- Modify: `include/signalk.h`
- Modify: `src/signalk.cpp`

This task has no host test (it is device-side WS plumbing + PSRAM allocation); it is verified by a device build. The pure logic it relies on (`captureDynamic`, `PathStore`) is already covered by Tasks 1–2.

- [ ] **Step 1: Export the store accessor**

In `include/signalk.h`, add near the top of `namespace sk` (after the existing includes; add `#include "path_store.h"`):

```cpp
// Process-wide dynamic path store, fed from every numeric WS delta. The
// renderer resolves authored-field paths against this when they are not a
// typed sk::Data field. PSRAM-allocated on first use (see signalk.cpp).
PathStore &dynamicStore();
```

- [ ] **Step 2: Allocate (PSRAM) + define the accessor**

In `src/signalk.cpp`, add `#include "path_store.h"` and `#include "esp_heap_caps.h"` with the other includes, then add near the other file-scope statics:

```cpp
// PSRAM-allocated so the ~5 KB store never sits in scarce internal SRAM
// (see CLAUDE.md "Memory traps": large structs belong in PSRAM, never on a
// task stack or in .bss-internal). Constructed once, lives for the session.
sk::PathStore &sk::dynamicStore() {
    static sk::PathStore *s = nullptr;
    if (!s) {
        void *mem = heap_caps_calloc(1, sizeof(sk::PathStore), MALLOC_CAP_SPIRAM);
        s = mem ? new (mem) sk::PathStore() : new sk::PathStore();  // fallback to heap
    }
    return *s;
}
```

Add `#include <new>` for placement-new if not already present.

- [ ] **Step 3: Feed the store on each numeric delta**

In `src/signalk.cpp`, find where incoming deltas are applied (the loop that calls `applyValue(p, v["value"], out)` — it is in the `parseDelta`/frame path of `signalk_parser.cpp`, invoked from the WS `onText`/frame handler in `signalk.cpp`). In the WS frame handler in `signalk.cpp`, where it iterates `updates[].values[]`, after the existing `applyValue` call for each value, add a generic capture for numeric values:

```cpp
// Mirror every numeric delta into the dynamic store so authored fields can
// render arbitrary paths (typed sk::Data still drives the built-in screens).
if (val["value"].is<float>() || val["value"].is<double>() ||
    val["value"].is<int>()) {
    const char *p = val["path"] | "";
    if (p[0]) widget_data::captureDynamic(p, val["value"].as<double>(), sk::dynamicStore());
}
```

Add `#include "widget_data_resolver.h"` to `signalk.cpp` if not already included. If the WS iteration lives inside `signalk_parser.cpp::parseDelta` rather than `signalk.cpp`, place the capture there instead and thread `sk::dynamicStore()` in — but prefer keeping `signalk_parser.cpp` host-pure and doing the capture in `signalk.cpp`'s device-side handler.

- [ ] **Step 4: Build the device firmware**

Run: `pio run -e esp32-4848s040`
Expected: SUCCESS (compiles + links). If `heap_caps_calloc` or placement-new errors, confirm `#include "esp_heap_caps.h"` and `#include <new>`.

- [ ] **Step 5: Run host tests + lint to confirm no regression**

Run: `pio test -e native` then `make pre-commit`
Expected: all native tests PASS; clang-format clean.

- [ ] **Step 6: Commit**

```bash
git add include/signalk.h src/signalk.cpp
git commit -m "feat(fw): feed PSRAM dynamic path store from WS deltas

Co-Authored-By: Claude Opus 4.8 (1M context) <noreply@anthropic.com>"
```

---

### Task 4: Verify end-to-end resolution path (regression guard)

**Files:**
- Test: `test/test_widget_data/test_widget_data.cpp`

This locks the contract that an authored field bound to an arbitrary path renders its live value once captured, so later slices (editor, subscriptions) can rely on it.

- [ ] **Step 1: Write the failing test**

Add to `test/test_widget_data/test_widget_data.cpp` and register in `main()`:

```cpp
static void test_capture_then_resolve_arbitrary_path() {
    sk::Data d;
    sk::PathStore store;
    // Simulate a WS delta for a path the typed parser does not know.
    widget_data::captureDynamic("electrical.solar.0.power", 142.0, store);
    // The renderer resolves the authored field by its path string.
    TEST_ASSERT_EQUAL_DOUBLE(142.0,
        widget_data::resolve_numeric("electrical.solar.0.power", d, &store));
}
```

- [ ] **Step 2: Run to verify it passes**

Run: `pio test -e native -f test_widget_data`
Expected: PASS (the wiring from Tasks 1–2 already satisfies this; this test is the regression guard).

- [ ] **Step 3: Commit**

```bash
git add test/test_widget_data/test_widget_data.cpp
git commit -m "test(fw): capture->resolve round trip for arbitrary paths

Co-Authored-By: Claude Opus 4.8 (1M context) <noreply@anthropic.com>"
```

---

## Self-Review

**Spec coverage (slice 1 scope):** ✔ generic path→value store (Task 1); ✔ render-by-path resolution via `resolve_numeric` step-3 fallback (Task 2); ✔ device feed from WS deltas, PSRAM-allocated per the memory-trap rule (Task 3); ✔ end-to-end regression guard (Task 4). Out-of-scope-for-slice-1 items (manifest, per-screen subscribe, flash persistence, editor, zoom) are explicitly deferred to slices 2–6.

**Placeholder scan:** No TBD/TODO; every code step shows full code; every run step states the exact command and expected result.

**Type consistency:** `sk::PathStore` (`set`/`get`/`has`/`clear`/`size`, `CAP`, `PATH_LEN`) is used identically across Tasks 1–4. `widget_data::resolve_numeric(path, d, store)` and `widget_data::captureDynamic(path, value, store)` signatures match between header (Task 2 Step 1), implementation (Task 2 Step 4), and call sites (Task 3 Step 3, Task 4). `sk::dynamicStore()` returns `PathStore&` consistently (Task 3 Steps 1–3).

**Risk notes carried from spec:** store is PSRAM-allocated (Task 3 Step 2) and fixed-capacity (`CAP`) — no task-stack large struct, no unbounded heap. Capacity `CAP=64` covers the manifest caps (`maxViews 8 * maxTilesPerScreen 4 * 2 named paths`).
