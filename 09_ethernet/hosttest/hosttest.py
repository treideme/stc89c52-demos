"""Exercise the board's network stack over a real cable.

Run against a board flashed with a net_*_rung*.hex image:

    uv run hosttest.py                 # ping + UDP echo, default 169.254.194.20
    uv run hosttest.py --ip 192.168.7.2 --count 20
    uv run hosttest.py --no-ping       # UDP only (rung 3+)

Exit status is 0 only if every requested test passes, so it works as a gate.

Why ping goes through the system `ping` command: an ICMP echo from Python
needs a raw socket, which on Windows needs administrator. `ping.exe` needs
neither, and this is a bring-up tool, not a library.

Measured on the bench (STC89C52RC at 12 MHz, bit-banged SPI ~1.8 kB/s):
ICMP round trip ~60 ms, UDP echo 50-85 ms for 10-64 B. That is the SPI, not
the network: every byte crosses the link twice at ~250 us per byte.
"""

from __future__ import annotations

import argparse
import platform
import socket
import statistics
import subprocess
import sys
import time

DEFAULT_IP = "169.254.194.20"       # netcfg.h NET_STATIC_IP for the bench build
UDP_ECHO_PORT = 7                    # netcfg.h NET_UDP_ECHO_PORT


def ping(ip: str, count: int, timeout_ms: int = 1500) -> tuple[int, int]:
    """(replies, sent) using the system ping."""
    if platform.system() == "Windows":
        cmd = ["ping", "-n", str(count), "-w", str(timeout_ms), ip]
        marker = f"Reply from {ip}"
    else:
        cmd = ["ping", "-c", str(count), "-W", str(max(1, timeout_ms // 1000)), ip]
        marker = "bytes from"
    out = subprocess.run(cmd, capture_output=True, text=True).stdout
    return sum(marker in line for line in out.splitlines()), count


def udp_echo(ip: str, payloads: list[bytes], timeout: float = 3.0):
    """[(sent_len, ok, rtt_ms or None)] against the UDP echo port."""
    results = []
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
        for payload in payloads:
            # Drain anything still in flight first. The board answers in
            # 50-85 ms, so a reply that missed its timeout would otherwise be
            # handed to the NEXT recvfrom() and every later exchange would
            # report a mismatch - which is exactly what happened on the first
            # run of this script.
            s.setblocking(False)
            try:
                while True:
                    s.recvfrom(2048)
            except (BlockingIOError, socket.timeout, OSError):
                pass
            s.settimeout(timeout)

            t0 = time.perf_counter()
            s.sendto(payload, (ip, UDP_ECHO_PORT))
            deadline = t0 + timeout
            matched, rtt = False, None
            while time.perf_counter() < deadline:
                try:
                    data, addr = s.recvfrom(2048)
                except socket.timeout:
                    break
                if data == payload and addr[0] == ip:
                    matched = True
                    rtt = (time.perf_counter() - t0) * 1000.0
                    break
                # a stale reply to an earlier payload: keep waiting for ours
            results.append((len(payload), matched, rtt))
    return results


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--ip", default=DEFAULT_IP)
    ap.add_argument("--count", type=int, default=8, help="ICMP echoes to send")
    ap.add_argument("--no-ping", action="store_true")
    ap.add_argument("--no-udp", action="store_true")
    args = ap.parse_args()

    ok = True
    print(f"target {args.ip}")

    if not args.no_ping:
        replies, sent = ping(args.ip, args.count)
        good = replies == sent
        ok &= good
        print(f"  ICMP  : {replies}/{sent} replies      {'PASS' if good else 'FAIL'}")

    if not args.no_udp:
        # 1 byte, an odd length (checksum padding), and a chunk larger than
        # the firmware's copy buffer, which is 32 B below rung 5.
        payloads = [b"x", b"hello 8051", bytes(range(37)), b"A" * 64, b"Z" * 100]
        results = udp_echo(args.ip, payloads)
        good = all(r[1] for r in results)
        ok &= good
        rtts = [r[2] for r in results if r[2] is not None]
        for n, matched, rtt in results:
            state = f"{rtt:6.1f} ms" if matched else ("MISMATCH" if rtt else "TIMEOUT")
            print(f"  UDP   : {n:4d} B echoed  {state}")
        if rtts:
            print(f"  UDP   : median {statistics.median(rtts):.1f} ms over "
                  f"{len(rtts)} exchanges")
        print(f"  UDP   : {sum(r[1] for r in results)}/{len(results)} exact "
              f"{'PASS' if good else 'FAIL'}")

    print("RESULT:", "PASS" if ok else "FAIL")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
