# FroggersTiga Device Manual

Firmware for **Daisy Field**: external input plus three VCOs, drive/DSP, algorithmic reverb, CV modulation, and Marbles-style random sources.

## Quick Start

- Power on and connect audio in/out.
- **`SW1` / `SW2`**: previous / next parameter page.
- **Knobs 1–7**: page parameters (after modulation and fuegoization — see below).
- **Knob 8 (`FUEG`)**: fuegoizer amount on pages that use it.
- **`A1..A7`**: hold to assign modulation from `M1..M7`.
- **`A8` / `B8`**: VCO1 / VCO2 waveform.

## Signal flow (audio)

```text
input + oscillator mix
  → Drive (polynomial drive, fuzz, digital reorganizer, SRR)
  → Pure delay
  → Comb filter
  → Resonant bump
  → Reverb (wet/dry)
  → output
```

Modulation and Marbles run in parallel; they shape **parameter values**, not a separate audio bus.

## Fuegoizer (`FUEG`, knob 8)

The fuegoizer is key to how FroggersTiga feels on the Field. The FUEG parameter controls how strongly knobs **1–7** are “fuegoized” before they reach the DSP.

Each affected parameter is a value from **0 to 1** (after CV/modulation). The firmware treats that value like an **8-bit number** (0–255). With fuegoization active:

1. **Modulation is applied first** (your `M1..M7` amounts).
2. **FUEG** picks how many **low bits** participate in a scramble (more `FUEG` → more low bits).
3. Those low bits are **XOR-mangled** (bit flips, shifts) in a pattern that depends on **which knob** on the page you are on.
4. The result is what the synth actually uses.

**At `FUEG` = minimum:** knobs behave normally — smooth, predictable control.

**As you raise `FUEG`:** the **fine resolution** of knobs 1–7 gets destroyed on purpose — stepped, gritty, “wrong” values between stable islands. Small knob moves can jump the sound dramatically. This is the main performance character of the instrument.

### What it does to the hardware knobs (pickup)

When fuegoization is active, the firmware does **not** require the physical knob to match the stored value exactly. It only compares the **high bits** (the part fuego leaves alone). So:

- **High `FUEG`:** easier to “catch” a parameter when you touch a knob, but **coarser** effective resolution.
- **Low `FUEG`:** normal pick-up behavior; you must land closer to the stored value.

Display tracking badges (`<`, `>`, space, `-`) still reflect this logic.

### Which pages have `FUEG`

| Page   | Knobs 1–7 fuegoized? | Knob 8 |
|--------|----------------------|--------|
| Audio  | Yes                  | `FUEG` (also PM3 — see Audio page) |
| Marbles| Yes                  | `FUEG` |
| Reverb | Yes                  | `FUEG` |
| Filter | Yes                  | `FUEG` |
| Drive  | Yes                  | `FUEG` |

`B1` randomize **does not** randomize `FUEG` itself (so you can randomize the other seven without losing your fuego setting).

### Audio page exception: PM3

On the **Audio** page only, the **raw `FUEG` knob** (not fuegoized) is also read as **PM3** depth: extra phase modulation from **VCO2 → VCO3** when the cross-coupler is on the 2→3 side. So on Audio, knob 8 is **both** the page fuegoizer **and** a dedicated PM depth control.

---

## Page order (`SW1` / `SW2`)

1. **Audio** — oscillators, coupling, PM, level  
2. **Marbles** — random mod sources `M6` / `M7`  
3. **Reverb** — stereo-ish algorithmic reverb send  
4. **Filter** — delay, comb, resonant peak (in series before reverb)  
5. **Drive** — distortion / digital destruction (first in the FX chain after osc mix)

---

## Buttons

### B row

