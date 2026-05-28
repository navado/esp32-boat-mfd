const fs = require('fs')
const os = require('os')
const path = require('path')
const { EspDispManager } = require('../lib/manager')

function makeManager (options) {
  const dataDir = fs.mkdtempSync(path.join(os.tmpdir(), 'espdisp-manager-'))
  const app = {
    getDataDirPath: () => dataDir,
    debug: () => {}
  }
  return {
    dataDir,
    app,
    manager: new EspDispManager(app, options || {}),
    auth: { bearer: options && options.auth && options.auth.devToken ? options.auth.devToken : 'espdisp-dev' }
  }
}

module.exports = { makeManager }
