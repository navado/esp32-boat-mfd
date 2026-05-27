#pragma once

// NMEA0183-over-WiFi source per docs/specs/12 Feature 2 (extended).
//
// Reads NMEA0183 sentences from a TCP server (e.g. a multiplexer like
// kplex / OpenCPN-shared / openplotter) or a UDP broadcast feed. Each
// recognized sentence publishes its fields into boat::Snapshot with
// SourceKind::NmeaWifi.
//
// Configuration (NVS keys under namespace "n0183w"):
//   enabled  uint8  - 0 or 1
//   proto    uint8  - 0 = TCP client, 1 = UDP listen
//   host     string - TCP host (ignored in UDP mode)
//   port     uint16 - TCP or UDP port (default 10110)
//
// Runs on its own low-priority task pinned to core 0 so it cannot
// stall LVGL on core 1.

#include <Arduino.h>

namespace nmea_wifi {

enum class Protocol : uint8_t { Tcp = 0, Udp = 1 };

struct Status {
    bool enabled;
    Protocol proto;
    String host;
    uint16_t port;
    bool connected;
    uint32_t bytes_in;
    uint32_t sentences_ok;
    uint32_t sentences_bad;
    uint32_t last_rx_ms;
};

void setup();
Status status();

// Console command handler - "nmea-wifi ..." subcommands.
//   nmea-wifi enable | disable
//   nmea-wifi tcp <host> <port>
//   nmea-wifi udp <port>
//   nmea-wifi status
bool handleSerialCommand(const String &line);

}  // namespace nmea_wifi
