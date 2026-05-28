const assert = require('assert')
const { makeManager } = require('./test-utils')

const { manager, auth } = makeManager({
  auth: { mode: 'dev-shared-token', devToken: 'test-token' },
  network: { domain: 'local', hostnamePrefix: 'espdisp', namingPolicy: 'device-id' }
})

const reg = manager.registerDevice({
  device: {
    id: 'espdisp-aabbccddeeff',
    name: 'Bench Display',
    board: 'esp32-4848s040',
    mac: 'AA:BB:CC:DD:EE:FF',
    firmware: { version: '0.4.0' },
    capabilities: { touch: true, ota: true }
  }
}, auth)

assert.strictEqual(reg.status, 'registered')
assert.strictEqual(reg.deviceId, 'espdisp-aabbccddeeff')

const status = manager.updateStatus('espdisp-aabbccddeeff', {
  network: {
    hostname: 'espdisp-aabbccddeeff',
    domain: 'local',
    fqdn: 'espdisp-aabbccddeeff.local',
    ip: '192.168.1.42'
  },
  config: { hash: 'old' }
}, auth)

assert.strictEqual(status.status, 'ok')
assert.strictEqual(status.desiredConfig.reload, true)

const config = manager.generateConfig('espdisp-aabbccddeeff')
assert.strictEqual(config.protocol, 'espdisp.management.v1')
assert.strictEqual(config.network.fqdn, 'espdisp-aabbccddeeff.local')
assert.ok(config.hash.startsWith('sha256:'))

const command = manager.createCommand('espdisp-aabbccddeeff', {
  type: 'screen.set',
  payload: { screen: 'dashboard' }
})
assert.strictEqual(command.status, 'pending')

const commands = manager.getCommands('espdisp-aabbccddeeff', auth, 10)
assert.strictEqual(commands.commands.length, 1)
assert.strictEqual(commands.commands[0].status, 'delivered')

const ack = manager.ackCommand('espdisp-aabbccddeeff', command.id, {
  result: { ok: true, message: 'done' }
}, auth)
assert.strictEqual(ack.status, 'acknowledged')

const artifact = manager.addFirmwareArtifact({
  vendor: { id: 'navado', name: 'Navado' },
  product: { id: 'espdisp', name: 'ESP Display' },
  firmware: { version: '0.4.1' },
  compatibility: { boards: ['esp32-4848s040'] },
  file: { name: 'espdisp.bin', size: 123, sha256: 'sha256:test' }
})
assert.ok(artifact.artifactId)

const job = manager.createFirmwareJob('espdisp-aabbccddeeff', {
  artifactId: artifact.artifactId
})
assert.strictEqual(job.status, 'queued')
