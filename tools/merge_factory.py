# PlatformIO post-build action: merge the standard ESP32-S3 build artifacts
# (bootloader, partitions, boot_app0, firmware) into a single factory image at
# 0x0 that ESP Web Tools / esptool can flash in one part. Output:
# .pio/build/<env>/<progname>-factory.bin
Import("env")  # noqa: F821

import os


def _find_boot_app0(env):
    """Locate the framework boot_app0.bin used to seed the OTA-select slot.

    Only needed as a fallback when FLASH_EXTRA_IMAGES is empty; normally
    PlatformIO already injects boot_app0 at 0xe000 via that list.
    """
    candidates = []
    fw_dir = env.subst("$FRAMEWORK_DIR") or ""
    if fw_dir:
        candidates.append(os.path.join(fw_dir, "tools", "partitions", "boot_app0.bin"))
    pkg_dir = env.PioPlatform().get_package_dir("framework-arduinoespressif32") or ""
    if pkg_dir:
        candidates.append(os.path.join(pkg_dir, "tools", "partitions", "boot_app0.bin"))
    for c in candidates:
        if c and os.path.exists(c):
            return c
    return ""


def _norm_off(off):
    """Normalise an offset (int or str like '0x8000'/'0xe000') to an int."""
    if isinstance(off, int):
        return off
    return int(str(off), 0)


def merge_factory(source, target, env):
    build_dir = env.subst("$BUILD_DIR")
    prog = env.subst("$PROGNAME")
    app_bin = os.path.join(build_dir, prog + ".bin")
    boot = os.path.join(build_dir, "bootloader.bin")
    parts = os.path.join(build_dir, "partitions.bin")
    extra = env.get("FLASH_EXTRA_IMAGES", [])
    out = os.path.join(build_dir, prog + "-factory.bin")

    # Build an offset -> image map. FLASH_EXTRA_IMAGES on this platform already
    # carries bootloader (0x0), partitions (0x8000), and boot_app0 (0xe000), so
    # we MUST NOT also prepend our own bootloader/partitions or esptool reports
    # an overlap at 0x0. Seed from FLASH_EXTRA_IMAGES, then fill any gap.
    images = {}
    for off, img in extra:
        images[_norm_off(off)] = img
    if extra:
        print("[merge_factory] FLASH_EXTRA_IMAGES: %s"
              % ", ".join("0x%x:%s" % (_norm_off(o), os.path.basename(i))
                          for o, i in extra))
    else:
        print("[merge_factory] FLASH_EXTRA_IMAGES empty")

    # Ensure bootloader + partitions are present (they always are on this
    # platform, but stay robust for setups that only expose boot_app0).
    if 0x0 not in images and os.path.exists(boot):
        images[0x0] = boot
    if 0x8000 not in images and os.path.exists(parts):
        images[0x8000] = parts
    # Seed the OTA-select slot if the platform didn't already.
    if 0xe000 not in images:
        boot_app0 = _find_boot_app0(env)
        if boot_app0:
            images[0xe000] = boot_app0
            print("[merge_factory] fallback boot_app0 at 0xe000: %s" % boot_app0)
        else:
            print("[merge_factory] WARNING: boot_app0.bin not found; "
                  "OTA-select slot will be unseeded")
    # The application image.
    images[0x10000] = app_bin

    args = []
    for off in sorted(images):
        args += ["0x%x" % off, images[off]]

    flash_size = env.BoardConfig().get("upload.flash_size", "16MB")
    mcu = env.BoardConfig().get("build.mcu", "esp32s3")
    flash_mode = env.BoardConfig().get("build.flash_mode", "qio")
    esptool_dir = env.PioPlatform().get_package_dir("tool-esptoolpy") or ""
    esptool = '"%s" "%s"' % (env.subst("$PYTHONEXE"), os.path.join(esptool_dir, "esptool.py"))

    quoted = " ".join('"%s"' % a if a.endswith(".bin") else a for a in args)

    def _run(subcmd):
        cmd = ('%s --chip %s %s -o "%s" --flash_mode %s --flash_size %s %s'
               % (esptool, mcu, subcmd, out, flash_mode, flash_size, quoted))
        print("[merge_factory] " + cmd)
        return env.Execute(cmd)

    # merge_bin was renamed to merge-bin in newer esptool; try the underscore
    # form first, fall back to the hyphen form if it's rejected as unknown.
    rc = _run("merge_bin")
    if rc != 0 or not os.path.exists(out):
        print("[merge_factory] merge_bin failed (rc=%s); retrying merge-bin" % rc)
        rc = _run("merge-bin")

    if rc == 0 and os.path.exists(out):
        print("[merge_factory] wrote %s (%d bytes)" % (out, os.path.getsize(out)))
    else:
        print("[merge_factory] WARNING: merge failed (rc=%s)" % rc)


env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", merge_factory)  # noqa: F821
