const assert = require('assert')
const { makeManager } = require('./test-utils')
const { MockFirmware } = require('./mock-firmware')

const { manager, auth } = makeManager({
  auth: { mode: 'dev-shared-token', devToken: 'test-token' },
  network: { domain: 'local', hostnamePrefix: 'espdisp', namingPolicy: 'device-id' }
})

manager.upsertProfile({
  id: 'responsive',
  name: 'Responsive Layout',
  version: 3,
  config: {
    layout: {
      version: 1,
      variants: [
        {
          id: 'square-480',
          match: { display: { width: 480, height: 480 } },
          screens: [{ id: 'dashboard', type: 'grid', tiles: [{ widget: 'sog' }, { widget: 'mapPreview' }] }]
        },
        {
          id: 'wide-800x480',
          match: { display: { width: 800, height: 480 } },
          screens: [{ id: 'dashboard', type: 'grid', tiles: [{ widget: 'sog' }, { widget: 'heading' }] }]
        },
        {
          id: 'small-320x240',
          match: { display: { width: 320, height: 240 } },
          screens: [{ id: 'dashboard', type: 'grid', tiles: [{ widget: 'sog' }] }]
        }
      ]
    },
    widgets: {
      defaults: {
        fontSize: 18,
        labelFontSize: 12,
        valueFontSize: 35,
        unitFontSize: 14
      },
      variants: [
        {
          id: 'square-480',
          match: { display: { width: 480, height: 480 } },
          defaults: { valueFontSize: 42 }
        },
        {
          id: 'wide-800x480',
          match: { display: { width: 800, height: 480 } },
          defaults: { valueFontSize: 48 }
        },
        {
          id: 'small-320x240',
          match: { display: { width: 320, height: 240 } },
          defaults: { valueFontSize: 24 }
        }
      ],
      items: {
        sog: {
          type: 'numeric',
          title: 'SOG',
          path: 'navigation.speedOverGround',
          valueFontSize: 50
        },
        heading: {
          type: 'numeric',
          title: 'HDG',
          path: 'navigation.headingTrue'
        },
        mapPreview: {
          type: 'map',
          title: 'MAP',
          path: 'navigation.position'
        }
      }
    }
  }
})

const square = new MockFirmware(manager, {
  deviceId: 'espdisp-square',
  auth,
  display: { width: 480, height: 480, shape: 'square', rotation: 0, colorDepth: 16 }
})
square.register()
manager.assignProfile(square.deviceId, { profileId: 'responsive' })
const squareConfig = square.fetchConfig()
assert.strictEqual(squareConfig.display.selectedVariant, 'square-480')
assert.strictEqual(squareConfig.layout.variant, 'square-480')
assert.strictEqual(squareConfig.layout.screens[0].tiles.length, 1)
assert.strictEqual(squareConfig.widgets.variant, 'square-480')
assert.strictEqual(squareConfig.widgets.defaults.valueFontSize, 42)
assert.strictEqual(squareConfig.widgets.items.sog.valueFontSize, 48)
assert.strictEqual(squareConfig.widgets.items.mapPreview, undefined)

const wide = new MockFirmware(manager, {
  deviceId: 'espdisp-wide',
  auth,
  display: { width: 800, height: 480, shape: 'wide', rotation: 0, colorDepth: 16 }
})
wide.register()
manager.assignProfile(wide.deviceId, { profileId: 'responsive' })
const wideConfig = wide.fetchConfig()
assert.strictEqual(wideConfig.display.selectedVariant, 'wide-800x480')
assert.strictEqual(wideConfig.layout.screens[0].tiles.length, 2)
assert.strictEqual(wideConfig.widgets.defaults.valueFontSize, 48)

const small = new MockFirmware(manager, {
  deviceId: 'espdisp-small',
  auth,
  display: { width: 320, height: 240, shape: 'wide', rotation: 0, colorDepth: 16 }
})
small.register()
manager.assignProfile(small.deviceId, { profileId: 'responsive' })
const smallConfig = small.fetchConfig()
assert.strictEqual(smallConfig.display.selectedVariant, 'small-320x240')
assert.strictEqual(smallConfig.widgets.defaults.valueFontSize, 24)

manager.upsertProfile({
  id: 'wide-auto',
  name: 'Wide Auto Match',
  priority: 50,
  match: {
    display: { width: 800, height: 480 }
  },
  config: {
    settings: { defaultScreen: 'dashboard', theme: 'day' },
    layout: {
      version: 1,
      variants: [
        {
          id: 'wide-800x480',
          match: { display: { width: 800, height: 480 } },
          screens: [{ id: 'dashboard', type: 'grid', tiles: [] }]
        }
      ]
    },
    widgets: { defaults: { valueFontSize: 48 }, items: {} }
  }
})

const autoWide = new MockFirmware(manager, {
  deviceId: 'espdisp-wide-auto',
  auth,
  display: { width: 800, height: 480, shape: 'wide', rotation: 0, colorDepth: 16 }
})
const autoReg = autoWide.register()
assert.strictEqual(autoReg.assignedProfile, 'wide-auto')

const capabilities = manager.pluginCapabilities()
assert.ok(capabilities.widgets.types.includes('numeric'))
assert.ok(capabilities.fonts.properties.includes('valueFontSize'))
