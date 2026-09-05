# STC89C5x blog project ideas

**45 ranked ideas** for posts on the HC6800-ES V2.0 / STC89C5x — thirty-one
derived here (§3) and fourteen elaborated from the staged queue (§3b) — with a
summary table (§4), a gap analysis (§5) including a reconciliation against what
is already published or declined (§5.4), and a verdict on which of the 15 books
earn their place (§6).

**Status:** research only. Nothing here was written to `svelte-blog`, this
repository or `stc8demo`. Read-only git throughout.

**Date:** 2026-09-03, extended 2026-09-04 after a read-only pass over the
Anytype blog queue — that pass retired one top-ten idea, demoted another, and
added the cheapest post in the document. The Ibrahim and Pont findings from an
earlier run are carried forward and were not re-derived.

> ⚠️ **The Anytype blog page is upstream** for what is already written and what
> has been declined; this file is the derivative. When they disagree, the page
> wins. Re-read it before promoting anything.

**The 15 books cited as `[EN-165]`, `[Ibrahim]` and so on are under
`docs/refs/8051/`, which is gitignored** — they are third-party PDFs and are
not redistributable. The citations are page-accurate against those editions.

---

## 0. What this is grounded in

Where a number appears below, it came from one of these — not from judgement:

| Source | What it settled |
| --- | --- |
| The **official HC6800-ES V2.0 schematic** (16-page Puzhong original, plus the 1-sheet revision already in this repository's `doc/`) | the real pin map, confirmed against the vendor's own C header, not inferred from board class |
| **STC89C51RC/RD+ datasheet**, English 165 pp and 271 pp editions plus the Chinese 271 pp edition | 6T mechanism, IAP, watchdog, RAM split, port drive, wake-up sources — with page citations |
| **`stcgal` 1.10** source and its `doc/reverse-engineering/` captures (present in two of your own venvs) | the ISP option byte layout and the STC89 wire protocol |
| **SDCC 4.5.0 + ucsim `s51`**, both installed on the build machine | **measured** cycle counts for ISR entry, XRAM streaming and bit-banged SPI |
| Your 20 posts mentioning 8051/STC/8031, and this repository's own git history | what is already said, and what is already in flight |
| **Your Anytype page "Personal Website / Blog" and the task "Servo/Stepper Blog Article"** (read-only, 2026-09-04) | what you have already **published**, already **declined and why**, already **aborted and why**, and nine staged ideas §3 had never seen — §5.4 |
| 15 8051 books under `docs/refs/8051/` | which ideas have a printed reference behind them, and which are genuinely unwritten |

Five findings reshape everything downstream, so they go first.

### 0.1 The board has four peripherals you have never switched on

The HC6800-ES V2.0 is a Shenzhen Puzhong (深圳普中) design. Confirmed on the
schematic and cross-checked against the vendor's `hc6800v2.h`:

1. **An XPT2046 — a 12-bit SPI ADC.** `U1`, with `CS=P3.5`, `DCLK=P3.6`,
   `DIN=P3.4`, `DOUT=P3.7`. Its four channels are **already wired to real
   sensors**: `X+`/AIN0 = a 5 kΩ potentiometer (`AD1`), `Y+`/AIN1 = an **NTC
   thermistor**, `VBAT`/AIN2 = a **photoresistor** (`GR1`), `AUX`/AIN3 = a
   header. Twelve bits, four channels, three sensors, soldered down.
2. **An analog output.** Not a DAC chip — **PWM on `P2.1` → a 51 kΩ/100 nF RC
   filter → an LM358 (`U16A`) → net `DAC0`**. Which means the board's analog
   output *depends on software PWM working* (idea #12), on a pin that is also
   the I²C clock and LED `D2`. That constraint is more interesting than a DAC
   chip would have been.
3. **A DS1302 RTC** with its own 32.768 kHz crystal — **and no battery holder**
   (`VCC1` and `VCC2` both tie to `VCC`), so it forgets the time the moment
   power drops. That is a defect and therefore a better post than a working RTC.
4. **A 28BYJ-48 stepper drive**: `ULN2003D` `IN1`–`IN4` from **`P1.0`–`P1.3`**
   into a 5-pin header. Note those are **the same four pins as the keypad
   columns**.

Plus an **nRF24L01 header** and an **XPT2046-fed TFT/touch header**, and a
**CH340 with a working STC auto-download power-switch circuit** (DTR/RTS →
8550 → P-MOSFET switching board `VCC`, or a relay on the later revision).

**Correction to a plausible-looking wrong answer:** there is **no PCF8591 and no
ADC0804** on this board. The `AIN0`–`AIN3` / `DAC0` / `VREF` net labels belong
to the XPT2046 and the LM358 output stage. The I²C bus carries only the AT24C02.

### 0.2 6T is a flash-time option byte — settled from two directions

Datasheet, [EN-271] §1.11 p. 22:

> "If STC enhanced 8051 MCU is set to double the speed (6T mode, 6 clocks per
> machine cycle) **in the STC-ISP Writter/Programer when burning program**…"

And there is no runtime alternative: the complete SFR map ([EN-165] pp. 31–33)
contains **no `CLK_DIV`**; grepping all three datasheet editions for
`CLK_DIV|CLKS2` returns zero hits; and `AUXR` (8EH) on STC89 has only two live
bits, `EXTRAM` (bit 1) and `ALEOFF` (bit 0).

It lives in the single MCS0 option byte, which `stcgal` decodes as:

```
bit   7      6     5     4      3      2     1     0
    ~WDEN  XRAM  ALE   OSCG  ~EERE  ~BSLD   0   ~CPU6T
```

`stcgal -P stc89 -o cpu_6t_enabled=true …`. **And the option can only be written
as part of a full programming run** — the flow is handshake → erase → program →
program_options. You cannot flip 6T without reflashing. That single fact decides
how idea #8 has to be written.

Mechanism detail worth stealing: the RC/RD+ core is *physically a 6T core*
([EN-271] pp. 8–9). "12T mode" is a **÷2 prescaler in front of a 6T core**, not
a different core.

Family contrast: STC90C51RC — same option-byte mechanism. STC12C5A60S2 — fixed
1T core with a genuine runtime `CLK_DIV` SFR at 97H. STC15/STC8 — 1T plus a
runtime divider. **Only the STC89/STC90 generation does it at flash time.**

### 0.3 The chip will tell you its own crystal frequency over UART

From `Stc89Protocol.initialize_status`: the bootloader's status packet carries
**eight 16-bit frequency counters**, and

```python
self.mcu_clock_hz = (self.baud_handshake * freq_counter * cpu_t) / 7.0
```

The STC89 bootloader measures the host's bit period against its own oscillator
and reports the result. **You can read an unknown STC89 board's crystal
frequency over a serial cable with no oscilloscope.** I have not found this
written up anywhere, and — see §0.5 — you have a live reason to want it.

### 0.4 Two datasheet facts that constrain half the ideas below

**RAM.** 256 bytes of scratch-pad (128 direct + 128 indirect, sharing addresses
with the SFRs) **plus 256 bytes of on-chip auxiliary XRAM** reached by `MOVX`
([EN-165] §3.2.1 p. 22, p. 24). That is the "512 bytes". Critically, **the RC
series gets 256 bytes of XRAM; the RD+ series gets 1024**. Any framebuffer or
sector-buffer idea lives or dies on those 256 bytes.

`EXTRAM` = `AUXR.1` controls it, default enabled, with an ISP option bit
(`XRAM`, MCS0 bit 6) on top. **`AUXR` is write-only on this family** ([EN-271]
p. 22) — `AUXR |= 0x01` is a broken read-modify-write, and this bites people.

**Port drive.** From the DC table, [EN-271] Appendix C pp. 258–259:

| | P1/P2/P3/P4 | P0/ALE/PSEN |
| --- | --- | --- |
| Sink `IOL` (min) | 4 mA | 8 mA |
| **Source `IOH` (min)** | **150 µA** (typ 220 µA) | — |

**Source current on P1/P2/P3 is 150–220 microamps.** Everything must be
sink-driven. This is the number behind your own 2025.11.01 discussion of LED
buffers, and it is what makes idea #23 (Charlieplexing) a hard problem rather
than a trick.

### 0.5 The crystal is probably 12 MHz, not 11.0592 MHz

The schematic labels the MCU crystal net `12M` with 33 pF loading caps; the
official experiment manual says 外部12MHz晶振 and every vendor delay routine is
commented `@12.000MHz`. But retail listings claim 11.0592 MHz, the vendor's own
example set contains a folder named `17-1 红外遥控(11.0592MHz晶振)`, and on the
original revision the crystal sits in a **3-pin socket**.

Your own posts say both — 2025.11.01 says "12 MHz clock", 2025.11.22 says the
1-Wire timing is "for the HC6800-ES2 system", 2022.06.04 says 1–25 MHz is fine.

**This matters more than it looks.** At 12 MHz you cannot hit standard baud
rates with Timer 1 — 9600 has ~7 % error and will not work — while at
11.0592 MHz every rate is exact. It is the single most likely thing to break a
first UART bring-up, and §0.3 is the trick that resolves it without an
oscilloscope. **Every number in §2 is given for both.**

---

## 1. The HC6800-ES V2.0 pin map

Confirmed from the schematic and the vendor's `hc6800v2.h`, cross-checked
against pin `#define`s in your published demos.

| Pin | Everything wired to it | Free? |
| --- | --- | --- |
| `P0.0`–`P0.7` | 7-seg **segment data** via one 74HC245/74HC573 → 100 Ω arrays; **LCD1602 `DB0`–`DB7`**; LCD12864 data; **8×8 matrix columns** (direct, active-low). 10 kΩ pull-ups fitted (`RP11`+`RP16`). | shared — you have already repurposed `P0.0`–`P0.3` as bit-banged SPI for the ENC28J60 |
| `P1.0`–`P1.3` | keypad **columns** 4→1; **ULN2003 `IN1`–`IN4` → stepper**; `P1.0` also J21 (latch LE) and nRF24 `CE` | shared |
| `P1.4`–`P1.7` | keypad **rows** (`P1.7`=row 1 … `P1.4`=row 4) | shared |
| `P1.5` | **buzzer**, via ULN2003 `IN5`/`OUT5` — *and* keypad row 3, *and* ISP1 `MOSI` | no |
| `P2.0` | **LED `D1`**; **AT24C02 `SDA`**; ultrasonic header; nRF24 `MISO` | no |
| `P2.1` | LED `D2`; **AT24C02 `SCL`**; **the DAC's PWM input** (→ RC → LM358) | no |
| `P2.2` | LED `D3`; **74LS138 `A`** (`LSA`); nRF24 `MOSI` | no |
| `P2.3` | LED `D4`; **74LS138 `B`**; AD/DA header | no |
| `P2.4` | LED `D5`; **74LS138 `C`** | no |
| `P2.5` | LED `D6`; **LCD `RW`** (actually wired — busy-flag polling works) | no |
| `P2.6` | LED `D7`; **LCD `RS`** | no |
| `P2.7` | LED `D8`; **LCD `E`** | no |
| `P3.0` (RxD) | CH340; independent key **`K2`** | no — this is the flash path |
| `P3.1` (TxD) | CH340 via 1N4148; independent key **`K1`** | no |
| `P3.2` (`INT0`) | **IR receiver**; key `K3`; LCD12864 `PSB` | shared |
| `P3.3` (`INT1`) | key `K4`; LCD12864 pin 16; nRF24 `IRQ` | shared |
| `P3.4` | **595 `SER`** · **DS1302 `I/O`** · **XPT2046 `DIN`** · 12864 `RST` | triple-shared |
| `P3.5` | **595 `RCLK`** · **DS1302 `CE`** · **XPT2046 `/CS`** | triple-shared |
| `P3.6` | **595 `SRCLK`** · **DS1302 `SCLK`** · **XPT2046 `DCLK`** · nRF24 `SCK` | triple-shared |
| `P3.7` | **DS18B20 `DQ`** · **XPT2046 `DOUT`** · nRF24 `CSN` | triple-shared |

**Jumpers: there are only three, and none of them free a port.**

| Jumper | Effect |
| --- | --- |
| `JOE` (3-pin) | 74HC595 `/OE` → VCC / OE / GND. **Must be on GND** or the dot matrix and LED bank `D9`–`D16` are dead. |
| `JP595` (2-pin) | cuts power to the 595's LED bank `D9`–`D16` |
| `J21` (3-pin) | `U2` `DIR`/`LE` → VCC (always transparent, default) or `P1.0` (software latch) |

There is **no J-LED and no J-DIG**. The vendor's manual states the shorting caps
were *removed* from this revision. **Freeing `P2` requires cutting a trace or
lifting `RP7`/`RP9`.**

**Two structural consequences that shape the whole study:**

1. **`P2` is quadruple-booked.** The 8-LED bank is permanently on `P2.0`–`P2.7`,
   *on top of* I²C (`P2.0`/`P2.1`), the digit select (`P2.2`–`P2.4`) and the
   LCD control lines (`P2.5`–`P2.7`). Every I²C transaction and every display
   refresh flickers the LEDs. That is not a bug to fix — it is a post (#22).
2. **`P3.4`–`P3.7` carry three devices with no chip selects.** The 74HC595, the
   DS1302 and the XPT2046 share the same three-wire bus. There is one saving
   grace, and it is worth noticing: **DS1302 `CE` is active-high and XPT2046
   `/CS` is active-low on the same pin `P3.5`**, so those two are mutually
   exclusive *by construction*. The 595 is the casualty — clocking either the
   RTC or the ADC will latch garbage into it, so its `RCLK` must be treated as
   hostile territory.
3. **An external memory bus is impossible on this board.** `AD0`–`AD7` needs all
   of `P0` and `A8`–`A15` needs all of `P2`. Anything requiring `MOVX` to
   external RAM has to leave the dev kit — which is itself idea #5.

Your own first-hand hardware finding, worth reusing: 2025.11.15 documents that
**`P0.6` low pulls `P0.5` low** on your unit — a correlated segment failure you
traced to hardware. That is a debugging post on its own (#24).

---

## 2. Measured numbers (SDCC 4.5.0, compiled on this machine)

Cycle counts are read out of SDCC's own `.lst` bracket annotations, which are in
**clock** cycles — divide by 12 for machine cycles in 12T mode.

### 2.1 Interrupt-service-routine cost — measured

| ISR | SDCC output | Clocks | Machine cycles |
| --- | --- | --- | --- |
| 4-channel soft-PWM compare, default register bank | 44 instructions | 642 | **53** |
| 2-channel, `__using(1)` | 26 instructions | 405 | **33** |
| Minimal: clear flag, toggle a pin, `__using(2)` | 3 instructions | 48 | **4** |

Add ~2 MC for the vector `LJMP` and 3–8 MC of hardware interrupt response. The
practical floor for *any* 8051 ISR is **~11 MC**; a naive multi-channel compare
ISR is **~60 MC**.

| | @12 MHz 12T | @12 MHz 6T | @11.0592 MHz 12T | @11.0592 MHz 6T |
| --- | --- | --- | --- | --- |
| minimal ISR (11 MC) | 11.0 µs | 5.5 µs | 11.9 µs | 6.0 µs |
| naive 4-ch ISR (60 MC) | **60.0 µs** | 30.0 µs | **65.1 µs** | 32.6 µs |

Most of the 53-MC body is SDCC's context save, so this is as much a codegen
result as an architectural one — say so in the post.

### 2.2 Streaming bytes to a port

| Loop | MC/byte |
| --- | --- |
| SDCC C, `__xdata` array → `P1` (`--opt-code-speed`) | **20** |
| SDCC C, `__code` array → `P1` | **21** |
| Hand asm, `movx a,@dptr` / `mov P1,a` / `inc dptr` | **5** |

SDCC reloads `DPL`/`DPH` from `R6`/`R7` every iteration and writes them back —
**8 of the 20 cycles are pure DPTR shuffling**, and SDCC does not use the STC89's
second data pointer (`AUXR1` at A2H, bit 0 = `DPS`) that would eliminate it.

**This 4× C-vs-asm gap on the inner loop is the most important codegen caveat in
the study**, because every display, video and mass-storage idea below depends on
that loop.

### 2.3 Bit-banged SPI vs UART mode 0 — the biggest practical finding

| Method | MC/byte | @12 MHz 12T | @11.0592 MHz 12T |
| --- | --- | --- | --- |
| SDCC C bit-bang, MSB-first | **160** (20 MC × 8) | 6.3 kB/s | 5.8 kB/s |
| Hand-asm bit-bang (`rlc a` / `mov P1_0,c`) | ~48 | ~21 kB/s | ~19 kB/s |
| **UART mode 0** (`SBUF=d`; hardware shifts 8 bits at f<sub>osc</sub>/12) | 2 MC to load, then 8 µs of hardware | **125 kB/s** | **115 kB/s** |

UART mode 0 is a **hardware synchronous shift register**: `TXD` is the clock,
`RXD` is the data. That is an SPI master on a chip whose own datasheet
selection table prints `SPI: N`.

Three caveats, all real: mode 0 is **LSB-first** and SPI is MSB-first (a
256-entry `__code` reversal table costs 3 MC); there is **no chip select** (that
stays a GPIO); and **the rate is fixed at f<sub>osc</sub>/12** — `AUXR` on
STC89 has only `EXTRAM` and `ALEOFF`, so there is **no `UART_M0x6`
double-rate bit** and **6T does not speed mode 0 up**. The only way to make
mode 0 faster is a faster crystal, and the part is rated to **48 MHz in 12T**.

**This applies directly to code you have published**: the ENC28J60 driver
(`09_ethernet/enc28j60_cfg.h` bit-bangs SPI on `P0.0`–`P0.3`), the ST7920 driver
where you wrote *"the MCS-51 series does not have SPI peripherals, and
bit-banging SPI is not very efficient on this architecture"* (2025.11.18), the
74HC595 demos, and — once #9 exists — the XPT2046. Note also the commit
`5683697 "Uart not working"` in the ethernet history; this may literally be the
fix.

### 2.4 Scanline budgets

| Standard | Line | Active | MC/line @12 MHz 12T | MC/active | @12 MHz 6T | @11.0592 12T |
| --- | --- | --- | --- | --- | --- | --- |
| PAL | 64.00 µs | 52.0 µs | 64.0 | **52.0** | 128 / 104 | 59.0 / **47.9** |
| NTSC | 63.56 µs | 52.6 µs | 63.6 | 52.6 | 127 / 105 | 58.6 / 48.5 |
| VGA 640×480 | 31.78 µs | 25.4 µs | 31.8 | **25.4** | 63.6 / 50.8 | 29.3 / **23.4** |

Pixels across a 52 µs PAL active line:

| Serialisation | @12 MHz 12T | @12 MHz 6T | @40 MHz 12T |
| --- | --- | --- | --- |
| SDCC C, XRAM → port (20 MC/byte) | 21 | 42 | 69 |
| Hand asm, XRAM → port (5 MC/byte) | 83 | 166 | 277 |
| **UART mode 0 → 74HC595** (fixed at f<sub>osc</sub>/12) | **52** | **52** | **173** |

And the framebuffer that has to feed it, out of **256 bytes of XRAM (RC)** or
**1024 bytes (RD+)**:

| Width | Lines on an STC89C52**RC** | Lines on an STC89C5x**RD+** |
| --- | --- | --- |
| 48 px | 42 | 170 |
| 52 px | 39 | 157 |
| 64 px | 32 | 128 |

Verdicts in idea #3.

### 2.5 Everything else the datasheet fixes

- **Servo resolution.** Timer tick = 1 MC. At 12 MHz 12T that is **1.000 µs**,
  so the 1–2 ms servo range is **1000 discrete steps**; at 6T, 2000; at
  11.0592 MHz 12T, 922. Interrupt-latency jitter is 3–8 MC = **3–8 µs at 12T**,
  ≈0.3–0.8 % of range, roughly 1° of travel.
- **Max external count rate** on `T0`/`T1` is **f<sub>osc</sub>/24** — the input
  is sampled once per machine cycle and needs two cycles to recognise a
  transition. **500 kHz at 12 MHz, in both 6T and 12T** (it is a function of
  f<sub>osc</sub>, not the machine cycle).
- **`MUL AB` / `DIV AB` are 4 machine cycles** and 8-bit. Everything else in the
  instruction set is 1 or 2. Nothing exceeds 4.
- **Watchdog** (`WDT_CONTR` at E1H): timeout = `N × prescale × 32768 / SYSclk`
  with N=12 in 12T, N=6 in 6T. At 11.0592 MHz 12T that is **71.1 ms to
  9.10 s** across the eight prescaler settings; halve in 6T.
- **IAP** on the STC89C52RC: **4 KB of data flash, 8 sectors of 512 bytes**,
  address 0000H–0FFFH, a separate space from the 8 KB program flash. Erase is
  **page/sector only**. Trigger is `0x46` then `0xB9` into `ISP_TRIG`, after
  which **the CPU stalls until the operation completes** — it is not
  interrupt-driven. `ISP_ADDRH` bits [7:5] must be zero. At 11–12 MHz the
  wait-state field must be `WT=001`, i.e. **`ISP_CONTR = 0x81`**.
  **Endurance is not specified anywhere in any of the three datasheet editions**
  — STC quotes 100,000 cycles for STC15/STC8 in marketing material; do not
  assume it carries over.
- **UART multidrop.** Modes 2/3 with `SM2` work as required, *and* STC89 adds
  **hardware address recognition** via `SADDR` (A9H) and `SADEN` (B9H) — slaves
  filter addresses in hardware at zero CPU cost, with broadcast support. Plus
  **frame-error detection**: set `SMOD0` (PCON.6) to remap `SCON.7` as an `FE`
  flag, which is a bus-collision detector for RS-485.
- **Timer 2** is present (`T2CON` C8H, `RCAP2H/L`, vector 002BH) with four
  modes, including **a programmable clock-out on `P1.0`** via `T2OE` — a
  hardware square-wave generator, useful anywhere you need a clock the CPU
  cannot afford to generate.
- **Power-down wake-up sources**: RESET, `/INT0`, `/INT1`, `T0`, `T1`, `RXD`,
  `/INT2`, `/INT3`. **There is no power-down wake-up timer** — the selection
  table's "special timer for waking power down mode" column reads `N` for every
  STC89 part. That is an STC15/STC8 feature.
- **No ADC, no PCA, no PWM, no SPI, no I²C** — [EN-165] Appendix C p. 165 prints
  `N` in every one of those columns for every STC89 part.

---

## 3. Ranked project ideas

Ranking is engagement: nostalgia × novelty × visible artefact × "people assume
this is impossible on 1980s silicon", discounted for effort and for hardware you
does not own. **🟢 marks ideas needing no hardware you do not already have** —
eleven of the thirty-one, four of which use peripherals physically on your board.

Before the list, two things already in flight:

- **A rotary-encoder demo exists**, unpublished: branch `encoder_servo`, commit
  `abf3ee0`, `05_enc_servo/enc.c`, on `P1.5`/`P1.6`/`P1.7`. Rotary encoders are
  therefore not a new idea; the *servo* half of that folder is empty (#12).
- `01_led_button_hyst` exists as a build artefact but not in the published
  README — a hysteresis variant.

---

### #1 — Bring BASIC-52 back: a 1985 Intel BASIC interpreter on a $0.30 STC89C52

**Hook.** Intel shipped an 8 KB ROM BASIC for the 8052AH-BASIC in 1985. The
STC89C52 *is* an 8052 with exactly 8 KB of flash. Put the ROM back where it
belongs and type `PRINT 2+2` into a chip that costs less than a coffee lid.

**Substance.** BASIC-52 wants 8 KB of code space (fits exactly), external RAM for
program text and variables (needs the memory bus — #5), and a console UART whose
divisor it can compute. Covers `MOVX` addressing, `PSEN` vs `RD`, why BASIC-52
wants code and data overlapping, and the option-byte interaction. The payoff is a
terminal session screenshot, which is a shareable artefact.

**Books.** External bus: **Mazidi ch14 p395–418** (74LS373 latch, decoding,
MOVX); **ALE p6, p10** (latch, memory decoding, reading code space) and **p59**
(bank selection past 64 KB); **Walter §3.7 p47–60** (setup/hold arithmetic
proving whether your SRAM part is fast enough); **Limbach §2.1.2 p39–46** (SRAM
timing diagrams); **Mackenzie §11.2 p259–264** (the SBC-51 schematic: 8031 +
EPROMs + 6264 + 8155).

**STC deltas.** Three, all load-bearing. (a) BASIC-52's `PROG` command writes to
EPROM via a programming voltage the STC89 does not have — **replace it with IAP**
(4 KB, 8 sectors of 512 bytes, `ISP_CONTR = 0x81` at this crystal, §2.5) so
`PROG` saves to on-chip data flash. **That substitution is the post.** (b) `EA`
must stay high or you lose the internal flash *and* the ISP bootloader. (c) The
`ale_enabled` option byte must stay set or the address latch never latches.

**Hardware.** Off-board: a 62256, a 74HC573, glue, perfboard. Cannot be done on
the HC6800 (§1).

**Feasibility.** Medium — BASIC-52 sources are freely available and have been
ported before; the `PROG` substitution is the risk.

**Effort.** More than a weekend; a 2–3 post arc with #5.

**Rank 1 because** it is nostalgia-plus-modern-twist in its purest form, it has
an unbeatable screenshot, and the audience arriving on your STC content (your own
analytics, 2025.10.19) will recognise BASIC-52 on sight.

---

### #2 — Flash an STC89C52 from a browser tab: the ISP protocol, then a WebSerial tool

**Hook.** You have already been bitten by the toolchain — 2022.06.04 records that
mainline `stcgal` did not support your bootloader revision and you had to use
`nrife`'s fork. Build a flasher that needs no install at all.

**Honest framing, and it changes the post.** *Reverse-engineering the protocol is
a solved problem.* There are at least five independent implementations —
`stcgal` (Python), `stcflash` (Python, independently derived), `huangguiyang/STC-ISP`
(C, Linux and Windows), `ahappyforest/stc-isp` (C), `CrispStrobe/stcbsl` (Rust).
Writing a sixth CLI flasher is not a story. **What does not exist is a
JavaScript/WebSerial one**, and that is what makes this rank #2 rather than
disappear.

**Substance.** The protocol, documented properly with annotated hex:

```
46 B9 | 6A | LEN_HI LEN_LO | PAYLOAD | CKSUM | 16
```

magic `46 B9`; direction `6A` host→MCU, `68` MCU→host; terminator `16`; `LEN` is
big-endian and equals N+5; `CKSUM = sum(frame[2:]) & 0xFF` — an **8-bit** sum
with **no parity** (STC12+ switched to a 16-bit sum with even parity, which is
the headline difference between protocol generations). Commands: `0x8F` probe
baud · `0x8E` commit baud · `0x80` ping ×4 · `0x84` erase · `0x00` program a
128-byte block · `0x8D` write the option byte · `0x82` reset and run.
Handshake at 2400 baud, host sending `0x7F` every 30 ms.

Then the two details that make it publishable rather than a transcription:

1. **§0.3 — the chip reports its own crystal frequency.** Eight frequency
   counters in the status packet. Given §0.5 (12 MHz or 11.0592 MHz, nobody is
   sure), this is immediately *useful* to your readers, not just cute.
2. **The cold-reset requirement, from STC's own documentation** rather than from
   reverse engineering — [EN-271] §10.1 p. 217: *"Must be cold-reset (power-on
   reset), MCU will run from ISP monitor code; for any warm-reset (include
   reset-pin, watchdog), MCU will run user code directly. … PC application must
   send command at first then power on MCU."* Pulsing `RST` does not work. **Your
   board already solves this** — the CH340's `DTR#`/`RTS#` drives an 8550 into a
   P-MOSFET that switches board `VCC` (a relay on the later revision), which is
   exactly what `stcgal -a --resetpin dtr` expects. **WebSerial can assert DTR**,
   so the browser version can do the power cycle too.

You already ships `MCSTimerTool`, `MCSPortTool`, `MCSExtIntTool` and
`MCSUARTTool` on `/tools`. This is the fifth and the best one.

Known sharp edges worth documenting because they cost people hours: BSL 7.2.5C
parts need `-P stc89a` (different checksum, even parity) **but `stc89a` is
missing from the autodetect table**, so they mis-detect as `stc89` and die with
"packet checksum mismatch"; in released `stcgal` 1.10, `-a` combined with
`-P stc89a` raises a `TypeError`; some BSLs send the first status packet
**without** the `46 B9` magic; and **flash read-back is impossible** — the BSL
has no read command, which is silicon, not tooling.

**Books.** None. Entirely STC-specific, and that is why it ranks.

**Hardware.** 🟢 None.

**Feasibility.** High. Protocol fully documented, `stcgal`'s repo even contains
annotated bidirectional captures of official STC-ISP sessions **against a real
STC89C52RC**.

**Effort.** Evening for the protocol post; weekend for the tool. Split it.

**Rank 2 because** it produces something people *use* rather than read, it
extends an existing feature of your site, and "flash a 1980s microcontroller from
a browser tab" is a headline — with the caveat that it must be sold as a
delivery-mechanism story, not a reverse-engineering one.

---

### #3 — Composite video from an STC89C52: do the arithmetic before the soldering

**Hook.** Everyone has seen AVR TVout. Can a 12-clocks-per-machine-cycle 8051 do
it? Compute the budget first, then build exactly what the budget allows.

**Substance.** §2.4, presented as the post, in four moves:

1. **The CPU cannot be the pixel clock.** Best case is one port write per
   machine cycle = 1 Mpx/s at 12 MHz 12T, and that is with the CPU doing nothing
   else. Worse, a port write puts out 8 *parallel* bits, not 8 serial pixels.
2. **The 74HC595 already on the board is the serialiser, driven by UART mode 0**
   (§2.3) — hardware shifts 8 pixels while the CPU fetches the next byte.
3. **Then the wall is mode 0's fixed rate.** f<sub>osc</sub>/12 = 1 Mbit/s at
   12 MHz = 8 µs per byte = **52 pixels across a 52 µs active line**. There is no
   `UART_M0x6` on STC89 (§2.3), and **6T does not help** — 6T speeds the CPU, not
   the shift clock. **The only lever is the crystal**, and the part is rated to
   48 MHz in 12T: at 40 MHz mode 0 gives 2.4 µs/byte = **173 pixels**. That is
   the genuinely surprising result, and it is the best thing in the post.
4. **Then the wall is RAM.** 256 bytes of XRAM on an STC89C52**RC** (§0.4).
   52×39, or 64×32, and that consumes 100 % of it. **An STC89C54RD+ has 1024
   bytes and gets you 64×128.** So the honest answer to "can an 8051 do video" is
   *"yes, at 64×32, if you pick the right part number and overclock it."*
5. **Sync jitter.** Interrupt response is 3–8 MC = **3–8 µs on a 64 µs line,
   5–13 %**. A picture built on a timer ISR will tear. You need a cycle-counted,
   interrupt-free, fully unrolled loop — the CPU does *only* video, all frame.

**Verdicts, stated with the numbers, which is the point of the post.**

- **PAL/NTSC monochrome: yes, marginally.** 52 px × 32–39 lines at 12 MHz on an
  RC part; 64 × 128 on an RD+; 173 px wide if you fit a 40 MHz crystal. Whole CPU
  consumed, 1 bpp.
- **VGA: no.** 31.8 machine cycles for an entire line at 12 MHz 12T, 25.4 of them
  in the active region. There is no instruction sequence that emits a useful
  pixel stream in 25 instructions, and even at 6T (50.8 MC active) you are
  choosing between generating sync and generating pixels. **Say it with the
  numbers and stop.**

**Books.** Nothing — video is a confirmed gap across all fifteen.

**Hardware.** 🟢 74HC595 on board. A resistor DAC into an RCA jack, plus a TV
that still takes composite. Optionally a 40 MHz crystal (the socket is 3-pin on
the original revision, §0.5).

**Feasibility.** Analysis post: **high**, and publishable on its own. Working
picture: **medium** — better than it looked before the crystal finding.

**Effort.** Evening for the arithmetic. Weekend+ for a picture.

**Rank 3 because** "impossible on 1980s silicon" is the top criterion in the
brief, and because the honest-numbers version is publishable even if the build
fails — a post that cannot lose.

---

### #4 — Your 8051 has a hardware SPI and the datasheet says it doesn't

> **Corroborated from your own notes (§5.4).** You aborted an ENC28J60 Ethernet
> project because *"silicon errata prevents less than 8 MHz interfacing via SPI,
> this thing can effectively not be bitbanged."* That is this post's thesis
> arriving as a dead project: a peripheral with a clock floor is unreachable by
> bit-banging and reachable by UART mode 0. **Lead with the abort.**

**Hook.** [EN-165] Appendix C prints `SPI: N` for every STC89 part. You wrote in
public that *"the MCS-51 series does not have SPI peripherals, and bit-banging
SPI is not very efficient on this architecture."* Both are half true, and the
other half is a 20× speed-up.

**Substance.** §2.3 verbatim, with the SDCC listings and a logic-analyser capture
(you own one and used it in 2025.11.24). **160 machine cycles per byte in C
versus 2 to load and 8 µs of hardware shifting** — 6.3 kB/s to 125 kB/s. Then
retrofit it to code you have already shipped: the ENC28J60 driver, the ST7920
serial driver, the 74HC595 demos, and the XPT2046 of #9.

Cover the three gotchas properly: **LSB-first vs MSB-first** (256-byte `__code`
reversal table, 3 MC); **no chip select** (stays a GPIO — and on this board that
matters, because `P3.5` is simultaneously the 595 latch, the RTC enable and the
ADC select, §1); and **the rate is fixed** at f<sub>osc</sub>/12 with no
double-rate bit and no 6T benefit.

**Books.** **Ayala p189**, **Mackenzie §5.8 p119** and **§11.8 p282–290**,
**Schultz p310** all describe mode 0 as a shift-register mode. **None of them
frame it as "this is your SPI peripheral", and none give throughput numbers.**

**Hardware.** 🟢 None.

**Feasibility.** High — measured above, on this machine.

**Effort.** Evening to write, evening to retrofit one driver.

**Rank 4 because** it is a concrete, measurable, *useful* result on code your
readers have already downloaded, it corrects something you said in public (which
is inherently engaging), and it unblocks #3, #9, #18 and #19.

---

### #5 — Leave the dev kit: build a real 8031-style SBC with an external memory bus

**Hook.** Every one of your 25 demos runs inside 8 KB of flash and 512 bytes of
RAM because the dev board makes the memory bus unreachable (§1). Build the
machine the 8051 was actually designed to be.

**Substance.** `P0` as multiplexed `AD0`–`AD7`, `ALE` into a 74HC573, `P2` as
`A8`–`A15`, `PSEN` vs `RD`/`WR`, a 62256, and the decoding. Then the part almost
nobody writes: **does your SRAM actually meet timing?** Walter does
t<sub>LLIV</sub>/t<sub>AVIV</sub> arithmetic against real datasheet numbers to
prove a given part will or will not work at a given clock. That section is what
makes this engineering rather than a wiring diagram.

**Books.** **Walter §3.7 p47–60 is the sole source for the timing arithmetic**
(§3.7.1 a 16K EPROM on the 80535 p50; §3.7.2 32K RAM p57; §3.7.3 the EURO_535
board p58). Mazidi ch14 p395–418; ALE p6, p10, **p59** (bank switching); Limbach
§2.1.2 p39–46, §2.2 p47 (the 74573), **§2.4 p48–60** (design *and program* a PAL
as the decoder); Mackenzie **p264, p279–281** (**the 8155 RAM+I/O+timer combo
chip — Mackenzie is the only source in fifteen books**); Schultz p39–47, A4 p391;
Ayala ch7 p100–129; Gimenez §2.6–2.7 p80–86.

**STC deltas.** Three, and the first one will cost someone an evening:
1. The RC part has **256 bytes of on-chip XRAM at 00H–FFH**, and *"if address
   over FFH, off-chip external RAM becomes the target automatically"* ([EN-165]
   p. 24). So `MOVX` below 0x0100 silently hits internal RAM. Clear `EXTRAM`
   (`AUXR.1`) to force everything external — and remember **`AUXR` is
   write-only**, so `AUXR |= 0x02` does not work (§0.4).
2. `ale_enabled` is an **option byte**. Disabled for EMI, your latch never latches.
3. `EA` must be high for internal flash; pull it low to boot an external EPROM
   and **you lose ISP entirely**.

**Hardware.** 62256, 74HC573, perfboard or a small PCB. You have published PCB
design work (2023.06.06).

**Feasibility.** High. Well-trodden; the timing arithmetic is the interesting risk.

**Effort.** Weekend, and it is the prerequisite for #1.

**Rank 5 because** it is the one architectural thing you have never done, it
unlocks BASIC-52 and framebuffers, and "will this RAM actually work" arithmetic
essentially does not exist in English on the web.

---

### #6 — Two STC89C52s, one wire, in lockstep: shared-clock scheduling

**Hook.** Two microcontrollers sharing a single wire and a ground, ticking in
deterministic lockstep, with the slave detecting a dead master in ~100 ms.

**Substance.** Pont's **SCI** scheduler, pp. 553–607: tick = a falling edge on a
port pin into `INT0`, two port pins per node, all three timers left free on the
slave, 1 ms ticks via Timer 2, watchdog-based master-loss detection, a
backup-master sketch on p. 560. The framing question — why not just run one
faster chip — is answered by Pont's own sceptical passage §25.5 pp. 550–552:
100 devices at 99.99 % reliability each gives **37 % system reliability**;
adding processors can *reduce* reliability. That is the intellectual spine.

**Books.** **Pont Part F pp. 553–607** with full listings and timing analysis;
**§25.5 pp. 550–552**; ch. 29 **DATA UNION**, **LONG TASK**, **DOMINO TASK**.
Gehlot p123–127 has a naive 8051↔8051 UART link as the comparison.

**STC deltas.** Pont's Timer 2 reloads assume 12T at 12 MHz — which, per §0.5, is
probably exactly what this board is, so **your constants may transfer unchanged**
(worth checking and saying so). The STC89's **option-byte watchdog**
(`watchdog_por_enabled`) with its 71 ms–9.1 s range (§2.5) is a cleaner
master-loss detector than Pont's software version. Timer 2 is confirmed present.

**Hardware.** Two boards, or one plus a bare STC89C52 on a breadboard —
2022.06.04 already documents the minimal bootstrap circuit. One wire, one ground.

**Feasibility.** High. Two port pins and an interrupt.

**Effort.** Weekend.

**Rank 6 because** the previous session's read was right: nobody blogs
multi-8051 networking, it has real depth, and "two 8051s synchronised over one
wire" is a strong hook. It ranks 6 only because it needs a second board and the
artefact is a scope trace.

---

### #7 🟢 — Chapter 4 of your datasheet describes a different chip

**Hook.** The official STC89 English datasheet tells you to configure your pins
as push-pull. There are no port-mode registers on this part. Chapter 4 was
pasted in from the STC12/STC15 documentation and nobody noticed.

**Substance.** A documentation-archaeology post with receipts:

- **Chapter 4, "Configurable I/O Ports Configurations"** (pp. 29–30) claims every
  pin can be set quasi-bidirectional / **push-pull** / input-only / open-drain.
  Grepping all three datasheet editions for `P0M0|P1M0|P2M0|P3M0|P1M1` returns
  **zero hits** — there are no such registers in the SFR map. The identical grep
  on the STC12C5A60S2 datasheet returns ten hits, because *that* part has them.
  **Appendix G ("LED driven by an I/O port") repeats the same false advice.**
- The **Timer 2 example on p. 100** executes `ORL AUXR, #01000000B` with the
  comment "Use Timer 1 in 1T mode". AUXR bit 6 does not exist on STC89, and
  **AUXR is write-only**, so it is a broken read-modify-write of a nonexistent
  bit in a register you cannot read.
- The **watchdog section contradicts itself** — the feature list says
  "one-time-enabled", the body text on p. 17 says *"Clearing `EN_WDT` can stop
  WDT counting."* Both cannot be true. Bench it and report.
- The **English edition gets P0 wrong** ("bi-directional with pull-up
  resistance"); the Chinese §4.1.2 is unambiguous: *"P0口是开漏输出… 作为I/O口用时，
  需加上拉电阻"* — open-drain, needs pull-ups as GPIO.
- The **3.3 V claim conflicts with the DC table**: the feature list says
  "5.5 V ~ 3.3 V", the electrical table says **VDD min 3.8 V** for the C part.
  An STC89C52RC at 3.3 V is out of spec; you want the STC89LE52RC.
- And the number that matters most: **`IOH` on P1/P2/P3 is 150–220 µA** (§0.4).
  Sink-drive only. That is the real reason your LED-buffer discussion in
  2025.11.01 exists, and it is buried in an appendix while Chapter 4 tells you
  to use push-pull.

**Books.** **Limbach p16–22** is the ideal companion — the electrical structure
of 80C51 vs 80C32 ports, TTL vs CMOS inverter behaviour, why pull-ups are needed
and what the drive limits are. Gimenez ch1 (interface-circuit theory, load
driving); Walter p19 ("Empfehlungen für digitale Schaltungen").

**STC deltas.** The entire post is one.

**Hardware.** 🟢 None — a multimeter for the drive-current confirmation.

**Feasibility.** High. Every claim above is verified and citable.

**Effort.** Evening.

**Rank 7 because** documentation-failure posts travel unusually well, this one is
**genuinely useful** rather than merely snarky (people are designing circuits
from Chapter 4 right now), and it is entirely STC-specific — exactly the material
your audience arrives for and no book contains.

---

### #8 — IAP: the chip rewrites its own flash, and takes firmware over the wire

**Hook.** Serial firmware update on a core designed in 1980, with no bootloader
pin, no debugger, and no second chip.

**Substance.** Confirmed spec (§2.5): **4 KB of data flash, 8 sectors of 512
bytes** at 0000H–0FFFH, a space separate from the 8 KB program flash. Two
escalating projects: (a) a **wear-levelled key/value store** across the eight
sectors, replacing the AT24C02 you already uses, with an honest comparison; (b) a
**resident second-stage bootloader** that receives Intel hex over the UART and
rewrites the application region — field update with no `stcgal`.

**Books.** Braithwaite p136–140 (on-chip data EEPROM on the P89LPC932) is the
nearest any book gets; Subrata p248 (program lock bits) for the security angle.
**Self-programming is a confirmed gap across all fifteen.**

**STC deltas.** The whole idea, plus four flags that will otherwise burn a day:
- **Erase is sector-only**, 512 bytes at a time; byte-program only works on a
  byte already at 0xFF.
- **The CPU stalls** during the operation — the datasheet's own code has a `NOP`
  after the trigger with the comment "CPU will hold here until … complete".
  Nothing else runs, including your display refresh.
- **`ISP_CONTR = 0x81`** (`WT=001`) is required at 11–12 MHz. STC's own demo
  code says `#define ENABLE_IAP 0x81 //if SYSCLK<20MHz`. Using the 10 MHz
  setting here is out of spec.
- **Endurance is not specified in any edition of the datasheet.** Say so, derate,
  and do not repeat STC15's 100,000-cycle marketing number.

**Hardware.** 🟢 None.

**Feasibility.** **High** — confirmed against the datasheet, Chapter 9 pp. 124–131.

**Effort.** Weekend; the bootloader half is a second post.

**Rank 8 because** "it reprograms itself" reliably surprises people, a resident
bootloader is a genuinely useful artefact, and the unspecified endurance gives it
an honest edge most vendor-flavoured write-ups lack.

---

### #9 🟢 — You have had a 12-bit ADC on this board the whole time

**Hook.** The datasheet says the STC89C52 has no ADC. It doesn't. **The board
does** — a 12-bit XPT2046, with a potentiometer, a thermistor and a
photoresistor already soldered to its inputs, and it has never appeared in any of
your 25 demos.

**Substance.** §0.1. Then a sequence needing nothing but code:

- A **light meter** on the photoresistor, shown on the 8-digit display.
- A **second thermometer** on the NTC — and its disagreement with the DS18B20 you
  already characterised in 2025.11.22. Steinhart-Hart against a digital sensor,
  with the calibration honestly done, is a genuinely good post.
- The **analog output**, which is where it gets interesting: the board's `DAC0`
  is not a chip, it is **PWM on `P2.1` into an RC filter into an LM358** (§0.1).
  So generating a waveform *requires* #12 — and on a pin that is simultaneously
  the I²C clock and LED `D2`. Producing a clean sine out of that is real work.
- Then close the loop: read `AIN`, write the DAC.

**Books.** Mazidi **ch13 p355–394** is the reference chapter (ADC0804 p357–366,
ADC0808/0809 with channel addressing p362–366, DAC0808 with a sine table
p386–390, LM34/LM35 p391, **thermistor resistance-vs-temperature table
p386–387**); Mackenzie §11.11 p296–299 asm / §12.10 p341 C (analog out, sine
generation), §11.12 p300–302 / §12.11 p342 (analog in), §11.13 p303–305
(thermistor, phototransistor); Subrata ch20 p277–288 (sample-and-hold p278,
audio via DAC p284); Gimenez p39–41 (weighted and R-2R theory); Gehlot p63 (LDR).
Ibrahim pp. 132–145 has the interrupt-driven ADC0804 if you want a
parallel-converter contrast.

**STC deltas.** **This is the practical answer to the "STC89 has no ADC" trap.**
The silicon has none; the board has a better one than the books assume. Two
deltas to write down: (a) the books all target a *parallel* ADC0804/0808 with
`RD`/`WR`/`INTR` handshaking, while this is **SPI** — so #4's UART mode 0 is the
right transport, and the LSB-first reversal applies; (b) the XPT2046 shares
`P3.4`–`P3.7` with the 74HC595 and the DS1302 with no arbitration, so **reading
the ADC corrupts the shift register** unless you re-latch afterwards. That
constraint is the engineering content.

**Hardware.** 🟢 Zero.

**Feasibility.** High.

**Effort.** Evening each; at least three posts.

**Rank 9 because** the value-to-effort ratio is the best in the study and the
"I had this all along" reveal is a strong hook — and 12 bits is better than the
8-bit converters every book in the set uses.

---

### #10 🟢 — The clock that forgets: DS1302, and the mod the board needed

> ⚠️ **Demoted (§5.4).** DS1302 interfacing is already published — your completed
> list has "Dallas 1-Wire protocol: DS18B20, DS1302" marked `[Done]`. What is
> left is the battery mod and the drift measurement, which is a section of #33's
> desk-clock post rather than a post of its own.

**Hook.** Your 7-segment post (2025.11.15) is not about 7-segment displays. It is
about an East German clone of a Spartus digital clock that followed your
great-grandparents through vacations, hospital stays and chemotherapy, and
vanished when the estate was settled. **The board has a DS1302 RTC on it — and
the manufacturer connected the backup pin to `VCC`, so it forgets the time the
instant you unplug it.** Fix that, then build the clock.

**Substance.** The DS1302 three-wire protocol (`CE`/`SCLK`/`I/O`, burst mode,
write-protect, the trickle-charge register), BCD registers, setting the time over
the UART. Then the two things that make it real: **the supercap or CR2032 mod**
(`VCC1` is the battery pin and it is tied to `VCC` — cut and add a cell, with the
trickle-charge register configured to match), and a timer-driven display refresh
so the digits do not flicker while the RTC is read.

And the bus hazard, again: **DS1302 `CE` is `P3.5`, which is also the 74HC595
latch and the XPT2046 select** (§1). The saving grace is that `CE` is active-high
while `/CS` is active-low, so the RTC and the ADC are mutually exclusive by
construction — but every RTC access clocks the 595.

**Books.** Mazidi **ch16 p437–460** (DS12887 — battery-backed RTC with alarm and
periodic interrupt; a different part, but the register-model chapter);
Anbazhagan PDF p52 ff. and Gehlot p110 (DS1307 over I²C); Schultz p296 (a
software real-time clock as the fallback); Subrata p180; Mackenzie §11.7
p277–281 / §12.6 p329 (battery-backed NVRAM).

**STC deltas.** Bit-banged three-wire, same shape as the DS18B20 work you have
published — and the same timing loops that all shift under 6T (#11).

**Hardware.** 🟢 On-board. A supercap or CR2032 and a trace cut for the mod.

**Feasibility.** High.

**Effort.** Evening.

**Rank 10 because** the emotional throughline already exists in a published post,
the artefact is a photograph of a working clock, and the missing battery turns a
tutorial into a repair story — which is a strictly better post.

---

### #11 🟢 — One checkbox, twenty-five broken demos: the 6T/12T story

**Hook.** Tick a box in the flasher and your chip is twice as fast. Every delay
loop, every timer reload and every baud rate in the repository is now wrong.

**Substance.** §0.2 establishes the mechanism — **an ISP option byte written
during a full programming run, not a runtime SFR** — and that changes the post's
shape entirely: you cannot demonstrate it by poking an SFR, you ship two `.hex`
files, or one flashed twice with `stcgal -o cpu_6t_enabled=…`. Then the
consequences:

- delay loops halve — your `delay(uint16_t t)` in `05_enc_servo` is commented
  *"more than 1us at 12MHz"*, and is now more than 0.5 µs;
- timer reloads: same count, half the wall time; every `TH0`/`TL0` constant shifts;
- the **watchdog** timeout table halves (N = 12 → 6, §2.5);
- UART divisors change, and `stcgal`'s own `calculate_baud` uses **sample rate 32
  in 12T and 16 in 6T** — the bootloader itself has to know;
- **UART mode 0 does *not* change** (§2.3) — it is f<sub>osc</sub>/12 either way,
  which is a genuinely counterintuitive exception worth its own paragraph;
- and the mechanism note from §0.2: the core is *physically* 6T, and 12T is a
  ÷2 prescaler in front of it. So 6T is not overclocking; 12T is deliberate
  slowing, for EMI.

The fix is a single `CYCLES_PER_US` header plus computed reloads — a small,
portable, reusable artefact. **The side effect is the better post**, exactly as
suspected.

**Books.** None cover STC 6T. Ayala p110–116 (pure software delay vs
software-polled timer vs pure hardware delay) frames why software delays were
always a liability.

**Hardware.** 🟢 None.

**Feasibility.** High.

**Effort.** Evening.

**Rank 11 because** it is cheap, it touches every post you have already written —
so it links backwards into all of them, which is good for your traffic — and "my
whole repo broke" reads far better than a specification recital.

---

### #12 — Software PWM done properly, and 1000 steps of servo resolution

**Hook.** The STC89C52 has no PWM hardware. The naive timer-ISR approach fails
for a reason you can measure, and the fix gives better servo resolution than any
servo can use.

**Substance.** §2.1 as the post. **A four-channel compare in a fixed-tick ISR is
53 machine cycles of SDCC output plus ~7 of entry — 60 µs at 12 MHz 12T.** 8-bit
resolution at 100 Hz needs a tick every 39 machine cycles. **The ISR is half
again longer than its own period.** It cannot work, and now you know by how much.

The fix is not "optimise the ISR", it is **change the algorithm**: sort the
channel edges and reload the timer to the *next edge* rather than ticking at the
resolution. K channels cost K+1 interrupts per period regardless of resolution.
Eight channels at 100 Hz is 900 interrupts/s — about 1 % CPU at the measured
11-MC floor — with the timer's full 1 µs resolution intact.

Then servos:

| | 12 MHz 12T | 12 MHz 6T | 11.0592 MHz 12T |
| --- | --- | --- | --- |
| Timer tick | **1.000 µs** | 0.500 µs | 1.085 µs |
| Steps across the 1–2 ms range | **1000** | 2000 | 922 |
| Latency jitter (3–8 MC) | 3–8 µs ≈ 1° | 1.5–4 µs | 3.3–8.7 µs |

**Verdict: servo timing is comfortably achievable.** 1000 steps is more than any
hobby servo resolves; eight servos on a 20 ms frame is 450 interrupts/s. The one
honest caveat is jitter — 3–8 µs of interrupt-response variation is roughly 1° of
twitch unless the servo ISR is highest priority and nothing masks interrupts.

**And it is a prerequisite for #9**, because the board's only analog output is a
software-PWM pin into an RC filter.

**Books.** **Subrata ch23 p313–324 is the sole source in fifteen books for RC
servo control** (1/1.5/2 ms pulses, inside a servo, **home-position detection**
p318). Braithwaite p86–88 (PCA and hardware PWM — the contrast: what the silicon
would have done for you); Gimenez p36–38 (PWM theory, MOSFET load drive);
Mazidi ch17 p461–474 (duty cycle, H-bridge, optoisolated drive); Subrata p293
(speed via DAC vs via duty cycle).

**STC deltas.** **No PCA, no PWM module** — confirmed, [EN-165] Appendix C p. 165.
All numbers double at 6T, and 6T is an option byte, so this cross-links to #11.
Note also **Timer 2's `T2OE` clock-out on `P1.0`** (§2.5) as the one thing the
hardware *will* generate for free — a fixed-duty square wave, useful as a
comparison baseline.

**Hardware.** A servo (~£3). The multi-channel LED demo is 🟢.

**Feasibility.** High.

**Effort.** Weekend.

**Rank 12 because** it answers the question your readers will definitely ask, the
measured-ISR-cost framing is unusual and credible, it completes the half-finished
`encoder_servo` branch, and #9 depends on it.

---

### #13 — A debugger for a chip with no debug port: a UART monitor with a disassembler

**Hook.** No SWD, no JTAG, no ICE, and **the bootloader has no flash-read
command** — you cannot even read your own firmware back off the part. Write a
resident monitor that lets you halt, inspect, breakpoint and **disassemble**,
over the same cable you flash with.

**Substance.** A target-side monitor (breakpoint by patching `LCALL`, register
and IRAM/XRAM dump, single-step via a timer) plus a host front end. Given your
stack, the host should be **a Svelte page over WebSerial**, reusing #2's
transport. The disassembler is a table-driven 256-entry opcode decoder — small,
satisfying, forkable.

**Books.** **Limbach ch5 p179–220 is the reference** — DEBUG8051, a monitor on
the target (p179–195) plus a Windows front end *containing a disassembler*
(p196–220), with single-stepping, breakpoints and register/memory display.
**Schultz p249–259** has a complete monitor listing (EET Monitor v1.8) and
p262–267 on debugging strategy, including "use I/O pins as scope trigger points"
(p266) — which is literally what you did in 2025.11.24 with the `P3.4` heartbeat.
Mackenzie ch10 p247–257; Balaji ch5 p53–56.

**STC deltas.** The monitor must coexist with the ISP bootloader's reserved
region (`stcgal`'s model table implies ~2 KB reserved: `total=16384, code=8192,
eeprom=6144`). It composes with #8 — a monitor that can rewrite the application
via IAP is a full on-chip development environment. And the no-read-back
limitation is what makes it *necessary* rather than merely fun.

**Hardware.** 🟢 None.

**Feasibility.** Medium — breakpoint patching wants #8 first.

**Effort.** More than a weekend.

**Rank 13 because** "I wrote a debugger" is catnip for the HN-shaped audience,
it is genuinely rare (two of fifteen books), and it stacks on #2 — but it is the
largest build in the top fifteen.

---

### #14 🟢 — Cycle-accurate CI: run your firmware in ucsim and assert on the waveform

**Hook.** You ended the 2022 arc at *"successful build automation is the first
step"*, having shown `s51` counting **534 instructions and 6,408 cycles** to the
first `P2.0` toggle. Finish the thought: make that a test that fails a pull
request.

**Substance.** `s51` is installed here alongside SDCC 4.5.0 and is scriptable.
Drive it from a harness that loads the `.hex`, runs N cycles, and asserts on port
state *and timing*: "the 1-Wire reset pulse is 480–640 µs", "the display refresh
completes within one 5 ms slot", "the servo pulse is 1.50 ms ± 20 µs". Then wire
it to GitHub Actions. Bonus: the same harness catches every 6T regression from
#11 automatically, and every crystal-assumption bug from §0.5.

**Books.** Mackenzie ch10 p247–257; Schultz ch9 p219–267; Balaji ch5 p53–56;
**Ayala p107–108** for the on-target self-test half (#21).

**STC deltas.** ucsim models a generic 8052 — **no IAP SFRs, no option byte, no
6T, no `SADDR`/`SADEN`**. The simulator will happily run code the silicon will
not, and vice versa. Saying that clearly is the honest core of the post.

**Hardware.** 🟢 None — that is the point.

**Feasibility.** High. Both tools are already on the build machine.

**Effort.** Weekend.

**Rank 14 because** it converts your existing repo into something with a green
tick, appeals to the software half of your audience, and closes a loop you opened
in 2022 — which readers of a series notice.

---

### #15 — Four generations of one architecture: STC89C52 vs STC12 vs STC8H vs CH32V003

**Hook.** The 8051 did not stop in 1985. Benchmark a 12T STC89C52 against a 1T
STC8H and a $0.10 RISC-V on the same code, and find out what forty years bought.

**Substance.** You are already positioned: you have `stc8demo` (STC8H1K08S4A), a
published RISC-V post (2023.01.21, "32-bits at $0.10"), a semihosting post, and
you wrote publicly in 2025.10.19 that you like the STC8H and would take samples.
Run one workload — a fixed-point benchmark, the ST7920 driver, a 1-Wire
transaction — across 12T STC89C52, 6T STC89C52, STC12C5A60S2, STC8H at 1T and a
CH32V003, reporting cycles, wall time, flash size and cost per unit of work.

The feature table alone is a good graphic: **STC89C52RC** — 12T/6T option byte,
no ADC, no PCA, no SPI, 8 KB flash, 256+256 B RAM. **STC12C5A60S2** — 1T core
with a runtime `CLK_DIV`, **10-bit 8-channel ADC**, **2-channel PCA/PWM**, SPI,
two UARTs, 1280 B RAM. **STC8H8K64U** — 1T, 12-bit ADC, comparator, PWM, DMA,
I²C, SPI, and a **native USB bootloader needing no programmer at all**. The likely
headline is that **the 8051 wins on flash density and loses badly on time**,
which is quotable.

**Books.** Subrata ch26 p355–380 (AVR/ATmega8, MCS-96, ARM, Renesas — the same
exercise done in 2010); Walter ch8 p200–216 (survey incl. DSPs p207, C515C p213,
the USB-capable C8051F340 p214); Limbach appendix p265+ (AT89C52 vs Cygnal
C8051F000 vs Infineon C515C); Braithwaite ch6 p142–168 (the 8051 → 16-bit XA
migration, instruction-set differences p232).

**Hardware.** You have the STC89 and STC8H boards; a CH32V003 is ~£1.

**Feasibility.** High, though a *fair* benchmark is more design work than it looks.

**Effort.** Weekend.

**Rank 15 because** comparison posts travel further than tutorials, it unifies
three strands of your blog, and it is a natural vehicle for the sample request you
already made in public.

---

### #16 — RS-485 multidrop: nine-bit UART, hardware address filtering, thirty-two nodes

**Hook.** The 8051's UART has a ninth bit whose only purpose is addressing other
8051s — and the STC89 adds hardware that filters those addresses for you.

**Substance.** Modes 2/3 with `SM2` and `TB8`: master sends an address byte with
bit 9 set, slaves wake, the addressed one clears `SM2` and receives data, the
rest sleep. Then Pont's **SCU** on top: 9-bit tick and ack, unique slave IDs, and
the bandwidth rule — **22 bits per tick, so 1 ms ticks need ≥22,000 baud**;
latency fixed and predictable (~0.4 ms at 28.8 k, ~0.03 ms at 375 k). RS-485
variant: MAX489, up to 1 km at 90 kbaud, 120 Ω termination, up to 32 unit loads.

**The STC-specific upgrade is the best part of the post.** STC89 has
**`SADDR` (A9H) and `SADEN` (B9H)** — a Given-address mask, so slaves filter in
*hardware* with zero CPU cost, plus broadcast addressing. And **frame-error
detection**: set `SMOD0` (PCON.6) to remap `SCON.7` as `FE`, giving you a
bus-collision detector, which on a multidrop line is exactly what you want.
Neither exists on a stock Intel 8051, so **Pont's and Braithwaite's code can both
be simplified on this part** — a concrete, demonstrable improvement on a
published reference design.

**Books.** **Pont pp. 608–674** (SCU: local, RS-232 via MAX233 at ~30 m, RS-485
via MAX489 at up to 1 km); **Braithwaite Project 3 p185–191** (a single-wire
multiprocessor system on mode 3, explicitly contrasted with I²C and SPI — rare
and self-contained); Ayala p186 (star/ring/multi-drop topologies) and p197;
Mackenzie §5.8 p119; Subrata p234; Mazidi p264 (MAX232).

**STC deltas.** `SADDR`/`SADEN`/`FE` as above; baud divisors shift under 6T
(#11); **RS-485 is a confirmed gap across all fifteen books except Pont**.

**Hardware.** Two boards plus two MAX485 modules (~£2 the pair), twisted pair.

**Feasibility.** High.

**Effort.** Weekend.

**Rank 16 because** it is real industrial engineering that is still in use, and
the hardware-address-recognition angle makes it more than a rehash — but the
artefact is an oscilloscope trace.

---

### #17 🟢 — The board that fights itself: `P2` is quadruple-booked

**Hook.** On this board, reading the EEPROM makes the LEDs flicker, refreshing
the display makes the LEDs flicker, and writing to the LCD makes the LEDs
flicker. There is no jumper to stop it. Here is how to live with a board whose
designers ran out of pins.

**Substance.** §1, as a post about **pin-budget engineering**, which is a real
skill nobody teaches. `P2.0`–`P2.7` carry the 8-LED bank *and* I²C *and* the
74LS138 digit select *and* the LCD control lines, simultaneously and
permanently — the vendor's manual confirms the shorting jumpers were removed
from this revision. Meanwhile `P3.4`–`P3.7` carry the 74HC595, the DS1302 and the
XPT2046 with **no chip selects at all**.

Then the interesting half: what do you actually *do*? Time-division discipline
(a slot scheduler that owns the bus and never lets two drivers run concurrently —
which is Pont's cooperative scheduler applied to a hardware problem, and links to
#6); noticing the accidental arbitration that *does* work (DS1302 `CE`
active-high vs XPT2046 `/CS` active-low on the same pin); which trace to cut and
which resistor array to lift if you want `P2` back; and how to lay out a board so
this never happens, which is where Walter's design-process chapter earns its
place.

**Books.** **Walter ch9 p217–235** (requirements → CPU-load and data-flow
estimation → CPU choice → block diagram → schematic → **BOM and sourcing** →
interfaces → **layout and assembly plan** → manufacturing documentation →
fault-finding) — **the sole source in fifteen books for the engineering process**.
Ayala p103–106 (expanding I/O, memory-mapped I/O, part speed selection,
production concerns); Mackenzie §11.8 p282–290 (I/O expansion); Schultz ch12
p312–327 (the scheduler that makes time-division discipline practical).

**STC deltas.** None electrically — this is a board-design post. But it is the
piece of context that every other idea in this study needs, which is why it is
here rather than buried in an appendix.

**Hardware.** 🟢 None.

**Feasibility.** High.

**Effort.** Evening.

**Rank 17 because** it is the honest, unglamorous post that makes the rest of the
series make sense, and "your dev board is lying about how many pins you have" is
a frustration every reader shares.

---

### #18 🟢 — The stepper motor on the board you have never turned

**Hook.** 五线四相异步电机 — a five-wire four-phase stepper on a ULN2003, sitting
on the board since 2022, wired to the same four pins as the keypad.

**Substance.** Wave / full-step / half-step sequences, the 28BYJ-48's step angle
and gearbox ratio, steps-per-second ↔ RPM arithmetic, acceleration ramps
(commanding top speed directly stalls it), holding torque vs current. Then the
engineering content: **`IN1`–`IN4` are `P1.0`–`P1.3`, which are also the keypad
columns**, so a running motor and a scanned keypad are mutually exclusive unless
you interleave them — and a timer-driven step scheduler that yields to the keypad
scan is exactly the cooperative-scheduling problem of #6 and #17 in miniature.

**Books.** Mazidi **ch15 p424–436** (unipolar vs bipolar, step angle,
**steps/sec ↔ RPM p427**, holding torque, wave/full/half-step, H-bridge for
bipolar p431); Subrata **ch22 p301–312** (3.6° step angle p303, half-stepping
p304, driver ICs p306, direction and speed); Mackenzie §11.15 p310–314 asm /
§12.14 p347 C; Braithwaite Project 2 p175–184; Balaji §4.4 p36–38 (PC-controlled
over RS-232); **Schultz p180 and p198** (the same driver written modularly in
ASM51 and then mixed C/assembly — a good pairing with your SDCC listing habit).

**STC deltas.** The ULN2003 does the driving, so the 220 µA source limit of §0.4
does not bite here — worth saying explicitly, because it is *why* the ULN2003 is
there. `IN5` on the same chip is the buzzer, and `IN6`/`IN7` are spare for
relays.

**Hardware.** 🟢 On-board if the motor shipped with the kit (these normally do);
otherwise a £2 28BYJ-48.

**Feasibility.** High.

**Effort.** Evening.

**Rank 18 because** motion is the most photogenic artefact available — a GIF of
a turning shaft beats any LCD screenshot — and six books cover it, so the
write-up is low-risk.

---

### #19 — The SD card post that is honest about why FAT does not fit

**Hook.** Everyone wants to log to an SD card. On a chip with 256 bytes of
`MOVX`-addressable RAM, a single FAT sector buffer is **twice** your entire
memory. Here is what you can actually do.

**Substance.** Three walls, in order:

1. **Electrical.** The board is 5 V; SD cards are 3.3 V and not 5 V tolerant.
   `CLK`/`MOSI`/`CS` need dividers or a shifter. `MISO` at 3.3 V into a 5 V input
   is fine — **`VIH1` is 2.0 V** ([EN-271] Appendix C), so 3.3 V reads high with
   margin. And the supply problem is already solved: the later board revision
   carries an **AMS1117-3.3** for the TFT/SD block, and the nRF24L01 header has a
   3.3 V rail either way.
2. **RAM — and this is worse than the usual telling.** The often-quoted "512
   bytes" is **256 scratch-pad + 256 XRAM** (§0.4). A FAT sector buffer is 512
   bytes and there is nowhere to put it: the XRAM holds 256, and the scratch-pad
   is shared with the stack, which your own demos already use 218–248 bytes of
   (from the SDCC `.mem` files in `build/`). **You cannot buffer a sector at
   all.**
3. **Speed.** Bit-banged SPI in C is 6.3 kB/s (§2.3) — 81 ms per sector.
   **UART mode 0 takes it to 125 kB/s, or 4 ms.** #4 is a hard prerequisite.

**Verdict: raw-sector access without a filesystem is the honest fallback, and it
is a good one.** Pre-allocate a contiguous file on a PC, record its start LBA,
stream fixed-size records into raw sectors, read it back with `dd`. State plainly
that read-only FAT with a *streaming* parser is possible — you can parse a
directory entry without buffering a whole sector — but that write support is not,
and that **an STC89C54RD+ with 1024 bytes of XRAM changes the answer** if you
wants the filesystem badly enough.

**Books.** **SD/MMC/FAT is a confirmed gap across all fifteen.** Nearest
relatives: Mackenzie §11.7 p277–281 / §12.6 p329 (NVRAM — what people did
before); Mazidi ch18 p475–486 (SPI including burst read/write timing, which is
exactly the SD access pattern); Limbach §4.10 p140–178 (the data-logger, on the
PC side).

**STC deltas.** The 256/256 RAM split is the whole story. Also SDCC's `__xdata`
access is the 20-MC-per-byte loop of §2.2, so merely *moving* 256 bytes costs
5 ms in C.

**Hardware.** An SD breakout (~£2) and a few resistors.

**Feasibility.** Medium for raw sectors; **low for FAT, and saying so is the post.**

**Effort.** Weekend.

**Rank 19 because** the honest-negative framing is what the brief asks for and
what the audience respects — but the artefact is a hex dump.

---

### #20 🟢 — nRF24L01: the wireless header nobody uses

**Hook.** There is a 2.4 GHz radio header on the board, with its own 3.3 V rail
and series resistors, and none of the 25 demos touch it.

**Substance.** The nRF24L01's SPI register model, ShockBurst auto-ack and
auto-retransmit (the radio runs the retry protocol, so a 1 MIPS CPU can keep up),
pipes and addressing, channel and power settings. Then the good part: **combine
with #6** — shared-clock scheduling over the air rather than a wire, where
Pont's master-loss watchdog does real work because radio links genuinely drop.

**Books.** Nothing on the nRF24L01. Transport reference: Mazidi ch18 p475–486;
Braithwaite p129–135 (an on-chip SPI controller, as the contrast); Gehlot
p115–118 (SPI devices on an 8051) and the whole book for the "8051 as the I/O
half of a wireless pair" pattern.

**STC deltas.** No SPI peripheral → UART mode 0 (#4). The module is 3.3 V logic;
the board fits series resistors on `IRQ`/`MISO`/`MOSI`/`SCK`/`CSN`/`CE`, which
suggests the designers intended exactly this, but **the header's pin assignment
is the one item in the pin map I would meter before soldering** — the schematic's
text layer is offset in that block.

**Hardware.** 🟢 Header on board; two modules at ~£1.50 each.

**Feasibility.** Medium-high — these modules are famously fussy about decoupling.

**Effort.** Weekend.

**Rank 20 because** wireless is inherently appealing and the header is already
there, but the topic is well-trodden elsewhere; the novelty comes only from
pairing it with #6.

---

### #21 🟢 — Power-on self-test: does your crystal oscillate, is your flash intact, is your RAM real?

**Hook.** A device that boots into a broken state and says nothing is worse than
one that does not boot. Three tests, about a hundred bytes, before `main`.

**Substance.** Ayala's triad: a **crystal test** (does the timer advance at the
rate the code assumes — the same measurement the STC bootloader makes in §0.3),
a **flash checksum** with the expected value planted by the build, and a **RAM
march test** that walks patterns through internal RAM without needing RAM to do
it. Report failures on the one resource you still trust — a blink code.

**STC deltas, and this is where it gets good.** The crystal test on an STC has a
job no generic 8051 has: **it can detect which clock mode the part was flashed
in.** 6T is an option byte you cannot read from a runtime SFR (§0.2), so a POST
that measures the timer against a known interval can tell you whether you are in
6T or 12T and **refuse to run with the wrong timing constants** — which is
exactly the failure mode #11 is about. That is a small, clever, quotable result
that exists nowhere. Second delta: the checksum must know where the ISP
bootloader's reserved region begins or it checksums the wrong thing.

**Books.** **Ayala p107–108 is the sole source** (crystal test, ROM test, RAM
test) — genuinely absent from the other fourteen. Ayala p106 (part speed
selection, production concerns) frames why; Mackenzie ch10 p247–257 and Schultz
p262–267 for the surrounding discipline; Walter ch9 p232 (fault-finding).

**Hardware.** 🟢 None.

**Feasibility.** High.

**Effort.** Evening.

**Rank 21 because** it is small, genuinely useful, absent from the literature
except one 1991 textbook, and the 6T-detection trick makes it more than a
checklist — but it is undramatic.

---

### #22 🟢 — Idle, power-down, and telling a cold start from a warm one

> ⚠️ **You have already declined this** — `[Not Doing]` on low power, watchdog and
> power-down mode alike, annotated *"(not common)"*: the features vary too much
> between 8051 derivatives for a post to transfer to a reader with a different
> part (§5.4). Kept only because #32's NFC energy-harvesting question may make
> the measurements earn their place inside a project. Do not write it as a
> feature tour.

**Hook.** How long does an STC89C52 run on a coin cell? Measure it, in three
configurations, with the multimeter in the photograph.

**Substance.** `PCON`'s `IDL` and `PD` bits, what stops in each mode, what wakes
it. Then the two things that make it a real post: (a) **the same application
built three ways — no power management, idle, power-down — with measured current
and computed battery life** (Subrata's structure, and it is excellent); and
(b) **distinguishing a cold start from a warm start** by leaving a known pattern
in internal RAM and checking it at boot, so the device can tell "just plugged in"
from "woke up" from "the watchdog bit me".

The datasheet numbers to anchor it (§2.5, [EN-271] Appendix C): operating 4 mA
typical / 20 mA max; **idle 2.0 mA**; **power-down < 0.1 µA**.

**Books.** **Subrata ch24 p325–338 is the standout**: idle, power-down, exit
conditions, **cold-vs-warm-start detection p328** with the random-pattern
technique p336, then the three-case quantitative comparison p329–337. Gimenez
p90–91; Mackenzie p35; Walter §4.17 p106.

**STC deltas.** Three, all confirmed. **There is no power-down wake-up timer** —
the selection table reads `N` for every STC89 part; that is STC15/STC8. So "wake
every 10 seconds and log" needs an external interrupt source, and the on-board
DS1302 (#10) can be it. **Wake sources are richer than the classic 8051**:
RESET, `/INT0`, `/INT1`, `T0`, `T1`, `RXD`, `/INT2`, `/INT3`, level *or* edge.
And three sharp edges from [EN-165] p. 12: put at least one `NOP` after the
power-down instruction; the wake pin must stay asserted until the internal
32768-clock settling counter expires; do not re-enter power-down within 4 µs.

**Hardware.** 🟢 Board plus a multimeter.

**Feasibility.** High.

**Effort.** Evening plus measurement time.

**Rank 22 because** it is quantitative and produces a result table that travels —
but battery life on a mains-powered dev board is a slightly artificial framing.

---

### #23 — Charlieplexing, and why 220 microamps makes it hard

**Hook.** The matrix on the board uses 16 lines for 64 LEDs. Tri-state logic
gets you 56 from 8. Except the 8051's ports are not tri-state, and they source
0.22 mA.

**Substance.** The n(n−1) arithmetic, then the part that makes this an 8051 post
rather than a generic one: **Charlieplexing needs a pin to be genuinely
high-impedance, genuinely high and genuinely low.** The 8051's
quasi-bidirectional ports are none of those cleanly — they are weak-pull-up
outputs, so the "off" pins leak and the array glows where it should be dark.
And per §0.4, `IOH` on P1/P2/P3 is **150–220 µA minimum**, so the "source" leg of
every Charlieplex pair is 20× too weak to light an LED at a usable duty cycle.
Meanwhile [EN-165] Chapter 4 tells you to just set push-pull mode, and **there is
no such mode** (#7).

**Verdict, which is the post:** on P1/P2/P3, Charlieplexing works only in a
sink-only variant with external high-side drive, or at brightness so low it is
useless. **P0 is the interesting exception** — true open-drain with 8 mA sink and
external pull-ups fitted on this board — but open-drain cannot source either, so
you still need a high-side element. Work it through, give the number, and let the
answer be "mostly no, and here is the circuit that makes it yes".

**Books.** **Charlieplexing is a confirmed gap across all fifteen.** Supporting
theory: **Limbach p16–22** (80C51 vs 80C32 port electrical structure); Mackenzie
§11.8 p282–290 (I/O expansion); Gimenez ch1 (load driving).

**Hardware.** LEDs and resistors on a breadboard.

**Feasibility.** Medium — likely a partially negative result, which is fine and
is why it is worth writing.

**Effort.** Evening.

**Rank 23 because** it is a classic retro trick and the "does the 8051's port
structure even allow this" question makes it more than a rehash — but it is a
small idea with a probably-disappointing answer.

---

### #24 🟢 — The board that lies: debugging a correlated 7-segment failure

**Hook.** You already found it and wrote two paragraphs: *"Whenever `P0.6` is
low, it pulls `P0.5` low as well. This is not a software issue."* (2025.11.15)

**Substance.** The diagnosis method — bisecting software against hardware, what a
solder bridge or a failed 74HC245/74HC573 output looks like electrically, proving
it with a meter and a logic analyser rather than guessing. Then the generalisable
lesson, which is bigger than your board: **`P0` is not a normal port.** It is
open-drain; the board fits 10 kΩ pull-up arrays (`RP11`+`RP16`) *and* a
buffer/latch *and* 100 Ω segment resistors, and the 8×8 matrix hangs off `P0`
directly relying on it to sink. Four different loading regimes on one port.

A useful wrinkle for reproducibility: **two revisions ship as "V2.0"** —
`U2` is a **74HC245 buffer** on the original and a **74HC573 latch** on the
revision you have, in the same socket with the same 1:1 map. Which one is fitted
changes what a stuck output means. Both are readable with the naked eye.

**Books.** **Limbach p16–22** is the best reference (port electrical structure,
why pull-ups, drive limits). Gimenez ch1; Mackenzie §2.2–2.3; Walter p19.

**STC deltas.** The drive numbers must come from the STC datasheet
(§0.4: `IOL` 4 mA on P1–P3, 8 mA on P0; `IOH` 150–220 µA), **not** from a 1991
Intel textbook — and per #7 not from Chapter 4 of the STC datasheet either.

**Hardware.** 🟢 None — you have already hit the fault.

**Feasibility.** High. Half of it is written.

**Effort.** Evening.

**Rank 24 because** debugging stories reliably outperform tutorials and this one
is already lived — it ranks here only because it is short and specific to your
unit.

---

### #25 — Frequency counter and pulse-width measurement, with a hard ceiling

**Hook.** The two counter inputs are on `P3.4` and `P3.5`, which the board wires
to three other devices. Free them and the chip becomes an instrument — up to
exactly 500 kHz and not one hertz more.

**Substance.** Counter mode (`C/T=1`) with a gated 1-second window, then the
three regimes Schultz distinguishes: low frequencies (measure period, not count),
high frequencies (count over a window), and the broad-range case where you switch
strategy at runtime. Then pulse-width measurement by capturing a timer on an
`INT0` edge — which is exactly the technique you already used, unnamed, to decode
NEC IR frames in 2025.11.24.

**STC deltas, and they are good numbers.** The counter input is sampled once per
machine cycle and needs two cycles to recognise a transition, so the ceiling is
**f<sub>osc</sub>/24 = 500 kHz at 12 MHz** — and because it is a function of
f<sub>osc</sub> rather than the machine cycle, **6T does not raise it**, which is
the second counterintuitive 6T exception after UART mode 0 (#11). Measurement
resolution is one machine cycle: 1.000 µs at 12 MHz 12T. And **Timer 2's `T2OE`
clock-out on `P1.0`** (§2.5) gives you a hardware reference square wave to
calibrate against, from the same chip.

**Books.** **Schultz p298–301** is the best source (fast events and high
frequencies: infrequent/low, in-between, broad-range). Ayala p158–161; Balaji
§4.8 p50 (a frequency counter as a shipped product); Limbach §3.6.1 p89.

**Hardware.** 🟢 Board plus a signal source — or a second board generating a
known frequency, which makes a self-calibration story.

**Feasibility.** High.

**Effort.** Evening.

**Rank 25 because** turning a dev board into an instrument is a good frame and
the 500 kHz ceiling is a satisfying hard number — but frequency counters are a
well-worn genre.

---

### #26 🟢 — Tic-tac-toe on the 8×8 matrix, and how much game fits in 8 KB

**Hook.** 64 LEDs, sixteen keypad keys, 8 KB of flash, 256 bytes of usable RAM.
Write a game.

**Substance.** Minimax on 3×3 is trivially small; the content is everything
around it — a display refresh that never blocks, debouncing that survives an
animation, state machines with no dynamic memory, and a flash budget shown as an
SDCC `.mem` breakdown. Then the honest extension: what *does* fit? A snake, yes.
Anything with a search tree, no. And the constraint that makes it an 8051 post:
the matrix rows come from the 74HC595 on `P3.4`–`P3.6`, which the RTC and the ADC
also want (§1), so the game owns the bus.

**Books.** Mackenzie **ch13 p353–374** has the specification-level student
projects: **Tic-Tac-Toe p358**, Home Security p353, Elevator p355, Calculator
p363, **Micromouse p366**, Soccer Robot p369, Smart Card p371 — specs and system
design without listings, which is the right level for a post that wants readers
to build it themselves.

**STC deltas.** None. Pure resource-budget content; the `.mem` files in your
`build/` already show the shape (218–248 bytes of stack headroom across your
demos).

**Hardware.** 🟢 None.

**Feasibility.** High.

**Effort.** Evening.

**Rank 26 because** it is fun, photogenic and free — but the least technically
novel item in the list.

---

### #27 🟢 — What the silicon genuinely cannot do

**Hook.** Five things people will ask for that an STC89C52 cannot deliver, each
with the arithmetic.

**Substance.** One short post is worth more than five disappointed attempts:

| Ask | Verdict | The number |
| --- | --- | --- |
| **VGA output** | **No** | 31.8 machine cycles for a whole 31.78 µs line at 12 MHz 12T, 25.4 in the active region. No useful pixel stream fits in 25 instructions, and 6T only buys you 50.8. |
| **WS2812 / NeoPixel** | **No** | The bit period is 1.25 µs = **1.25 machine cycles at 12 MHz 12T**. You cannot place one edge, let alone the two the encoding needs. At 6T it is 2.5 MC — still short of a compliant 0.4/0.85 µs split. UART mode 0 cannot rescue it either: it is fixed at f<sub>osc</sub>/12 = 1 Mbit/s, and one WS2812 bit needs ~3 shift bits at ≥2.4 Mbit/s. **The only route is a much faster crystal, and even then it is a stunt.** |
| **On-chip ADC or PWM** | **No** | [EN-165] Appendix C p. 165 prints `N` in the A/D and PCA/PWM columns for every STC89 part. **But the board has a 12-bit XPT2046** (#9), which is the practical answer. |
| **FAT on SD** | **No** | 512-byte sector buffer against **256 bytes** of `MOVX` RAM (#19). |
| **Push-pull port drive** | **No** | No `PxM0`/`PxM1` registers exist. `IOH` is 150–220 µA. **The datasheet says otherwise and the datasheet is wrong** (#7). |

Plus the smaller ones: no hardware multiply-accumulate (`MUL`/`DIV` are 4 machine
cycles and 8-bit); **no power-down wake-up timer** (STC15/STC8 only); **no flash
read-back** — the bootloader has no read command, so you cannot dump your own
firmware; **dual DPTR exists but SDCC does not use it**, costing 8 of the 20
machine cycles in the XRAM loop of §2.2; and **`AUXR` is write-only**, so
read-modify-write on it is silently broken.

**Books.** N/A — this is the negative space around them. Braithwaite p86–92
(PCA, hardware PWM, watchdog on Philips parts) is the "what the same money buys
elsewhere" reference.

**Hardware.** 🟢 None.

**Feasibility.** High.

**Effort.** Evening.

**Rank 27 because** it is a service post rather than a project — but it is the
one most likely to be linked from forums as the definitive answer, which is
disproportionate long-tail traffic.

---

### #28 — Optocoupler and TRIAC: switching mains from a 5 V logic pin

**Hook.** The point at which an embedded project stops being a toy.

**Substance.** Optocoupler selection, the MOC-series zero-cross triac drivers,
gate resistor sizing, snubbers, creepage and clearance. Then phase-angle dimming
as the harder follow-up: detect the mains zero crossing on `INT0`, delay, fire
the gate — needing 1 % accuracy on a 10 ms half-cycle, i.e. 100 µs, i.e. **100
machine cycles at 12 MHz 12T**. Comfortable, and the interrupt jitter of 3–8 MC
is well inside tolerance.

**Books.** **Gimenez p38 is the sole source in fifteen books for AC load
switching via optocoupler + SCR/TRIAC, with bias-resistor design (R1/R2/RG).**
Mazidi ch15 p419–436 (relays, solid-state relays, optoisolators, optoisolated
motor drive); Subrata p294; Balaji §4.7 p46 (an 8-channel sequential controller
driving relays, solenoids and contactors — a bag-sealing machine). **Zero-cross
detection, phase-angle dimming and energy metering are confirmed gaps across all
fifteen**, so the dimming half is unreferenced.

**STC deltas.** None electrically; the timing budget above is the only one. The
ULN2003's spare `IN6`/`IN7` (#18) are the low-voltage relay path if you want to
stay away from mains.

**Hardware.** Optocoupler, triac, mains. **The only idea here with a real safety
warning**, and it needs to be prominent.

**Feasibility.** Medium — technically straightforward, but mains.

**Effort.** Weekend.

**Rank 28 because** it is a genuine literature gap with a grown-up framing, but
the liability and the photography make it a poor fit for a hobby blog.

---

### #29 — The 8051 as a printer: Centronics *input*

**Hook.** Every book shows the 8051 driving a printer. One book shows it *being*
one.

**Substance.** Receiving on the Centronics interface — `STROBE`, `BUSY`, `ACK`
handshaking from the peripheral side, and the timing a host PC expects. The
payoff is a bridge: an ancient DOS box or a 90s test instrument prints to a
parallel port and an STC89C52 turns it into a serial stream, a file or a modern
display. A real retro-computing rescue tool.

**Books.** **ALE p27–28 is the sole source for Centronics *input*** (hardware and
software), with p33–34 for the conventional direction and App B p73 for cabling.
Mackenzie §11.10 p294–295 asm / §12.9 p339 C covers output only. Subrata p238 has
the simplex serial-printer trick out of `TxD`.

**STC deltas.** Needs 8 data lines plus 3 handshake lines = 11 pins, which this
board cannot spare (§1, #17). Off-board — or a **74HC165** to serialise the data
byte down to three pins, at which point **UART mode 0 reads it in hardware**
(#4), which closes the loop neatly.

**Hardware.** A DB25 and a machine with a parallel port. You have a "90s Offline
Linux" post, so you plausibly have one.

**Feasibility.** Medium — the handshake timing is fussy and the host is
unforgiving.

**Effort.** Weekend.

**Rank 29 because** it is a wonderfully odd idea with strong retro-rescue appeal
and exactly one printed source in fifteen books — but the audience owning both a
parallel-port device and an 8051 is small.

---

### #30 🟢 — Where your 8 KB actually goes: an SDCC codegen post-mortem

**Hook.** You did this once, in 2022.07.03, walking the `.lst` for a blinking LED
and finding **534 instructions and 6,408 cycles before `main`**. Do it again with
real code and real stakes.

**Substance.** §2.2 is the anchor: **SDCC spends 8 of 20 machine cycles per byte
shuffling `DPTR` through `R6`/`R7`** in a trivial XRAM copy, and does not use the
STC89's second data pointer (`AUXR1` at A2H, bit 0 = `DPS`; the idiomatic switch
is `INC AUXR1`, which toggles `DPS` without disturbing `GF2` — the datasheet's own
example, p. 35) that would eliminate it. Then the survey: `--opt-code-size` vs
`--opt-code-speed` across your actual demos with `.mem` and cycle deltas; where
`__data`/`__idata`/`__xdata`/`__pdata` put things and what each costs; why `__bit`
is nearly free; what `__using(n)` saves on an ISR (**measured: 53 → 33 machine
cycles**); and when inline assembly pays (**measured: 20 → 5**).

**Books.** Schultz ch8 p174–218 (scope conventions across C and ASM51,
mixed-language drivers p198, building your own C library p208–213, overlaying
p217); Mackenzie ch7 p151–190 (macros with parameters, local labels, linker and
relocatable segments p173–182); **Walter §5.6 and ch7** (the *same project*
written in assembler and then in C — a controlled A/B); Schultz A3 p387–389
(language-switching hints).

**STC deltas.** Dual DPTR is the headline: the part has it, SDCC ignores it, and
the cost is measurable. Whether SDCC can be persuaded (`__naked`, hand-written
critical sections, `--model-large`) is the experiment.

**Hardware.** 🟢 None — SDCC 4.5.0 and `s51` are already on the build machine, and half
the measurements are in §2 of this study.

**Feasibility.** High.

**Effort.** Evening.

**Rank 30 because** it is deep, useful and squarely in your established voice —
but compiler posts have a narrower audience than anything with a photograph.

---

### #31 — LONG TASK, DOMINO TASK, and when adding a processor makes things worse

**Hook.** The counterintuitive one: 100 devices at 99.99 % reliability each yields
**37 % system reliability**. Adding processors can make a system *less* reliable.

**Substance.** The follow-up to #6, once the shared-clock link works. Three
patterns: **LONG TASK** (offload a slow computation to a slave on a different
tick rate so the master's schedule stays deterministic), **DOMINO TASK**
(pipeline across processors for *data coverage*, not latency — the subtle and
interesting one), and **DATA UNION** (getting a `float` across a byte-wide link
without undefined behaviour). Framed by §25.5's scepticism throughout, with the
honest counter-question: would one chip and a decent scheduler have been better?

**Books.** **Pont ch. 29** (DATA UNION, LONG TASK, DOMINO TASK) and **§25.5
pp. 550–552**. Schultz ch10 p271–279 and ch13 p328–343 supply the
single-processor alternative — cooperative round-robin, time-slice, priority
preemption, **counting semaphores p336**, message passing p337, and the survey of
DCX51/RTX51/USX/CMX/Byte-BOS — so the post can answer its own question fairly.
Also Schultz ch12 p312–327 for tick overload (p326) and catching all ticks
(p327), which is the failure mode that motivates offloading in the first place.

**STC deltas.** Same as #6.

**Hardware.** Two boards.

**Feasibility.** High if #6 exists.

**Effort.** Weekend.

**Rank 31 because** it is the most intellectually interesting item in the list
and the least visual — a distributed-systems post with no picture. It belongs in
the series, at the end.

---

---

## 3b. The queue you had already staged — ideas #32–#45

**Added 2026-09-04**, after reading the Anytype page *"Personal Website / Blog"*
(fetched read-only via the local API on 2026-09-04) and the task
*"Servo/Stepper Blog Article"*. §3 above was derived from the books, the
datasheet, the schematic and the git history; it did not know this queue
existed, and the queue changes the picture in three ways:

1. **Eight of the topics §3 treats as available are already published.** §5.4
   lists them and marks the collisions.
2. **Three are decisions you have already made against**, with a stated reason
   that is better than my ranking's.
3. **Nine more are staged that §3 never considered**, several of which are
   stronger than things I ranked in the top ten.

What follows elaborates the staged ones, plus five generic embedded ideas from
the same page that turn out to be realisable on the HC6800-ES. Provenance is
mixed and marked: the *idea* is your, in your words where quoted; the
*feasibility arithmetic* is mine and uses §2's measured numbers; **part-number
capabilities for chips not on this board (AT89LS52, ST25DV, CH554, CC2540) come
from general knowledge and your own saved links, and were not datasheet-verified
in this session** — check before ordering.

### #32 — The business card that boots: AT89LS52 + an NFC mailbox

Your note: *"Embedded business card for retro / Use AT89LS52 as candidate"* with
links to an ST25DV I²C project and ST's Fast Transfer Mode SDK.

The obvious version of this post — a PCB shaped like a card with an LED on it —
has been written a hundred times. Two things make your version different, and
both are technical rather than aesthetic.

**First, the ISP contrast.** The AT89LS52 is Atmel's low-voltage
serial-programmable 8051: you flash it over **SPI, with the reset line held**,
which is a completely different philosophy from the STC's UART bootloader that
#2 reverse-engineers. Same core, same instruction set, two incompatible answers
to "how does firmware get in". A post that flashes the same `.ihx` onto both
parts, with a logic-analyser capture of each, is the clearest possible
explanation of why the 8051 world has no equivalent of SWD.

**Second, the power budget, which is where this gets honest.** The ST25DV is a
dual-interface EEPROM: I²C to the MCU, NFC to the phone, with an energy
harvesting output and a **Fast Transfer Mode mailbox** — a small RAM buffer both
sides can read and write without touching EEPROM. That last feature is what
makes a *live* conversation between phone and card possible rather than a static
URL. But harvested NFC power is on the order of a milliwatt, and an 8051 at
12 MHz is not a milliwatt part. So the design question — and the post — is:
what can the card do in the ~100 ms a phone is actually near it, and does it
need a coin cell after all? **Do the arithmetic before the PCB**, exactly as #3
does for composite video.

⚠️ Note the collision with your own `[Not Doing]` on low power (§5.4): if
harvesting is the interesting part, that decision may deserve revisiting for
this one post.

**Effort:** weekend ×2 plus a board spin. **Hardware:** PCB, AT89LS52, ST25DV,
coin cell. **Feasibility:** high for the ISP-contrast half, unknown for the
harvesting half until measured.

### #33 — A desk clock with the Robotron allure: industrial design as a firmware constraint

Your note pairs *"Desktop clock with KiCAD PCB"* with a 1970s Spartus Comet II
LED clock, a Hammond **1593KBK** enclosure *"as they have CAD"*, and fake
wood-grain vinyl.

DS1302 interfacing is already published (§5.4), so this is not an RTC post. It
is a post about the parts of an embedded project that firmware people skip: the
enclosure came first, Hammond publishes a STEP model, and therefore the KiCad
board outline is *derived* rather than guessed. That is a genuinely useful
workflow to document and it is nearly absent from hobby writing.

The firmware spine is real too. A red-filtered LED display in a wooden box is a
brightness problem, and §2.4's scanline budgets plus the **220 µA per segment**
ceiling from #23 decide the multiplex duty cycle. 6T versus 12T (#11) changes
the refresh rate. So the aesthetic choice — deep red filter, dim room, 1979 —
propagates all the way down to an option byte.

**Effort:** weekend ×2. **Hardware:** PCB, Hammond case, filter, vinyl.

### #34 — Unit tests and coverage for a chip with no debugger

Your note: `[CI CD] Unittests`. The generic list adds *"Bare-metal GCOV"*. They
are the same post and it is the most useful unwritten one in this document.

Two tiers, and the second is the novel part:

- **Host-compiled tests.** Compile the pure logic — protocol decoders, fixed-
  point maths, the scheduler — against a fake SFR layer with `gcc` and run them
  in CI in milliseconds. Standard practice everywhere except embedded hobby
  work, where it is rare enough to be worth demonstrating.
- **Coverage on target, which SDCC does not give you.** There is no `gcov` for
  mcs51. But ucSim can dump an execution trace, SDCC emits a `.cdb` debug file
  mapping addresses to source lines, and a hundred lines of Python turns one
  into the other. **Line coverage for an 8051 firmware, produced from a
  simulator trace, is a post nobody has written**, and it composes with #14,
  which asserts on cycles rather than lines.

**Effort:** weekend. **Hardware:** 🟢 none. **Feasibility:** high; the `.cdb`
format is documented in the SDCC manual and both tools are already installed on
this machine (§0).

### #35 — Does PlatformIO actually work for MCS-51?

Your note links `platformio.org`'s `intel_mcs51` platform. The post is short and
the search volume is real: PlatformIO wraps SDCC for the 8051, but does its
upload path know about **STC's UART bootloader**, which needs a power cycle and
`stcgal` rather than a programmer? Does it preserve the option byte that #11 is
about? A concrete, working `platformio.ini` that flashes an STC89C52 — or an
honest "it cannot, here is the `extra_scripts` hook that makes it" — is an
evening's work and answers a question people actually type into search engines.

**Effort:** evening. **Hardware:** 🟢 none.

### #36 — TinyML on an 8051, and why the lookup table wins

Your note: *"TinyMaix on 8051 / TinyML sine fitting example blog post from
Warden / Neurofuzzy Washing Machines / Maybe cover Microflow too."*

**This one is already measured.** `projects/tinyml-8051` in this repo built a
fixed-point MLP for the STC89C52 and drove it through ucSim:

| | flash | RAM | machine cycles | @12T |
| --- | ---: | ---: | ---: | ---: |
| fixed-point MLP kernel | **592 B** | **28 B** | **6 681** | 7.25 ms |
| whole program | 774 B of 8192 (**9.5 %**) | 39 B of 256 | — | — |
| SDCC software float | ×2.4 | ×2.4 | ×2.3 | 16.7 ms |

And the finding that makes it a post rather than a demo: on the sensor-
linearisation problem it was given, **a 28-byte lookup table meets the same
0.5 °C accuracy bar that costs the neural network 116 bytes of weights.** The
NN reaches 0.35 °C if you spend those bytes, but the honest headline is that
for a one-dimensional monotone curve the table wins on every axis.

So the post is *"I ran a neural network on a 1980s 8-bit microcontroller"* —
which is the catchy framing — with an ending that respects the reader. TinyMaix
itself will not fit (it targets parts with kilobytes of RAM); the contribution
is showing what does fit and what baseline it has to beat. Warden's sine
example is the canonical TinyML hello-world, and the 1990s neuro-fuzzy washing
machines are the historical anchor: this is not new, it is *returning*.

**Effort:** evening — the engineering is done, only the writing remains.
**Hardware:** 🟢 none. **This is the highest ratio of finished work to
remaining work in the whole document.**

**Added 2026-09-04 — the post now has a hook from outside itself.** The
*Embedded AI Podcast* **E22, "deploying models on small microcontrollers"**
(2026-09-04, 45:44) has Bosch's embedded-AI team describing exactly this problem
one hardware tier up, and it hands the post three things it did not have:

- **A named, counterintuitive industry finding to argue with.** Bosch: *"to go
  from a quantized model to a float model can actually help you... exactly the
  opposite of what people do on these larger systems"*, because quantisation
  costs accuracy and a low-dimensional input has little to spare — qualified by
  *"Cortex-M4 has [SIMD] instructions for quantized networks. Then there's a
  huge performance penalty if you switch to float. So this decision also depends
  on your hardware."* **The STC89C52 has neither an FPU nor quantised SIMD**, so
  it is the point where the trade-off stops being a trade-off. Measured in
  `projects/tinyml-8051`: float costs 2.4× flash, 2.4× RAM, 2.3× cycles.
  *"The industry argues about int8 versus float. On a part with no help for
  either, the argument is already settled — here is what it costs."*
- **A deployment threshold to sit below.** *"If you have a device large enough to
  run the TensorFlow Lite interpreter... if you can run ONNX runtime..."* — both
  are far above 8 KB. Bosch's escape is a code generator emitting plain C, for
  **functional safety**; this project hand-wrote C because nothing else fits.
  Same architecture, two unrelated constraints.
- **A silence worth naming.** Across 45 minutes on deploying models to small
  microcontrollers, **the lookup table is never mentioned once.** The tooling
  starts from an ONNX model and asks how to shrink it; nothing in the pipeline
  asks whether a table would do. That is the post's ending, arriving from
  outside rather than from its own measurements — and it is a much stronger
  ending than "my LUT was smaller".

Reference the episode by name and link; transcript held privately in
`docs/refs/podcast-e22/` (gitignored — it is copyrighted), quotations only.

### #37 — Getting grey out of a one-bit LCD: an ST7920 dithering tool

Your note: *"Maybe ST7920 dithering tool."* ST7920 interfacing is published, so
this is the follow-up: a host-side converter that error-diffuses an image into
the 128×64 monochrome buffer, and then the harder half — **temporal dithering**,
alternating frames to fake intermediate levels.

Whether the second half is possible at all is decided by §2.3's measured
finding: **UART mode 0 as a hardware SPI is 20× faster than bit-banging**. At
bit-banged rates the frame rate is too low and temporal dithering flickers
visibly; at mode-0 rates it may not. That is a computable answer and the post
should compute it before showing a photograph.

**Effort:** weekend. **Hardware:** 🟢 none (the board has the display).

### #38 — Banked RAM: an 8051 that addresses more than 64 KB

Your note: *"Computer with banked RAM, similar to Aki-80 board"* with the PJRC
TB51 as a candidate design.

This extends #5 rather than repeating it. Once you have an external bus, banking
is cheap in hardware — spare port pins become A16/A17 and page 32 KB windows —
and expensive in software, because **SDCC has no banked data model**. Every
pointer that might cross a bank becomes a hand-written accessor. The post is
about that asymmetry, which is the same asymmetry that killed 8-bit machines
generally, and the Z80 comparison (the Aki-80 lineage) writes itself.

**Effort:** more than a weekend. **Hardware:** SRAM, latch, glue logic.

### #39 — The 8051 did not die, it got absorbed: CH554 and CC2540

Your notes stage two parts with 8051 cores and peripherals the STC89C52 will
never have: the **CH554** (USB device and host, roughly a quarter) and the
**CC2540** (Bluetooth Low Energy plus USB, with your own note *"Needs Adapter
Layer"* pointing at a toolchain problem).

The framing that makes this more than a parts review: your 8051 assembly
knowledge ports unchanged, and nothing else does. Same core, three
irreconcilable toolchains, three flashing protocols, three sets of vendor
headers. It is the natural companion to #15's four-generations comparison, and
it is the answer to *"is it worth learning this architecture in 2026"* —
qualified yes, because it is still shipping inside things.

**Effort:** weekend each. **Hardware:** a CH554 board is inexpensive; CC2540 is
the harder and more interesting one.

---

### Five from the general embedded list, tested against this board

The same Anytype page carries a long *"Embedded Stuff and Retro Stuff"* queue
that is not 8051-specific. Most of it belongs to other platforms. Five items
turn out to be realisable on the HC6800-ES V2.0 with no new hardware, and one is
worth writing precisely because the answer is no.

### #40 — How much speech fits in 8 KB?

From *"Speech Synthesis on a 10-cent microcontroller."* On this part the
arithmetic is brutal and therefore interesting. Straight 8 kHz 8-bit PCM gives
you **about one second** of audio in 8 KB of flash. 2-bit ADPCM stretches that
to roughly four seconds — enough for a handful of spoken words, which is all a
talking clock needs. Real LPC synthesis in the TMS5220 tradition stores a few
hundred bytes per word instead, but needs a lattice filter evaluated at 8 kHz;
with `MUL AB` measured at **4 machine cycles**, that budget is tight at 12 MHz
and should be computed before anyone writes code.

Verdict is likely "ADPCM playback yes, LPC synthesis marginal", and the post is
the calculation plus a buzzer and an RC filter. **Effort:** weekend.
**Hardware:** 🟢 none (on-board buzzer) plus two passives.

### #41 — The reaction trainer, finished

Your note: *"Finish Reaction Trainer — charge pin issues, power management, IMU,
USB."* The unfinished hardware is a separate, later post. What is finishable
today is the measurement itself, on the board you own: keypad, 7-segment,
Timer0. And the content is better than it sounds, because **honest human
reaction timing is mostly a story about the errors** — debounce latency, the
measured ISR entry cost from §2.1, and display refresh aliasing all sit between
the stimulus and the number you print. **Effort:** evening. **Hardware:** 🟢 none.

### #42 — A four-function calculator, and where the bytes went

From *"Build Calculator"* (the Elektor multicalculator kit). The 4×4 keypad and
the 1602 LCD are both on the board and both already covered by published posts,
so the new material is arithmetic under constraint: no FPU, an 8-bit `DIV AB`,
and SDCC's software float costing **2.4× the flash and 2.3× the cycles** of
fixed point (measured, #36). A 16-bit fixed-point calculator that fits with room
to spare, next to a float one that does not, is the same lesson as #30 with a
usable artefact at the end. **Effort:** weekend. **Hardware:** 🟢 none.

### #43 — The 8051 as a battery lab: a NiCd rejuvenator

From *"RnD, turn into project NiCD rejuvinator, elektor 2023 circuit special."*
Everything it needs is already on the board or already ranked: software PWM for
a current source (#12), the **12-bit XPT2046 ADC nobody has switched on** (#9)
for cell voltage, and a timer. The dendrite-zapping pulse is the part that
deserves an honest safety paragraph rather than a schematic and a shrug.
**Effort:** weekend. **Hardware:** a little analogue.

### #44 — Which wavelength does this surface like?

From the LED-matrix reflectance eval-kit link in your *Vision Mastery* section.
Drive LEDs of several wavelengths in turn, read a photodiode through the XPT2046
(#9), and plot reflectance per wavelength. This is real machine-vision
groundwork — illumination selection is the first thing a vision engineer
actually does — with an 8051 as the acquisition box. Your own note observes it
*"could potentially be a BCard template"*, which links it to #32.
**Effort:** weekend. **Hardware:** LEDs, photodiode.

### #45 — SimpleFOC on an 8051: a feasibility post that ends in no

From *"Motors make the best knobs with SimpleFOC."* Field-oriented control needs
Clarke and Park transforms — sine, cosine, and signed multiplies — evaluated at
a few kilohertz. Against §2's measured costs and #36's float penalty, the budget
does not close at 12 MHz, and showing the arithmetic is more useful than another
"it works!" video. Then pivot to what the board *can* do: the stepper (#18) and
1000-step software PWM servos (#12). Pairs directly with #27's *what the silicon
genuinely cannot do*. **Effort:** evening, analysis only. **Hardware:** 🟢 none.

## 4. Summary table

| # | Title | Effort | Feasibility | Extra hardware | Books behind it |
| --- | --- | --- | --- | --- | --- |
| 1 | BASIC-52 revival on an STC89C52 | more | med | SRAM + latch | Mazidi 395–418; ALE 6/10/59; **Walter 47–60**; Mackenzie 259–264 |
| 2 | ISP protocol → WebSerial flasher in the browser | evening + weekend | high | 🟢 none | none (stcgal captures) |
| 3 | Composite video: the arithmetic, then the demo | evening → more | high (analysis) / med (picture) | 🟢 + RCA, opt. 40 MHz xtal | none (gap) |
| 4 | UART mode 0 is your hardware SPI (20× measured) | evening | high | 🟢 none | Ayala 189; Mackenzie 119/282–290; Schultz 310 |
| 5 | Build a real SBC with an external memory bus | weekend | high | SRAM, 74HC573 | **Walter 47–60**; Mazidi 395–418; **Limbach 39–60**; Mackenzie 259–264/279–281 |
| 6 | Two 8051s, one wire, shared-clock scheduling | weekend | high | 2nd board | **Pont 553–607**, **550–552** |
| 7 | Your datasheet describes a different chip | evening | high | 🟢 none | **Limbach 16–22**; Walter 19 |
| 8 | IAP: self-programming flash and OTA update | weekend | high | 🟢 none | Braithwaite 136–140 (nearest) |
| 9 | **The 12-bit XPT2046 you already own** | evening ×3 | high | 🟢 **none** | Mazidi 355–394; Mackenzie 296–305; Subrata 277–288; Gimenez 39–41 |
| 10 | The clock that forgets: DS1302 + the battery mod | evening | high | 🟢 none (+cell) | Mazidi 437–460; Anbazhagan 52; Schultz 296 |
| 11 | 6T mode: one checkbox, 25 broken demos | evening | high | 🟢 none | Ayala 110–116 (framing only) |
| 12 | Software PWM done right + 1000-step servos | weekend | high | servo | **Subrata 313–324**; Braithwaite 86–88; Gimenez 36–38 |
| 13 | A UART monitor + disassembler | more | med | 🟢 none | **Limbach 179–220**; **Schultz 249–259** |
| 14 | Cycle-accurate CI in ucsim | weekend | high | 🟢 none | Mackenzie 247–257; Schultz 219–267; Ayala 107–108 |
| 15 | STC89 vs STC12 vs STC8H vs CH32V003 | weekend | high | CH32V003 | Subrata 355–380; Walter 200–216; Limbach 265+; Braithwaite 142–168 |
| 16 | RS-485 multidrop, 9-bit + SADDR/SADEN | weekend | high | 2× MAX485 | **Pont 608–674**; **Braithwaite 185–191**; Ayala 186/197 |
| 17 | The board that fights itself: P2 is quadruple-booked | evening | high | 🟢 none | **Walter 217–235**; Ayala 103–106; Schultz 312–327 |
| 18 | The on-board stepper motor | evening | high | 🟢 none | Mazidi 424–436; Subrata 301–312; Mackenzie 310–314; Schultz 180/198 |
| 19 | SD card: why FAT does not fit | weekend | med (raw) / low (FAT) | SD breakout | none (gap); Mazidi 475–486 nearest |
| 20 | nRF24L01 on the unused header | weekend | med-high | 2 modules | Mazidi 475–486; Gehlot 115–118 |
| 21 | Power-on self-test (and 6T detection) | evening | high | 🟢 none | **Ayala 107–108** |
| 22 | Idle/power-down measured + cold vs warm start | evening | high | 🟢 none | **Subrata 325–338**; Gimenez 90–91; Walter 106 |
| 23 | Charlieplexing, and why 220 µA makes it hard | evening | med (likely negative) | LEDs | none (gap); Limbach 16–22 |
| 24 | The board that lies: the P0.5/P0.6 fault | evening | high | 🟢 none | **Limbach 16–22**; Walter 19 |
| 25 | Frequency counter, ceiling 500 kHz | evening | high | 🟢 none | **Schultz 298–301**; Ayala 158–161; Balaji 50 |
| 26 | Tic-tac-toe, and what fits in 8 KB | evening | high | 🟢 none | Mackenzie 353–374 |
| 27 | What the silicon genuinely cannot do | evening | high | 🟢 none | Braithwaite 86–92 (contrast) |
| 28 | Optocoupler + TRIAC: switching mains | weekend | med | mains ⚠ | **Gimenez 38**; Mazidi 419–436; Balaji 46 |
| 29 | Centronics *input*: the 8051 as a printer | weekend | med | DB25 | **ALE 27–28** |
| 30 | Where your 8 KB goes: SDCC codegen post-mortem | evening | high | 🟢 none | Schultz 174–218; Mackenzie 151–190; Walter ch5/ch7 |
| 31 | LONG TASK, DOMINO TASK, and when more CPUs hurt | weekend | high | 2nd board | **Pont ch.29**, 550–552; Schultz 271–343 |
| 32 | Business card: AT89LS52 ISP contrast + ST25DV NFC mailbox | weekend ×2 + PCB | high (ISP) / ⚠ (harvesting) | PCB, AT89LS52, ST25DV | none (your queue) |
| 33 | Desk clock: Hammond CAD → KiCad outline, red-filter duty cycle | weekend ×2 | high | PCB, case, filter | none (your queue) |
| 34 | **Unit tests + line coverage from a ucSim trace** | weekend | high | 🟢 none | none — genuinely unwritten |
| 35 | Does PlatformIO's `intel_mcs51` flash an STC? | evening | med | 🟢 none | none (your queue) |
| 36 | **TinyML: the NN runs, and the 28-byte LUT still wins** | evening (measured already) | **high — done** | 🟢 none | `projects/tinyml-8051` |
| 37 | ST7920 dithering, and whether temporal dithering is possible | weekend | med | 🟢 none | none (your queue) |
| 38 | Banked RAM, and why SDCC has no data model for it | more | med | SRAM + glue | Walter 47–60 (bus half) |
| 39 | CH554 and CC2540: the 8051 got absorbed | weekend ×2 | med | CH554/CC2540 | none (your queue) |
| 40 | How much speech fits in 8 KB? | weekend | med (likely ADPCM-only) | 🟢 + RC filter | none (gap) |
| 41 | Reaction trainer: the errors are the content | evening | high | 🟢 none | Schultz 298–301 (timing) |
| 42 | Four-function calculator, fixed point vs float | weekend | high | 🟢 none | Schultz 174–218 |
| 43 | NiCd rejuvenator: PWM source + the 12-bit ADC | weekend | med | analogue front end | Mazidi 355–394 |
| 44 | LED reflectance per wavelength (vision groundwork) | weekend | high | LEDs, photodiode | Mazidi 355–394 |
| 45 | SimpleFOC on an 8051: the budget does not close | evening | high (analysis) | 🟢 none | Braithwaite 86–92 |

**Nineteen of the forty-five need no hardware you do not already own**, and four
of those use peripherals physically on your board that have never been switched
on. Seventeen are evening-sized, and #36 is written already.

---

## 5. Gap analysis

### 5.1 What the fifteen books collectively do not contain

Verified across all fifteen. Anything here is original work:

RS-485/MAX485 (except Pont) · CAN (except Pont) · LIN · Modbus · DMX512 ·
WS2812/NeoPixel · **charlieplexing** · rotary/quadrature encoders · capacitive
touch · **SD/MMC/FAT** · **VGA/composite video** · PS/2 host · USB device ·
e-paper/OLED/SSD1306/Nokia 5110 · HX711/load cell (mentioned once, never built) ·
FFT/digital filters · **PID and closed-loop control of any kind** · fuzzy logic ·
neural networks · LFSR/CRC as a project · **bootloaders, ISP and IAP
self-programming** · zero-cross detection · phase-angle dimming · energy
metering · SMPS/boost converters · appliance controllers.

The absence of **PID and any closed-loop control** across all fifteen is
striking — including Ibrahim, whose reputation is control systems and whose 8051
book returns zero hits for PID, feedback, closed-loop, filter or sampling. If you
wants an unoccupied niche with real depth, **closed-loop control on an 8051** is
unwritten anywhere in this corpus — a temperature controller using the on-board
NTC (#9) and a heater, tuned properly, with the sampling-rate arithmetic done
against the 60 µs ISR budget of §2.1. It did not make the ranked list only
because it needs a plant to control; if you ever build one, it belongs around #10.

### 5.2 The three gaps that matter most for *your* blog

1. **Everything STC-specific.** All fifteen books are generic Intel 8051 or
   Philips/Siemens derivatives. Not one covers ISP, IAP, the option byte, 6T,
   `SADDR`/`SADEN`, or the bootloader. Your audience — per your own analytics in
   2025.10.19, heavily Asian and arriving on STC clone content — is there *for
   exactly the material no book contains*. Ideas #2, #7, #8, #11 and the §0.3
   crystal trick are the highest-leverage content available to you, full stop.
2. **Four unused peripherals on your own board**, plus a fifth thing nobody
   documents: the board's analog output is software PWM, so #9 and #12 are one
   project. Zero acquisition cost, zero shipping wait, each extending a published
   post.
3. **Your own published code has a 20× performance bug.** The UART mode 0 finding
   (§2.3) applies to the ENC28J60 driver, the ST7920 driver and the 74HC595
   demos, and directly contradicts a sentence you published in 2025.11.18. There
   is even a commit in the ethernet branch history titled *"Uart not working"*.
   Correcting yourself in public with measurements is reliably good content.

### 5.3 Already in flight — do not propose as new

- Branch `encoder_servo`, commit `abf3ee0`, `05_enc_servo/enc.c`: **a rotary
  encoder demo** on `P1.5`/`P1.6`/`P1.7`, unpublished. The servo half of that
  folder is empty; #12 completes it.
- Branch `ethernet` is the current head; the ENC28J60 work is recent (`790128f`
  → `460708b`, with `5683697 "Uart not working"` in between).
- `01_led_button_hyst` exists as a build artefact but not in the published README.

---

### 5.3b Two finished posts stranded on branches — and a correction

**Written 2026-09-04, corrected the same day.** From `~/workspace/svelte-blog`.

First, a trap worth recording: **the repo has two long-lived branches and only
one is live.** `origin/HEAD` points at `master`, which carries **85 posts**.
`main` holds **3**, last moved 2025-01-21, and is 137 commits behind — a dead
leftover from the Svelte 5 migration. Measuring "is this published?" against
`main` gives the wrong answer for everything, and doc 23's own §5.4
reconciliation would have been wrong had it used `main`.

Against `master`, **twenty published posts mention 8051/STC/8031**, including a
dense run from 2025-10-19 to 2025-11-24. That published record holds.

> ⚠️ **Correction.** An earlier version of this section claimed that fourteen
> branches carried *unmerged corrections to live posts that readers were not
> seeing*. **That was wrong, and backwards.** Checking all branch/post pairs
> rather than a sample: **`master` holds the newer text in every one**, several
> are byte-identical, and merging those branches would **revert** published
> fixes. The decisive case is `continued-karate` — **4,483 deletions against 15
> insertions** relative to `master`, and `master`'s commits on the same files
> are titled "Link fix", "Stylistic fixes" and "Description fix". They are stale
> forks left behind by a squash-merge workflow, not withheld work. The original
> claim came from treating "diverged from `master`" as "ahead of `master`",
> which is exactly the ancestry error §5.3b now exists to warn about.

**What is genuinely lost is smaller and worse.** Two branches add a complete
post file that has never appeared on `master`:

| Branch | Adds | Stranded since |
| --- | --- | --- |
| **`encoder_stepper`** | `2025.11.20.md` | 2025-11-20 |
| **`ethernet`** | `2025.11.29.md` | 2025-12-07 |

- **`encoder_stepper`.** §5.3 recorded a rotary-encoder demo as *unpublished
  code*. The **article is written too**. With the Anytype task placing servo and
  stepper work in Q4 backlog, **#12 and #18 are closer to done than anything else
  in this document.**
- **`ethernet`** — 11 commits, the largest of the set. §5.4 records the ENC28J60
  project as `[Aborted]` over the SPI errata, so this is **a finished,
  unpublished write-up of the project you abandoned** — and per §5.4 that abort
  is the strongest argument for #4 (UART mode 0 as a hardware SPI). A post about
  why a project failed, already written, is #4's opening section sitting on a
  branch nine months old.

**[I] The actionable reading, revised.** Not "merge seventeen branches" — that
would undo published fixes. It is: **publish two finished posts, and delete the
stale forks.** Smaller than the original claim, and the two posts are worth more
than any new idea in §3 or §3b, because the writing is already done.

### 5.4 Reconciled against your own queue in Anytype

**Read 2026-09-04, read-only, from the page "Personal Website / Blog" and the
task "Servo/Stepper Blog Article".** §3's original thirty-one were derived from
the books, the datasheet, the schematic and the git history. None of those
sources knows what you have already published or already decided against. This
does.

**Eight of the topics §3 assumed were open are already written.** Your completed
list, verbatim from the page: rotary encoder as input; 7-segment and 1602 LCD;
ST7920 interfacing; button, keypad and buzzer; LED interfacing and LED matrix;
Dallas 1-Wire (DS18B20, DS1302); I²C for EEPROM; serial IrDA with remote
decoding. The collisions:

| Idea | Status against the published set |
| --- | --- |
| **#10 DS1302 clock** | ⚠️ **Largely spent.** "Dallas 1-Wire: DS18B20, DS1302" is marked `[Done]`. What survives is the *battery mod* and the drift measurement, not the interfacing. **Demote from the top ten.** |
| **#24 the P0.5/P0.6 fault** | Survives. 7-segment interfacing is written; a post about a *correlated hardware fault* found while using it is new and better. |
| **#26 tic-tac-toe on the matrix** | Survives. LED matrix interfacing is written; the game and the 8 KB budget are not. |
| **#23 charlieplexing** | Survives, narrowly — same peripheral, genuinely different technique. |
| **#37 ST7920 dithering** | Deliberately positioned *after* the published interfacing post; that is why it is a tool, not a tutorial. |

**Three are decisions you have already made against, and your reason beats my
ranking.** The page marks `[Not Doing]` on *"Low Power + tool"*, *"Watchdog
Timer if common on 8052 + tool"* and *"Power Down Mode"*, each annotated **"(not
common)"** — i.e. these features vary so much between 8051 derivatives that a
post about the STC's version does not transfer to a reader holding a different
part. That is a sound editorial rule and I did not apply it.

> **This kills #22 as ranked** ("Idle, power-down, and telling a cold start from
> a warm one", which §3 marked 🟢 and rated high), and it undercuts the
> power-management half of **#21**. Both are left in place with this note rather
> than deleted, because the *self-test* half of #21 stands on its own and
> because #32's NFC harvesting question may make the low-power material earn its
> place after all — in service of a project, not as a feature tour.

**One is aborted, and the reason is technically load-bearing.** Ethernet via
ENC28J60 is marked `[Aborted]`: *"Silicon errata prevents less than 8 MHz
interfacing via SPI, this thing can effectively not be bitbanged."* §5.3 of the
first draft reported branch `ethernet` as the current head and treated it as
live work; it is not. More usefully, this is **independent corroboration of
§2.3's biggest measured finding** — bit-banged SPI on this part is too slow for
a peripheral with a real clock floor, and UART mode 0 (#4) is the only way to
reach one. The abort is the strongest argument for #4 in the document, and #4
should cite it.

**One is already scheduled.** The task *"Servo/Stepper Blog Article"* sits in
**Q4, Backlog, initiative "Blog & Leads"**, carrying a single link to
Microchip's stepper application note **doc4214**. So **#12 and #18 are not new
ideas — they are committed work**, which raises them: the servo half of the
empty `05_enc_servo` folder (§5.3) is the same project.

**And one is adjacent, not colliding.** `[Not doing]` covers a *JavaScript
simulation of the 8051*; **#2 is a JavaScript flasher**, which is a different
thing, and #14's simulator work runs in ucSim rather than a browser. Neither is
affected.

**Nine staged ideas §3 never considered** are elaborated as #32–#39 and #45
above. The one to notice is **#36**: the engineering already exists in
`projects/tinyml-8051`, measured, so it is the cheapest post in the entire
document to finish.

## 6. Which books earn their place

Of the fifteen, **six** carry the study, **six** pull their weight in one place,
and **three** could be deleted without loss.

### Earn their place

| Book | Why | Ideas |
| --- | --- | --- |
| **Pont, *Patterns for Time-Triggered Embedded Systems* (2008)** | The richest single seam, as the first session found. Part F is 150 pages of multi-processor scheduling with full listings and timing analysis that exists nowhere else, and §25.5's reliability scepticism is the best-argued page in any of the fifteen. | 6, 16, 31 |
| **Schultz, *C and the 8051* (2004)** | The only book about embedded *software architecture*: schedulers, RTOS concepts, semaphores, tick-overload handling, mixed C/assembly, a complete monitor listing, and debugging as a discipline. | 4, 12, 13, 14, 17, 25, 30, 31 |
| **Walter, *Mikrocomputertechnik* (2008, DE)** | Two things nobody else has: **memory-bus timing arithmetic against real datasheets** (§3.7 p47–60) and a **hardware-development-process chapter** (ch9: requirements → CPU load → BOM → layout → manufacturing docs → diagnosis). Plus the true-RMS meter built three ways as a controlled experiment in code organisation, and four mock exams with solutions. | 5, 15, 17, 24, 30 |
| **Mazidi (2014)** | The thorough reference for the standard peripheral set with real datasheet detail. Where you go when the chapter needs to be *right* rather than novel. | 5, 9, 10, 16, 18, 19, 28 |
| **Mackenzie & Phan (2007)** | The widest bank of discrete interface examples, and uniquely **every one appears twice — once in assembly, once in C**. Sole source for the 8155, and the only book with a bank of specification-level student projects. | 4, 5, 9, 10, 18, 25, 26 |
| **Limbach, *Kompaktkurs* (2002, DE)** | Not really an 8051 book — a book about building *and instrumenting* a small computer. Semiconductor physics up through port electrical structure, SRAM timing diagrams, PAL address-decoder design, then 150 pages of host-side software ending in a target monitor plus a Windows debugger **with a disassembler**. The only book here about the tooling. | 5, 7, 13, 23, 24 |

### Pull their weight in one place

- **Subrata (2010)** — sole source for **RC servo control with home-position
  detection** (ch23 p313–324) and for the **quantitative power-management
  comparison with cold-vs-warm-start detection** (ch24 p325–338). Two chapters,
  both irreplaceable. (12, 18, 22)
- **Ayala (1991)** — sole source for **power-on self-test** (p107–108) and for
  **hardware interrupt-expansion circuits** (p166–176). Scanned, no text layer,
  still the only place those appear. (5, 21, 25)
- **Braithwaite/Calcutt (2004)** — the Philips angle: PCA, hardware PWM, on-chip
  I²C/SPI/ADC/DAC, and **Project 3, a single-wire multiprocessor bus on 9-bit
  mode** (p185–191). Mostly useful as "what the STC89 does not have". (8, 12, 15, 16, 27)
- **Gimenez (2019)** — sole source for **optocoupler + TRIAC AC load switching
  with component sizing** (p38); good on interface-circuit theory. (9, 12, 28)
- **ALE (1991)** — 78 pages, two unique ideas: **Centronics as an input** (p27)
  and, with Schultz, **memory bank switching** (p59). Also the only dual-UART
  expansion treatment (p43). (5, 29)
- **Balaji (2002)** — one unrepeatable chapter of Indian telecom field work:
  **16 kHz telephone metering-pulse detection** (p31–35), an **ICL7109 dual-slope
  ADC** data acquisition system (p43), a frequency counter as a product (p50), an
  8-channel sequential controller (p46). Nothing else in the set resembles it, and
  little of it fits this blog — but it is the only place those exist. (25, 28)

### Do not earn their place

- **Ibrahim (2000)** — confirmed by the first session: 178 pages, 24 projects,
  almost entirely inside existing coverage, and zero control-systems content
  despite the author's reputation. **Two things survive**: the DS1620 thermostat
  (p108–118) and the interrupt-driven ADC0804 (p132–145). Its per-project
  template — *Function / Circuit / PDL / Listing / Components* — is worth
  stealing as a blog-post structure and is arguably its most valuable
  contribution. *(The DS1620 idea — a sensor with non-volatile setpoints and
  hardware comparator outputs that keeps working after you remove the CPU — is a
  good evening post and would slot around #24; it fell off the ranked list only
  because it needs a part you do not have.)*
- **Anbazhagan (2020)** — machine-paraphrased text of poor quality, though the
  schematics and listings are real. Everything in it (RFID, GSM, Bluetooth,
  DS1307, code lock, DTMF robot) is better sourced elsewhere or available in a
  thousand module tutorials. Skip.
- **Gehlot (2021)** — a competent 8051+ESP8266 cookbook, but its premise (the
  8051 does I/O, an ESP8266 does the thinking) is the opposite of what makes this
  blog interesting, and its sensor chapters duplicate Mazidi's. Skip, except as a
  cross-check on module wiring.

**Practical note for any future session:** **ALE, Ayala and Schultz are scanned
with no text layer** and there is no OCR tool on this box — they can only be read
page by page, not grepped. Budget accordingly.

---

## 7. Loose ends — the honest list

Most of the first draft's open questions were closed by the datasheet. These
remain:

1. **Which crystal is actually fitted** (§0.5). The schematic and the vendor
   manual say 12 MHz; retail listings and one vendor example folder say
   11.0592 MHz; the socket is 3-pin on the original revision. **Every number in
   §2 is given for both.** §0.3's bootloader trick resolves it without an
   oscilloscope, and #2 turns that into a post.
2. **Which `U2` is fitted** — 74HC245 buffer (original) or 74HC573 latch
   (revision). Same socket, same 1:1 map, different failure signatures. Matters
   for #24. Readable with the naked eye.
3. **The nRF24L01 header pin assignment** is the one item in §1 marked *likely*
   rather than confirmed — the schematic's text layer is offset in that block.
   Meter it before soldering (#20).
4. **The watchdog self-contradiction** (§2.5): the feature list says
   "one-time-enabled", the body text says `EN_WDT` can be cleared. Bench it. This
   affects #6 (master-loss detection) and #22.
5. **IAP endurance is unspecified** in all three datasheet editions. #8 should
   say so rather than borrowing STC15's number.
6. **Bootloader reserved size.** `stcgal`'s model table gives STC89C52
   `total=16384, code=8192, eeprom=6144`, implying ~2 KB reserved. #13's monitor
   and #8's second-stage bootloader both have to live somewhere; confirm before
   committing to a memory map.
