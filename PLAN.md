# Yey Boats Instruments — work in progress

Snapshot of in-flight design / engineering threads so they can be picked
up without re-deriving context. Not a roadmap; not advice to future
readers beyond the project owner.

## Live state (verified over BLE bench)

| | |
|---|---|
| FPS | 60 Hz, flush 1.76 ms avg / 2.27 ms peak |
| Internal SRAM | 18.4 % (60 KB) |
| PSRAM | 597 KB used / 7595 KB free |
| WiFi | STA on MeGeNaGo, multi-network store w/ MeGeNaGo + sohohotels free+ |
| Screens | 10 detached, swapped via lv_screen_load |

## Things that just landed

- **PSRAM-backed LVGL pool** (#38) - `LV_USE_STDLIB_MALLOC = LV_STDLIB_CUSTOM`
  + hooks in `src/lvgl_alloc.cpp` route every LVGL alloc into PSRAM
  (`heap_caps_*(MALLOC_CAP_SPIRAM)`). Frees ~96 KB internal SRAM and
  lifts the per-screen widget budget. Display flush buffers stay in
  DMA-capable SRAM.
- **Native multi-screen** (#34) - each screen is a detached
  `lv_obj_create(NULL)`; manager swaps via `lv_screen_load`. Inactive
  screens consume no draw budget. Global overlays (MOB, alarm banner,
  breadcrumb) + gesture handler attach to `lv_layer_top()` so they
  survive swaps. Possible now because of PSRAM pool.
- **Captive portal in AP mode** (#37) - DNSServer on `*:53` -> AP IP,
  every HTTP probe URL or foreign Host: returns the config page inline
  (200 OK, not 302). Phone OS auto-opens browser on join.
- **DHCP rescue** (in AP fallback path) - full WiFi reset
  (`disconnect(true,true) -> WIFI_OFF -> WIFI_AP -> softAPConfig`) +
  500 ms grace before clients associate. Without this the DHCP server
  task didn't initialise after failed STA attempts.
- **Multi-network WiFi store** (#36) - up to 8 saved networks in NVS
  (`Preferences ns=wifi key=nets` JSON), boot iterates them in order,
  most-recently-saved first. Legacy single-pair migrated on first boot.
- **WiFi management endpoints** + UI panel - `GET /api/wifi/saved`,
  `DELETE /api/wifi/saved/<ssid>`, `POST /api/wifi/scan`,
  `GET /api/wifi/networks`, `POST /api/wifi/connect` /
  `POST /api/wifi/forget`. Quoted-SSID + open-network parsing in the
  console too.
- **QR provisioning** on the WiFi setup screen - encodes
  `WIFI:T:nopass;S:espdisp-setup;;`, big 220 px so phones scan from
  across the cabin.
- **/api/screenshot.bmp** via cross-task marshaling - web task posts
  request, blocks on semaphore; LVGL task picks it up in `ui_refresh`,
  runs `lv_snapshot_take`, builds 16 bpp BMP into PSRAM, signals
  completion. Unblocks #33 (design loop).
- **Backlight brightness control** - LEDC PWM ch 0 / 5 kHz / 8 bit.
  `bright N` console + web slider + NVS persistence.
- **FPS rescue** - one `lv_obj_invalidate(lv_screen_active())` per
  ui_refresh cycle. The Arduino_GFX RGB panel driver has no way to tell
  LVGL "I need a frame anyway" - without forced invalidation, FPS
  silently fell to 0.

## Pending work threads

### #20 OpenHASP-style runtime addressing
Goal: web UI / BLE can set any widget attribute by path
(`wind.awa_marker.color`, `nav.sog_value.font_size`, ...). Needed for
the live design loop (#33).

Sketch:
- Each screen module exposes a small `set_attr(name, json_value)`
  function and a `widgets[]` table mapping names -> setters.
- Screen manager has `set_attr(screen, name, value)` that dispatches.
- Web endpoints:
  - `GET  /api/widgets` -> array of `{screen, id, attrs}`
  - `POST /api/widget/<screen>/<id>/<attr>` body `value` (json)
  - `GET  /api/widget/<screen>/<id>` -> current attrs
- BLE Configuration write also accepts `{"set":{"path":"...","value":...}}`.
- Console: `set <screen.id.attr> <value>`.

Heavy across all 10 screens. Could be done incrementally - one screen
at a time starting with Wind (most likely to need design iteration).

### #22 JSONL layout format
Accept newline-separated single-widget records as an alternative to the
big nested JSON. Detect by leading `{` on a newline boundary in
`layout::apply_json`. Small.

### #33 Re-run screen design session
Blocked by #20. Once attrs are addressable: external tool fetches
`/api/screenshot.bmp`, posts widget changes, fetches again, diffs.
Iterate on fonts, spacing, colour until each screen looks right; bake
final values into the C++ defaults.

### #35 Captive portal walkthrough (hotel-side)
Probe `http://www.google.com/generate_204` after STA join. If response
isn't 204, follow Location header to portal landing page, attempt to
POST form fields. Hotel credentials kept in PLAN.md history (Boris /
Sorochkin / 210 / sbornava@gmail.com). Heuristic field matching - fall
back to surfacing the portal URL on the WiFi setup screen if the form
doesn't match known shapes.

### #11 Marine charts via SignalK chart plugin
Large external scope; requires a SignalK chart-tile server upstream.
Out of scope for the near term.

### #26 Companion smartphone app
Separate repo / project; nothing to do here until we have it.

## Things to know about this codebase

- **Renderer is hardcoded C++**, not JSON-driven yet (that's #20). The
  layout JSON round-trips via `PUT /api/layout` but doesn't actually
  change widget visuals today.
- **PSRAM is the LVGL pool** since commit `3b3224f`. Don't accidentally
  switch `LV_USE_STDLIB_MALLOC` back to `LV_STDLIB_BUILTIN` - the
  static 96 KB array overflowed internal SRAM at boot once.
- **LVGL pool < canvas needs**: `lv_qrcode_create(240)` allocates
  `240*240*2 = 115 KB`. Only works because we're in PSRAM. If you ever
  revert to internal SRAM, the QR widget breaks.
- **The forced `lv_obj_invalidate` per cycle is load-bearing**. Without
  it the panel reads idle even though widgets change. Don't remove.
- **AP isolation on the user's WiFi (hotspots/hotels) blocks peer
  access**. Device is on the LAN, just unreachable from peers. Not a
  device bug. CLAUDE.md documents the recovery flow.
- **PSRAM-backed LVGL is slightly slower than internal SRAM** in
  theory; in practice the 60 Hz flush hits the same ~1.8 ms / frame as
  before. Buffers used for the RGB panel push are still in internal
  DMA SRAM.

## Commits in this push (newest first)

```
744012d feat: backlight brightness control (LEDC PWM)
408f779 feat(web): screenshot endpoint via cross-task marshaling
45281c1 refactor(ui): native LVGL multi-screen, overlays on top layer
3b3224f feat(lvgl): PSRAM-backed LVGL pool, restore QR / canvas / snapshot
72ac13d fix(ui): restore rendering - force invalidate per refresh cycle
89a03ff fix(wifi): full reset before AP fallback so DHCP starts
983a603 fix(captive): serve config inline + explicit probe routes + logging
5726b12 feat(web): captive portal redirect when in AP mode
5ea5b6d feat(wifi): multi-network store, AP-mode QR provisioning, web UI
0ad8feb feat(web/net): WiFi config from web UI + quoted-SSID + open networks
```
