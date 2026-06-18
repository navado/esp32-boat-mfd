# Manager device console: merged UI, system settings, USB/OTA flashing & provisioning

**Status:** Design · **Date:** 2026-06-18 · **Author:** navado and contributors
**Scope:** Two repos — `Instruments-manager` (SignalK manager plugin) and the
ESP32 firmware (`esp32-boat-mfd`). One spec, implemented in ordered slices.

## Problem

The SignalK manager today is a registry + config/firmware-job console, but a
fresh device still has to be flashed and provisioned out-of-band (USB + serial
console, compile-time secrets). Operators want a single console that:

1. Presents one merged **Devices** page (not split Overview/Devices) with working
   bulk actions, and stops silently failing when the SignalK login expires.
2. Flashes a USB-cabled device from the browser (ESP Web Tools) and provisions it
   so it joins the network and becomes controllable + OTA-updatable.
3. Holds **system settings** (network/WiFi, OTA password, device numbering) that
   are pushed to devices **from the server**, and lets each device be re-flashed
   via **serial or OTA** with connection validation.

This requires firmware changes too: the OTA password is currently compile-time
(`#define OTA_PASSWORD` in `secrets.h`) and there is no Improv-Serial
provisioning. Those become runtime/standard so the manager can drive them.

## Goals

1. **Merged Devices page** that is the real device console (stats + pending
   discovery + registered table + working Clear-offline/Clear-all), with
   `Presets` and `Layout editor` removed from the nav.
2. **Relogin-on-expiry**: every manager action detects `401`/login-redirect and
   prompts re-login instead of no-op.
3. **System Settings**: manager-stored global defaults (WiFi creds, OTA password,
   device-numbering scheme, network identity) applied per device, server-pushed.
4. **USB flashing** via embedded ESP Web Tools (esptool-js / WebSerial), gated to
   secure contexts, fed by a manager-generated firmware manifest + merged image.
5. **Server-sourced provisioning**: the client relays only the *server's* WiFi
   creds over the cable; the **server** pushes OTA password + number + config and
   runs OTA — because the cabled client may be on a different network.
6. **Per-device Update Firmware**: pick firmware + method (serial / OTA), validate
   the connection, then flash (serial in-browser; OTA as a server-side job).
7. **Firmware**: runtime OTA password (NVS), Improv-Serial provisioning,
   config-push of number/OTA-password/network, a merged factory-image build.

### Non-goals

- Replacing the device's existing BLE/captive-portal WiFi setup (kept as a
  fallback). Server-driven provisioning is the new primary path.
- Flashing a device cabled to the *lab server* from a remote browser (WebSerial
  is client-side; server-side esptool is out of scope — OTA covers networked
  re-flashing).
- Building firmware in the manager. Binaries come from the firmware catalog /
  artifacts (CI releases or upload).

## Architecture

```
  Browser (central computer; localhost / HTTPS = secure context)
    ├─ ESP Web Tools (esptool-js, WebSerial) ── flashes USB-cabled ESP
    │     └─ relays SERVER's WiFi creds over serial (network bootstrap only)
    ├─ Manager UI: merged Devices · System Settings · device Update-Firmware
    └─ fetch/POST ─► Manager plugin (SignalK)
                       ├─ GET firmware manifest.json + merged .bin (for web-tools)
                       ├─ System Settings store (network, OTA pw, numbering)
                       ├─ config-push: device number + OTA password + settings
                       └─ OTA job runner: espota from the central computer ─► ESP
  Firmware (new): OTA password in NVS (applied at OTA init) · Improv-Serial ·
                  config-push of number/ota-pw/network · merged-image build
```

**Repo split.** Manager: merged page, settings page+store, manifest endpoint,
web-tools embed, update-firmware modal + validation, OTA runner, relogin. Firmware:
runtime OTA password, Improv-Serial, config-push setters, merged-image target.

## §1 UI restructure (manager, slice 1)

