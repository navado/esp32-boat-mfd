#pragma once

// BLE GATT service exposing two characteristics that a smartphone app
// can read/write to configure the device:
//
//   CONNECTION   - JSON snapshot of network + SignalK state and config
//   CONFIGURATION - the layout JSON document (same schema as SignalK
//                  resource at configuration.boat-mfd.layouts)
//
// Service / characteristic UUIDs are stable and may be relied on by
// companion apps. See README "BLE GATT schema" for the JSON shapes.

#include <Arduino.h>

namespace bleconfig {

constexpr const char *SERVICE_UUID = "a3f7e000-7a6b-4f47-b3a5-c4d2e5f6a000";
constexpr const char *CONN_UUID = "a3f7e001-7a6b-4f47-b3a5-c4d2e5f6a000";
constexpr const char *CONFIG_UUID = "a3f7e003-7a6b-4f47-b3a5-c4d2e5f6a000";

// Wire the service into an already-created NimBLEServer. Idempotent.
void setup();

// Refresh the values backing the characteristics and notify any
// subscribed clients. Safe to call frequently.
void notifyAll();

}  // namespace bleconfig
