#!/usr/bin/env python3
"""Discover an espdisp MFD on the local network or over BLE.

Resolution order (each step capped by --timeout, default 5 s):

  1. mDNS `_espdisp._tcp` - the firmware's own service advertisement.
     Carries IP + TXT records (device_id, board, firmware, version).
  2. mDNS `_arduino._tcp` - ArduinoOTA's service. Carries IP + the
     OTA port (3232 by default).
  3. BLE NUS - scan for advertisers whose name starts with `espdisp`,
     connect, send `ip`, parse the `ip=<x.x.x.x>` line from the log
     stream (the device echoes log lines over NUS notifications).

Output:
  - Default: prints `<ip>` on stdout (one line, no trailing data) so
    callers can do `IP=$(tools/discover_device.py)`.
  - `--json`: prints `{"ip","name","port","source","txt"}` on stdout.

Filtering:
  - `--name <substring>` matches case-insensitively against the mDNS
    instance name and the BLE advertised name. With multiple devices
    on the same lab this is how you pick one.
  - `--method auto|mdns|ble` skips the other transport.

Exit codes:
  - 0 on success.
  - 1 if no device was found within the timeout(s).
"""
from __future__ import annotations

import argparse
import asyncio
import json
import re
import socket
import sys
import time
from dataclasses import asdict, dataclass, field
from typing import Optional


@dataclass
class Device:
    ip: str
    name: str = ""
    port: int = 0
    source: str = ""
    txt: dict = field(default_factory=dict)


def _match_name(name: str, needle: Optional[str]) -> bool:
    if not needle:
        return True
    return needle.lower() in (name or "").lower()


def discover_mdns(service: str, timeout: float, name_filter: Optional[str]) -> Optional[Device]:
    """Browse one _<service>._tcp.local. service for `timeout` seconds.

    Returns the first record whose instance name matches `name_filter`
    (or any record if no filter). Returns None on timeout.
    """
    try:
        from zeroconf import ServiceBrowser, ServiceListener, Zeroconf
    except ImportError:
        print("zeroconf not installed; skipping mDNS (pip install zeroconf)",
              file=sys.stderr)
        return None

    found: list[Device] = []

    class Listener(ServiceListener):
        def add_service(self, zc: "Zeroconf", typ: str, name: str) -> None:
            info = zc.get_service_info(typ, name, timeout=int(timeout * 1000))
            if not info:
                return
            # Prefer IPv4 addresses; the firmware speaks v4 only.
            addrs = info.parsed_scoped_addresses() or info.parsed_addresses()
            ipv4 = next((a for a in addrs if ":" not in a), None)
            if not ipv4:
                return
            txt = {}
            for k, v in (info.properties or {}).items():
                try:
                    txt[k.decode() if isinstance(k, bytes) else k] = (
                        v.decode() if isinstance(v, bytes) else v
                    )
                except UnicodeDecodeError:
                    continue
            instance = name.split(f"._{service}._tcp.")[0]
            if not _match_name(instance, name_filter) and not _match_name(
                    txt.get("device_id", ""), name_filter):
                return
            found.append(
                Device(ip=ipv4, name=instance, port=info.port or 0,
                       source=f"mdns:{service}", txt=txt))

        def update_service(self, *a, **kw):  # noqa: D401 - zeroconf API
            pass

        def remove_service(self, *a, **kw):  # noqa: D401 - zeroconf API
            pass

    zc = Zeroconf()
    try:
        ServiceBrowser(zc, f"_{service}._tcp.local.", Listener())
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            if found:
                # Brief settle window so a slightly-slower second
                # responder isn't missed when the filter is ambiguous.
                time.sleep(0.2)
                break
            time.sleep(0.1)
    finally:
        zc.close()
    return found[0] if found else None


async def discover_ble(timeout: float, name_filter: Optional[str]) -> Optional[Device]:
    """Scan for `espdisp*` BLE advertisers, then query each over NUS
    for its IP. Returns the first device that reports a valid IPv4."""
    try:
        from bleak import BleakClient, BleakScanner
    except ImportError:
        print("bleak not installed; skipping BLE (pip install bleak)",
              file=sys.stderr)
        return None

    NUS_RX = "6e400002-b5a3-f393-e0a3-9f4dd9e3a05a"
    NUS_TX = "6e400003-b5a3-f393-e0a3-9f4dd9e3a05a"
    ip_re = re.compile(r"ip=(\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3})")

    devices = await BleakScanner.discover(timeout=timeout)
    candidates = [
        d for d in devices
        if (d.name or "").lower().startswith("espdisp")
        and _match_name(d.name or "", name_filter)
    ]
    if not candidates:
        return None

    for d in candidates:
        ip_holder: dict[str, str] = {}
        try:
            async with BleakClient(d) as client:
                got = asyncio.Event()

                def on_notify(_: int, data: bytearray) -> None:
                    text = data.decode("utf-8", "replace")
                    m = ip_re.search(text)
                    if m and m.group(1) != "0.0.0.0":
                        ip_holder["ip"] = m.group(1)
                        got.set()

                await client.start_notify(NUS_TX, on_notify)
                await client.write_gatt_char(NUS_RX, b"ip", response=False)
                try:
                    await asyncio.wait_for(got.wait(), timeout=timeout)
                except asyncio.TimeoutError:
                    continue
                return Device(ip=ip_holder["ip"], name=d.name or "",
                              source="ble")
        except Exception as exc:  # noqa: BLE001 - best-effort scan
            print(f"ble: {d.name} ({d.address}): {exc}", file=sys.stderr)
            continue
    return None


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--method", choices=("auto", "mdns", "ble"), default="auto",
                    help="Discovery transport (default: auto - mDNS then BLE).")
    ap.add_argument("--name", default=None,
                    help="Filter by name substring (matches mDNS instance, "
                         "device_id TXT, or BLE adv name).")
    ap.add_argument("--timeout", type=float, default=5.0,
                    help="Per-method timeout in seconds (default: 5).")
    ap.add_argument("--json", action="store_true",
                    help="Emit full JSON record instead of just the IP.")
    args = ap.parse_args()

    dev: Optional[Device] = None
    if args.method in ("auto", "mdns"):
        for svc in ("espdisp", "arduino"):
            dev = discover_mdns(svc, args.timeout, args.name)
            if dev:
                break
    if not dev and args.method in ("auto", "ble"):
        dev = asyncio.run(discover_ble(args.timeout, args.name))

    if not dev:
        print("no espdisp device found", file=sys.stderr)
        return 1

    if args.json:
        print(json.dumps(asdict(dev)))
    else:
        print(dev.ip)
    return 0


if __name__ == "__main__":
    sys.exit(main())
