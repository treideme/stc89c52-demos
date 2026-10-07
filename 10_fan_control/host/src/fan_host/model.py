"""The fan as a control problem: model fit, feedforward table, PI tuning.

No I/O here, so all of it is testable without the board.

After a duty step from u0 to u1 at t = 0 the speed is modelled as first order
plus dead time (FOPDT):

    rpm(t) = rpm0 + (rpm1 - rpm0) * (1 - exp(-(t - theta) / tau)),  t > theta

with the static gain K = (rpm1 - rpm0) / (u1 - u0) in RPM per permille, tau
the time constant and theta the dead time.
"""

import numpy as np

TICK_S = 0.020  # the firmware's control and telemetry tick
DUTY_MAX = 1024  # firmware duty counts at 100%


def fopdt(ts, rpm0, rpm1, tau, theta):
    """Model response at times ts relative to the step."""
    ts = np.asarray(ts, float)
    rise = np.where(ts > theta, 1.0 - np.exp(-(ts - theta) / tau), 0.0)
    return rpm0 + (rpm1 - rpm0) * rise


def fit_fopdt(t, y, t_step, pre_s=0.5):
    """Least-squares FOPDT fit: grid search over tau and theta, with rpm0 and
    rpm1 solved exactly at each grid point (the model is linear in them).

    t, y: samples; t_step: when the duty changed. Uses the samples from
    pre_s before the step on. Fitting rpm1 rather than averaging the tail
    keeps a slow step that has not quite settled from biasing the gain.
    """
    t = np.asarray(t, float)
    y = np.asarray(y, float)
    use = t - t_step > -pre_s
    ts, yu = t[use] - t_step, y[use]

    thetas = np.arange(0.0, 0.60 + 1e-9, 0.005)
    taus = np.geomspace(0.03, 10.0, 200)
    best = (np.inf, 0, 0, 0.0, 0.0)
    syy = float((yu * yu).sum())
    for i, th in enumerate(thetas):
        b = 1.0 - np.exp(-np.maximum(ts - th, 0.0)[None, :] / taus[:, None])  # rise
        a = 1.0 - b
        saa, sab, sbb = (a * a).sum(1), (a * b).sum(1), (b * b).sum(1)
        say, sby = a @ yu, b @ yu
        det = saa * sbb - sab * sab
        ok = det > 1e-9
        r0 = np.where(ok, (sbb * say - sab * sby) / np.where(ok, det, 1.0), 0.0)
        r1 = np.where(ok, (saa * sby - sab * say) / np.where(ok, det, 1.0), 0.0)
        sse = np.where(ok, syy - r0 * say - r1 * sby, np.inf)
        j = int(np.argmin(sse))
        if sse[j] < best[0]:
            best = (float(sse[j]), i, j, float(r0[j]), float(r1[j]))
    sse, i, j, rpm0, rpm1 = best
    rms = float(np.sqrt(max(sse, 0.0) / len(yu)))
    return dict(rpm0=rpm0, rpm1=rpm1, tau=float(taus[j]), theta=float(thetas[i]), rms=rms)


def ff_table(duty, rpm, points=10, min_rpm=200.0):
    """Feedforward (rpm, permille) pairs for the firmware's L command.

    duty, rpm: a static sweep (any order, repeats allowed: up and down runs
    are averaged). Starts at (0, 0); then the lowest duty from which the fan
    turns at min_rpm or more in every run at that duty and above, and points
    evenly spaced in speed up to full duty. Near the stall a fan may start
    on the way up and stop on the way down at the same duty, and such a
    point cannot anchor the table. Speed is made strictly increasing first,
    so a noisy sweep cannot produce a table that inverts.
    """
    by = {}
    for d, r in zip(duty, rpm, strict=True):
        by.setdefault(int(d), []).append(float(r))
    d = np.array(sorted(by))
    r = np.array([np.mean(by[k]) for k in d])
    turns = np.array([min(by[k]) >= min_rpm for k in d])
    stopped = np.nonzero(~turns)[0]
    first = stopped[-1] + 1 if len(stopped) else 0
    if first >= len(d):
        raise ValueError("the fan never turned in this sweep")
    d, r = d[first:], r[first:]
    keep = np.concatenate(([True], r[1:] > np.maximum.accumulate(r)[:-1]))
    d, r = d[keep], r[keep]
    targets = np.linspace(r[0], r[-1], points)
    table = [(round(float(x)), round(float(np.interp(x, r, d)))) for x in targets]
    return [(0, 0), *table]


def simc_pi(k_rpm_per_permille, tau, theta, tc=None):
    """PI gains by Skogestad's SIMC rules, in the firmware's units.

    theta should already include the extra tick of the sampled loop. tc is
    the desired closed-loop time constant; the default, max(tau/2, 2*theta),
    roughly halves the open-loop time constant without approaching the
    dead-time limit. Returns physical gains and the Q16 integers for K.
    """
    if tc is None:
        tc = max(tau / 2.0, 2.0 * theta)
    k_counts = k_rpm_per_permille * 1000.0 / DUTY_MAX  # rpm per duty count
    kc = tau / (k_counts * (tc + theta))  # duty counts per rpm
    ti = min(tau, 4.0 * (tc + theta))
    kp = round(kc * 65536)
    ki = round(kc / ti * TICK_S * 65536)
    if not (0 <= kp <= 32767 and 0 <= ki <= 32767):
        raise ValueError(f"gains out of the firmware's range: kp {kp}, ki {ki}")
    return dict(tc=tc, kc=kc, ti=ti, kp_q16=kp, ki_q16=ki)


def step_metrics(ts, ys, sp, prev, band_frac=0.02, band_min=20.0, tail_s=1.0):
    """Settling time into a +-2% band, overshoot, and the settled mean/sd.

    ts: times since the setpoint change; ys: speeds; prev: the previous
    setpoint, which says which way overshoot points.
    """
    ts = np.asarray(ts, float)
    ys = np.asarray(ys, float)
    band = max(band_frac * sp, band_min)
    outside = np.nonzero(np.abs(ys - sp) > band)[0]
    settle = float(ts[outside[-1]]) if len(outside) else 0.0
    over = (ys.max() - sp) if sp > prev else (sp - ys.min())
    tail = ys[ts >= ts.max() - tail_s]
    return dict(
        sp=sp,
        prev=prev,
        settle_s=round(settle, 2),
        overshoot_rpm=round(float(max(0.0, over))),
        mean=round(float(tail.mean())),
        sd=round(float(tail.std()), 1),
    )
