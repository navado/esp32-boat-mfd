#!/usr/bin/env python3
"""Serial stability soak that stresses the per-screen subscription path.

Cycles the active screen on an interval (exercising slice-3 subscribe/
unsubscribe churn) while continuously monitoring the serial console for
reboots, crash panics, heap-leak trend, and SignalK stalls. Prints a
PASS/FAIL verdict at the end.
"""
import re
import sys
import time
import serial

PORT = sys.argv[1] if len(sys.argv) > 1 else "/dev/cu.usbserial-110"
DURATION = int(sys.argv[2]) if len(sys.argv) > 2 else 1200  # seconds
CYCLE = int(sys.argv[3]) if len(sys.argv) > 3 else 15       # screen-switch interval

SCREENS = ["nav", "wind", "depth", "steering", "route", "wind_steer",
           "autopilot", "trip", "status", "dashboard", "wind_classic"]

REBOOT = re.compile(r"rst:0x|ESP-ROM|entry 0x|boot:0x|RTC_SW_SYS_RST")
CRASH = re.compile(r"Guru Meditation|Backtrace:|abort\(\)|StoreProhibited|"
                   r"LoadProhibited|panic|assert failed|__stack_chk_fail|CORRUPT HEAP")
HEAP = re.compile(r"int_free=(\d+)")
SUBS = re.compile(r"\[sk\] subs \+(\d+) -(\d+) \(active=(\d+)\)")

ser = serial.Serial(PORT, 115200, timeout=1)
time.sleep(0.5)
ser.reset_input_buffer()

reboots = crashes = sk_disc = subs_events = 0
heaps = []
min_active = None
start = time.time()
last_cycle = 0.0
last_report = 0.0
si = 0

print(f"# serial soak: {DURATION}s, screen-cycle every {CYCLE}s on {PORT}", flush=True)

while time.time() - start < DURATION:
    now = time.time()
    el = int(now - start)

    if now - last_cycle >= CYCLE:
        ser.write(("\r\nscreen " + SCREENS[si % len(SCREENS)] + "\r\n").encode())
        ser.flush()
        si += 1
        last_cycle = now

    line = ser.readline().decode("utf-8", "replace").rstrip()
    if line:
        if REBOOT.search(line):
            reboots += 1
            print(f"!! [{el}s] REBOOT: {line}", flush=True)
        if CRASH.search(line):
            crashes += 1
            print(f"!! [{el}s] CRASH: {line}", flush=True)
        if "WS disconnected" in line:
            sk_disc += 1
        m = HEAP.search(line)
        if m:
            heaps.append((el, int(m.group(1))))
        m = SUBS.search(line)
        if m:
            subs_events += 1
            a = int(m.group(3))
            min_active = a if min_active is None else min(min_active, a)

    if now - last_report >= 60:
        h = heaps[-1][1] // 1024 if heaps else 0
        print(f"   [{el}s] heap={h}k reboots={reboots} crashes={crashes} "
              f"sk_disc={sk_disc} subs_events={subs_events}", flush=True)
        last_report = now

ser.close()

# ---- verdict ----
print("\n=== SOAK VERDICT ===", flush=True)
h0 = heaps[0][1] if heaps else 0
hN = heaps[-1][1] if heaps else 0
hmin = min(h for _, h in heaps) if heaps else 0
leak = (h0 - hN)
print(f"duration:      {int(time.time() - start)}s")
print(f"reboots:       {reboots}")
print(f"crashes:       {crashes}")
print(f"sk_disconnects:{sk_disc}")
print(f"subs_events:   {subs_events}  (min active paths seen: {min_active})")
print(f"heap first/min/last: {h0//1024}k / {hmin//1024}k / {hN//1024}k  (drift {leak//1024}k)")
ok = (reboots == 0 and crashes == 0 and hmin > 20 * 1024 and leak < 12 * 1024)
print("VERDICT:", "PASS" if ok else "FAIL", flush=True)
sys.exit(0 if ok else 1)
