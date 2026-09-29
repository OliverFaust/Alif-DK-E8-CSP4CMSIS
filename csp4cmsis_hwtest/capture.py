#!/usr/bin/env python3
"""Record the DK-E8 console (UART) of one regression run into a log file.

usage: capture.py <port> <logfile> <timeout_s> [header lines...]

Writes the header lines first (host-side facts: configuration, library
commit, toolchain versions), then everything the board prints, until the
SUMMARY line and EOT (0x04), or until <timeout_s> seconds pass without a
SUMMARY (the log then ends with "HWTEST: TIMEOUT").
Exit status: 0 = SUMMARY seen, 1 = timeout, 2 = serial error.
"""
import sys
import time

import serial


def main():
    if len(sys.argv) < 4:
        print(__doc__, file=sys.stderr)
        return 2
    port, logfile, timeout_s = sys.argv[1], sys.argv[2], float(sys.argv[3])
    header = sys.argv[4:]
    try:
        ser = serial.Serial(port, 115200, timeout=0.5)
    except serial.SerialException as e:
        print(f"capture: cannot open {port}: {e}", file=sys.stderr)
        return 2
    seen_summary = False
    deadline = time.monotonic() + timeout_s
    line = b""
    with open(logfile, "w", encoding="utf-8", errors="replace") as log:
        for h in header:
            log.write(f"# {h}\n")
        log.flush()
        while time.monotonic() < deadline:
            data = ser.read(256)
            if not data:
                if seen_summary:
                    break
                continue
            for b in data:
                if b == 0x04 and seen_summary:          # EOT after SUMMARY: run complete
                    deadline = 0
                    break
                if b in (0x0D, 0x04):
                    continue
                if b == 0x0A:
                    text = line.decode("utf-8", errors="replace")
                    log.write(text + "\n")
                    log.flush()
                    print(text, flush=True)
                    if text.startswith("SUMMARY:"):
                        seen_summary = True
                    line = b""
                else:
                    line += bytes([b])
        if line:
            log.write(line.decode("utf-8", errors="replace") + "\n")
        if not seen_summary:
            log.write(f"HWTEST: TIMEOUT after {timeout_s:.0f} s without a SUMMARY line\n")
    ser.close()
    return 0 if seen_summary else 1


if __name__ == "__main__":
    sys.exit(main())
