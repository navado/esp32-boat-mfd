#pragma once

#include <Arduino.h>

namespace boat {

// `boat` console command:
//   boat            - dump fused snapshot + source per field
//   boat priority   - show ranked priority
//   boat reset      - clear all fields (debug)
//   boat timeout <source> <ms> - tune freshness window
bool handleSerialCommand(const String &line);

}  // namespace boat
