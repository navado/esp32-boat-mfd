#include "config_runtime.h"

#include <Arduino.h>
#include <Preferences.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <string.h>

#include "net.h"

namespace config {

// ---------------------------------------------------------------------------
// State

static SemaphoreHandle_t s_mtx = nullptr;
static RuntimeConfig s_cfg;
static TaskHandle_t s_worker = nullptr;

// Debounce timestamps per domain (millis() of last mutation).
static uint32_t s_last_mutate_ms[(uint8_t)Domain::COUNT] = {0};

static volatile uint32_t s_jobs_queued = 0;
static volatile uint32_t s_jobs_completed = 0;
static volatile uint32_t s_jobs_failed = 0;
static volatile uint32_t s_coalesced = 0;

// Debounce windows per domain (spec 08 defaults).
static constexpr uint32_t DEBOUNCE_UI_MS = 1500;
static constexpr uint32_t DEBOUNCE_ALARMS_MS = 1500;
static constexpr uint32_t DEBOUNCE_SK_MS = 500;  // signalk target rarely changes; quick save is fine

static uint32_t debounce_for(Domain d) {
    switch (d) {
    case Domain::Ui:      return DEBOUNCE_UI_MS;
    case Domain::Alarms:  return DEBOUNCE_ALARMS_MS;
    case Domain::SignalK: return DEBOUNCE_SK_MS;
    default:              return DEBOUNCE_UI_MS;
    }
}

// ---------------------------------------------------------------------------
// Persistence: one namespace per domain (spec 08 Persistence Details).

static const char *NS_UI = "cfg_ui";
static const char *NS_ALARMS = "cfg_alarm";
static const char *NS_SK = "cfg_sk";

static void persist_ui(const UiConfig &c, DomainMeta &meta) {
    Preferences p;
    if (!p.begin(NS_UI, false)) {
        meta.persist_error = true;
        strncpy(meta.last_error, "nvs begin failed", sizeof(meta.last_error) - 1);
        s_jobs_failed++;
        return;
    }
    p.putUShort("schema", meta.schema);
    p.putUInt("rev", meta.revision);
    p.putUChar("theme", (uint8_t)c.theme);
    p.putUChar("bright", c.brightness);
    p.putUChar("pos_fmt", (uint8_t)c.pos_format);
    p.putString("default", c.default_screen);
    p.end();
    meta.persist_error = false;
    meta.last_error[0] = 0;
    s_jobs_completed++;
}

static void load_ui(UiConfig &c, DomainMeta &meta) {
    Preferences p;
    p.begin(NS_UI, true);
    if (p.isKey("rev")) {
        c.theme = (Theme)p.getUChar("theme", (uint8_t)Theme::Night);
        c.brightness = p.getUChar("bright", 200);
        c.pos_format = (PosFormat)p.getUChar("pos_fmt", (uint8_t)PosFormat::DDM);
        String dft = p.getString("default", "dashboard");
        strncpy(c.default_screen, dft.c_str(), sizeof(c.default_screen) - 1);
        meta.source = Source::Storage;
        meta.revision = p.getUInt("rev", 0);
        meta.schema = p.getUShort("schema", 1);
    } else {
        // Legacy compatibility: read old "ui" namespace if present.
        p.end();
        Preferences legacy;
        legacy.begin("ui", true);
        if (legacy.isKey("bright")) {
            c.brightness = legacy.getUChar("bright", 200);
        }
        if (legacy.isKey("theme")) {
            String t = legacy.getString("theme", "night");
            c.theme = parse_theme(t.c_str(), Theme::Night);
        }
        if (legacy.isKey("pos_fmt")) {
            c.pos_format = (PosFormat)legacy.getUChar("pos_fmt", (uint8_t)PosFormat::DDM);
        }
        legacy.end();
        meta.source = Source::Storage;
        meta.revision = 0;
        meta.schema = 1;
        return;
    }
    p.end();
}

static void persist_alarms(const AlarmConfig &c, DomainMeta &meta) {
    Preferences p;
    if (!p.begin(NS_ALARMS, false)) {
        meta.persist_error = true;
        strncpy(meta.last_error, "nvs begin failed", sizeof(meta.last_error) - 1);
        s_jobs_failed++;
        return;
    }
    p.putUShort("schema", meta.schema);
    p.putUInt("rev", meta.revision);
    p.putDouble("depth_m", c.depth_alarm_m);
    p.putDouble("batt_v", c.battery_alarm_v);
    p.putBool("audible", c.audible);
    p.end();
    meta.persist_error = false;
    meta.last_error[0] = 0;
    s_jobs_completed++;
}

static void load_alarms(AlarmConfig &c, DomainMeta &meta) {
    Preferences p;
    p.begin(NS_ALARMS, true);
    if (p.isKey("rev")) {
        c.depth_alarm_m = p.getDouble("depth_m", 3.0);
        c.battery_alarm_v = p.getDouble("batt_v", 11.5);
        c.audible = p.getBool("audible", false);
        meta.source = Source::Storage;
        meta.revision = p.getUInt("rev", 0);
        meta.schema = p.getUShort("schema", 1);
        p.end();
        return;
    }
    p.end();
    // Legacy compatibility: old "ui" namespace stored depth_alarm_m / batt_alarm_v.
    Preferences legacy;
    legacy.begin("ui", true);
    if (legacy.isKey("depth_alarm_m")) {
        c.depth_alarm_m = legacy.getDouble("depth_alarm_m", 3.0);
    }
    if (legacy.isKey("batt_alarm_v")) {
        c.battery_alarm_v = legacy.getDouble("batt_alarm_v", 11.5);
    }
    legacy.end();
    meta.source = Source::Storage;
}

static void persist_signalk(const SignalKConfig &c, DomainMeta &meta) {
    Preferences p;
    if (!p.begin(NS_SK, false)) {
        meta.persist_error = true;
        strncpy(meta.last_error, "nvs begin failed", sizeof(meta.last_error) - 1);
        s_jobs_failed++;
        return;
    }
    p.putUShort("schema", meta.schema);
    p.putUInt("rev", meta.revision);
    p.putString("host", c.host);
    p.putUShort("port", c.port);
    if (c.token[0]) p.putString("token", c.token);
    p.end();
    meta.persist_error = false;
    meta.last_error[0] = 0;
    s_jobs_completed++;
}

static void load_signalk(SignalKConfig &c, DomainMeta &meta) {
    Preferences p;
    p.begin(NS_SK, true);
    if (p.isKey("rev")) {
        String h = p.getString("host", "");
        strncpy(c.host, h.c_str(), sizeof(c.host) - 1);
        c.port = p.getUShort("port", 3000);
        String t = p.getString("token", "");
        strncpy(c.token, t.c_str(), sizeof(c.token) - 1);
        meta.source = Source::Storage;
        meta.revision = p.getUInt("rev", 0);
        meta.schema = p.getUShort("schema", 1);
        p.end();
        return;
    }
    p.end();
    // Legacy: old "sk" namespace.
    Preferences legacy;
    legacy.begin("sk", true);
    if (legacy.isKey("host")) {
        String h = legacy.getString("host", "");
        strncpy(c.host, h.c_str(), sizeof(c.host) - 1);
        c.port = (uint16_t)legacy.getUInt("port", 3000);
    }
    if (legacy.isKey("token")) {
        String t = legacy.getString("token", "");
        strncpy(c.token, t.c_str(), sizeof(c.token) - 1);
    }
    legacy.end();
    meta.source = Source::Storage;
}

// ---------------------------------------------------------------------------
// Storage worker - one task that scans for pending domains and flushes
// after the debounce window has elapsed. Runs on core 0 so LVGL is
// undisturbed.

static void persist_domain_locked(Domain d) {
    DomainMeta &m = s_cfg.meta[(uint8_t)d];
    if (!m.persist_pending) return;
    m.persist_pending = false;
    m.dirty = false;
    switch (d) {
    case Domain::Ui:      persist_ui(s_cfg.ui, m); break;
    case Domain::Alarms:  persist_alarms(s_cfg.alarms, m); break;
    case Domain::SignalK: persist_signalk(s_cfg.signalk, m); break;
    default: break;
    }
}

static void worker_task(void *) {
    for (;;) {
        bool any_pending = false;
        uint32_t now = millis();
        for (uint8_t i = 0; i < (uint8_t)Domain::COUNT; ++i) {
            Domain d = (Domain)i;
            uint32_t since;
            bool ready = false;
            if (xSemaphoreTake(s_mtx, portMAX_DELAY)) {
                if (s_cfg.meta[i].persist_pending) {
                    since = now - s_last_mutate_ms[i];
                    if (since >= debounce_for(d)) {
                        persist_domain_locked(d);
                        ready = true;
                    } else {
                        any_pending = true;
                    }
                }
                xSemaphoreGive(s_mtx);
            }
            if (ready) {
                net::logf("[cfg] persisted %s rev=%u", domain_name(d),
                          (unsigned)s_cfg.meta[i].revision);
            }
        }
        // Idle quickly if nothing pending; tight loop if a debounce is
        // close to expiry.
        vTaskDelay(pdMS_TO_TICKS(any_pending ? 200 : 500));
    }
}

// ---------------------------------------------------------------------------
// Public API

void setup() {
    if (s_mtx) return;
    s_mtx = xSemaphoreCreateMutex();
    if (xSemaphoreTake(s_mtx, portMAX_DELAY)) {
        load_ui(s_cfg.ui, s_cfg.meta[(uint8_t)Domain::Ui]);
        clamp_ui(s_cfg.ui);
        load_alarms(s_cfg.alarms, s_cfg.meta[(uint8_t)Domain::Alarms]);
        clamp_alarms(s_cfg.alarms);
        load_signalk(s_cfg.signalk, s_cfg.meta[(uint8_t)Domain::SignalK]);
        clamp_signalk(s_cfg.signalk);
        uint32_t now = millis();
        for (uint8_t i = 0; i < (uint8_t)Domain::COUNT; ++i) {
            s_cfg.meta[i].updated_ms = now;
            s_cfg.meta[i].dirty = false;
            s_cfg.meta[i].persist_pending = false;
        }
        xSemaphoreGive(s_mtx);
    }
    net::logf("[cfg] loaded ui(theme=%s bright=%u fmt=%s) alarms(depth=%.1fm batt=%.1fV) sk(%s:%u)",
              theme_name(s_cfg.ui.theme), (unsigned)s_cfg.ui.brightness,
              pos_format_name(s_cfg.ui.pos_format), s_cfg.alarms.depth_alarm_m,
              s_cfg.alarms.battery_alarm_v, s_cfg.signalk.host, s_cfg.signalk.port);
    xTaskCreatePinnedToCore(worker_task, "cfg_store", 4096, nullptr, 1, &s_worker, 0);
}

void snapshot(RuntimeConfig &out) {
    if (!s_mtx) return;
    xSemaphoreTake(s_mtx, portMAX_DELAY);
    out = s_cfg;
    xSemaphoreGive(s_mtx);
}

UiConfig ui() {
    UiConfig c;
    if (!s_mtx) return c;
    xSemaphoreTake(s_mtx, portMAX_DELAY);
    c = s_cfg.ui;
    xSemaphoreGive(s_mtx);
    return c;
}

AlarmConfig alarms() {
    AlarmConfig c;
    if (!s_mtx) return c;
    xSemaphoreTake(s_mtx, portMAX_DELAY);
    c = s_cfg.alarms;
    xSemaphoreGive(s_mtx);
    return c;
}

SignalKConfig signalk() {
    SignalKConfig c;
    if (!s_mtx) return c;
    xSemaphoreTake(s_mtx, portMAX_DELAY);
    c = s_cfg.signalk;
    xSemaphoreGive(s_mtx);
    return c;
}

DomainMeta meta(Domain d) {
    DomainMeta m;
    if (!s_mtx || (uint8_t)d >= (uint8_t)Domain::COUNT) return m;
    xSemaphoreTake(s_mtx, portMAX_DELAY);
    m = s_cfg.meta[(uint8_t)d];
    xSemaphoreGive(s_mtx);
    return m;
}

static void mark_dirty_locked(Domain d, Source src) {
    DomainMeta &m = s_cfg.meta[(uint8_t)d];
    if (m.persist_pending) s_coalesced++;
    m.dirty = true;
    m.persist_pending = true;
    m.source = src;
    m.revision++;
    m.updated_ms = millis();
    s_last_mutate_ms[(uint8_t)d] = millis();
    s_jobs_queued++;
}

bool mutate(const Mutation &m) {
    if (!s_mtx) return false;
    xSemaphoreTake(s_mtx, portMAX_DELAY);
    bool ok = true;
    Domain d = Domain::Ui;
    switch (m.kind) {
    case MutationKind::SetTheme:
        s_cfg.ui.theme = m.theme;
        clamp_ui(s_cfg.ui);
        d = Domain::Ui;
        break;
    case MutationKind::SetBrightness:
        s_cfg.ui.brightness = m.u8;
        clamp_ui(s_cfg.ui);
        d = Domain::Ui;
        break;
    case MutationKind::SetPosFormat:
        s_cfg.ui.pos_format = m.pos_format;
        clamp_ui(s_cfg.ui);
        d = Domain::Ui;
        break;
    case MutationKind::SetDefaultScreen:
        strncpy(s_cfg.ui.default_screen, m.s, sizeof(s_cfg.ui.default_screen) - 1);
        clamp_ui(s_cfg.ui);
        d = Domain::Ui;
        break;
    case MutationKind::SetDepthAlarm:
        s_cfg.alarms.depth_alarm_m = m.d;
        clamp_alarms(s_cfg.alarms);
        d = Domain::Alarms;
        break;
    case MutationKind::SetBatteryAlarm:
        s_cfg.alarms.battery_alarm_v = m.d;
        clamp_alarms(s_cfg.alarms);
        d = Domain::Alarms;
        break;
    case MutationKind::SetSignalKTarget:
        strncpy(s_cfg.signalk.host, m.s, sizeof(s_cfg.signalk.host) - 1);
        s_cfg.signalk.port = m.u16;
        clamp_signalk(s_cfg.signalk);
        d = Domain::SignalK;
        break;
    case MutationKind::SetSignalKToken:
        strncpy(s_cfg.signalk.token, m.s, sizeof(s_cfg.signalk.token) - 1);
        d = Domain::SignalK;
        break;
    default:
        ok = false;
        break;
    }
    if (ok) mark_dirty_locked(d, m.source);
    xSemaphoreGive(s_mtx);
    return ok;
}

void flush_pending() {
    if (!s_mtx) return;
    xSemaphoreTake(s_mtx, portMAX_DELAY);
    for (uint8_t i = 0; i < (uint8_t)Domain::COUNT; ++i) {
        persist_domain_locked((Domain)i);
    }
    xSemaphoreGive(s_mtx);
}

uint32_t persist_jobs_queued() { return s_jobs_queued; }
uint32_t persist_jobs_completed() { return s_jobs_completed; }
uint32_t persist_jobs_failed() { return s_jobs_failed; }
uint32_t coalesced_writes() { return s_coalesced; }

}  // namespace config
