# Configuration Storage And Synchronization

Status: proposed architecture plan. This document defines how device
configuration should be stored, versioned, synchronized, and reconciled between
internal and external sources.

## Goal

The device needs one coherent configuration model even though updates can come
from several places:

- baked-in firmware defaults
- NVS / flash cache on the ESP32
- web UI
- BLE configuration writes
- serial / BLE console commands
- Signal K layout/config endpoint
- future companion app

The goal is deterministic behavior: every value has an owner, every update has
a revision, and the UI can explain whether the device is using local config,
server config, or unsynchronized local changes.

## Configuration Domains

Do not store all settings as one untyped JSON blob. Use domain-specific records
with clear ownership and persistence rules.

| Domain | Examples | Local Storage | External Sync |
|---|---|---|---|
| Identity | device id, hostname | NVS | optional publish |
| WiFi | saved networks, AP setup SSID | NVS | local only by default |
| UI state | theme, brightness, default screen | NVS | optional |
| Alarm thresholds | shallow depth, low battery | NVS | optional |
| Layout | screens, widgets, alarms, paths | NVS or LittleFS cache | yes |
| Widget overrides | runtime-tuned attrs | NVS or layout overlay | yes |
| Signal K target | host, port, token | NVS | optional |
| Runtime status | IP, RSSI, last errors | RAM only | publish/read only |

Security-sensitive values such as WiFi passwords and Signal K tokens should not
be sent to external layout/config endpoints unless an explicit export flow is
implemented.

## Source Model

Each persisted configuration domain should carry metadata:

```json
{
  "domain": "layout",
  "schema": 1,
  "source": "local|server|ble|web|serial|default",
  "revision": 42,
  "etag": "optional-server-token",
  "updated_ms": 123456,
  "dirty": false,
  "payload": {}
}
```

Embedded representation can be fixed structs plus a JSON payload where needed.
The metadata is the important contract.

Definitions:

- `schema`: format version for migration.
- `source`: last writer.
- `revision`: monotonic local revision per domain.
- `etag`: external version token when the server provides one.
- `updated_ms`: device uptime timestamp for diagnostics. If RTC/time sync is
  added later, also store wall-clock time.
- `dirty`: local changes exist that have not been pushed or acknowledged by the
  external source.

## RAM-First Configuration Subsystem

The live configuration source of truth is a validated RAM object. Storage is a
checkpoint, not the runtime model.

Principles:

- Apply config changes to RAM first.
- Render and behavior read from RAM, never directly from NVS/LittleFS.
- Persistence is asynchronous and coalesced.
- External fetch/push is asynchronous and cannot block UI/touch/rendering.
- A failed write to storage does not roll back live state.
- A failed external push does not roll back live state.
- Boot reads storage once per domain, validates it, then works from RAM.

### Ownership

One config owner serializes all writes to the RAM config model.

Recommended task ownership:

```text
UI/app task
  - applies config that affects LVGL or live UI behavior
  - receives typed config mutations
  - marks domains dirty
  - schedules persistence

Config storage worker, core 0
  - writes dirty snapshots to NVS/LittleFS
  - reads storage only at boot, explicit reload, or recovery
  - reports write/read failures

Network worker, core 0
  - fetches external config
  - pushes dirty external-owned domains
  - never applies config directly to LVGL
```

### Runtime Objects

Add a central module:

