# Changelog

All notable changes are documented here. Format roughly follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and the project
uses [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- Initial public release scaffolding (LICENSE, README, CI, release workflow)
- ESP32-4848S040 board support (ST7701 RGB panel + GT911 touch)
- SignalK WebSocket client subscribing to navigation / wind / depth / battery
- 2×2 LVGL dashboard at 480×480, 5 Hz refresh
- WiFi STA + AP fallback, ArduinoOTA, mDNS
- BLE Nordic UART service for logs + console commands
- UDP log broadcast on port 9999
- `tools/ble_console.py` — host-side BLE debug client
- `tools/fake_boat.py` — synthetic SignalK data pusher for development
- Host-side unit tests for the SignalK delta parser
