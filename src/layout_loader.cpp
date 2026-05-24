#include "layout_loader.h"
#include "net.h"
#include "ble_config.h"

#include <HTTPClient.h>
#include <WiFi.h>

namespace layout {

// Baked-in default layout. Mirrors what the firmware currently renders
// so the parsed Config is immediately useful as a runtime model even
// before the SignalK fetcher lands. Keep this in sync with the on-screen
// reality until the renderer takes over.
static const char *DEFAULT_LAYOUT_JSON =
    "{"
    "\"version\":1,"
    "\"settings\":{\"default_screen\":\"dashboard\",\"demo_period_ms\":3000},"
    "\"screens\":[{"
    "\"id\":\"dashboard\",\"title\":\"Dashboard\",\"type\":\"quadrants\","
    "\"tiles\":["
    "{\"id\":\"wind\",\"title\":\"WIND\",\"type\":\"wind\","
    "\"paths\":{"
    "\"awa\":\"environment.wind.angleApparent\","
    "\"aws\":\"environment.wind.speedApparent\""
    "}},"
    "{\"id\":\"nav\",\"title\":\"NAV\",\"type\":\"nav\","
    "\"paths\":{"
    "\"sog\":\"navigation.speedOverGround\","
    "\"cog\":\"navigation.courseOverGroundTrue\","
    "\"hdg\":\"navigation.headingTrue\","
    "\"position\":\"navigation.position\""
    "}},"
    "{\"id\":\"depth\",\"title\":\"DEPTH / TEMP\",\"type\":\"depth_temp\","
    "\"paths\":{"
    "\"depth\":\"environment.depth.belowTransducer\","
    "\"temp\":\"environment.water.temperature\""
    "}},"
    "{\"id\":\"status\",\"title\":\"STATUS\",\"type\":\"device_status\"}"
    "]"
    "}],"
    "\"alarms\":["
    "{\"id\":\"shallow\",\"path\":\"environment.depth.belowTransducer\","
    "\"level\":\"alarm\",\"lt\":3.0,\"message\":\"SHALLOW WATER\"},"
    "{\"id\":\"batt_low\",\"path\":\"electrical.batteries.house.voltage\","
    "\"level\":\"warn\",\"lt\":11.5,\"message\":\"BATTERY LOW\"}"
    "]"
    "}";

// Config is ~34 KB. Putting it in internal SRAM starves the BLE controller
// (which silently hangs during AP+BLE setup). Allocate in PSRAM instead.
static Config *s_current = nullptr;
static bool s_loaded = false;

// Last successfully applied JSON document, stored verbatim so BLE/HTTP
// reads can return what the user wrote (round-trip-safe).
static char *s_last_json = nullptr;
static size_t s_last_json_len = 0;

static void remember_json(const char *src, size_t len) {
    if (s_last_json) {
        heap_caps_free(s_last_json);
        s_last_json = nullptr;
    }
    s_last_json = (char *)heap_caps_malloc(len + 1, MALLOC_CAP_SPIRAM);
    if (!s_last_json) {
        s_last_json_len = 0;
        net::logf("[layout] remember_json: PSRAM alloc failed (%u bytes)", (unsigned)len);
        return;
    }
    memcpy(s_last_json, src, len);
    s_last_json[len] = 0;
    s_last_json_len = len;
    net::logf("[layout] remember_json: stored %u bytes", (unsigned)len);
}

static bool ensure_alloc() {
    if (s_current) return true;
    s_current = (Config *)heap_caps_calloc(1, sizeof(Config), MALLOC_CAP_SPIRAM);
    if (!s_current) {
        net::logf("[layout] PSRAM alloc failed (%u bytes)", (unsigned)sizeof(Config));
        return false;
    }
    return true;
}

bool load_default() {
    return apply_json(DEFAULT_LAYOUT_JSON, strlen(DEFAULT_LAYOUT_JSON));
}

bool apply_json(const char *json, size_t len) {
    if (!ensure_alloc()) return false;
    int rc = parse(json, len, *s_current);
    if (rc != 0) {
        net::logf("[layout] apply_json FAILED (rc=%d) - keeping previous config", rc);
        // Restore from the last good JSON if we have one, else reload default.
        if (s_last_json) {
            parse(s_last_json, s_last_json_len, *s_current);
        } else {
            parse(DEFAULT_LAYOUT_JSON, strlen(DEFAULT_LAYOUT_JSON), *s_current);
        }
        return false;
    }
    s_loaded = true;
    remember_json(json, len);
    net::logf("[layout] applied %u-byte doc: %u screens, %u alarms", (unsigned)len,
              (unsigned)s_current->screen_count, (unsigned)s_current->alarm_count);
    bleconfig::notifyAll();  // refresh BLE characteristics with the new layout
    return true;
}

const char *last_json(size_t *out_len) {
    if (out_len) *out_len = s_last_json_len;
    return s_last_json;
}

bool fetch_from_signalk(const String &host, uint16_t port) {
    if (host.length() == 0) {
        net::logf("[layout] fetch: empty host");
        return false;
    }
    if (WiFi.status() != WL_CONNECTED) {
        net::logf("[layout] fetch: no WiFi, skipping");
        return false;
    }
    String url = "http://" + host + ":" + String(port) +
                 "/signalk/v1/api/vessels/self/configuration/boat-mfd/layouts/value";
    HTTPClient http;
    http.setTimeout(5000);
    if (!http.begin(url)) {
        net::logf("[layout] fetch: HTTPClient.begin failed for %s", url.c_str());
        return false;
    }
    int code = http.GET();
    if (code != 200) {
        net::logf("[layout] fetch %s -> HTTP %d (keeping current config)", url.c_str(), code);
        http.end();
        return false;
    }
    String body = http.getString();
    http.end();
    if (body.length() == 0) {
        net::logf("[layout] fetch: empty body");
        return false;
    }
    if (!ensure_alloc()) return false;
    bool ok = apply_json(body.c_str(), body.length());
    if (ok) {
        net::logf("[layout] fetched from %s:%u", host.c_str(), port);
    }
    return ok;
}

// Returns the live config. Caller MUST call load_default() first.
// If PSRAM alloc failed, this returns *nullptr - intentional, because if
// PSRAM is broken there is no useful fallback. Callers should check
// loaded() before using.
const Config &current() {
    return *s_current;
}

bool loaded() {
    return s_loaded;
}

static const char *screen_type_name(ScreenType t) {
    switch (t) {
    case SCREEN_QUADRANTS:
        return "quadrants";
    case SCREEN_STEERING:
        return "steering";
    case SCREEN_AUTOPILOT:
        return "autopilot";
    case SCREEN_ROUTE:
        return "route";
    case SCREEN_TRIP:
        return "trip";
    case SCREEN_CHART:
        return "chart";
    default:
        return "?";
    }
}

static const char *tile_type_name(TileType t) {
    switch (t) {
    case TILE_WIND:
        return "wind";
    case TILE_NAV:
        return "nav";
    case TILE_DEPTH_TEMP:
        return "depth_temp";
    case TILE_DEVICE_STATUS:
        return "device_status";
    case TILE_BIG_NUMBER:
        return "big_number";
    case TILE_COMPASS:
        return "compass";
    default:
        return "?";
    }
}

void show_summary() {
    if (!s_loaded) {
        net::logf("[layout] not loaded");
        return;
    }
    net::logf("[layout] version=%d default=%s demo=%lums", s_current->version,
              s_current->settings.default_screen,
              (unsigned long)s_current->settings.demo_period_ms);
    for (size_t i = 0; i < s_current->screen_count; ++i) {
        const Screen &s = s_current->screens[i];
        net::logf("[layout] screen %u id=%s type=%s tiles=%u", (unsigned)i, s.id,
                  screen_type_name(s.type), (unsigned)s.tile_count);
        for (size_t j = 0; j < s.tile_count; ++j) {
            const Tile &t = s.tiles[j];
            net::logf("[layout]   tile %u id=%s type=%s paths=%u", (unsigned)j, t.id,
                      tile_type_name(t.type), (unsigned)t.path_count);
        }
    }
    for (size_t i = 0; i < s_current->alarm_count; ++i) {
        const AlarmRule &a = s_current->alarms[i];
        net::logf("[layout] alarm %s path=%s lt=%g msg=%s", a.id, a.path, a.has_lt ? a.lt : 0.0,
                  a.message);
    }
}

bool handleSerialCommand(const String &line) {
    if (line == "layout-show" || line == "layout") {
        show_summary();
        return true;
    }
    if (line == "layout-reload-default") {
        load_default();
        return true;
    }
    if (line.startsWith("layout-fetch ")) {
        String rest = line.substring(13);
        rest.trim();
        int colon = rest.indexOf(':');
        String host = colon < 0 ? rest : rest.substring(0, colon);
        uint16_t port = colon < 0 ? 3000 : (uint16_t)rest.substring(colon + 1).toInt();
        if (port == 0) port = 3000;
        fetch_from_signalk(host, port);
        return true;
    }
    if (line == "layout-fetch") {
        net::logf("usage: layout-fetch <host>[:<port>]");
        return true;
    }
    return false;
}

}  // namespace layout
