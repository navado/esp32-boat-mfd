# Arduino → ESP-IDF Migration

## Goal

Drop the Arduino-ESP32 framework wrapper and call ESP-IDF directly. The
firmware already runs on top of ESP-IDF — Arduino-ESP32 is a layer of
C++ classes plus a handful of helpers (`Serial`, `String`,
`Preferences`, `WiFi`, `HTTPClient`, `ESPmDNS`, `ArduinoOTA`) wrapping
ESP-IDF and FreeRTOS calls. Removing the wrapper saves ~120 KB of flash,
unlocks the streaming HTTP / lower-level LCD APIs we already want, and
removes one set of release notes to track.

## Non-goals

- Rewriting LVGL (it's framework-agnostic C; already linked directly).
- Rewriting ArduinoJson (also framework-agnostic; host tests already
  link it without Arduino).
- Rewriting the SignalK plugin or any spec 17/18/19/20 contracts.
- Changing PCB / pin maps. The verified ST7701 init table in
  `board_pins.h` and the GT911 quirks must round-trip unchanged.

## Migration model

Hybrid PlatformIO framework (`framework = arduino, espidf`) used as a
transition vehicle. Both stay linked, so each module migrates in a
self-contained commit and the bench stays usable throughout. The
final cutover removes `arduino` from the list.

Migration is sequenced by dependency (storage and WiFi must be alive
before HTTP can move; HTTP must be alive before OTA can move) and by
risk (display + BLE are the biggest rewrites; schedule them after the
cheap mechanical work has shaken out the build system).

---

## A. Hybrid framework + IDF version bump

### Scope

Switch `platformio.ini`:

```ini
[env:esp32-4848s040]
platform = espressif32@^6.9.0     ; was ^6.7.0
framework = arduino, espidf       ; was arduino
```

This bumps Arduino-ESP32 2.0.x → 3.x and ESP-IDF 4.4 → 5.1 in one shot
(PlatformIO ties them together at this platform version).

### Design

No code changes intended; this is a build-system change. The Arduino
3.x release contains breaking changes the build will surface:

- `ledcSetup` / `ledcAttachPin` signatures changed — `board::set_backlight`
  needs the new `ledcAttach` 4-arg form.
- `WiFi.onEvent` callback shape changed.
- `attachInterrupt` arg cast tightened.
- `NimBLE-Arduino` major-version bump (2.x → 3.x); some characteristic
  ctor signatures move.

### Risks

The IDF 4 → 5 bump is the real surface area. Some component-manager
components require IDF 5.

### Test plan

USB flash + boot + walk all 14 screens + run host suite + Lane A
smoke (SK live, manager-status hb=200).

### Estimate

L — 5-7 days. Confidence: high (revised after a Jan-2025 spike).

### Spike findings (2025-05-29)

Attempted the platform bump in isolation. What surfaced:

1. **Platform 6.9 ships Arduino-ESP32 2.0.17 + IDF 5.3.1**, not Arduino
   3.x as the spec originally guessed. Good news: no Arduino major bump.
2. **A stale Tasmota fork** (`espressif32@2023.6.2`) at the global
   PlatformIO platforms dir won the SemVer resolution race against
   `^6.9.0`. Diagnostic cost: 30 minutes. Mitigation: `pio platform
   uninstall espressif32 -y` before retrying.
3. **`sdkconfig.defaults` required** to set `CONFIG_FREERTOS_HZ=1000`,
   `CONFIG_SPIRAM_MODE_OCT=y`, `CONFIG_SPIRAM_SPEED_80M=y`. The pure
   Arduino framework set these implicitly; hybrid does not.
4. **36 `%lu` / `%ld` format-string sites become errors** under the new
   stricter compiler flags (`uint32_t` is `unsigned int` on the
   xtensa toolchain, not `unsigned long`). Each needs an explicit cast
   `(unsigned long)x` or a flag change to `%u`/`%d`. Mechanical,
   ~1-2 hours; in scope for B and not A.
5. **NimBLE-Arduino 1.4.x does not compile on IDF 5** —
   `NimBLEDevice.h` includes `esp_bt.h` which moved out of the global
   include path. Either bump NimBLE-Arduino to a 5.x-compatible version
   (no longer maintained at 1.4.x; 2.x is for Arduino 3.x only), or
   land **Spec J ahead of schedule**. A in practice forces J to run
   first or in parallel.
6. **One pre-existing `-Wmisleading-indentation` bug** in
   `src/boards/board_cli.cpp:30` revealed by the stricter flags.
   Pre-existing issue, fix in B.

Revised dependency: **A practically requires J first.** Update the
execution order accordingly.

---

## B. Mechanical IDF call replacements

### Scope

Sed-pass replacements that don't need their own module structure:

- `Serial.printf(...)` → `printf(...)` (or `ESP_LOGI(TAG, ...)`)
- `millis()` → `(uint32_t)(esp_timer_get_time() / 1000)`
- `delay(n)` → `vTaskDelay(pdMS_TO_TICKS(n))` (most are already done)
- `pinMode` / `digitalWrite` / `digitalRead` → `gpio_config_t` +
  `gpio_set_level` / `gpio_get_level`
- `ledcSetup` / `ledcAttachPin` / `ledcWrite` → `ledc_timer_config` +
  `ledc_channel_config` + `ledc_set_duty` + `ledc_update_duty`
- `attachInterrupt(digitalPinToInterrupt(pin), fn, mode)` →
  `gpio_install_isr_service` + `gpio_isr_handler_add`

### Design

Where the IDF call has 3+ steps (LEDC, GPIO config) wrap in a tiny
helper at the point of use. No new module abstractions.

### Risks

- GPIO ISR service is process-wide; install exactly once.
- LEDC clock source choice (`LEDC_USE_APB_CLK` vs `LEDC_USE_RTC8M_CLK`)
  affects PWM frequency stability for the backlight.

### Test plan

Backlight set/get round-trip + touch IRQ ISR count grows.

### Estimate

S — 3 days, can run in parallel with C.

---

## C. Preferences → storage::Namespace (NVS direct)

### Scope

Replace ~24 call sites of `Preferences p; p.begin(NS); p.getString/...`
with a thin C++ wrapper over `nvs_handle_t`. Existing NVS data (same
namespaces, same keys, same types) stays compatible.

### Design

```cpp
// include/storage.h
namespace storage {

class Namespace {
 public:
    Namespace(const char *name, bool readonly);
    ~Namespace();  // commits on RW, closes always

    std::string get_string(const char *key, const char *default_ = "");
    void        put_string(const char *key, const char *value);
    uint8_t     get_u8 (const char *key, uint8_t  default_ = 0);
    void        put_u8 (const char *key, uint8_t  value);
    uint32_t    get_u32(const char *key, uint32_t default_ = 0);
    void        put_u32(const char *key, uint32_t value);
    int8_t      get_i8 (const char *key, int8_t   default_ = 0);
    void        put_i8 (const char *key, int8_t   value);
    void        remove(const char *key);
    bool        ok() const;
 private:
    nvs_handle_t handle_;
    bool readonly_;
    bool ok_;
};

}  // namespace storage
```

Each existing `load_prefs`/`save_prefs` becomes a `storage::Namespace
p("ns", ro);` + identical key list. Per-module commits.

### Risks

- NVS namespace names are limited to 15 chars in IDF. Audit the existing
  names: `mgr`, `n0183w`, `n2k`, `ap`, `ui`, `web`, `sk`, `wifi-store`
  — all ≤ 15. ✓
- Existing values get written by the Arduino `Preferences` wrapper today.
  Verify the binary layout for u8/u32/string is identical between the
  two (it should be; `Preferences` IS a thin wrapper over `nvs_*`).

### Test plan

Round-trip every key. Reboot persistence verified by `make flash` then
checking `/api/state` reports the last-saved value.

### Estimate

M — 2 days. Confidence: high.

---

## D. WiFi → esp_wifi + esp_netif

### Scope

Rewrite `src/net.cpp` STA + AP path. Replace `WiFi.begin`,
`WiFi.softAP`, `WiFi.status`, `WiFi.localIP`, `WiFi.RSSI`,
`WiFi.scanNetworks`, `WiFi.onEvent`, `DNSServer` (captive portal) with
ESP-IDF equivalents.

### Design

Event-driven state machine on the IDF default event loop:

```text
WIFI_EVENT_STA_START          → esp_wifi_connect()
WIFI_EVENT_STA_DISCONNECTED   → backoff + reconnect or rotate to next
                                stored network
IP_EVENT_STA_GOT_IP           → set s_wifi_state = Up; net::wifiUp() true
WIFI_EVENT_AP_STACONNECTED    → log
WIFI_EVENT_SCAN_DONE          → publish results to /api/wifi/scan
```

Multi-network store keeps the same shape (post-Spec C). Captive portal
gets a 50-line custom DNS server (UDP/53, respond to all queries with
device IP). Drop Arduino's `DNSServer` dependency.

Public surface stays:
- `net::wifiUp()`, `net::wifiStateName()`, `net::ipString()`,
  `net::rssi()`, `net::deviceId()`, `net::handleSerialCommand("wifi ...")`

### Risks

- Auto-reconnect timing differs from Arduino's defaults.
- `WiFi.scanNetworks(false, true)` async behavior maps to
  `esp_wifi_scan_start(NULL, false)` + `WIFI_EVENT_SCAN_DONE` — make
  sure the BLE-side `scan` removal from 00-findings doesn't regress.
- AP captive portal regression risk; need the custom DNS responder to
  be byte-for-byte compatible with iOS / Android captive portal probes
  (`/hotspot-detect.html`, `/generate_204`).

### Test plan

`make flash` → AP-mode QR provisioning → STA reconnect after Wi-Fi
flap → captive portal redirects iOS to setup page. Lane A live SK
connect.

### Estimate

L — 5-7 days. Confidence: medium. The captive portal is the chewy
part; the basic STA/AP swap is well-trodden.

---

## E. mDNS → IDF mdns component

### Scope

Replace `ESPmDNS.h` with the IDF `mdns` component. Sites:

- `MDNS.begin(s_device_id.c_str())` in `net.cpp`
- `MDNS.addService("arduino", "tcp", 3232)`,
  `MDNS.addService("espdisp", "tcp", 80)`
- `MDNS.queryService("espdisp-mgmt", "tcp")` +
  `MDNS.hostname(i)`/`MDNS.port(i)`/`MDNS.IP(i)` in `manager-discover`

### Design

Thin wrapper to keep call sites identical:

```cpp
namespace net::mdns {
bool begin(const char *hostname);
void add_service(const char *instance, const char *proto, uint16_t port);
struct Hit { std::string hostname; ip4_addr_t ip; uint16_t port; };
std::vector<Hit> query_service(const char *service, const char *proto,
                                uint32_t timeout_ms);
}  // namespace net::mdns
```

`query_ptr` + `query_srv` + `query_a` chain wrapped behind the single
`query_service` call.

### Risks

- IDF mDNS has occasional race issues during STA re-association.
  Already documented in `manager-discover` comment that iOS hotspot
  filters mDNS; behavior on real LAN stays.

### Test plan

`manager-discover` → 1+ hits when the plugin is up.
`avahi-browse -r _espdisp._tcp` (or `dns-sd -B`) from a peer sees the
device's two services.

### Estimate

S — 1-2 days. Confidence: high.

---

## F. HTTPClient → esp_http_client

### Scope

Replace 7 sites in `src/manager.cpp` (register, heartbeat, fetch_config,
poll_commands, ack_command, post_ota_progress, post_firmware_confirm)
and the OTA download loop. `http.begin/setConnectTimeout/setTimeout/
addHeader/GET/POST/getString/getSize/end` all need new homes.

### Design

```cpp
// include/net/http.h
namespace net::http {
struct Response {
    int    code;
    std::string body;        // empty for streamed bodies
    int    content_length;   // -1 for chunked
};

class Request {
 public:
    explicit Request(const char *url);
    void set_method(esp_http_client_method_t m);
    void set_header(const char *name, const char *value);
    void set_body(const std::string &body);  // POST/PUT
    void set_timeout_ms(int connect, int read);

    // Read-into-string. Refuses bodies larger than `cap` via a
    // header-callback check on Content-Length (drop-in for
    // resp_within_cap()).
    Response perform(int response_cap_bytes);

    // Streaming variant for OTA: each chunk handed to `sink` as it
    // arrives; sink returns false to abort.
    int perform_streaming(int response_cap_bytes,
                          std::function<bool(const uint8_t*, size_t)> sink);
};
}  // namespace net::http
```

The new module respects the four existing caps
(`MAX_DISCOVERY_BYTES`, `MAX_HEARTBEAT_RESP_BYTES`, `MAX_CONFIG_BYTES`,
`MAX_COMMANDS_BYTES`) via a header-event check.

### Risks

- TLS support is currently unused but free with `esp_http_client`. Keep
  the `embed_txtfiles` cert bundle slot ready for when SK moves to wss://
- Chunked encoding handling differs; verify the SK plugin doesn't send
  chunked.

### Test plan

Lane A: live SK register/heartbeat 200. Lane B: every Lane B test that
hits `firmware.update` / `screen.set` / etc. (all 20 in
`test_manager_commands.py`) since they trigger an ack POST.

### Estimate

M — 4 days. Confidence: high.

---

## G. OTA: Update.h → esp_ota_* + drop ArduinoOTA

### Scope

- Replace `Update.begin/write/end` in the manager OTA task with
  `esp_ota_*` calls direct (we already use `esp_ota_mark_app_valid_*`).
- Remove `ArduinoOTA` (port 3232 push-OTA). `make ota` deprecated; pull-
  OTA via the `firmware.update` v1 command is the only path.

### Design

Pull-OTA path (after Spec F lands):

```cpp
esp_ota_handle_t h;
const esp_partition_t *next = esp_ota_get_next_update_partition(nullptr);
esp_ota_begin(next, OTA_SIZE_UNKNOWN, &h);
net::http::Request req(url);
req.set_header("Authorization", auth);
req.perform_streaming(MAX_OTA_BYTES, [&](const uint8_t *chunk, size_t n) {
    if (esp_ota_write(h, chunk, n) != ESP_OK) return false;
    mbedtls_sha256_update(&sha_ctx, chunk, n);
    return true;
});
esp_ota_end(h);
// sha verify, esp_ota_set_boot_partition, esp_restart
```

The existing policy gates (`s_ota_enabled` / `s_ota_max_size` /
`s_ota_require_sha`) stay in `execute_command`.

Drop `ArduinoOTA.handle()` from the main loop; remove port 3232
listener. Update `docs/specs/17 §10`.

### Risks

- Loss of `make ota` removes a path the dev workflow has used a lot
  this session. Document the `firmware.update` command-line equivalent
  via `curl` to the plugin.
- Rollback safety: the `PENDING_VERIFY` → `mark_app_valid` sequence
  must run on first successful heartbeat after the new image boots.
  Already implemented; preserve.

### Test plan

Cycle: bench → plugin `firmware.update` push → device downloads,
verifies, reboots → first heartbeat marks partition valid → `/devices`
shows new build_time.

### Estimate

S — 2 days. Confidence: high.

---

## H. Display: Arduino_GFX + custom ST7701 → esp_lcd_panel_rgb

### Scope

The biggest single rewrite. Replace `Arduino_SWSPI` + `Arduino_RGB_Display`
+ the embedded ST7701 init table in `board_pins.h` with:

1. `esp_lcd_panel_io_3wire_spi` for boot SPI commands
2. `esp_lcd_new_rgb_panel` for the RGB parallel transfer
3. LVGL `lv_display_set_flush_cb` → `esp_lcd_panel_draw_bitmap`

### Design

Maps onto **spec 13 step 3 (display extraction)**. Per-board
implementation under `board::display_begin()`:

```cpp
bool board::display_begin() {
    esp_lcd_panel_io_3wire_spi_config_t io_cfg = {
        .line_config = { .cs_io_type=...,.cs_gpio_num=ST7701_CS, ... },
        .expect_clk_speed = 1 * 1000 * 1000,
        // ...
    };
    esp_lcd_new_panel_io_3wire_spi(&io_cfg, &s_panel_io);
    apply_st7701_init_table(s_panel_io);   // verified bytes verbatim

    esp_lcd_rgb_panel_config_t rgb_cfg = {
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .timings = {
            .pclk_hz = ..., .h_res=480, .v_res=480,
            .hsync_pulse_width=..., .hsync_back_porch=..., /* etc */
        },
        .data_width = 16,
        .psram_trans_align = 64,
        .num_fbs = 2,           // double-buffer in PSRAM
        .bounce_buffer_size_px = 0,
        .hsync_gpio_num = RGB_HSYNC, .vsync_gpio_num = RGB_VSYNC,
        .de_gpio_num = RGB_DE,  .pclk_gpio_num = RGB_PCLK,
        .data_gpio_nums = { RGB_B0, RGB_B1, RGB_B2, RGB_B3, RGB_B4,
                            RGB_G0, RGB_G1, RGB_G2, RGB_G3, RGB_G4, RGB_G5,
                            RGB_R0, RGB_R1, RGB_R2, RGB_R3, RGB_R4 },
        .flags = { .fb_in_psram = 1, .double_fb = 1 },
    };
    esp_lcd_new_rgb_panel(&rgb_cfg, &s_panel);
    esp_lcd_panel_reset(s_panel);
    esp_lcd_panel_init(s_panel);
    return true;
}
```

The verified init table (`// clang-format off` section in
`board_pins.h`) ports byte-for-byte into `apply_st7701_init_table`. Do
**not** rederive timings from web sources — the verified ones go in
verbatim.

LVGL hookup:

```cpp
static void flush_cb(lv_display_t *d, const lv_area_t *area,
                     uint8_t *px) {
    esp_lcd_panel_draw_bitmap(s_panel, area->x1, area->y1,
                              area->x2 + 1, area->y2 + 1, px);
    lv_display_flush_ready(d);
}
```

Or in `LV_DISPLAY_RENDER_MODE_FULL` mode, swap framebuffers via
`esp_lcd_rgb_panel_get_frame_buffer` + `esp_lcd_rgb_panel_set_pep_bb_invalidate_cache`.

### Risks

- **The trickiest migration.** RGB timing is unforgiving — wrong porch
  values give jitter, color shifts, or no signal. Mitigation: preserve
  the exact timings in `board_pins.h` and copy them straight into the
  IDF struct.
- ST7701 init table — keep `clang-format off` wrap; copy each
  `WRITE_COMMAND_8` / `WRITE_BYTES` / `WRITE_C8_D16` sequence into the
  equivalent `esp_lcd_panel_io_tx_param` / `esp_lcd_panel_io_tx_color`
  call. One-for-one. Camera color test (white/red/green/blue gradients
  visible on screen with no banding) is the acceptance gate.
- LVGL flush cadence and dirty-rect handling — the existing
  performance work (PSRAM double-buffer, 60 Hz) must not regress.

### Test plan

USB flash, walk every screen, camera color test, frame-rate check
(target ≥ 50 Hz on dashboard). Stay on `framework = arduino, espidf`
hybrid during this step so a bad rev can be reverted with one commit.

### Estimate

L — 7 days. Confidence: low-medium. Allow extra contingency.

---

## I. Touch: GT911-Arduino → esp_lcd_touch_gt911

### Scope

Replace TAMCTec GT911 library with the IDF `esp_lcd_touch_gt911`
component. Remove the manual big-endian decode workaround from
`main.cpp` (`((uint16_t)hi << 8) | lo` reads).

### Design

Maps onto **spec 13 step 4 (touch into `input::` module)**. Per-board
`board::touch_begin()`:

```cpp
bool board::touch_begin() {
    esp_lcd_panel_io_i2c_config_t io_cfg =
        ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();
    esp_lcd_new_panel_io_i2c(s_i2c_bus, &io_cfg, &s_touch_io);

    esp_lcd_touch_config_t cfg = {
        .x_max = 480, .y_max = 480,
        .rst_gpio_num = GPIO_NUM_NC,
        .int_gpio_num = (gpio_num_t)TOUCH_INT,
        .flags = { .swap_xy = 0, .mirror_x = 0, .mirror_y = 0 },
    };
    esp_lcd_touch_new_i2c_gt911(s_touch_io, &cfg, &s_touch);
    return true;
}
```

`input::poll()` calls `esp_lcd_touch_read_data` +
`esp_lcd_touch_get_coordinates`. IRQ path (`touch.mode = "irq"`) keeps
its current shape — `esp_lcd_touch` doesn't manage the ISR itself, we
keep `gpio_isr_handler_add` from Spec B and notify the polling task.

### Risks

- The big-endian quirk for this panel — verified in tests: the official
  IDF driver implements the standard GT911 protocol and reads the
  registers in the documented order, so the workaround should not be
  needed. Confirm with a tap on (10,10) reading as (10,10) not
  (2560,2560).

### Test plan

`touch_grid` + `touch_cal` screens still calibrate. Lane B touch
injection paths (`tap`, `swipe`, `gesture`) work via BLE/serial console.

### Estimate

S — 2-3 days. Confidence: high.

---

## J. NimBLE-Arduino → esp_nimble (Apache NimBLE direct)

### Scope

Rewrite `src/ble_config.cpp` (~600 lines) to use Apache NimBLE's C API
directly. GAP advertising + GATT service definition + characteristic
read/write callbacks all move.

### Design

GATT service definition becomes the IDF-style struct array:

```cpp
static const struct ble_gatt_chr_def s_nus_chars[] = {
    { .uuid = (ble_uuid_t*)&NUS_TX_UUID,
      .access_cb = nus_tx_read_cb,
      .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
      .val_handle = &s_nus_tx_handle },
    { .uuid = (ble_uuid_t*)&NUS_RX_UUID,
      .access_cb = nus_rx_write_cb,
      .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_NO_RSP,
      .val_handle = &s_nus_rx_handle },
    { 0 },
};
static const struct ble_gatt_svc_def s_services[] = {
    { .type = BLE_GATT_SVC_TYPE_PRIMARY,
      .uuid = (ble_uuid_t*)&NUS_SERVICE_UUID,
      .characteristics = s_nus_chars },
    /* CONNECTION + CONFIGURATION services ... */
    { 0 },
};
```

C function pointers replace C++ method-callback objects; per-service
`user_data` carries `this` for the state.

The on-disk protocol (UUIDs, characteristic shapes, 512-byte MTU
constraint, `truncated` JSON summary above 512) is unchanged.

Preserve the `setValue(uint8_t*, len)` trap fix from CLAUDE.md — the C
API takes `len` explicitly, so the trap doesn't exist any more.

### Risks

- Largest single rewrite by line count. Schedule late so the rest of
  the build has stabilized first.
- BLE pairing — currently `none-in-current-firmware` per `/api/security`.
  Don't introduce pairing in this migration; spec 20 §BLE is the place
  to add it if desired.

### Test plan

`make ble` console: `ip` + `sk-status` round-trip. Lane B BLE tests
(`test_input_injection.py` BLE path). CONFIGURATION characteristic
512-byte truncation summary still emits.

### Estimate

L — 5-6 days. Confidence: medium.

---

## K. WebSockets: links2004 → esp_websocket_client

### Scope

Replace `WebSocketsClient` in `src/signalk.cpp`. Event callback shape
(WS_CONNECTED, WS_DISCONNECTED, WS_TEXT, WS_ERROR) maps 1:1 to
`WEBSOCKET_EVENT_*`.

### Design

```cpp
esp_websocket_client_config_t cfg = {
    .uri = ws_url.c_str(),     // "ws://10.x.x.x:3000/signalk/v1/stream?token=..."
    .reconnect_timeout_ms = 5000,
    .network_timeout_ms = 10000,
    .buffer_size = 4096,
};
esp_websocket_client_handle_t s_ws = esp_websocket_client_init(&cfg);
esp_websocket_register_events(s_ws, WEBSOCKET_EVENT_ANY,
                              on_ws_event, NULL);
esp_websocket_client_start(s_ws);
// later:
esp_websocket_client_send_text(s_ws, payload, len, portMAX_DELAY);
```

429-retry/backoff logic stays. Subscription is one `send_text` after
WS_CONNECTED. Delta parsing in `signalk_parser.cpp` is unchanged.

### Risks

- TLS / wss:// readiness — `esp_websocket_client` supports it natively;
  add cert bundle slot for future SK https setups.
- The 429-recovery code path I added earlier this session
  (persistent connection + retry-after) needs to map onto the IDF
  event loop's reconnect timer.

### Test plan

Lane A: SK live, sees deltas, recovers from a forced restart of the SK
container.

### Estimate

M — 3-4 days. Confidence: high.

---

## L. Cutover: framework = espidf

### Scope

```ini
framework = espidf     ; was arduino, espidf
```

Audit and remove any remaining `#include <Arduino.h>`, Arduino lib_deps
(`Arduino_GFX`, `gt911-arduino`, `NimBLE-Arduino`, `WebSockets`).

### Risks

- Any sneaky `String` / `Preferences` use that escaped the audit will
  fail to link.

### Test plan

Full host suite + USB flash + walk every screen + Lane A + B.

### Estimate

S — 1 day. Confidence: high (a build failure here is mechanical and
the linker tells you exactly where).

---

## Summary

### Ordered by complexity (lowest → highest)

| Step | Title              | Complexity | Risk    | Confidence |
|------|--------------------|------------|---------|------------|
| L    | Cutover            | S          | Low     | High       |
| E    | mDNS               | S          | Low     | High       |
| G    | OTA (after F)      | S          | Low     | High       |
| I    | Touch              | S          | Low     | High       |
| B    | IDF mechanicals    | S-M        | Low     | High       |
| C    | Storage (NVS)      | M          | Low     | High       |
| K    | WebSockets         | M          | Low     | High       |
| F    | HTTP client        | M          | Medium  | High       |
| A    | Hybrid + IDF 5     | L          | Medium  | High       |
| D    | WiFi STA/AP        | L          | Medium  | Medium     |
| J    | NimBLE C API       | L          | Medium  | Medium     |
| H    | Display (RGB+LCD)  | L          | High    | Low-Medium |

### Ordered by dev speed (fastest → slowest)

| Days  | Step | Title                |
|-------|------|----------------------|
| 1     | L    | Cutover              |
| 1-2   | E    | mDNS                 |
| 2     | C    | Storage (NVS)        |
| 2     | G    | OTA                  |
| 2-3   | I    | Touch                |
| 3     | B    | IDF mechanicals (incl. 36 fmt-string fixes) |
| 3-4   | K    | WebSockets           |
| 4     | F    | HTTP client          |
| 5-6   | J    | NimBLE C API (effective prereq for A) |
| 5-7   | A    | Hybrid + IDF 5 (revised; see spike findings) |
| 5-7   | D    | WiFi STA/AP          |
| 7     | H    | Display              |

**Total: 41-56 dev-days** (revised; was 38-50), i.e. **8-11 calendar
weeks at one dev's pace** with code review, integration, and the
inevitable "but on the bench..." debugging baked in.

### Recommended execution order (dependency-aware, post-spike)

The spike showed **A is effectively gated on J** (NimBLE-Arduino 1.4
doesn't compile on IDF 5), and a chunk of "B mechanicals" is in fact
on the A critical path (the 36 fmt-string fixes block the compile).
Recommended new order:

1. **B-mechanicals-only** (fmt-string casts + the
   `-Wmisleading-indentation` fix in `board_cli.cpp:30`). Lands while
   still on Arduino 2.x + IDF 4.4 - low risk, decouples B from A.
2. **J** — NimBLE C API rewrite. Bench validates BLE NUS console +
   CONFIGURATION characteristic. Lands on the current platform first
   if Apache NimBLE is exposed there too; otherwise piggybacks on A.
3. **A** — Hybrid framework + IDF 5.3 bump. With J + the fmt-string
   prep already merged, A becomes a config-only commit again.
4. **C** — Storage (unblocks D and re-validates J's NVS use).
5. **E** — mDNS (independent quick win after A).
6. **D** — WiFi (unblocks F, K).
7. **F** — HTTP (unblocks G).
8. **G** — OTA (fast after F).
9. **K** — WebSockets (after D).
10. **I** — Touch (low risk, do whenever after A).
11. **H** — Display (biggest risk; do alone after the rest has
    shaken out the build system).
12. **L** — Cutover.

### Recommended commit cadence

- A, B, C, E, G, I, L are single-commit changes.
- D, F, K want 2-3 commits (one per sub-area: STA / AP / scan; or
  GET / POST / streaming).
- H, J want 5-7 commits (per-board for H; per-service for J).

Worst case (everything slowest, every risk fires) is ~14 weeks. Best
case (the IDF 5 bump is clean and the display panel timings copy
verbatim) is ~7 weeks.