```cpp
namespace config {

enum class Domain : uint8_t {
  Identity,
  Wifi,
  SignalK,
  Ui,
  Alarms,
  Layout,
  Widgets,
  Hardware,
};

enum class Source : uint8_t {
  Default,
  Storage,
  Web,
  Ble,
  Serial,
  SignalK,
  Companion,
};

enum class PersistPolicy : uint8_t {
  Manual,        // write only on explicit close/apply
  Debounced,     // write after quiet period
  ImmediateSafe, // write soon, but still through worker
  Never,
};

struct DomainMeta {
  uint16_t schema;
  Source source;
  uint32_t revision;
  char etag[48];
  uint32_t updated_ms;
  bool dirty;
  bool persist_pending;
  bool external_pending;
  bool conflict;
  char last_error[96];
};

struct RuntimeConfig {
  UiConfig ui;
  AlarmConfig alarms;
  LayoutConfig layout;
  WidgetOverrideConfig widgets;
  SignalKConfig signalk;
  HardwareConfig hardware;
  DomainMeta meta[(uint8_t)Domain::Hardware + 1];
};

bool boot_load();
bool snapshot(RuntimeConfig &out);
bool snapshot_domain(Domain domain, void *out, size_t out_size);
bool mutate(const Mutation &m);
bool close_edit_session(Domain domain);
bool request_persist(Domain domain, PersistReason reason);
bool request_external_sync(Domain domain);

}  // namespace config
```

`RuntimeConfig` is intentionally domain-structured. Do not replace it with one
large untyped JSON object. Layout documents may remain JSON-backed internally,
but the live layout domain still has metadata and dirty state.

### Mutation Flow

All changes use typed mutations:

```text
web/BLE/serial/touch event
  -> validate command shape
  -> enqueue ConfigMutate(domain, key/value or blob)
  -> config owner validates domain constraints
  -> update RAM object
  -> apply live side effects
  -> increment revision
  -> mark dirty/persist_pending
  -> schedule persistence according to policy
  -> notify observers
```

Live side effects examples:

- brightness: update RAM value, call board backlight API immediately, schedule
  debounced persistence
- theme: update RAM value, repaint/apply theme, schedule persistence on close or
  after debounce
- alarm threshold: update RAM value, alarm checks immediately use new threshold,
  schedule debounced persistence
- layout: parse and validate into a staging object, swap RAM layout, rebuild UI
  on UI task, schedule persistence after successful apply

### Edit Sessions

Interactive settings screens should have explicit edit sessions:

```text
ConfigEditBegin(domain)
  -> copy current domain into draft RAM
  -> UI controls mutate draft or live depending on setting class

ConfigEditApply(domain)
  -> validate draft
  -> swap live RAM
  -> apply side effects
  -> request persistence once

ConfigEditCancel(domain)
  -> discard draft
  -> restore live side effects if any were previewed
```

For the current Settings screen, use two classes:

- immediate live settings: brightness, theme preview, alarm thresholds
- staged settings: source priority, NMEA2000 enable, board orientation, layout
  changes that require rebuild/reboot

Immediate live settings still should not write NVS on every tap if the user is
moving through choices. They write on close/apply or after a short debounce.

### Persistence Policy

Persist only after meaningful stability.

Recommended defaults:

| Domain | Policy | Reason |
| --- | --- | --- |
| Identity | ImmediateSafe | rare, important |
| WiFi | ImmediateSafe | must survive reboot after save |
| SignalK target | ImmediateSafe | rare, may reboot |
| UI | Debounced or Manual on settings close | avoid flash churn |
| Alarm thresholds | Debounced or Manual on settings close | user may step through buckets |
| Layout | Debounced after valid apply | larger payload, last-good checkpoint |
| Widget overrides | Debounced batch | many small edits possible |
| Hardware | Manual/apply | may require reboot |
| Runtime status | Never | RAM only |

Debounce defaults:

```text
ui settings: 1500 ms quiet period
alarm thresholds: 1500 ms quiet period
widget overrides: 3000 ms quiet period
layout: 5000 ms quiet period after successful apply
```

If a Settings screen is closed while domains are dirty, schedule persistence
immediately for those domains.

### Storage Read/Write Rules

Reads:

- boot only
- explicit recovery/reload command
- migration step
- diagnostics may read cached metadata, not raw storage

Writes:

- go through one serialized storage worker
- write only changed domains
- coalesce multiple changes into one write
- update metadata after payload write succeeds
- maintain last-known-good payload for layout and large domains

Do not:

- read NVS in render/refresh loops
- write NVS in LVGL callbacks directly
- write on every slider movement or repeated button tap
- use storage write success as a condition for live apply

### Dirty State

Each domain tracks:

