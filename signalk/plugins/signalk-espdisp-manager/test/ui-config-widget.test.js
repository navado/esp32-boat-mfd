const assert = require('assert')
const fs = require('fs')
const path = require('path')

const source = fs.readFileSync(path.join(__dirname, '..', 'index.js'), 'utf8')

assert.ok(source.includes('function renderDeviceConfigWidget'), 'device config widget renderer is missing')
assert.ok(source.includes('renderWidgetsTable(config.widgets)'), 'widget summary table is not rendered')
assert.ok(source.includes('renderScreensTable(config.layout)'), 'screen layout table is not rendered')
assert.ok(source.includes('SignalK and NMEA'), 'data source section is not rendered')
assert.ok(!source.includes('JSON.stringify(config'), 'device config UI must not render raw JSON')
assert.ok(!source.includes('Generated config preview'), 'device page must not label a raw config preview')