- **Merge** `/ui` (Overview) and `/ui/devices` (Devices) into one page that is the
  landing route: stat tiles (Devices / Online / Drift / Pending / Firmware-jobs),
  then the pending-discovery section, then the registered-devices table with the
  working **Clear offline (N)** / **Clear all (N)** actions. Nav collapses
  "Overview"+"Devices" → a single **Devices** entry.
- **Remove `Presets` and `Layout editor` from the nav.** Keep their routes
  (`/ui/profiles`, `/ui/layout`) reachable directly (the layout editor is still
  used by the device live-view), just not as top-level nav.
- **Relogin-on-expiry.** Replace the bare server-rendered form POSTs (clear-all,
  apply, etc.) with a small `fetch()` wrapper that, on `401`/login-redirect,
  shows a **"Session expired — re-login"** modal linking to `/admin/#/login`
  (and returns the user to the action after). No action silently no-ops.

## §2 System Settings (manager, slice 2)

New **System Settings** page + a plugin-config/store section holding **global
provisioning defaults**:

- **Network:** WiFi SSID + password (the creds pushed to new devices), DHCP/static
  hint, mDNS hostname domain.
- **OTA:** the OTA password (manager-held; pushed to devices; used by the OTA
  runner).
- **Device numbering:** prefix + next-number with auto-increment (e.g.
  `espdisp-001`), so each provisioned device gets the next name.
- Optional identity defaults (theme, etc.).

Stored manager-side (the plugin's persistent store). The OTA password is treated
as a **secret** — stored in the manager's config (not in any moved/public repo),
masked in the UI, write-only edits. Applied to devices via §4/§5, never embedded
in the firmware image.

## §3 Firmware runtime config (firmware, slice 3)

- **Runtime OTA password.** Store an OTA password in NVS; settable via a `net`
  console command (e.g. `ota-pass <pw>`) and via the config-push channel; apply
  with `ArduinoOTA.setPassword(<nvs value>)`, falling back to the compile-time
  `OTA_PASSWORD` define when unset. Host-test the NVS get/set + "which password
  applies" selection logic (pure function over (nvsValue, compileDefault)).
- **Device number / name.** Reuse the existing `id <name>` console command (sets
  the NVS device id + reboots); expose the same setter to config-push.
- **Network creds.** Already persist in `wifi_store`; add a config-push path so the
  server can set/update them post-onboarding.

## §4 USB flashing + server-sourced provisioning (firmware slice 4 + manager slice 5)

- **Embed ESP Web Tools.** Add `<esp-web-install-button manifest=…>` (esp-web-tools
  loaded from a vendored/CDN script) on a **"Flash new device (USB)"** action on
  the merged Devices page. The manager serves a generated **`manifest.json`**
  (`GET /firmware/manifest/:artifactId`) describing chipFamily `ESP32-S3` and a
  single **merged factory image** flashed at `0x0`.
- **Merged image (firmware).** Add a PlatformIO build target that emits a single
  merged `.bin` (bootloader+partitions+boot_app0+app) so the manifest is one part
  at `0x0`. The manager catalog stores/serves it.
- **Secure-context gate.** If `navigator.serial` is undefined (plain-HTTP LAN
  access), render a clear notice — "open the manager via `http://localhost` (or
  HTTPS) on the computer the device is plugged into" — instead of a dead button.
- **Provisioning is server-sourced.** Only the cabled client can drive serial, but
  the client may be on a different network than the device's deployment target, so
  the client must not supply network/OTA config of its own:
  - **Network bootstrap (over serial):** the client fetches the **server's** WiFi
    creds from System Settings (via the manager API) and pushes *those* to the
    device over serial using **Improv-Serial** (firmware addition). The device
    joins the server's network. The client never uses its own SSID/password and
    never sets the OTA password over serial.
  - **Everything else (over network, from the server):** once the device joins and
    registers, the **server** auto-assigns the next device number and pushes the
    OTA password + remaining settings via **config-push**; OTA runs server-side.