```text
clean: RAM == persisted checkpoint
dirty: RAM differs from persisted checkpoint
persist_pending: write queued/debounced
persist_error: last write failed, RAM still active
external_dirty: external-owned domain has local change not pushed
conflict: local and remote changed from same base
```

UI/API should distinguish:

- "applied" means RAM/live state changed.
- "saved" means local persistence succeeded.
- "synced" means external owner acknowledged the change.

This matters for responsiveness: a user should see the brightness/theme/layout
change immediately, even if flash or network work happens later.

### External Change Flow

External systems never write storage directly.

```text
external fetch receives payload
  -> network worker validates transport status and size
  -> enqueue ConfigExternalCandidate(domain, payload, etag)
  -> config owner validates schema/domain rules
  -> if accepted, update RAM and live state
  -> mark source/etag/revision
  -> schedule local persistence as last-good checkpoint
```

If local RAM is dirty:

- same external etag/base: push local or keep local
- newer external etag: mark conflict
- local policy says remote authoritative and no dirty local changes: accept
  remote

External changes apply live from RAM after validation; persistence follows.

### Observer Notifications

Config mutations should notify interested systems without polling storage:

```cpp
enum class ConfigEventType {
  DomainApplied,
  PersistScheduled,
  PersistSucceeded,
  PersistFailed,
  ExternalSyncStarted,
  ExternalSyncSucceeded,
  ExternalSyncFailed,
  Conflict,
};
```

Subscribers:

- UI screens/settings
- web state API
- BLE configuration characteristic
- logger/bench diagnostics
- alarm subsystem
- data-source subsystem

Observers receive domain + revision, then read a RAM snapshot if needed.

## Precedence Rules

On boot, choose configuration in this order:

1. valid local persisted config
2. baked-in firmware defaults
3. external fetch result, applied only after validation

External config should not block boot. The device should start from the last
known good local config, then fetch external config asynchronously and apply it
only if it is valid and newer according to the domain policy.

Recommended domain policies:

- WiFi: local authoritative. External sources may not overwrite saved networks.
- Identity: local authoritative unless the user explicitly accepts server rename.
- UI state: local authoritative for brightness/theme unless server policy is
  enabled.
- Layout: server authoritative when a Signal K layout endpoint is configured,
  otherwise local authoritative.
- Widget overrides: local dirty changes overlay the last server layout until
  synchronized.

## Storage Backends

### NVS

Use NVS for small bounded values:

- identity
- WiFi saved networks
- Signal K target
- brightness/theme/default screen
- shallow depth and battery alarm thresholds
- small metadata records

Keep existing `Preferences` usage for simple domains. For larger structured
domains, store compact JSON with an explicit maximum size.

### LittleFS Or Flash Partition

Use LittleFS or a dedicated flash partition for larger documents if layout and
widget overrides outgrow safe NVS value sizes.

Candidates:

- last good layout JSON
- widget override overlay
- external sync journal

Do not move to LittleFS until the size pressure is real. The first
implementation can continue with PSRAM parsing plus bounded persisted JSON.

### RAM Snapshots

Runtime status and model snapshots live in RAM only:

- current Signal K values
- WiFi scan result
- last operation status
- queue depths and bench metrics

These should be exposed via API but not persisted on every change.

## Synchronization Flow

### Boot

```text
load baked defaults
load persisted config domains
validate each domain
apply last good local config
start UI
start WiFi/BLE/web
if external layout/config source is configured:
  enqueue ConfigFetch(domain)
```

Boot must not wait for network availability.

### External Fetch

```text
net worker fetches config document
validate schema, size, and domain constraints
compare external version with local metadata
if accepted:
  post ConfigApply(domain, payload, metadata) to UI/config owner
  persist as last good
else:
  keep local config and log reason
```

Layout application still happens on the LVGL/UI task if it mutates LVGL-backed
state.

### Local Change

```text
web/BLE/serial/view emits typed config event
controller validates update
model is updated on owner task
revision increments
dirty = true if an external source owns that domain
persist domain or overlay
enqueue optional ConfigPush(domain)
```

