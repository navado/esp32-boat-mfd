# Slice 4 — Merged factory image build target Implementation Plan

> REQUIRED SUB-SKILL: superpowers:subagent-driven-development.

**Goal:** Produce a single merged factory `.bin` (bootloader + partitions + boot_app0 + app) from the `esp32-4848s040` build, so ESP Web Tools (Slice 5) can flash it at offset `0x0` in one part. (Improv is dropped — provisioning uses the firmware's existing serial commands via a manager WebSerial console in Slice 5/7.)

**Architecture:** A PlatformIO `post:` extra-script that, after a successful build, runs `esptool merge_bin` over the build artifacts into `.pio/build/<env>/<name>-factory.bin`. Pure build tooling; no firmware source change.

**Tech Stack:** PlatformIO build scripting (SCons/Python), esptool (bundled with the espressif32 platform), `pio run -e esp32-4848s040`.

**Spec:** `docs/superpowers/specs/2026-06-18-manager-device-console-design.md` §4 (merged image).

**Repo:** `/Users/borissorochkin/code/embedded/espdisp` (`main`). Note: this is build-verifiable here (the merged .bin appears); the actual USB flash of it is exercised on hardware in Slice 5/8.

---

## File Structure
- **Create** `tools/merge_factory.py` — PlatformIO post-build action that emits the merged factory image.
- **Modify** `platformio.ini` — append `post:tools/merge_factory.py` to the `esp32-4848s040` env's `extra_scripts`.

---

## Task 1: Merged-image post-build script

**Files:** Create `tools/merge_factory.py`; Modify `platformio.ini`.

- [ ] **Step 1 — read** how `extra_scripts = pre:tools/version.py` is set on `[env:esp32-4848s040]` (line ~51). `extra_scripts` takes a list; we add a `post:` script. Read `tools/version.py` for the `Import("env")` PlatformIO scripting idiom.

- [ ] **Step 2 — write `tools/merge_factory.py`:**

```python
# PlatformIO post-build action: merge the standard ESP32-S3 build artifacts
# (bootloader, partitions, boot_app0, firmware) into a single factory image at
# 0x0 that ESP Web Tools / esptool can flash in one part. Runs after the .bin
# is built. Output: .pio/build/<env>/<progname>-factory.bin
Import("env")  # noqa: F821

import os

def merge_factory(source, target, env):
    build_dir = env.subst("$BUILD_DIR")
    prog = env.subst("$PROGNAME")  # e.g. firmware
    app_bin = os.path.join(build_dir, prog + ".bin")
    boot = os.path.join(build_dir, "bootloader.bin")
    parts = os.path.join(build_dir, "partitions.bin")
    # boot_app0 ships with the framework; PlatformIO exposes it via FLASH_EXTRA_IMAGES
    extra = env.get("FLASH_EXTRA_IMAGES", [])  # list of (offset, path) tuples
    out = os.path.join(build_dir, prog + "-factory.bin")

    # Build the (offset, image) argument list. Standard ESP32-S3 layout:
    #   0x0     bootloader
    #   0x8000  partitions
    #   <extra> boot_app0 (and any others) from FLASH_EXTRA_IMAGES
    #   0x10000 app
    args = ["0x0", boot, "0x8000", parts]
    for off, img in extra:
        args += [str(off), img]
    args += ["0x10000", app_bin]

    flash_size = env.BoardConfig().get("upload.flash_size", "16MB")
    mcu = env.BoardConfig().get("build.mcu", "esp32s3")
    esptool = env.subst("$PYTHONEXE") + " " + '"' + os.path.join(
        env.PioPlatform().get_package_dir("tool-esptoolpy") or "", "esptool.py") + '"'

    cmd = (f'{esptool} --chip {mcu} merge_bin -o "{out}" '
           f'--flash_mode {env.BoardConfig().get("build.flash_mode", "qio")} '
           f'--flash_size {flash_size} ' + " ".join(f'"{a}"' if a.endswith(".bin") else a for a in args))
    print("[merge_factory] " + cmd)
    rc = env.Execute(cmd)
    if rc == 0 and os.path.exists(out):
        print(f"[merge_factory] wrote {out} ({os.path.getsize(out)} bytes)")
    else:
        print("[merge_factory] WARNING: merge failed (rc=%s) — ESP Web Tools image not produced" % rc)

# Run after the app .bin is built.
env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", merge_factory)  # noqa: F821
```

(Read PlatformIO's env API as needed — `FLASH_EXTRA_IMAGES` is the standard way PlatformIO exposes boot_app0 + offsets; `tool-esptoolpy` is the esptool package. If `FLASH_EXTRA_IMAGES` is empty in this setup, fall back to including the framework's `boot_app0.bin` at `0xe000` from the framework package dir — but try FLASH_EXTRA_IMAGES first; it's the supported path.)

- [ ] **Step 3 — wire it in** `platformio.ini`: change the `[env:esp32-4848s040]` line `extra_scripts = pre:tools/version.py` to:
```
extra_scripts =
    pre:tools/version.py
    post:tools/merge_factory.py
```

- [ ] **Step 4 — build + verify the artifact:**
Run: `pio run -e esp32-4848s040` (use `~/.platformio/penv/bin/pio` if needed).
Expected: SUCCESS, and the `[merge_factory]` line prints, and `.pio/build/esp32-4848s040/firmware-factory.bin` exists and is larger than `firmware.bin` (it includes bootloader+partitions+boot_app0+app — typically ~app size + ~64KB).
Verify: `ls -l .pio/build/esp32-4848s040/firmware.bin .pio/build/esp32-4848s040/firmware-factory.bin` — factory is bigger.
Sanity the merged image is a valid esptool image: `~/.platformio/penv/bin/python -m esptool --chip esp32s3 image_info .pio/build/esp32-4848s040/firmware-factory.bin 2>&1 | head` (or `python .../esptool.py image_info ...`) — it should parse without "invalid" errors (a merged_bin is a raw flash image; `image_info` may warn it's not a single app image — acceptable as long as it doesn't error hard; the real proof is the size + that merge_bin returned 0).

- [ ] **Step 5 — native tests unaffected:** `pio test -e native` → still all pass (this change is build-only).

- [ ] **Step 6 — commit:** `git commit -am "build(fw): emit merged factory image for ESP Web Tools (esptool merge_bin)"`
(Use `git add tools/merge_factory.py platformio.ini` then commit, to avoid sweeping unrelated dirty files.)

---

## Self-Review
- **Spec §4 (merged image) coverage:** post-build merge into a single factory .bin at 0x0. ✓
- **Improv dropped** per the spec update (provisioning via existing serial commands in the manager). ✓
- **Build-verifiable:** the merged .bin appears + is larger than the app bin; merge_bin returns 0. On-device flash of it is Slice 5/8.
- **No firmware source change**, no native-test impact.
- **No placeholders:** full script + exact platformio.ini change + verification commands.
