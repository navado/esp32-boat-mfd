const assert = require('assert')
const { makeManager } = require('./test-utils')
const { MockFirmware } = require('./mock-firmware')

const { manager, auth } = makeManager({
  auth: { mode: 'dev-shared-token', devToken: 'test-token' },
  network: { domain: 'local', hostnamePrefix: 'espdisp', namingPolicy: 'device-id' }
})

const helm = new MockFirmware(manager, {
  deviceId: 'espdisp-dashboard-helm',
  auth,
  display: { width: 480, height: 480, shape: 'square', rotation: 0, colorDepth: 16 }
})
helm.register()
helm.fetchConfig()
helm.heartbeat()

const cockpit = new MockFirmware(manager, {
  deviceId: 'espdisp-dashboard-cockpit',
  auth,
  display: { width: 800, height: 480, shape: 'wide', rotation: 0, colorDepth: 16 }
})
cockpit.register()
cockpit.fetchConfig()

manager.createCommand(helm.deviceId, {
  type: 'screen.set',
  payload: { screen: 'dashboard' }
})

const dashboard = manager.dashboard()
assert.strictEqual(dashboard.protocol, 'espdisp.management.v1')
assert.strictEqual(dashboard.counts.devices, 2)
assert.strictEqual(dashboard.counts.online, 2)
assert.strictEqual(dashboard.counts.pendingCommands, 1)
assert.strictEqual(dashboard.devices.length, 2)
assert.ok(dashboard.devices.find((device) => device.id === helm.deviceId))
assert.ok(dashboard.devices.find((device) => device.id === cockpit.deviceId))
assert.ok(dashboard.groups.find((group) => group.id === 'all'))
assert.strictEqual(dashboard.recentCommands[0].type, 'screen.set')
