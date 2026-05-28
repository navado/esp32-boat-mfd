# SignalK Autopilot Emulator Test Support

Status: implemented on the local SignalK test server.

## Goal

Provide a queryable autopilot endpoint for firmware testing without requiring a
physical autopilot controller on NMEA 2000 or Seatalk.

The test server should let us verify:

- autopilot state commands are received
- the resulting autopilot state can be queried through SignalK REST
- heading adjustment commands change a queryable target value

## Implementation

Use the official SignalK plugin:

```text
@signalk/signalk-autopilot
```

Configure the plugin with its emulator backend:

```json
{
  "enabled": true,
  "configuration": {
    "type": "emulator",
    "enableV2API": true
  }
}
```

The local Docker test server stores this at:

```text
/home/node/.signalk/plugin-config-data/autopilot.json
```

inside the `signalk-server` container. The repo source is:

```text
signalk/config/plugin-config-data/autopilot.json
```

## Query Paths

Use these REST reads to verify received commands:

```text
GET /signalk/v1/api/vessels/self/steering/autopilot/state/value
GET /signalk/v1/api/vessels/self/steering/autopilot/target/headingMagnetic/value
```

Known local verification:

```text
state before command: "standby"
PUT state auto:       200
state after command:  "auto"
PUT adjustHeading 10: 200
target heading:       0.17453292519943295
```

The target heading value above is radians in the SignalK API.

## Authenticated Command Test

SignalK rejects unauthenticated writes when security is enabled. Login first
and pass the returned token:

```sh
TOKEN=$(curl -s -H 'Content-Type: application/json' \
  -d '{"username":"admin","password":"admin"}' \
  http://localhost:3000/signalk/v1/auth/login | jq -r .token)

curl -s -X PUT \
  -H "Authorization: Bearer $TOKEN" \
  -H 'Content-Type: application/json' \
  -d '{"value":"auto"}' \
  http://localhost:3000/signalk/v1/api/vessels/self/steering/autopilot/state

curl -s \
  http://localhost:3000/signalk/v1/api/vessels/self/steering/autopilot/state/value
```

## Firmware Compatibility Notes

The current firmware subscribes to:

```text
steering.autopilot.target.headingTrue
steering.autopilot.state
```

and sends commands to:

```text
steering/autopilot/target/headingTrue
steering/autopilot/state
```

The emulator accepts and exposes autopilot state, so it is suitable for testing
engage/standby command delivery.

The emulator does not accept a writable `target.headingTrue` command. Heading
changes should be tested through:

```text
PUT steering/autopilot/actions/adjustHeading
GET steering/autopilot/target/headingMagnetic/value
```

or the firmware test mode should add an emulator-compatible command path.
