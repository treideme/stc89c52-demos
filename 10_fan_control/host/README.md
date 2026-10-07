# fan-host

Host side of `10_fan_control`: characterize the fan over the board's UART, fit a
first-order-plus-dead-time model, derive PI gains and a feedforward table,
and test the closed loop.

```sh
cd 10_fan_control/host
uv run fan status
uv run fan --out char sweep --period 1000     # RPM against duty, up and down
uv run fan --out char step --period 1000      # duty steps, fitted
uv run fan --out char tune --period 1000      # gains + feedforward, loaded
uv run fan --out char track --period 1000     # setpoint steps, scored
uv run pytest && uv run ruff check
```

The serial port needs the `dialout` group. Results go to `--out` as CSV, JSON
and PNG.