Local changes must be durable before the UI reports them as saved.

### External Push

```text
net worker sends changed domain or patch
if server accepts:
  update etag/revision metadata
  dirty = false
  persist metadata
else:
  keep dirty = true
  store last error
```

Failed push must not revert local UI state.

## Conflict Handling

Avoid silent last-writer-wins for user-visible layout changes.

Recommended rules:

- If local `dirty == false` and external version is newer: accept external.
- If local `dirty == true` and external version is unchanged from the base:
  push local changes.
- If local `dirty == true` and external version also changed: mark conflict.
- In conflict, keep local active state and expose the conflict via logs/API/UI.

Minimum conflict record:

```json
{
  "domain": "layout",
  "local_revision": 43,
  "base_etag": "abc",
  "remote_etag": "def",
  "status": "conflict"
}
```

The first implementation may resolve layout conflicts manually through web/API:

- keep local
- accept remote
- export local JSON
- retry push

## Patch And Overlay Strategy

For layout and widgets, prefer an overlay model:

```text
baked default layout
  + server layout
  + local widget overrides
  = effective runtime layout
```

This lets runtime tuning remain small and syncable without rewriting the whole
layout document on every slider/color/font change.

Widget override record:

```json
{
  "path": "wind.awa_marker.color",
  "value": "#ffffff",
  "revision": 12,
  "source": "web"
}
```

The layout controller should be able to:

- apply full layout
- apply one override
- export effective layout
- export local override patch
- clear overrides

## Event Integration

The event-driven MVC design should include configuration events:

```cpp
enum class EventType : uint8_t {
    ConfigLoad,
    ConfigFetch,
    ConfigApply,
    ConfigPatch,
    ConfigPush,
    ConfigConflict,
    ConfigClearLocal,
    ConfigEditBegin,
    ConfigEditApply,
    ConfigEditCancel,
    ConfigPersist,
    ConfigPersistResult,
};
```

Events should identify the domain and carry either a fixed payload, a small
argument string, or a heap-owned PSRAM blob with explicit ownership.

Task ownership:

- UI/config owner validates and applies UI/layout-affecting config.
- Net worker fetches and pushes external config.
- Web/BLE/serial only enqueue events.
- Persistence happens in the owner responsible for the domain, or through a
  small config storage service with serialized access.

## Proposed Module Design

### `config_model`

Pure C++ domain structs and validators. Host-testable.

Files:

- `include/config_model.h`
- `src/config_model.cpp`

Responsibilities:

- defaults
- schema versions
- domain validation
- domain equality/hash for changed detection
- migration from older domain schema

### `config_runtime`

RAM owner and mutation dispatcher.

Files:

- `include/config_runtime.h`
- `src/config_runtime.cpp`

Responsibilities:

- owns `RuntimeConfig`
- applies mutations
- manages edit sessions/drafts
- increments revisions
- tracks dirty/persist/external/conflict state
- posts observer events
- exposes snapshot APIs

Only this module mutates live config domain objects.

### `config_store`

Serialized persistence worker.

Files:

- `include/config_store.h`
- `src/config_store.cpp`

Responsibilities:

- load domains at boot
- persist one or more dirty domains
- store last-good layout JSON
- write metadata after payload success
- keep read/write counters and last error
- never call LVGL

Storage worker queue:

```cpp
enum class StoreJobType {
  LoadDomain,
  PersistDomain,
  PersistAllDirty,
  Compact,
};

struct StoreJob {
  StoreJobType type;
  config::Domain domain;
  uint32_t revision;
};
```

The worker ignores stale persist jobs if RAM has moved to a newer revision and
a newer persist job is already pending.

### `config_sync`

External fetch/push coordinator.

Files:

- `include/config_sync.h`
- `src/config_sync.cpp`

Responsibilities:

- fetch external config/layout
- push external-owned dirty domains
- pass candidates to `config_runtime`
- record etag/base revision
- never mutate storage or LVGL directly

### `config_api`

HTTP/BLE/serial adapters.

Responsibilities:

