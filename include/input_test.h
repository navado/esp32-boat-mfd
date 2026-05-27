#pragma once

// Touch / gesture injection for system tests.
//
// Two injection levels:
//
//   1. Touchscreen-manager level (synthesises GT911-equivalent samples
//      into the shared touch snapshot the LVGL indev callback reads).
//      Useful for tap/swipe tests that exercise the full pipeline:
//      input -> LVGL hit-test -> CLICKED -> tile_clicked_cb -> app::post.
//
//   2. Action-queue level (skips touch entirely and posts a ShowScreen
//      command). Useful for tests that don't care about the touch
//      pipeline but want to verify the screen-router / handler logic.
//
// Always available - the endpoints are POST-only under /api/test/* and
// the web API is intended for trusted-LAN use. Production builds can
// stub this out by gating the registrations in web.cpp on a
// -DDISABLE_TEST_API flag if ever needed.

#include <Arduino.h>
#include <stdint.h>

namespace input_test {

// --- Level 1: touchscreen manager ---------------------------------------

// Set the shared touch snapshot directly. Returns false if the touch
// subsystem hasn't initialised yet.
//   pressed=true   -> indev reports PRESSED at (x, y) on the next poll
//   pressed=false  -> indev reports RELEASED; (x, y) ignored
bool inject_touch(int16_t x, int16_t y, bool pressed);

// Synthesised tap: press at (x, y), hold `hold_ms`, release. Blocks the
// caller; the LVGL indev poll runs every ~5 ms so a 50 ms hold lets
// the CLICKED event fire reliably.
bool inject_tap(int16_t x, int16_t y, uint32_t hold_ms = 50);

// Synthesised swipe. Generates `steps` intermediate samples spaced
// (dur_ms / steps) apart so LVGL sees motion, then releases. Also
// directly invokes the project's high-level swipe detector
// (detect_swipe_release) so the gesture is recognised even if LVGL's
// own gesture path is debounced.
bool inject_swipe(int16_t x0, int16_t y0,
                  int16_t x1, int16_t y1,
                  uint32_t dur_ms = 300,
                  uint8_t steps = 8);

// --- Level 2: action queue ----------------------------------------------

// Post a synthetic gesture directly into the UI command queue,
// bypassing the touch + detection layers. Accepts:
//   "left"  -> next screen
//   "right" -> previous screen
//   "up"    -> settings drawer
//   "down"  -> dashboard
// Returns false if `dir` is unknown.
bool post_gesture(const char *dir);

}  // namespace input_test
