// Host generator: emits the YB-MIDL capabilities manifest JSON for one
// resolution class to stdout. Build with a host C++17 compiler; no
// Arduino/ESP deps. Usage: yb_midl_gen <class-id> <board>
#include <cstdio>
#include <cstring>
#include <string>
#include "yb_midl_catalog.h"

using namespace yb_midl_catalog;

static std::string esc(const char *s) {
    std::string o;
    for (; *s; ++s) {
        if (*s == '"' || *s == '\\') o += '\\';
        o += *s;
    }
    return o;
}

static std::string strArray(const char *const *items) {
    std::string o = "[";
    bool first = true;
    for (; items && *items; ++items) {
        if (!first) o += ", ";
        o += "\"" + esc(*items) + "\"";
        first = false;
    }
    o += "]";
    return o;
}

int main(int argc, char **argv) {
    if (argc < 3) {
        std::fprintf(stderr, "usage: %s <class-id> <board>\n", argv[0]);
        return 2;
    }
    const ResClass *cls = find_class(argv[1]);
    if (!cls) {
        std::fprintf(stderr, "unknown class: %s\n", argv[1]);
        return 1;
    }
    const char *board = argv[2];

    std::string o;
    o += "{\n";
    o += "  \"midl\": \"" + std::string(MIDL_VERSION) + "\",\n";
    o += "  \"board\": \"" + esc(board) + "\",\n";
    o += "  \"maxMarkersPerDial\": " + std::to_string(MAX_MARKERS_PER_DIAL) + ",\n";

    // classes (this board has exactly one class)
    o += "  \"classes\": [\n    {\n";
    o += "      \"id\": \"" + std::string(cls->id) + "\",\n";
    o += "      \"width\": " + std::to_string(cls->width) + ",\n";
    o += "      \"height\": " + std::to_string(cls->height) + ",\n";
    o += "      \"maxTiles\": " + std::to_string(cls->max_tiles) + ",\n";
    o += "      \"maxDepth\": " + std::to_string(cls->max_depth) + ",\n";
    o += "      \"presets\": [";
    for (size_t i = 0; i < PRESET_COUNT; ++i) {
        if (i) o += ", ";
        o += "\"" + std::string(PRESETS[i]) + "\"";
    }
    o += "],\n      \"elements\": [";
    for (size_t i = 0; i < ELEMENT_COUNT; ++i) {
        if (i) o += ", ";
        o += "\"" + std::string(ELEMENTS[i].token) + "\"";
    }
    o += "]\n    }\n  ],\n";

    // elements
    o += "  \"elements\": [\n";
    for (size_t i = 0; i < ELEMENT_COUNT; ++i) {
        const ElementType &e = ELEMENTS[i];
        o += "    { \"type\": \"" + std::string(e.token) + "\"";
        o += ", \"bindings\": " + strArray(e.bindings);
        o += ", \"attrs\": " + strArray(e.attrs);
        if (e.has_glyphs) o += ", \"glyphs\": " + strArray(GLYPHS);
        o += " }";
        o += (i + 1 < ELEMENT_COUNT) ? ",\n" : "\n";
    }
    o += "  ],\n";

    // top-level capability lists
    o += "  \"sources\": " + strArray(SOURCES) + ",\n";
    o += "  \"actionKinds\": " + strArray(ACTION_KINDS) + ",\n";
    o += "  \"presets\": " + strArray(PRESETS) + ",\n";
    o += "  \"glyphs\": " + strArray(GLYPHS) + ",\n";
    o += "  \"themes\": " + strArray(THEMES) + ",\n";
    o += "  \"fonts\": [";
    for (size_t i = 0; i < FONT_COUNT; ++i) {
        if (i) o += ", ";
        o += std::to_string(FONTS[i]);
    }
    o += "]\n}\n";

    std::fputs(o.c_str(), stdout);
    return 0;
}