| Key | Action |
|-----|--------|
| `B1` | Randomize knobs 1–7 on **current** page (not `FUEG`) |
| `B2` | Randomize knobs 1–7 on **all** pages |
| `B3` | Randomize modulation assignments on **current** page |
| `B4` | Randomize modulation on **all** pages |
| `B5` | **Marbles increment** — step both Marbles bags (see [Marbles](#marbles-page)) |
| `B6` | Randomize **XCPL** toward **1→2** coupling (lower half of knob) |
| `B7` | Randomize **XCPL** toward **2→3** coupling (upper half of knob) |
| `B8` | Cycle **VCO2** wave: sine → saw → square → sine |

### A row

| Key | Action |
|-----|--------|
| `A1..A7` | Hold to select modulation source **`M1..M7`**; turn knobs to set depth |
| `A8` | Cycle **VCO1** wave: sine → saw → square → sine |

---

## Modulation (`M1..M7`)

### Assignment

1. Open the page with the targets you want.  
2. Hold **`A1..A7`** for the source you want (`M1` = first CV, …, `M7` = Marbles 2).  
3. Turn knobs to set how much that source pushes each parameter.  
4. Release the key to exit mod-assign mode.

The display shows **`M#`** and per-knob tracking when assigning.

### Sources

| Source | Origin |
|--------|--------|
| `M1..M4` | External CV inputs |
| `M5` | **VCO feature**: every 64 samples, average \|VCO1+VCO2+VCO3\| is sampled, tanh-compressed and smoothed → slow 0–1 control (not audio-rate) |
| `M6` | Marbles channel 1 (smoothed marble value) |
| `M7` | Marbles channel 2 |

### External CV bypass

If `M1..M4` is assigned but no active CV is detected, that mod source is **ignored** and the parameter uses knob + other mods only.

---

## Audio page

Three oscillators mixed into the input, with cross-coupling and three PM paths.

| Knob | Label | What it does |
|------|-------|----------------|
| 1 | `V1VO` | VCO1 frequency (exponential, ~20 Hz–20 kHz range via phase increment) |
| 2 | `V2VO` | VCO2 frequency (same mapping) |
| 3 | `V3VO` | VCO3 frequency (sine only) |
| 4 | `XCPL` | **Cross-coupler** — one knob, two directions from noon: **CCW** enables 1→2 coupling strength; **CW** enables 2→3; **noon** = independent VCOs. Strength uses exponential curve from noon. |
| 5 | `PM1A` | Phase-mod depth: **VCO2 modulates VCO1** when 1→2 coupling is active |
| 6 | `PM2A` | Phase-mod depth: **VCO1 and VCO3 modulate VCO2** (1→2 and 2→3 paths) |
| 7 | `OLVL` | How much the oscillator mix is added to the **external input** before Drive |
| 8 | `FUEG` | Fuegoizer for knobs 1–7; **also PM3 depth** (VCO2 → VCO3 PM when 2→3 coupling is on) |

**Waveforms:** VCO1 (`A8`) and VCO2 (`B8`): sine → saw → square. VCO3 is always sine.

**Coupling detail:** PM amounts are gated by coupling — e.g. `PM1` only does work when `XCPL` is turned toward 1→2.

---

## Marbles page

Two independent “bags” of random values, **manually** advanced with **`B5`**. Outputs feed **`M6`** and **`M7`**.

| Knob | Label | What it does |
|------|-------|----------------|
| 1 | `PROB` | Per-channel chance that a **`B5`** press actually steps (higher = more likely) |
| 2 | `DJV1` | Channel 1 **déjà vu** — below noon: step through bag, sometimes **re-roll** current marble; above noon: mostly step, sometimes **random jump** in bag |
| 3 | `SZ1` | Channel 1 bag size (**2–8** marbles) |
| 4 | `SLW1` | Channel 1 output slew (low-pass on the active marble value → `M6`) |
| 5 | `DJV2` | Same as `DJV1` for channel 2 |
| 6 | `SZ2` | Bag size channel 2 |
| 7 | `SLW2` | Slew channel 2 → `M7` |
| 8 | `FUEG` | Fuegoizer for knobs 1–7 on this page |

### `B5` (increment trigger)

Not a clock input — **you** are the clock.

Each press, **per channel**:

1. Roll against **`PROB`**; on fail, that channel does nothing this press.  
2. Otherwise advance the bag (next index, maybe jump or re-randomize per **`DJV`**).  
3. The active marble value is smoothed and written to **`M6`** / **`M7`**.

No presses = frozen random level (still smoothed). Fast presses = fast random walks.

**Typical use:** set Marbles on this page; assign `M6`/`M7` on Reverb/Filter/Audio; tap **`B5`** in time with music.

---

## Reverb page

Dual delay-line reverb with pre-delay, damping, and delay-time LFO.

| Knob | Label | What it does |
|------|-------|----------------|
| 1 | `RVMX` | Wet/dry mix (0 = dry only, 1 = wet only) |
| 2 | `RSIZ` | Room size — scales base delay lengths of the two lines |
| 3 | `RDEC` | Feedback / decay time (higher = longer tail) |
| 4 | `RPRE` | Pre-delay before energy enters the tank |
| 5 | `RDMP` | High-frequency damping in the feedback path (higher knob = brighter tail) |
| 6 | `RMOD` | LFO depth on delay times (chorus-like motion) |
| 7 | `RRAT` | LFO rate for that modulation |
| 8 | `FUEG` | Fuegoizer for knobs 1–7 |

---

## Filter page

Serial tone shaping **after** Drive, **before** reverb.

| Knob | Label | What it does |
|------|-------|----------------|
| 1 | `DELF` | **Pure delay** line frequency / length (short delay, comb-ish color) |
| 2 | `BUPF` | **Resonant bump** center frequency |
| 3 | `BUPR` | Bump **gain / resonance** (peak height) |
| 4 | `BUPW` | Bump **width** (Q) |
| 5 | `COMF` | **Comb** delay time (comb filter pitch) |
| 6 | `COMQ` | Comb **feedback** (resonance of comb peaks) |
| 7 | `CMLP` | Low-pass inside the comb feedback loop (darker comb when higher) |
| 8 | `FUEG` | Fuegoizer for knobs 1–7 |

---

## Drive page

First processing stage on `(input + oscillators)`.

| Knob | Label | What it does |
|------|-------|----------------|
| 1 | `GAIN` | Polynomial **drive** amount |
| 2 | `SHAPE` | Polynomial **curve / coefficients** (harmonic character) |
| 3 | `SRR1` | **Sample-rate reducer** 1 — lower rate = heavier decimation |
| 4 | `SRR2` | **Sample-rate reducer** 2 (second stage) |
| 5 | `DIGR` | **Digital reorganizer** XOR flip amount on sample bits |
| 6 | `HASH` | How many low bits the reorganizer scrambles (more = harsher digital grit) |
| 7 | `FUZZ` | Blend between sine-shaped and tanh-saturated distortion in the drive core |
| 8 | `FUEG` | Fuegoizer for knobs 1–7 |

**Drive chain (inside `FrogBlock`):** polynomial drive (+ fuzz blend) at 2× oversampling → digital reorganizer → SRR1 → SRR2.

---

## Build and flash (firmware update)

From `src/FroggersTiga`:

```sh
make clean
make
make program-dfu
```

Then reset or power-cycle the Field.

Use this after code changes so the flashed `.bin` matches your tree.

---

## Troubleshooting

- **Weird knob behavior on a page:** check **`FUEG`** — high fuegoization is supposed to make small moves jumpy.  
- **Modulation seems dead on CV:** confirm cable and that the input is above the auto-bypass threshold.  
- **Marbles not changing:** you must press **`B5`**; it does not free-run.  
- **After editing firmware:** run `clean → make → program-dfu` before judging behavior.
