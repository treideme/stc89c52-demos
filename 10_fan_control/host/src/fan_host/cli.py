"""fan: characterize the fan on 10_fan_control, fit a model, tune and test the loop.

    fan status                      one read-back line
    fan sweep --period 1000         RPM against duty, up then down
    fan step  --period 1000         duty steps, each fitted to FOPDT
    fan tune  --period 1000         PI gains and a feedforward table from
                                    the sweep and steps, loaded into the board
    fan track --period 1000         closed-loop setpoint steps
    fan plot                        redraw every plot from the saved data

Everything is written to --out (default: char/), as CSV, JSON and PNG.
"""

import argparse
import csv
import json
from pathlib import Path

import numpy as np

from . import plots
from .link import Fan
from .model import TICK_S, ff_table, fit_fopdt, simc_pi, step_metrics


def save_csv(path, header, rows):
    with open(path, "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(header)
        w.writerows(rows)


def cmd_status(fan, args):
    print(fan.status())


def cmd_sweep(fan, args):
    fan.cmd(f"F {args.period}")
    duties = list(range(0, 1001, args.step))
    plan = [("up", d) for d in duties] + [("down", d) for d in reversed(duties)]
    rows = []
    fan.cmd("D 0")
    fan.wait(3)
    for direction, d in plan:
        fan.cmd(f"D {d}")
        fan.wait(args.settle)
        rpm = np.array([r[1] for r in fan.record(args.avg, every=5)], float)
        rows.append(
            (args.period, direction, d, round(rpm.mean(), 1), round(rpm.std(), 1), len(rpm))
        )
        print(
            f"{direction:4} {d / 10:5.1f}%  {rpm.mean():7.1f} rpm  sd {rpm.std():5.1f}", flush=True
        )
    fan.cmd("D 0")
    save_csv(
        args.out / f"sweep_{args.period}us.csv",
        ["period_us", "direction", "duty_permille", "rpm", "rpm_sd", "n"],
        rows,
    )
    plots.sweeps(args.out)


def cmd_step(fan, args):
    fan.cmd(f"F {args.period}")
    fits = []
    for pair in args.steps.split(","):
        u0, u1 = (int(x) for x in pair.split(":"))
        fan.cmd(f"D {u0}")
        fan.wait(args.settle)
        rows = fan.record(args.pre + args.post, every=1, at=[(args.pre, f"D {u1}")])
        arr = np.asarray(rows, float)
        t, d = arr[:, 0], arr[:, 2]
        # The duty changed between the last sample at u0 and the first at u1.
        k = int(np.argmax(np.abs(d - d[0]) > 0.5))
        t_step = float((t[k - 1] + t[k]) / 2) if k > 0 else args.pre
        f = fit_fopdt(t, arr[:, 1], t_step)
        f.update(u0=u0, u1=u1, t_step=t_step, k=(f["rpm1"] - f["rpm0"]) / (u1 - u0))
        fits.append(f)
        save_csv(
            args.out / f"step_{args.period}us_{u0}_{u1}.csv", ["t_s", "rpm", "duty_permille"], rows
        )
        print(
            f"{u0 / 10:5.1f}% -> {u1 / 10:5.1f}%: K {f['k']:5.2f} rpm/permille  "
            f"tau {f['tau']:.3f} s  theta {f['theta']:.3f} s  rms {f['rms']:4.0f} rpm",
            flush=True,
        )
    fan.cmd("D 0")
    with open(args.out / f"steps_{args.period}us.json", "w") as fh:
        json.dump(fits, fh, indent=1)
    plots.steps(args.out, args.period)


def cmd_tune(fan, args):
    fits = json.load(open(args.out / f"steps_{args.period}us.json"))
    # Up-steps only: those are what the loop drives. Size for the highest
    # gain, the shortest time constant and the longest dead time seen, so
    # the loop is stable everywhere; the feedforward carries the curve.
    up = [f for f in fits if f["u1"] > f["u0"] and f["u0"] > 0] or fits
    k = max(abs(f["k"]) for f in up)
    tau = min(f["tau"] for f in up)
    theta = max(f["theta"] for f in up) + TICK_S
    gains = simc_pi(k, tau, theta, args.tc or None)
    rows = list(csv.DictReader(open(args.out / f"sweep_{args.period}us.csv")))
    table = ff_table([int(r["duty_permille"]) for r in rows], [float(r["rpm"]) for r in rows])
    summary = dict(
        period_us=args.period, k_rpm_per_permille=k, tau_s=tau, theta_s=theta, **gains, ff=table
    )
    print(json.dumps(summary, indent=1))
    with open(args.out / f"tune_{args.period}us.json", "w") as fh:
        json.dump(summary, fh, indent=1)
    if fan:
        fan.load(args.period, gains["kp_q16"], gains["ki_q16"], 0, table)
        print("loaded into the board")


def cmd_track(fan, args):
    if args.load:
        s = json.load(open(args.out / f"tune_{args.period}us.json"))
        fan.load(args.period, s["kp_q16"], s["ki_q16"], 0, s["ff"])
    sps = [int(x) for x in args.setpoints.split(",")]
    fan.cmd(f"S {sps[0]}")
    fan.wait(6)
    lead = 2.0
    at = [(lead + i * args.hold, f"S {sp}") for i, sp in enumerate(sps[1:])]
    rows = fan.record(lead + args.hold * (len(sps) - 1), every=1, at=at)
    fan.cmd("D 0")
    tag = args.tag or "track"
    save_csv(args.out / f"{tag}_{args.period}us.csv", ["t_s", "rpm", "duty_permille"], rows)

    def sp_of_t(x):
        i = int((x - lead) // args.hold) + 1 if x >= lead else 0
        return sps[min(i, len(sps) - 1)]

    arr = np.asarray(rows, float)
    report = []
    for i, sp in enumerate(sps[1:]):
        t0 = lead + i * args.hold
        seg = (arr[:, 0] >= t0) & (arr[:, 0] < t0 + args.hold)
        report.append(step_metrics(arr[seg, 0] - t0, arr[seg, 1], sp, sps[i]))
        print(report[-1], flush=True)
    with open(args.out / f"{tag}_{args.period}us.json", "w") as fh:
        json.dump(report, fh, indent=1)
    plots.track(args.out, tag, args.period, rows, sp_of_t)


def cmd_plot(fan, args):
    plots.sweeps(args.out)
    for p in args.out.glob("steps_*us.json"):
        plots.steps(args.out, int(p.stem.split("_")[1].removesuffix("us")))


def main():
    p = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    p.add_argument("--port", default="/dev/ttyUSB0")
    p.add_argument("--out", type=Path, default=Path("char"))
    sub = p.add_subparsers(dest="cmd", required=True)
    sub.add_parser("status")
    s = sub.add_parser("sweep")
    s.add_argument("--period", type=int, default=1000, help="PWM period, us")
    s.add_argument("--step", type=int, default=50, help="duty step, permille")
    s.add_argument("--settle", type=float, default=5.0)
    s.add_argument("--avg", type=float, default=2.0)
    s = sub.add_parser("step")
    s.add_argument("--period", type=int, default=1000)
    s.add_argument(
        "--steps",
        default="0:500,200:400,400:600,600:800,800:600,600:400,400:200,500:0",
        help="duty pairs in permille, from:to",
    )
    s.add_argument("--settle", type=float, default=6.0)
    s.add_argument("--pre", type=float, default=1.0)
    s.add_argument("--post", type=float, default=8.0)
    s = sub.add_parser("tune")
    s.add_argument("--period", type=int, default=1000)
    s.add_argument("--tc", type=float, default=0.0, help="closed-loop time constant, s")
    s.add_argument("--offline", action="store_true", help="compute only, do not load")
    s = sub.add_parser("track")
    s.add_argument("--period", type=int, default=1000)
    s.add_argument("--setpoints", default="2000,4000,6000,3000,1500,5000")
    s.add_argument("--hold", type=float, default=6.0)
    s.add_argument("--load", action="store_true", help="load the tune_*.json gains first")
    s.add_argument("--tag", default="")
    sub.add_parser("plot")
    args = p.parse_args()

    args.out.mkdir(parents=True, exist_ok=True)
    offline = args.cmd == "plot" or (args.cmd == "tune" and args.offline)
    fan = None if offline else Fan(args.port)
    try:
        {
            "status": cmd_status,
            "sweep": cmd_sweep,
            "step": cmd_step,
            "tune": cmd_tune,
            "track": cmd_track,
            "plot": cmd_plot,
        }[args.cmd](fan, args)
    finally:
        if fan:
            fan.close()
