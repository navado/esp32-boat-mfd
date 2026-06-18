# Manager Device Console — Slice 2 (System Settings) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: superpowers:subagent-driven-development.

**Goal:** A System Settings page in the manager holding the global provisioning defaults — network/WiFi creds, OTA password (masked secret), and a device-numbering scheme with auto-increment — persisted in the manager store and editable from the UI.

**Architecture:** Add a `settings` section to the manager's JSON store (`lib/store.js`), manager methods to read/update it + allocate the next device number (`lib/manager.js`), and a `/ui/settings` page + POST route + nav entry (`index.js`). Secrets (WiFi password, OTA password) are masked in the UI and write-only (empty submit = keep existing).

**Tech Stack:** Node 18, JSON-file store, `node test/run.js`.

**Spec:** `docs/superpowers/specs/2026-06-18-manager-device-console-design.md` §2/§6 (numbering).

**Repo:** `/Users/borissorochkin/code/embedded/signalk-espdisp-manager` (`main`).

---

## File Structure
- **Modify** `lib/store.js`: add a `settings` store section (`settingsFile`, `init` read, `saveSettings()`, `defaultSettings()`).
- **Modify** `lib/manager.js`: `getSettings()`, `getSettingsMasked()`, `updateSettings(patch)`, `allocateDeviceNumber()`.
- **Modify** `index.js`: `/ui/settings` GET+POST routes; add `Settings` to `nav()`; a `renderSettingsPage()`.
- **Create** `test/settings.test.js`; register in `test/run.js`.

---

## Task 1: Store + manager settings model (+ auto-numbering)

**Files:** Modify `lib/store.js`, `lib/manager.js`; Create `test/settings.test.js`; Modify `test/run.js`.

- [ ] **Step 1 — read** `lib/store.js` (the `init()` section list + `saveX()` methods + how `this.dataDir`/`*File` paths are set) and `lib/manager.js` (constructor `this.store.init()`, a method that uses `this.store.saveRegistry()` for the pattern).

- [ ] **Step 2 — failing test** `test/settings.test.js`:

```js
const assert = require('assert')
const { makeManager } = require('./test-utils')
const { manager } = makeManager({
  auth: { mode: 'dev-shared-token', devToken: 't' },
  network: { domain: 'local', hostnamePrefix: 'espdisp', namingPolicy: 'device-id' }
})

// defaults
const s0 = manager.getSettings()
assert.equal(typeof s0.network, 'object')
assert.equal(typeof s0.ota, 'object')
assert.equal(s0.numbering.prefix, 'espdisp-')
assert.equal(s0.numbering.next, 1)

// update: set ssid + passwords + numbering
manager.updateSettings({ network: { ssid: 'BoatNet', password: 'sekret' }, ota: { password: 'otapw' }, numbering: { prefix: 'disp-', pad: 2 } })
const s1 = manager.getSettings()
assert.equal(s1.network.ssid, 'BoatNet')
assert.equal(s1.network.password, 'sekret')
assert.equal(s1.ota.password, 'otapw')
assert.equal(s1.numbering.prefix, 'disp-')

// masked view hides secrets but signals they are set
const m = manager.getSettingsMasked()
assert.equal(m.network.password, '')
assert.equal(m.network.passwordSet, true)
assert.equal(m.ota.password, '')
assert.equal(m.ota.passwordSet, true)
assert.equal(m.network.ssid, 'BoatNet')

// empty-password update KEEPS the existing secret
manager.updateSettings({ network: { ssid: 'BoatNet2', password: '' } })
assert.equal(manager.getSettings().network.password, 'sekret', 'empty password keeps existing')
assert.equal(manager.getSettings().network.ssid, 'BoatNet2')

// auto-numbering increments + formats with padding
assert.equal(manager.allocateDeviceNumber(), 'disp-01')
assert.equal(manager.allocateDeviceNumber(), 'disp-02')
assert.equal(manager.getSettings().numbering.next, 3)

console.log('settings.test: OK')
```

Register `require('./settings.test')` in `test/run.js` after `display-widgets`.
Run `node test/settings.test.js` → FAIL (`getSettings` not a function).

- [ ] **Step 3 — implement store** in `lib/store.js`:
  - Add a settings file path alongside the others (e.g. in the constructor/init where `registryFile` etc. are set): `this.settingsFile = path.join(this.dataDir, 'settings.json')` (match how the other `*File` are built).
  - In `init()`: `this.settings = readJson(this.settingsFile, defaultSettings())`.
  - Add `saveSettings () { writeJson(this.settingsFile, this.settings) }`.
  - Add a `defaultSettings()` near `defaultProfiles()`:
    ```js
    function defaultSettings () {
      return {
        network: { ssid: '', password: '', mdnsDomain: 'local' },
        ota: { password: '' },
        numbering: { prefix: 'espdisp-', next: 1, pad: 3 }
      }
    }
    ```

