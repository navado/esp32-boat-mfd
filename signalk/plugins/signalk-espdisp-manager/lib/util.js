const crypto = require('crypto')

function now () {
  return new Date().toISOString()
}

function randomId (prefix) {
  return `${prefix}-${crypto.randomBytes(8).toString('hex')}`
}

function sha256Json (value) {
  const body = JSON.stringify(stable(value))
  return `sha256:${crypto.createHash('sha256').update(body).digest('hex')}`
}

function stable (value) {
  if (Array.isArray(value)) return value.map(stable)
  if (!value || typeof value !== 'object') return value
  return Object.keys(value).sort().reduce((acc, key) => {
    acc[key] = stable(value[key])
    return acc
  }, {})
}

function clone (value) {
  return JSON.parse(JSON.stringify(value))
}

function sanitizeDeviceId (id) {
  return String(id || '').trim().toLowerCase().replace(/[^a-z0-9_.-]/g, '-')
}

function hostnameFromPolicy (device, network) {
  const policy = network.namingPolicy || 'device-id'
  const prefix = network.hostnamePrefix || 'espdisp'

  if (device.networkIdentity && device.networkIdentity.desiredHostname) {
    return cleanHostname(device.networkIdentity.desiredHostname)
  }

  if (policy === 'manual' && device.hostname) {
    return cleanHostname(device.hostname)
  }

  if (policy === 'role-location') {
    const bits = [device.location, device.role || 'display'].filter(Boolean)
    if (bits.length > 0) return cleanHostname(bits.join('-'))
  }

  if (device.id && device.id.startsWith(`${prefix}-`)) return cleanHostname(device.id)
  return cleanHostname(`${prefix}-${device.id || 'device'}`)
}

function cleanHostname (value) {
  return String(value || 'espdisp')
    .trim()
    .toLowerCase()
    .replace(/[^a-z0-9-]/g, '-')
    .replace(/^-+|-+$/g, '')
    .slice(0, 63) || 'espdisp'
}

function mergeDeep (base, override) {
  const out = clone(base || {})
  if (!override || typeof override !== 'object') return out
  Object.keys(override).forEach((key) => {
    const ov = override[key]
    if (ov && typeof ov === 'object' && !Array.isArray(ov)) {
      out[key] = mergeDeep(out[key] || {}, ov)
    } else {
      out[key] = ov
    }
  })
  return out
}

module.exports = {
  now,
  randomId,
  sha256Json,
  clone,
  sanitizeDeviceId,
  hostnameFromPolicy,
  mergeDeep
}
