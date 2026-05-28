# SignalK NMEA 0183 WiFi Test Output

Status: implemented on the local SignalK test server.

## Goal

Expose the same live test data through both:

- SignalK HTTP/WebSocket on TCP `3000`
- NMEA 0183 over WiFi on TCP `10110`

This lets the ESP32 firmware keep using SignalK while other apps/instruments
can consume NMEA 0183 from the same server during testing.

## Implementation

Use the official SignalK plugin:

```text
@signalk/signalk-to-nmea0183
```

The plugin emits converted sentences on SignalK's internal `nmea0183out` event.
SignalK's built-in `nmea-tcp` interface publishes that event stream to TCP
port `10110`.

The current local container is recreated with:

```text
-p 3000:3000
-p 10110:10110
-v ./signalk/config:/home/node/.signalk
```

Config files in the container:

```text
/home/node/.signalk/settings.json
/home/node/.signalk/plugin-config-data/sk-to-nmea0183.json
```

Repo sources for those files:

```text
signalk/config/settings.json
signalk/config/plugin-config-data/sk-to-nmea0183.json
```

`settings.json` must include:

```json
{
  "interfaces": {
    "nmea-tcp": true
  }
}
```

The plugin config is enabled with `conversions` entries. The test setup enables
every sentence supported by the plugin with a `1000` ms throttle; sentences
whose required SignalK paths are absent do not emit.

## Verification

Start or restart the demo data source:

```sh
make demo-up
```

Connect to the NMEA TCP output:

```sh
nc localhost 10110
```

Verified output from the official plugin includes:

```text
$GPGGA,...
$GPRMC,...
$IIHDT,...
$IIMWV,...
$IIVWR,...
$IIVWT,...
$IIDBT,...
$IIMTW,...
```

## Notes

- This is server-side test output, not ESP32 firmware NMEA input support.
- The official plugin publishes TCP NMEA 0183. It is not a UDP broadcast
  plugin.
- Docker must publish `10110/tcp`; enabling the plugin alone is not enough for
  clients outside the container.
