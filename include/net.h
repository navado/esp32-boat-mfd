#pragma once

#include <Arduino.h>

namespace net {

void setup();  // call once after Serial.begin
void loop();   // call frequently from main loop

// Multi-target log: Serial + UDP broadcast + BLE notify (if connected).
// Safe to call before setup() - it falls through to Serial only.
void logf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

// Handle a line of input on Serial console (e.g. "wifi <ssid> <pass>").
// Returns true if the line was consumed by net.
bool handleSerialCommand(const String &line);

bool wifiUp();
String ipString();
int rssi();

// Optional extra command handler registered by main; called for lines net::
// and sk:: didn't consume. Returns true if the line was handled.
using ExtraCommandHandler = bool (*)(const String &line);
void setExtraCommandHandler(ExtraCommandHandler h);

}  // namespace net