- [ ] **Step 4 — implement manager methods** in `lib/manager.js`:
  ```js
  getSettings () { return this.store.settings }

  getSettingsMasked () {
    const s = this.store.settings
    return {
      network: { ssid: s.network.ssid || '', mdnsDomain: s.network.mdnsDomain || 'local', password: '', passwordSet: Boolean(s.network.password) },
      ota: { password: '', passwordSet: Boolean(s.ota.password) },
      numbering: { ...s.numbering }
    }
  }

  updateSettings (patch) {
    const s = this.store.settings
    if (patch.network) {
      if (typeof patch.network.ssid === 'string') s.network.ssid = patch.network.ssid
      if (typeof patch.network.mdnsDomain === 'string') s.network.mdnsDomain = patch.network.mdnsDomain
      if (patch.network.password) s.network.password = patch.network.password // empty = keep
    }
    if (patch.ota && patch.ota.password) s.ota.password = patch.ota.password // empty = keep
    if (patch.numbering) {
      if (typeof patch.numbering.prefix === 'string') s.numbering.prefix = patch.numbering.prefix
      if (Number.isInteger(patch.numbering.pad)) s.numbering.pad = patch.numbering.pad
      if (Number.isInteger(patch.numbering.next)) s.numbering.next = patch.numbering.next
    }
    this.store.saveSettings()
    this.store.audit('settings.updated', 'system')
    return this.getSettingsMasked()
  }

  allocateDeviceNumber () {
    const n = this.store.settings.numbering
    const num = n.next
    n.next = num + 1
    this.store.saveSettings()
    return `${n.prefix}${String(num).padStart(n.pad || 0, '0')}`
  }
  ```

- [ ] **Step 5 — green:** `node test/settings.test.js` → `settings.test: OK`. `node test/run.js` → no new failures (known env failures only).

- [ ] **Step 6 — commit:** `git commit -am "feat(settings): store + manager model for provisioning defaults + auto-numbering"`

---

## Task 2: System Settings UI page + route + nav

**Files:** Modify `index.js`.

- [ ] **Step 1 — add nav entry.** In `nav()` `items`, add `['settings', '/plugins/espdisp-manager/ui/settings', 'Settings']` after the Firmware entry → nav is now Devices, Firmware, Settings.

- [ ] **Step 2 — render + routes.** Add a `renderSettingsPage(manager, req)` that builds a `<form class="config-form" method="post" action="/plugins/espdisp-manager/ui/settings">` with fields from `manager.getSettingsMasked()`:
  - Network: `ssid` (text), `network_password` (password, placeholder `"(unchanged)"` if `network.passwordSet`), `mdnsDomain` (text).
  - OTA: `ota_password` (password, placeholder `"(unchanged)"` if `ota.passwordSet`).
  - Numbering: `prefix` (text), `pad` (number), `next` (number) + a preview of the next name.
  - A "Saved" banner when `req.query.saved`.
  Add the routes:
  ```js
  router.get('/ui/settings', wrap(getManager, (manager, req, res) => {
    res.setHeader('content-type', 'text/html')
    res.end(renderUiShell('Settings', renderSettingsPage(manager, req), manager.dashboard(), 'settings'))
  }))
  router.post('/ui/settings', wrap(getManager, (manager, req, res) => {
    const b = req.body || {}
    manager.updateSettings({
      network: { ssid: b.ssid, mdnsDomain: b.mdnsDomain, password: b.network_password || '' },
      ota: { password: b.ota_password || '' },
      numbering: { prefix: b.prefix, pad: b.pad != null ? parseInt(b.pad, 10) : undefined, next: b.next != null ? parseInt(b.next, 10) : undefined }
    })
    res.statusCode = 303
    res.setHeader('location', '/plugins/espdisp-manager/ui/settings?saved=1')
    res.end()
  }))
  ```
  (Match how other routes parse `req.body` — confirm the body parser used by the other POST routes, e.g. `register-from-signalk`, and mirror it. If routes read `req.body` directly it's already parsed; if they parse a raw body, follow that.)

- [ ] **Step 2b — title map.** Add `settings: 'Settings'` to the `renderUi` title lookup map (the one near line 1148 where `firmware`/`devices` titles live) so the page title renders.

- [ ] **Step 3 — sanity:** `node -e "require('./index.js'); console.log('ok')"` → ok. `node test/run.js` → no new failures.

- [ ] **Step 4 — extend the test.** Append to `test/settings.test.js`:
  ```js
  const idx = require('../index.js')
  const html = idx.__renderSettingsPage ? idx.__renderSettingsPage(manager, { query: {} }) : null
  if (html) {
    assert.ok(/name="ssid"/.test(html), 'settings form has ssid field')
    assert.ok(/name="ota_password"/.test(html), 'settings form has ota password field')
    assert.ok(/\(unchanged\)/.test(html), 'masked password shows (unchanged) placeholder')
    console.log('settings.test: page OK')
  }
  ```
  Export it: `module.exports.__renderSettingsPage = renderSettingsPage`. Run `node test/settings.test.js` → page OK.

- [ ] **Step 5 — commit:** `git commit -am "feat(settings): System Settings page + routes + nav entry"`

---

## Task 3: Deploy + verify

- [ ] Push; rsync to mythra-nav (exclude node_modules/test-results/.git/.gitignore/deploy/*.bak); restart signalk-server; verify `/ui/settings` renders the form, nav shows Settings, saving persists (round-trip), passwords show "(unchanged)" and submitting blank keeps them.

---

## Self-Review
- **Spec §2 coverage:** network/WiFi creds (Task 1/2), OTA password masked secret (getSettingsMasked + form), device numbering + auto-increment (allocateDeviceNumber), UI page + nav (Task 2). ✓
- **Secrets:** WiFi + OTA passwords masked in `getSettingsMasked`, write-only (empty keeps existing). ✓
- **No placeholders:** full code for store, manager methods, routes, tests.
- **Consistency:** `getSettings`/`getSettingsMasked`/`updateSettings`/`allocateDeviceNumber` names used consistently; `numbering.{prefix,next,pad}` shape consistent across default, update, allocate.
