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