- parse transport payloads
- reject obviously oversized/bad requests early
- enqueue typed mutations or sync requests
- expose RAM status snapshots
- never read storage directly for normal status

## Current Code Migration

Known current storage/config call sites:

| Area | Current behavior | Target behavior |
| --- | --- | --- |
| `ui_data.cpp` brightness | RAM cache + immediate NVS write | RAM config + board apply + debounced persist |
| `ui_data.cpp` thresholds | RAM cache + immediate NVS write | RAM config + alarm notify + debounced persist |
| `ui_data.cpp` pos format | immediate NVS write | RAM config + screen notify + settings-close persist |
| `main.cpp` theme | immediate Preferences write | config mutation + repaint + debounced persist |
| `app_events.cpp` SetTheme/SetBrightness | direct Preferences writes | config mutation |
| `layout_loader.cpp` | live PSRAM config + last JSON RAM only | layout domain RAM + last-good persistent checkpoint |
| `signalk.cpp` target | Preferences read/write | Signal K domain RAM + immediate-safe persist |
| `wifi_store.cpp` | direct Preferences load/save | WiFi domain with immediate-safe persist |
| `touch_cal.cpp` | direct calibration storage | Input/hardware domain, explicit apply |
| `screen_trip.cpp` trip counters | direct Preferences | trip runtime domain or separate trip store; not config |

Migration order:

1. Add `config_model` for UI, alarms, and layout metadata.
2. Move `ui_data` brightness/threshold/position getters to read RAM config.
3. Keep old NVS keys as boot-load compatibility.
4. Add debounced persist for UI and alarm domains.
5. Route Settings close through `ConfigEditApply`/`ConfigPersist`.
6. Move theme persistence from `main.cpp` and `app_events.cpp` into config.
7. Wrap layout apply with `config_runtime` metadata and last-good persistence.
8. Add `/api/config/status` from RAM metadata.
9. Move Signal K target to config domain.
10. Move WiFi store only after UI/layout domains are stable.

## Persistence Details

### Small Domains In NVS

Use one namespace per domain:

| Domain | Namespace | Keys |
| --- | --- | --- |
| UI | `cfg_ui` | `schema`, `rev`, `theme`, `bright`, `pos_fmt`, `default_screen` |
| Alarms | `cfg_alarm` | `schema`, `rev`, `depth_m`, `batt_v`, `audible` |
| Signal K | `cfg_sk` | `schema`, `rev`, `host`, `port`, `token_ref` |
| Hardware | `cfg_hw` | `schema`, `rev`, `rotation`, `gesture`, feature flags |
| Metadata | `cfg_meta` | per-domain source/etag/dirty/conflict summary |

Keep legacy namespaces readable during migration:

- `ui`
- `sk`
- `touch_cal`
- `wifi`
- `net`

After loading a legacy key, write the new domain only when that domain is next
persisted. Do not rewrite all settings during boot.

### Large Domains

Layout and widget overrides may exceed comfortable NVS sizes.

Use either:

- NVS blob with explicit maximum size while documents are small
- LittleFS/dedicated flash partition when layout JSON grows

Required records:

```text
layout/current.json      last successfully applied local/runtime layout
layout/base.json         last external version accepted
layout/overrides.json    local widget override patch
layout/meta.json         schema/revision/etag/dirty/conflict
```

Write pattern:

```text
write payload to temp
validate temp can be read back
rename/swap temp to active where backend supports it
write metadata after payload success
notify PersistSucceeded
```

If the backend is NVS and cannot rename, store a checksum and only promote the
record after the payload read-back validates.

## Responsiveness Requirements

- UI mutation handler target: under 5 ms for small domains.
- Layout validation/apply may take longer but must run on the UI/config owner
  in bounded phases when it affects LVGL.
- Storage worker must never block LVGL.
- External fetch/push must never block LVGL.
- Repeated Settings taps must not produce repeated NVS writes.
- Render/refresh paths must perform zero storage reads.

Metrics:

