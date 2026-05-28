const assert = require('assert')
const path = require('path')
const { readJson } = require('../lib/store')

const config = readJson(
  path.join(__dirname, '..', '..', '..', 'config', 'plugin-config-data', 'signalk-app-dock.json'),
  null
)

assert.ok(config)
const apps = config.configuration && config.configuration.apps
assert.ok(Array.isArray(apps))
const tile = apps.find((app) => app.url === '/signalk-espdisp-manager/')
assert.ok(tile)
assert.strictEqual(tile.enabled, true)
assert.strictEqual(tile.label, 'ESP Displays')
assert.strictEqual(tile.icon, '/signalk-espdisp-manager/icon.svg')
