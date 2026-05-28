#pragma once

// Font resolver per docs/specs/19 §"Font Resolver Details".
//
// Pure resolve: given a requested font size and the compiled-in
// supported sizes, return the resolved size (nearest-lower of
// supported, capped at smallest and largest). The Arduino-only
// font_resolver_lvgl.cpp then maps the resolved size to a real
// lv_font_t pointer.
//
// Resolution algorithm (spec 19):
//   requested <= smallest supported -> smallest
//   requested exactly supported     -> requested
//   requested between sizes         -> nearest LOWER (not absolute)
//   requested > largest supported   -> largest
//
// Nearest-lower bias avoids accidental overflow on fixed-size tiles.

#include <stdint.h>
#include <stddef.h>

namespace font_resolver {

// Default compiled-in sizes for this firmware (matches lv_conf.h
// LV_FONT_MONTSERRAT_* enables + device_identity::to_json_doc fonts).
constexpr uint16_t DEFAULT_SIZES[] = {14, 20, 28, 48};
constexpr size_t DEFAULT_COUNT = sizeof(DEFAULT_SIZES) / sizeof(DEFAULT_SIZES[0]);

// Pure resolver. `sizes` must be sorted ascending and have at least
// one entry. Returns the resolved size from `sizes`. If `count == 0`,
// returns `requested` unchanged.
uint16_t resolve(uint16_t requested, const uint16_t *sizes, size_t count);

// Convenience: resolve against the firmware's compiled-in table.
inline uint16_t resolve_default(uint16_t requested) {
    return resolve(requested, DEFAULT_SIZES, DEFAULT_COUNT);
}

}  // namespace font_resolver
