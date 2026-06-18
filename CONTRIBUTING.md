# Contributing

Thanks for your interest in `esp32-boat-mfd`. This is a small project
maintained in spare hours, so a few simple norms keep it healthy.

## Reporting bugs

Open a GitHub issue using the bug template. Helpful information:

- Board variant (silkscreen markings, e.g. `ESP32-4840S040`)
- Firmware version (from `git rev-parse --short HEAD` or a release tag)
- Output of `sk-status`, `sk-dump`, `ip` over BLE/Serial
- Photo of the screen if rendering is wrong
- Steps to reproduce

## Proposing changes

1. Open an issue first if it's non-trivial — saves wasted work.
2. Fork, create a feature branch from `main`.
3. Keep PRs scoped. One feature/fix per PR.
4. Conventional commit messages (`feat:`, `fix:`, `refactor:`, `docs:`, `chore:`, `test:`) — used by release notes generation.
5. Run `pio test -e native` and `pio run` before pushing.
6. PR template will ask for: what changed, why, how tested.

## Code style

- C++17 for firmware, Python 3.9+ for tools.
- 4-space indent, no tabs.
- LLVM-ish brace style (`if (x) {` on the same line).
- Comments only where the *why* isn't obvious. Avoid restating *what*.

## Development workflow

```sh
# Native unit tests
pio test -e native

# Firmware build (no upload)
pio run

# OTA upload to a running device
pio run -e ota -t upload --upload-port <ip>

# Serial monitor (if CH340 is cooperating)
pio device monitor

# BLE console (works without serial/WiFi)
python3 tools/ble_console.py sk-status
```

## Adding a SignalK path

1. Add the field to `sk::Data` in `include/signalk_parser.h`
2. Add the path → field mapping in `applyDelta()` in `src/signalk_parser.cpp`
3. Add a test case in `test/test_parser/test_parser.cpp`
4. Subscribe to it from `signalk.cpp::subscribe()`
5. (Optionally) render it in `main.cpp::ui_refresh`

## Adding a new board

1. New `include/board_pins_<board>.h` with all GPIO definitions
2. New `[env:<board>]` in `platformio.ini` with a `-D BOARD_<NAME>` flag
3. Switch on the macro in `main.cpp` to include the right pins file
4. Add a CI matrix entry for it in `.github/workflows/ci.yml`

## Releases

Maintainers cut releases by pushing a `v*` tag (e.g. `v0.2.0`).
The `release.yml` workflow builds the firmware, generates release notes
from commit messages, and attaches `firmware.bin` + `merged_firmware.bin`
to the GitHub release.

## License of contributions

This project is source-available under the [PolyForm Noncommercial License
1.0.0](LICENSE) and is **dual-licensed**: the maintainer also offers commercial
licenses (see [COMMERCIAL.md](COMMERCIAL.md)). For that to work, every
contribution must come with the right to include it under both the noncommercial
and the commercial license.

We use the **Developer Certificate of Origin (DCO)** — no separate CLA to sign.
Certify each commit by adding a `Signed-off-by` line:

```
git commit -s -m "your message"
```

which appends:

```
Signed-off-by: Your Name <your.email@example.com>
```

By signing off you agree to the DCO below. Use your real name and a reachable
email. PRs whose commits are not signed off cannot be merged.

## Developer Certificate of Origin 1.1

```
By making a contribution to this project, I certify that:

(a) The contribution was created in whole or in part by me and I have the right
    to submit it under the open source license indicated in the file; or

(b) The contribution is based upon previous work that, to the best of my
    knowledge, is covered under an appropriate open source license and I have the
    right under that license to submit that work with modifications, whether
    created in whole or in part by me, under the same open source license (unless
    I am permitted to submit under a different license), as indicated in the file; or

(c) The contribution was provided directly to me by some other person who
    certified (a), (b) or (c) and I have not modified it.

(d) I understand and agree that this project and the contribution are public and
    that a record of the contribution (including all personal information I submit
    with it, including my sign-off) is maintained indefinitely and may be
    redistributed consistent with this project or the open source license(s) involved.
```
