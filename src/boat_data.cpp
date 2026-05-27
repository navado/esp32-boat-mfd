#include "boat_data.h"

#include <string.h>

#ifdef ARDUINO
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#endif

namespace boat {

namespace {

Snapshot g_snap;
Priority g_prio;
Timeouts g_timeouts;

#ifdef ARDUINO
SemaphoreHandle_t g_mtx = nullptr;
void mtx_lock() {
    if (!g_mtx) g_mtx = xSemaphoreCreateMutex();
    if (g_mtx) xSemaphoreTake(g_mtx, portMAX_DELAY);
}
void mtx_unlock() {
    if (g_mtx) xSemaphoreGive(g_mtx);
}
#else
void mtx_lock() {}
void mtx_unlock() {}
#endif

struct Lock {
    Lock() { mtx_lock(); }
    ~Lock() { mtx_unlock(); }
};

}  // namespace

uint8_t rank_of(const Priority &p, SourceKind src) {
    if (src == SourceKind::None) return 255;
    for (uint8_t i = 0; i < sizeof(p.order) / sizeof(p.order[0]); ++i) {
        if (p.order[i] == src) return i;
        if (p.order[i] == SourceKind::None) break;
    }
    return 255;
}

uint32_t timeout_for(const Timeouts &t, SourceKind src) {
    switch (src) {
        case SourceKind::Nmea2000: return t.nmea2000_ms;
        case SourceKind::NmeaWifi: return t.nmea_wifi_ms;
        case SourceKind::SignalK:  return t.signalk_ms;
        case SourceKind::Demo:     return t.demo_ms;
        default:                   return 0;
    }
}

bool fresh(const Field &f, uint32_t now_ms, uint32_t timeout_ms) {
    if (f.source == SourceKind::None) return false;
    if (timeout_ms == 0) return false;
    return (now_ms - f.updated_ms) < timeout_ms;
}

double value_or_nan(const Field &f, uint32_t now_ms, uint32_t timeout_ms) {
    return fresh(f, now_ms, timeout_ms) ? f.value : NAN;
}

bool should_accept(SourceKind incoming, SourceKind current,
                   uint32_t current_updated_ms, uint32_t now_ms,
                   const Priority &p, const Timeouts &t) {
    if (incoming == SourceKind::None) return false;
    if (current == SourceKind::None) return true;
    uint8_t r_in = rank_of(p, incoming);
    uint8_t r_cur = rank_of(p, current);
    if (r_in <= r_cur) return true;  // same or higher priority
    // Incoming is lower priority - only accept if current owner is stale.
    uint32_t to = timeout_for(t, current);
    if (to == 0) return true;
    return (now_ms - current_updated_ms) >= to;
}

const char *source_name(SourceKind s) {
    switch (s) {
        case SourceKind::None:     return "none";
        case SourceKind::Demo:     return "demo";
        case SourceKind::SignalK:  return "signalk";
        case SourceKind::NmeaWifi: return "nmea-wifi";
        case SourceKind::Nmea2000: return "nmea2000";
        default:                   return "?";
    }
}

void set_priority(const Priority &p) {
    Lock _;
    g_prio = p;
}

Priority get_priority() {
    Lock _;
    return g_prio;
}

void set_timeouts(const Timeouts &t) {
    Lock _;
    g_timeouts = t;
}

Timeouts get_timeouts() {
    Lock _;
    return g_timeouts;
}

bool publish(Field Snapshot::*field, SourceKind src, uint32_t now_ms, double value) {
    Lock _;
    Field &f = g_snap.*field;
    if (!should_accept(src, f.source, f.updated_ms, now_ms, g_prio, g_timeouts)) {
        return false;
    }
    f.value = value;
    f.updated_ms = now_ms;
    f.source = src;
    return true;
}

bool publish_autopilot_state(SourceKind src, uint32_t now_ms, const char *state) {
    if (!state) return false;
    Lock _;
    if (!should_accept(src, g_snap.autopilot_state_source,
                       g_snap.autopilot_state_updated_ms, now_ms,
                       g_prio, g_timeouts)) {
        return false;
    }
    strncpy(g_snap.autopilot_state, state, sizeof(g_snap.autopilot_state) - 1);
    g_snap.autopilot_state[sizeof(g_snap.autopilot_state) - 1] = 0;
    g_snap.autopilot_state_updated_ms = now_ms;
    g_snap.autopilot_state_source = src;
    return true;
}

void copy_snapshot(Snapshot &out) {
    Lock _;
    out = g_snap;
}

void reset_all() {
    Lock _;
    g_snap = Snapshot{};
    g_prio = Priority{};
    g_timeouts = Timeouts{};
}

}  // namespace boat
