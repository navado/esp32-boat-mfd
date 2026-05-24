#pragma once

// Layout loader: owns the live `Config` and is responsible for getting
// it from somewhere (baked-in default, SignalK REST, or BLE write).

#include <Arduino.h>
#include "layout.h"

namespace layout {

// Parse the baked-in default layout into the live config. Returns true on
// success.
bool load_default();

// Parse + apply an arbitrary JSON layout document. Returns true on success
// and updates the live config; on parse error, restores the previous good
// config (or the default if nothing was loaded yet).
bool apply_json(const char *json, size_t len);

// Fetch the layout from a SignalK server's REST API:
//   GET http://<host>:<port>/signalk/v1/api/vessels/self/configuration/boat-mfd/layouts/value
// On success, replaces the live config. On failure, keeps the current one.
bool fetch_from_signalk(const String &host, uint16_t port);

// The last successfully applied JSON document, stored verbatim. Length is
// written to *out_len. Returns nullptr if nothing has been loaded yet.
const char *last_json(size_t *out_len);

// Read-only access to the currently active config. Caller MUST check
// loaded() first - dereferencing before load_default() succeeds is UB.
const Config &current();

// True once load_default (or any apply_json / fetch) succeeded.
bool loaded();

// Print a one-screen-per-line summary via net::logf. Used by console
// `layout-show`.
void show_summary();

// Handle layout-related console lines. Returns true if consumed.
bool handleSerialCommand(const String &line);

}  // namespace layout
