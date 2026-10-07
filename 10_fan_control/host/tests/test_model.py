import numpy as np
import pytest

from fan_host.model import TICK_S, ff_table, fit_fopdt, fopdt, simc_pi, step_metrics


def sampled_step(rpm0, rpm1, tau, theta, t_step=1.0, secs=9.0, noise=0.0, seed=1):
    t = np.arange(0.0, secs, TICK_S)
    y = fopdt(t - t_step, rpm0, rpm1, tau, theta)
    if noise:
        y = y + np.random.default_rng(seed).normal(0.0, noise, len(t))
    return t, y


@pytest.mark.parametrize("tau,theta", [(0.4, 0.05), (1.2, 0.10), (2.0, 0.0)])
def test_fit_recovers_a_clean_step(tau, theta):
    t, y = sampled_step(1500.0, 4500.0, tau, theta)
    f = fit_fopdt(t, y, 1.0)
    assert f["rpm0"] == pytest.approx(1500.0, abs=5)  # tau grid steps are 3%
    assert f["rpm1"] == pytest.approx(4500.0, rel=0.01)
    assert f["tau"] == pytest.approx(tau, rel=0.05)
    assert f["theta"] == pytest.approx(theta, abs=0.011)


def test_fit_with_noise_and_a_down_step():
    t, y = sampled_step(6000.0, 3000.0, 0.8, 0.06, noise=40.0)
    f = fit_fopdt(t, y, 1.0)
    assert f["tau"] == pytest.approx(0.8, rel=0.15)
    assert f["theta"] == pytest.approx(0.06, abs=0.03)
    assert f["rms"] == pytest.approx(40.0, rel=0.2)


def test_ff_table_is_monotonic_and_starts_at_the_threshold():
    duty = list(range(0, 1001, 50))
    # Stalls below 10%, then a noisy rising curve with one dip.
    rpm = [0, 0] + [76 * d / 10 - 100 for d in duty[2:]]
    rpm[10] -= 400
    t = ff_table(duty * 2, rpm * 2, points=8)
    assert t[0] == (0, 0)
    assert t[1][1] == 100  # lowest duty that turns it
    assert all(b[0] >= a[0] and b[1] >= a[1] for a, b in zip(t, t[1:], strict=False))
    assert t[-1] == (7500, 1000)


def test_ff_table_skips_a_start_that_does_not_hold():
    # At 5% the fan started on the way up but not on the way down.
    duty = [0, 50, 100, 150, 1000] * 2
    rpm = [0, 175, 63, 526, 7500] + [0, 41, 0, 560, 7500]
    t = ff_table(duty, rpm, points=3)
    assert t[1] == (543, 150)


def test_ff_table_rejects_a_dead_fan():
    with pytest.raises(ValueError):
        ff_table([0, 500, 1000], [0, 0, 0])


def test_simc_units():
    # 7.5 rpm per permille, tau 1 s, theta 0.1 s, tc 0.5 s:
    # kc = 1 / (7.32 * 0.6) counts per rpm, ti = min(1, 2.4) = 1 s.
    g = simc_pi(7.5, 1.0, 0.1, tc=0.5)
    kc = 1.0 / (7.5 * 1000 / 1024 * 0.6)
    assert g["kc"] == pytest.approx(kc)
    assert g["ti"] == pytest.approx(1.0)
    assert g["kp_q16"] == round(kc * 65536)
    assert g["ki_q16"] == round(kc * TICK_S * 65536)


def test_simc_refuses_gains_the_firmware_cannot_hold():
    with pytest.raises(ValueError):
        simc_pi(0.01, 5.0, 0.01, tc=0.01)


def test_step_metrics():
    ts = np.arange(0.0, 6.0, TICK_S)
    ys = fopdt(ts, 2000.0, 4000.0, 0.5, 0.0)
    m = step_metrics(ts, ys, 4000, 2000)
    # Inside 2% (80 rpm) once exp(-t/0.5) < 0.04: t > 1.61 s.
    assert m["settle_s"] == pytest.approx(1.6, abs=0.03)
    assert m["overshoot_rpm"] == 0
    assert m["mean"] == 4000
