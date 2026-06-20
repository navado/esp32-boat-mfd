# Status/System screen review (status.png)

Title "SYSTEM". Ground truth: WiFi/SignalK link UP and live.

## Displayed values (5 questions each)

### BATTERY — 13.78 V
1. Plausible: OK — 13.78 V is a healthy 12V system under charge/float.
2. Unit/format: OK — "V".
3. Precision/scaling: OK — 2 decimals, appropriate for voltage.
4. Label: OK.
5. Legible: OK.

### SoC — 86% (green bar)
1. Plausible: OK — state of charge 86%.
2. Unit/format: OK — "%".
3. Precision/scaling: OK — integer; bar length visually matches ~86%.
4. Label: OK — "SoC".
5. Legible: OK — green bar, no overlap with text column.

### FUEL — 99% (amber bar)
1. Plausible: OK — near-full tank.
2. Unit/format: OK — "%".
3. Precision/scaling: OK — bar ~full, matches 99%.
4. Label: OK.
5. Legible: OK.

### WATER — 100% (blue bar)
1. Plausible: OK — full fresh-water tank.
2. Unit/format: OK — "%".
3. Precision/scaling: OK — full bar.
4. Label: OK.
5. Legible: OK.

### WIFI — STA
1. Plausible: OK — station mode, correct for a provisioned device joined to yey-net.
2. Unit/format: OK — mode string.
3. Precision/scaling: n/a.
4. Label: OK — "WIFI".
5. Legible: OK.

### SSID — yey-net
1. Plausible: OK — matches the renamed lab network (esp-lab -> yey-net).
2. Unit/format: OK.
3. Precision/scaling: n/a.
4. Label: OK.
5. Legible: OK.

### IP — 10.42.0.67
1. Plausible: OK — valid private IPv4 on the lab subnet.
2. Unit/format: OK — dotted quad.
3. Precision/scaling: n/a.
4. Label: OK.
5. Legible: OK.

### RSSI — -63 dBm
1. Plausible: OK — -63 dBm is a good signal.
2. Unit/format: OK — "dBm", negative correct.
3. Precision/scaling: OK — integer.
4. Label: OK.
5. Legible: OK.

### BLE — yey-d-28372f8a0290
1. Plausible: OK — BLE/device name with MAC-derived suffix. (Note: render-perf branch reportedly disables BLE; here a BLE name still shows — may be the advertised/device id rather than active BLE state. Not wrong, just ambiguous.)
2. Unit/format: OK — identifier string.
3. Precision/scaling: n/a.
4. Label: MINOR — "BLE" labels a name string, not a state (on/off). Reads as the device id; acceptable but could be clearer.
5. Legible: OK — long string fits, no truncation/overflow.

### SIGNALK — live
1. Plausible: OK — matches ground truth (link UP, data flowing).
2. Unit/format: OK — state word "live".
3. Precision/scaling: n/a.
4. Label: OK — "SIGNALK".
5. Legible: OK.

### HEAP — 22 kB
1. Plausible: CONCERN — 22 kB free internal heap is low. Per project memory, low internal-heap = SRAM starvation territory (NimBLE/draw-buffer risk). Plausible as a real reading but flags a tight-memory condition worth noting.
2. Unit/format: OK — "kB".
3. Precision/scaling: OK.
4. Label: OK.
5. Legible: OK.

### PSRAM — 7314 kB
1. Plausible: OK — ~7.3 MB free PSRAM, healthy for the N16R8.
2. Unit/format: OK — "kB" (could be MB but consistent with HEAP unit; fine).
3. Precision/scaling: OK.
4. Label: OK.
5. Legible: OK.

### UPTIME — 00:07:01
1. Plausible: OK — 7m 01s since boot; recent reboot (consistent with bench/render-perf work).
2. Unit/format: OK — HH:MM:SS.
3. Precision/scaling: OK.
4. Label: OK.
5. Legible: OK.

### BUILD — Jun 20 2026
1. Plausible: OK — build date matches today's bench session.
2. Unit/format: OK — date string.
3. Precision/scaling: n/a.
4. Label: OK.
5. Legible: OK — last row sits near card bottom edge but not clipped.

## Problems
- HEAP 22 kB free internal heap is low — borderline SRAM-starvation per documented memory traps. Not a render defect but a stability flag worth watching (especially with draw buffers / any BLE).
- BLE row shows a device-name string under a "BLE" label; on the render-perf branch BLE is reportedly disabled, so this reads as the device id, not an active BLE state. Mildly ambiguous label, not a layout bug.
- All link fields live and correct (WIFI STA / SSID yey-net / IP / SIGNALK live). No stale/zero values, no clipping, no overflow. Bars match their percentages.

Verdict: MINOR
