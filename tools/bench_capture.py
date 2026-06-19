#!/usr/bin/env python3
"""Send a console command over USB serial and capture bench output.

Usage:
  tools/bench_capture.py [--port /dev/cu.usbserial-110] [--cmd bench-sweep]
                         [--timeout 180] [--out results.csv]

Waits briefly for the device, sends <cmd>\\n, then streams lines until it sees
"[bench-sweep] complete" (or "[bench] lat" for plain `bench`) or the timeout
elapses. Extracts the CSV (header + rows) from the [bench-sweep] log lines and
writes it to --out if given. Echoes everything to stdout.
"""
import argparse
import sys
import time

import serial  # pyserial


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", default="/dev/cu.usbserial-110")
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--cmd", default="bench-sweep")
    ap.add_argument("--timeout", type=float, default=180.0)
    ap.add_argument("--settle", type=float, default=2.0, help="seconds to drain before sending")
    ap.add_argument("--out", default=None, help="write extracted CSV here")
    args = ap.parse_args()

    ser = serial.Serial(args.port, args.baud, timeout=1)
    time.sleep(0.3)
    # Drain any boot/log backlog so our capture starts clean.
    t_drain = time.time()
    while time.time() - t_drain < args.settle:
        ser.read(4096)
    ser.reset_input_buffer()

    ser.write((args.cmd + "\n").encode())
    ser.flush()
    print(f"[capture] sent: {args.cmd!r}; reading up to {args.timeout}s", file=sys.stderr)

    csv_lines = []
    deadline = time.time() + args.timeout
    done_marker = "[bench-sweep] complete"
    buf = b""
    while time.time() < deadline:
        chunk = ser.read(4096)
        if not chunk:
            continue
        buf += chunk
        while b"\n" in buf:
            raw, buf = buf.split(b"\n", 1)
            line = raw.decode("utf-8", "replace").rstrip("\r")
            print(line)
            tag = "[bench-sweep] "
            if tag in line:
                payload = line.split(tag, 1)[1]
                # header line starts with "screen,mode," ; data rows contain commas
                if payload.startswith("screen,mode,") or ("," in payload and
                                                           not payload.startswith("start:") and
                                                           not payload.startswith("complete")):
                    csv_lines.append(payload)
            if done_marker in line:
                deadline = 0  # break outer
                break

    ser.close()
    if args.out and csv_lines:
        with open(args.out, "w") as f:
            f.write("\n".join(csv_lines) + "\n")
        print(f"[capture] wrote {len(csv_lines)} CSV lines -> {args.out}", file=sys.stderr)
    elif args.out:
        print("[capture] no CSV lines captured", file=sys.stderr)


if __name__ == "__main__":
    main()
