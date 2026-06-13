#pragma once
// Hand-written pure protocol logic: version/compat, auth, the target-side
// session table. No Arduino/LVGL/BLE — host-testable (links in the native env).
#include <stdint.h>
#include "proto/records_generated.h"

namespace proto {

constexpr int kMaxSessions = 8;
constexpr long kDefaultTtlMs = 10000;

// "1.7" -> major 1, minor 7. Returns false on malformed input.
bool parse_version(const char *v, int &major, int &minor);
// Same-major required; our minor must be >= 0; unknown-minor accepted.
bool version_compatible(const char *peer_v);

// Shared-key check. configured_key == "" => open (accept any). Constant-ish compare.
bool auth_ok(const char *configured_key, const char *presented_key);

// Target-side session table (pure; the firmware feeds it now-millis).
struct SessionTable {
    Session sessions[kMaxSessions];
    char sessionId[kMaxSessions][16];
    bool used[kMaxSessions] = {false};
    long mostRecentIdx = -1;

    void clear();
    // Attach: returns index >=0 (assigns sessionId_out), or -1 if full.
    int attach(const Attach &a, long now_ms, char *sessionId_out, size_t cap);
    // Returns true and refreshes lastSeen; false if unknown sessionId.
    bool heartbeat(const char *sid, long now_ms);
    bool detach(const char *sid);
    // Reap sessions with lastSeen older than ttl_ms; returns count reaped.
    int reap(long now_ms, long ttl_ms);
    int active_count() const;
    // Fill a ControlState for serialization.
    void to_control_state(ControlState &out, const char *currentView) const;
};

}  // namespace proto