- **Improv-Serial (firmware).** Integrate the Improv-WiFi-Serial protocol, gated so
  it coexists with the normal serial console; it accepts the SSID/password
  esp-web-tools sends (sourced from System Settings) and reports the resulting IP.

## §5 Per-device Update Firmware (manager, slice 6)

On the device detail page, an **Update Firmware** button opens a modal:

1. **Pick firmware** — from the manager catalog/artifacts (or upload).
2. **Pick method** — **Serial (USB / ESP Web Tools)** or **OTA**.
3. **Connection validation** (flash blocked until it passes, shown as a checklist):
   - *Serial:* esp-web-tools opens the port, identifies the chip (+ reads the
     running version where possible).
   - *OTA:* manager pre-flight — device online (`/api/state` 200), OTA enabled,
     address + OTA password known.
4. **Flash** — *Serial:* in-browser via esp-web-tools (secure-context gated).
   *OTA:* a **server-side job** running `espota` from the central computer with the
   System-Settings OTA password, tracked through the existing firmware-job
   progress/confirm mechanism with live progress.

## §6 Provisioning orchestration + auto-numbering (both, slice 7)

- On first registration after a USB flash, the manager detects an
  unprovisioned/new device, **assigns the next number** from the numbering scheme,
  and pushes number + OTA password + network/settings via config-push; the device
  applies them (NVS) and becomes controllable + OTA-ready.
- Idempotent: re-provisioning an existing device updates only changed fields;
  numbering never reassigns an already-numbered device.

## §7 Slices, ordering, testing

Ordered, each independently shippable:

1. **Manager UI restructure** — merge Devices+Overview, drop nav items, relogin.
2. **System Settings** — page + store + config schema (incl. masked OTA pw).
3. **Firmware: runtime OTA password** — NVS + setter + apply; native test.
4. **Firmware: Improv-Serial + merged-image build** — device-verified via web-tools.
5. **Manager: ESP Web Tools embed + manifest endpoint + Flash-new-device** —
   secure-context handling.
6. **Manager: per-device Update Firmware (serial/OTA + validation) + OTA runner.**
7. **Provisioning config-push + auto-numbering** — both repos; end-to-end.
8. **Deploy + lab end-to-end** verify (flash a bench device, watch it self-provision
   and appear OTA-ready).

**Testing.**
- Manager: `node test/run.js` extended (settings store, numbering, manifest JSON,
  validation-state logic, relogin handling) + Playwright for the merged page,
  settings form, update-firmware modal states.
- Firmware: native/Unity for the OTA-password selection + provisioning-field parse
  (pure); device verify for Improv flashing + merged image + OTA-with-runtime-pw.
- esp-web-tools flashing verified on the central computer (localhost); OTA via the
  existing `make ota-verify` path with the runtime password.

## Risks / mitigations

- **WebSerial secure-context.** Hard browser rule; mitigated by the
  central-computer/localhost model + an explicit in-UI notice. No silent dead UI.
- **OTA password as a secret.** Manager-held, masked, never in a repo or the
  firmware image; pushed over the device's authenticated config-push channel.
- **Provisioning chicken-and-egg.** The device needs *some* network to reach the
  server; solved by the serial network-bootstrap (server creds relayed by the
  client) before any server-side push.
- **Merged-image drift.** The manifest must match the chip/partition layout; the
  merged-image build target + a manifest fixture test keep them in lockstep.
- **Improv is interactive by default.** Stock `<esp-web-install-button>` prompts
  the operator for WiFi. To use the *server's* creds (not the client's network),
  the plan picks one: pre-fill/auto-submit the Improv step from a manager-fetched
  System-Settings payload, or drive Improv-Serial directly from a small custom
  WebSerial step after esptool flashing. Either way the creds originate server-side.
- **Two-repo coordination.** Slices are ordered so firmware capabilities (3,4) land
  before the manager features that depend on them (5,6,7); a shared contract
  (config-push fields, manifest shape) is fixtured on both sides.
