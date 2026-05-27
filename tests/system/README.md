# System tests

End-to-end tests against a flashed `esp32-boat-mfd` device on the same
network. Split between **unattended** (pytest, no human) and
**attended** (markdown checklists, requires touching the device).

The tests exercise every data path that ships today:

| Path                | Coverage                                       |
| ------------------- | ---------------------------------------------- |
| SignalK WebSocket   | Auto-discovery, manual host, value ingest      |
| NMEA0183 over WiFi  | UDP listen + TCP client, all parsed sentences  |
| NMEA2000 (CAN bus)  | Status reporting only (hardware absent)        |
| Layout templates    | Per-template render + screenshot               |
| Source priority     | NMEA2k > NMEA-WiFi > SignalK switchover        |
| Touch / gestures    | Calibration (attended); tap / swipe / gesture injection (unattended) |
| Settings UI         | Brightness, theme, screen pick (attended)      |
| WiFi captive portal | First-boot provisioning (attended)             |

## Quick start

```sh
# 1. Flash a build that has the test endpoints (any tag after v0.3 or main HEAD)
make flash

# 2. Bring up demo SignalK server in docker
make demo-up

# 3. Run unattended suite (point at device by IP or .local hostname)
ESPDISP_HOST=esp32-boat-mfd.local pytest tests/system/unattended

# 4. Step through attended checklists
open tests/system/attended/README.md
```

Set `ESPDISP_HOST` to either an IP or a `.local` mDNS name (the latter
requires Bonjour / Avahi on the host). Optional:

- `ESPDISP_SK_HOST` — SignalK server (default: `localhost`)
- `ESPDISP_NMEA_WIFI_PORT` — default `10110`
- `ARTIFACTS_DIR` — screenshot output dir (default: `tests/system/artifacts`)

## What screens look like after each test

Every unattended test that visits a screen writes `tests/system/artifacts/<test-name>.bmp`.
Re-run with `--keep-artifacts` to retain across runs (default overwrites).
The CI workflow uploads the directory as a build artifact.

## NMEA2000 expectations

Without a CAN transceiver wired to the board, the `n2k` test asserts
`compiled_in=false` (default build). To verify the listen path, build
`-DENABLE_NMEA2000` and attach an SN65HVD230 (or equivalent) to the
configured `rx_pin`/`tx_pin`, then re-run `test_nmea2000_listen.py`.
The test currently `xfail`s with `reason="hardware not validated"` so
it never blocks CI.

## Touch / gesture injection

For UI tests that can't bring a finger to the device, the firmware
exposes `/api/test/*` (POST, JSON body) at two levels:

| Level | Endpoint            | Body                                                   |
|-------|---------------------|--------------------------------------------------------|
| 1     | `/api/test/touch`   | `{"x":N,"y":N,"pressed":bool}` — raw snapshot write    |
| 1     | `/api/test/tap`     | `{"x":N,"y":N,"hold_ms":50}` — press + hold + release  |
| 1     | `/api/test/swipe`   | `{"x0":..,"y0":..,"x1":..,"y1":..,"dur_ms":300}`       |
| 2     | `/api/test/gesture` | `{"dir":"left"\|"right"\|"up"\|"down"}` — direct queue |

Level 1 drives the full pipeline (touch snapshot → LVGL hit-test →
CLICKED → tile_clicked_cb → `app::post`) so it exercises calibration,
gesture-detection thresholds, and the screen-local suppression rules
(MOB, WiFi keyboard, settings drawer).

Level 2 bypasses touch entirely and posts a `ShowScreen` command, so
tests of the screen-router and handler logic don't depend on coordinate
geometry.

The `Device` fixture wraps both: `device.tap(x, y)`, `device.swipe(...)`,
`device.gesture("left")`. See `test_input_injection.py` for examples.

## Adding a new test

1. Drop a `test_*.py` under `unattended/`. Use the `device` fixture
   for HTTP helpers and `udp_logs` for capturing `net::logf` output.
2. Use one of the injectors under `inject/` (or add a new one) to
   feed data into the device.
3. Call `device.screenshot("<label>")` to drop a BMP into the
   artifacts dir.
4. Update this README's coverage table.

For an attended test, add a numbered markdown file under `attended/`
with steps, expected results, and pass criteria.
