"""The firmware's UART protocol: commands, read-back and telemetry."""

import re
import time

import serial

from .model import TICK_S

TEL = re.compile(r"^([0-9A-F]{2}) (\d+) (\d+)$")


class Fan:
    """One command per line, answered "ok" or "err". With telemetry on, the
    firmware also sends "<seq hex> <rpm> <duty permille>" lines, which can
    arrive in between."""

    def __init__(self, port="/dev/ttyUSB0"):
        self.s = serial.Serial(port, 9600, timeout=0.05)
        # The CH340's RTS=1 with DTR=0 is the one state that cuts the
        # board's power (the flashing circuit), so keep both released.
        self.s.rts = self.s.dtr = False
        self.buf = b""
        self.pending = []
        time.sleep(0.3)
        self.s.reset_input_buffer()

    def close(self):
        self.s.close()

    def lines(self, secs):
        """Yield decoded lines for secs seconds."""
        end = time.monotonic() + secs
        while time.monotonic() < end:
            self.buf += self.s.read(4096)
            while b"\n" in self.buf:
                line, self.buf = self.buf.split(b"\n", 1)
                yield line.decode(errors="replace").strip()

    def wait(self, secs):
        for _ in self.lines(secs):
            pass

    def cmd(self, line, timeout=2.0):
        """Send a command and return its reply lines. Telemetry that arrives
        meanwhile is kept in self.pending for record()."""
        self.s.write((line + "\r\n").encode())
        reply = []
        for got in self.lines(timeout):
            if TEL.match(got):
                self.pending.append(got)
                continue
            if not got:
                continue
            reply.append(got)
            if got in ("ok", "err") or got.startswith("rpm "):
                break
        if not reply or reply[-1] == "err":
            raise RuntimeError(f"{line!r} -> {reply}")
        return reply

    def status(self):
        f = self.cmd("R")[-1].split()
        return dict(rpm=int(f[1]), duty=int(f[3]), set=int(f[5]), mode=f[6], pwm_us=int(f[8]))

    def load(self, period_us, kp, ki, kd, table):
        """PWM period, gains and feedforward table in one go."""
        self.cmd(f"F {period_us}")
        self.cmd(f"K {kp} {ki} {kd}")
        self.cmd("C")
        for i, (rpm, permille) in enumerate(table[:12]):
            self.cmd(f"L {i} {rpm} {permille}")

    def record(self, secs, every=1, at=()):
        """Telemetry for secs seconds, one sample every <every> ticks.

        Returns (t, rpm, duty) rows. t comes from the sequence numbers, not
        the host clock, so USB latency stays out of it and a dropped line
        shows as a gap. at: (seconds, command) pairs sent on the way.
        """
        self.pending = []
        self.cmd(f"T {every}")
        rows, seq0, wraps, last = [], None, 0, None
        actions = sorted(at)
        start = time.monotonic()
        while time.monotonic() - start < secs:
            while actions and time.monotonic() - start >= actions[0][0]:
                self.cmd(actions.pop(0)[1])
            got = self.pending + list(self.lines(0.05))
            self.pending = []
            for line in got:
                m = TEL.match(line)
                if not m:
                    continue
                seq = int(m.group(1), 16)
                if seq0 is None:
                    seq0 = seq
                if last is not None and seq < last:
                    wraps += 1
                last = seq
                n = seq - seq0 + 256 * wraps
                rows.append((round(n * every * TICK_S, 3), int(m.group(2)), int(m.group(3))))
        self.cmd("T 0")
        return rows
