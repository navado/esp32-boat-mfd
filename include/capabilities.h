#pragma once

// Device capability manifest (spec 2026-06-17 "device-mirrored layout editor",
// §Capability manifest). The device self-reports, per view type, the valid
// attributes/limits so the manager's layout editor can gate to exactly what
// this firmware can render. Emitted in the heartbeat under `ui.capabilities`.
//
// Pure C++ (ArduinoJson only) so it is host-testable and the manager↔device
// contract can be exercised on both sides from one source of truth.

#include <ArduinoJson.h>

namespace capabilities {

// Schema/manifest version. Bump when the manifest shape changes so the manager
// can detect an older/newer device.
constexpr int MANIFEST_VERSION = 1;

// Fill `out` with the full capability manifest. `out` should be a fresh object.
void build_manifest(JsonObject out);

}  // namespace capabilities
