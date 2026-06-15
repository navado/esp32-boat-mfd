const assert = require('assert')
const { makeManager } = require('./test-utils')

// Phase F1: projection helpers backing GET /devices/summary and
// GET /devices/:id/views (the Waveshare knob's remote enumeration + switch).

const { manager, auth } = makeManager({
  auth: { mode: 'dev-shared-token', devToken: 'test-token' }
})

const helmId = 'espdisp-aaaaaaaaaaaa'
const cockpitId = 'espdisp-bbbbbbbbbbbb'

manager.registerDevice({
  device: {
    id: helmId,
    name: 'Helm Display',
    role: 'display',
    board: 'sunton_4848s040',
    display: { width: 480, height: 480, shape: 'square' }
  }
}, auth)

manager.registerDevice({
  device: {
    id: cockpitId,
    name: 'Cockpit Display',
    role: 'display',
    board: 'sunton_4848s040'
  }
}, auth)

// --- deviceSummaries(): shape + online + currentScreen ---------------------

const summary = manager.deviceSummaries()
assert.ok(Array.isArray(summary.devices), 'summaries.devices is an array')
assert.strictEqual(summary.devices.length, 2)

const helmSummary = summary.devices.find((d) => d.id === helmId)
assert.ok(helmSummary, 'helm appears in summary')
assert.deepStrictEqual(Object.keys(helmSummary).sort(),
  ['currentScreen', 'id', 'name', 'online', 'role'])
assert.strictEqual(helmSummary.name, 'Helm Display')
assert.strictEqual(helmSummary.role, 'display')
// Freshly registered devices set lastSeen = now, so they read as online.
assert.strictEqual(helmSummary.online, true)
// No heartbeat yet -> no current screen.
assert.strictEqual(helmSummary.currentScreen, null)

// A heartbeat carrying ui.screen surfaces as currentScreen.
manager.updateStatus(helmId, {
  time: new Date().toISOString(),
  network: { mode: 'sta', ip: '192.168.1.10' },
  ui: { screen: 'wind', theme: 'day' }
}, auth)
const afterHb = manager.deviceSummaries().devices.find((d) => d.id === helmId)
assert.strictEqual(afterHb.currentScreen, 'wind')

// Stale device (lastSeen far in the past) reads as offline.
manager.store.registry.devices[cockpitId].lastSeen =
  new Date(Date.now() - 24 * 3600 * 1000).toISOString()
const cockpitSummary = manager.deviceSummaries().devices.find((d) => d.id === cockpitId)
assert.strictEqual(cockpitSummary.online, false)

// Summaries are sorted by id for stable menu ordering.
const ids = manager.deviceSummaries().devices.map((d) => d.id)
assert.deepStrictEqual(ids, [...ids].sort())

// --- deviceViews(): shape + current + fallback -----------------------------

const helmViews = manager.deviceViews(helmId)
assert.ok(Array.isArray(helmViews.views), 'views is an array')
assert.ok(helmViews.views.length > 0, 'helm has at least one view')
helmViews.views.forEach((v) => {
  assert.deepStrictEqual(Object.keys(v).sort(), ['id', 'title'])
  assert.strictEqual(typeof v.id, 'string')
  assert.strictEqual(typeof v.title, 'string')
})
// current reflects the last reported screen.
assert.strictEqual(helmViews.current, 'wind')

// Fallback: a device whose resolved layout has no screens still yields the
// standard known view ids (so the knob menu is never empty).
const fallbackViews = manager.deviceViews(cockpitId)
assert.ok(fallbackViews.views.length > 0, 'fallback views are non-empty')
const fallbackIds = fallbackViews.views.map((v) => v.id)
assert.ok(fallbackIds.includes('dashboard'), 'fallback includes dashboard')
assert.ok(fallbackIds.includes('autopilot'), 'fallback includes autopilot')

// Unknown device id is a 404 (httpError), consistent with getDevice().
assert.throws(() => manager.deviceViews('espdisp-does-not-exist'), /device not found/)

// --- screen.set command (backs POST /ui/devices/:id/switch-screen) ---------
// The new per-screen switcher queues a screen.set command the firmware maps to
// show_by_id on its next poll. Verify the command is well-formed + queued.
const screenCmd = manager.createCommand(helmId, { type: 'screen.set', payload: { screen: 'depth' } })
assert.strictEqual(screenCmd.type, 'screen.set', 'command type is screen.set')
assert.strictEqual(screenCmd.payload.screen, 'depth', 'payload carries target screen id')
const queuedScreenSet = manager.store.commands.commands.some(
  (c) => c.id === screenCmd.id && c.type === 'screen.set' && c.payload.screen === 'depth')
assert.ok(queuedScreenSet, 'screen.set command is queued for the device to poll')

// --- deviceViews prefers the device-reported screen list -------------------
// When the firmware reports ui.screens in its heartbeat, deviceViews uses that
// real list (authoritative) instead of the generated config — so the switcher
// offers screen ids the device actually has.
manager.updateStatus(helmId, {
  time: new Date().toISOString(),
  network: { mode: 'sta', ip: '192.168.1.10' },
  ui: {
    screen: 'wind_classic',
    screens: [
      { id: 'wind_classic', title: 'Wind (classic)' },
      { id: 'dashboard', title: 'Dashboard' },
      { id: 'depth', title: 'Depth' }
    ]
  }
}, auth)
const reportedViews = manager.deviceViews(helmId)
assert.deepStrictEqual(reportedViews.views.map((v) => v.id), ['wind_classic', 'dashboard', 'depth'],
  'deviceViews uses the device-reported screen list verbatim')
assert.strictEqual(reportedViews.views.find((v) => v.id === 'wind_classic').title, 'Wind (classic)',
  'reported titles are preserved')
assert.strictEqual(reportedViews.current, 'wind_classic', 'current reflects the reported screen')

console.log('device-projections test passed')
