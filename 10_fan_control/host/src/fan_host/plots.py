"""Plots of sweeps, step fits and closed-loop runs, as PNG files."""

import csv
import json
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402
import numpy as np  # noqa: E402

from .model import fopdt  # noqa: E402

DATASHEET = ([25, 50, 75, 100], [1800, 3700, 5600, 7500])  # at 25 kHz


def sweeps(out: Path):
    fig, ax = plt.subplots(figsize=(8, 5))
    for p in sorted(out.glob("sweep_*us.csv")):
        rows = list(csv.DictReader(open(p)))
        hz = 1e6 / int(rows[0]["period_us"])
        for direction, style in (("up", "-o"), ("down", "--x")):
            r = [x for x in rows if x["direction"] == direction]
            ax.plot(
                [int(x["duty_permille"]) / 10 for x in r],
                [float(x["rpm"]) for x in r],
                style,
                ms=3,
                label=f"{hz:.0f} Hz, duty {direction}",
            )
    ax.plot(*DATASHEET, "ks", label="datasheet, 25 kHz")
    ax.set_xlabel("duty, %")
    ax.set_ylabel("RPM")
    ax.set_title("Fan-4020-PWM-5V: speed against PWM duty")
    ax.legend(fontsize=7)
    ax.grid(alpha=0.3)
    fig.tight_layout()
    fig.savefig(out / "sweeps.png", dpi=120)
    plt.close(fig)


def steps(out: Path, period_us: int):
    fits = json.load(open(out / f"steps_{period_us}us.json"))
    fig, ax = plt.subplots(figsize=(9, 5))
    for f in fits:
        rows = np.loadtxt(
            out / f"step_{period_us}us_{f['u0']}_{f['u1']}.csv", delimiter=",", skiprows=1
        )
        t = rows[:, 0] - f["t_step"]
        ts = np.linspace(-0.5, t.max(), 400)
        (line,) = ax.plot(
            t, rows[:, 1], ".", ms=2, label=f"{f['u0'] / 10:.0f}->{f['u1'] / 10:.0f}%"
        )
        ax.plot(
            ts,
            fopdt(ts, f["rpm0"], f["rpm1"], f["tau"], f["theta"]),
            "-",
            color=line.get_color(),
            lw=1,
        )
    ax.set_xlabel("time after the duty step, s")
    ax.set_ylabel("RPM")
    ax.set_title(f"Duty steps at {1e6 / period_us:.0f} Hz PWM, with first-order fits")
    ax.legend(fontsize=7)
    ax.grid(alpha=0.3)
    fig.tight_layout()
    fig.savefig(out / f"steps_{period_us}us.png", dpi=120)
    plt.close(fig)


def track(out: Path, tag: str, period_us: int, rows, sp_of_t):
    arr = np.asarray(rows, float)
    t = arr[:, 0]
    fig, (a1, a2) = plt.subplots(2, 1, figsize=(9, 6), sharex=True)
    a1.plot(t, [sp_of_t(x) for x in t], "k--", lw=1, label="setpoint")
    a1.plot(t, arr[:, 1], lw=1, label="measured")
    a1.set_ylabel("RPM")
    a1.legend(fontsize=7)
    a1.grid(alpha=0.3)
    a2.plot(t, arr[:, 2] / 10, lw=1)
    a2.set_ylabel("duty, %")
    a2.set_xlabel("s")
    a2.grid(alpha=0.3)
    a1.set_title(f"Closed loop at {1e6 / period_us:.0f} Hz PWM")
    fig.tight_layout()
    fig.savefig(out / f"{tag}_{period_us}us.png", dpi=120)
    plt.close(fig)
