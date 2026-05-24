# Changelog

All notable changes are documented here. Format roughly follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and the project
uses [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.1.0] - 2026-05-24

### Added
- ESP32-4848S040 board support (ST7701 RGB panel + GT911 touch)
- SignalK WebSocket client subscribing to navigation / wind / depth / battery / tank levels
- 2×2 LVGL dashboard at 480×480, 5 Hz refresh
- WiFi STA + AP fallback, ArduinoOTA, mDNS
- BLE Nordic UART service for logs + console commands
- UDP log broadcast on port 9999
- Multi-target `logf()` (Serial / BLE / UDP)
- `tools/ble_console.py` — host-side BLE debug client
- `tools/fake_boat.py` — synthetic SignalK data pusher for development
- `tools/dump_chunked.sh` — chunked, resumable full-flash backup
- Host-portable `signalk_parser` with 11 unit tests on PlatformIO `native` env
- `Makefile` exposing the development pipeline
- GitHub Actions: CI (build + test + lint) and tag-triggered release with binaries

### License
- PolyForm Noncommercial 1.0.0 — free for personal, research, and noncommercial use; commercial use requires a separate license

[Unreleased]: https://github.com/navado/esp32-boat-mfd/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/navado/esp32-boat-mfd/releases/tag/v0.1.0
