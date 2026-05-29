#pragma once

// Pure validators for the ui.* config block consumed by
// manager::apply_config. Kept separate from manager.cpp so the rules
// are a host-testable surface and so future BLE / web / plugin entry
// points can share the same predicates.

namespace ui_config {

// True iff the value is a valid 8-bit backlight setting (0..255
// inclusive). Used both for the manager apply path and the
// brightness.set v1 command.
bool is_valid_brightness(int value);

// True iff `name` matches one of the supported theme tokens.
// Spec 17 §8 lists "day"/"night"/"auto"; case-sensitive match.
bool is_valid_theme(const char *name);

}  // namespace ui_config
