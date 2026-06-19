#pragma once
// Single source of truth for the YB-MIDL capability catalog.
// Plain C++17 data only — no Arduino/ESP/LVGL deps — so the host
// generator (tools/yb_midl_gen/gen.cpp) can include it directly.
#include <cstddef>

namespace yb_midl_catalog {

inline constexpr const char *MIDL_VERSION = "1.0.0";
inline constexpr int MAX_MARKERS_PER_DIAL = 12;

struct ElementType {
    const char *token;            // MIDL element type token
    const char *widget_kind;      // firmware WidgetKind name (for dispatch-covers-catalog)
    const char *const *bindings;  // null-terminated
    const char *const *attrs;     // null-terminated
    bool has_glyphs;
};

inline constexpr const char *B_VALUE[] = {"value", nullptr};
inline constexpr const char *B_VALUE_DIR[] = {"value", "dir", nullptr};
inline constexpr const char *B_NONE[] = {nullptr};

inline constexpr const char *A_NUM[] = {"title", "format", "size", "unit", "color", nullptr};
inline constexpr const char *A_RANGE[] = {"title", "size",  "unit", "color",
                                          "range", "zones", nullptr};
inline constexpr const char *A_BASIC[] = {"title", "size", "color", nullptr};
inline constexpr const char *A_WIND[] = {"title", "format", "size", "unit", "color", nullptr};
inline constexpr const char *A_TREND[] = {"title", "size", "unit", "color", nullptr};

inline constexpr ElementType ELEMENTS[] = {
    {"single-value", "Numeric", B_VALUE, A_NUM, false},
    {"text", "Text", B_VALUE, A_BASIC, false},
    {"gauge", "Gauge", B_VALUE, A_RANGE, false},
    {"bar", "Bar", B_VALUE, A_RANGE, false},
    {"compass", "Compass", B_VALUE_DIR, A_BASIC, true},
    {"windrose", "WindRose", B_VALUE_DIR, A_WIND, true},
    {"trend", "Trend", B_VALUE, A_TREND, false},
    {"autopilot", "Autopilot", B_VALUE, A_BASIC, false},
    {"button", "Button", B_NONE, A_BASIC, false},
};
inline constexpr size_t ELEMENT_COUNT = sizeof(ELEMENTS) / sizeof(ELEMENTS[0]);

inline constexpr const char *GLYPHS[] = {
    "triangle",      "diamond",        "circle",      "bar",
    "cross",         "chevron_in",     "chevron_out", "chevron_left",
    "chevron_right", "chevron_double", nullptr,
};
inline constexpr size_t GLYPH_COUNT = (sizeof(GLYPHS) / sizeof(GLYPHS[0])) - 1;

inline constexpr const char *SOURCES[] = {"signalk", nullptr};
inline constexpr size_t SOURCE_COUNT = (sizeof(SOURCES) / sizeof(SOURCES[0])) - 1;

inline constexpr const char *ACTION_KINDS[] = {"nav", "command", nullptr};
inline constexpr size_t ACTION_KIND_COUNT = (sizeof(ACTION_KINDS) / sizeof(ACTION_KINDS[0])) - 1;

inline constexpr int FONTS[] = {14, 20, 28, 48};
inline constexpr size_t FONT_COUNT = sizeof(FONTS) / sizeof(FONTS[0]);

inline constexpr const char *THEMES[] = {"day", "night", "high-contrast", nullptr};
inline constexpr size_t THEME_COUNT = (sizeof(THEMES) / sizeof(THEMES[0])) - 1;

inline constexpr const char *PRESETS[] = {"full", "hero-split", nullptr};
inline constexpr size_t PRESET_COUNT = (sizeof(PRESETS) / sizeof(PRESETS[0])) - 1;

struct ResClass {
    const char *id;
    int width, height, cols, rows, max_tiles, max_depth;
};
inline constexpr ResClass CLASSES[] = {
    {"square-480", 480, 480, 2, 2, 4, 3},
    {"landscape-800x480", 800, 480, 3, 2, 6, 3},
    {"landscape-1024x600", 1024, 600, 3, 2, 6, 4},
};
inline constexpr size_t CLASS_COUNT = sizeof(CLASSES) / sizeof(CLASSES[0]);

// Find a class by id; returns nullptr if unknown.
inline const ResClass *find_class(const char *id) {
    for (size_t i = 0; i < CLASS_COUNT; ++i) {
        const char *a = CLASSES[i].id;
        const char *b = id;
        while (*a && *b && *a == *b) {
            ++a;
            ++b;
        }
        if (*a == '\0' && *b == '\0') return &CLASSES[i];
    }
    return nullptr;
}

}  // namespace yb_midl_catalog