- config queue depth and high-water mark
- persist queue depth and high-water mark
- last persist duration per domain
- last load duration per domain
- skipped stale persist jobs
- coalesced write count
- storage read/write count since boot
- last config mutation duration

Expose these in `/api/config/status` and `bench`.

## API Surface

Recommended HTTP endpoints:

```text
GET  /api/config
GET  /api/config/<domain>
PUT  /api/config/<domain>
POST /api/config/<domain>/sync
POST /api/config/<domain>/push
POST /api/config/<domain>/clear-local
GET  /api/config/status
```

Recommended console commands:

```text
config-show
config-show <domain>
config-sync <domain>
config-push <domain>
config-use-local <domain>
config-use-remote <domain>
```

BLE can expose the same operations through the existing configuration
characteristic or console command path, with payload-size limits respected.

## Validation And Migration

Every persisted domain needs:

- schema version
- maximum serialized size
- validation before apply
- migration from previous schema where practical
- fallback to last good or baked default on failure

Migration should be domain-by-domain. Do not require wiping all settings when
one domain changes schema.

## Diagnostics

Expose sync state in `/bench` or `/api/config/status`:

- active source per domain
- local revision
- remote etag/version
- dirty flag
- conflict flag
- last fetch result
- last push result
- persisted size
- last validation error

UI should eventually surface at least:

- "local changes pending"
- "server layout active"
- "configuration conflict"
- "last sync failed"

## Migration Plan

### Phase 1: Inventory Current Settings

List every `Preferences` namespace/key and every layout/config payload. Assign
each to a domain and document max size.

Acceptance:

- No setting exists without an owning domain.
- Sensitive settings are marked local-only.

### Phase 2: Add Config Metadata

Add metadata for layout, UI state, Signal K target, and identity without
changing behavior.

Acceptance:

- Existing settings still load.
- New metadata appears in diagnostics.

### Phase 3: Last-Good Layout Cache

Persist validated layout JSON plus metadata as the last good layout. Boot from
it before any network fetch.

Acceptance:

- Bad external layout cannot brick the UI.
- Device boots with previous layout offline.

### Phase 4: Dirty Overlay For Widget Overrides

Store local widget changes as a small overlay instead of rewriting the full
layout.

Acceptance:

- Runtime widget tuning survives reboot.
- Overrides can be cleared independently.

### Phase 5: External Sync Protocol

Implement fetch/push for the layout domain with version tokens if the server
supports them.

Acceptance:

- External changes apply only after validation.
- Local dirty state is preserved on push failure.
- Conflicts are visible, not silent.

### Phase 6: Extend To Optional Domains

Consider syncing UI state and Signal K target only after layout sync is stable.
Keep WiFi credentials local-only unless a secure export/import design exists.

## Non-Goals

- Do not sync WiFi passwords by default.
- Do not block boot waiting for external config.
- Do not apply unvalidated external layout directly to LVGL.
- Do not implement a general cloud account system in firmware.
- Do not make every runtime status value persistent.
- Do not silently overwrite dirty local changes with remote config.

## Verification

Minimum checks after implementation steps:

```sh
pio run -e esp32-4848s040
pio test -e native
```

Hardware checks:

- Boot offline uses last good local config.
- Bad external config is rejected and logged.
- Local widget/theme changes survive reboot.
- External layout fetch does not block touch/rendering.
- Dirty/conflict status is visible through API or logs.

RAM-first config checks:

1. Changing brightness updates the panel immediately with no storage read in the
   render path.
2. Repeated brightness bucket taps coalesce into one persisted UI-domain write
   after debounce or settings close.
3. Changing an alarm threshold affects `alarm_check` immediately before any
   NVS write completes.
4. Simulated NVS write failure leaves RAM/live state active and reports
   `persist_error`.
5. External layout fetch applies only after validation and then schedules
   last-good persistence.
6. Reboot after failed external fetch uses the previous persisted last-good
   config.
7. `/api/config/status` reports applied revision, persisted revision,
   dirty/pending/error flags, and last write duration per domain.
8. Native tests cover mutation validation, dirty state, debounce/coalescing,
   stale persist job suppression, and source precedence.
