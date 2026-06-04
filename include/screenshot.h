#pragma once

// Cross-task screenshot for the web UI. lv_snapshot_take walks the LVGL
// widget tree and must run on the LVGL task. The web task posts a
// request, blocks on a semaphore; the LVGL refresh loop fulfils it and
// returns a heap-allocated BMP buffer the caller is responsible for
// freeing.

#include <Arduino.h>
#include <stddef.h>
#include <stdint.h>

namespace screenshot {

// Initialise the semaphore. Safe to call from setup().
void setup();

// Called from the LVGL task each frame. Cheap when no request is pending.
void serve_pending();

// Output formats. PNG uses ROM miniz to compress the 480x480 LVGL
// snapshot from ~460 kB (raw BMP) to ~20-60 kB so the body fits in a
// single TCP send window and avoids the chunked-streaming wedge that
// the Arduino-ESP32 WebServer hits with large bodies.
enum class Format {
    Bmp,
    Png,
};

// Called from any other task (e.g. the web task). Blocks up to
// timeout_ms; returns true on success, with *out and *out_len set.
// Caller must heap_caps_free(*out) when done.
bool request(uint32_t timeout_ms, uint8_t **out, size_t *out_len, Format fmt = Format::Bmp);

}  // namespace screenshot
