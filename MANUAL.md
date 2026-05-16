# FroggersTiga Device Manual

This manual describes how to use the current FroggersTiga firmware on Daisy Field, including pages, button mappings, modulation routing, and the safest update workflow.

## Quick Start

- Power on the device and connect audio input/output as normal.
- Use `SW1` and `SW2` to change pages.
- Use the 8 knobs to edit parameters on the current page.

## Page Navigation

- `SW1`: previous page
- `SW2`: next page

## Keyboard/Button Mapping

### B row

- `B1`: randomize current page
- `B2`: randomize all pages
- `B3`: randomize current page modulation assignments
- `B4`: randomize all pages modulation assignments
- `B5`: Marbles increment trigger
- `B6`: randomize cross-coupler toward 1->2 side
- `B7`: randomize cross-coupler toward 2->3 side
- `B8`: cycle VCO2 waveform (`sine -> saw -> square -> sine`)

### A row

- `A1..A7`: modulation source select/hold for `M1..M7`
- `A8`: cycle VCO1 waveform (`sine -> saw -> square -> sine`)

## Modulation Assignment Workflow

1. Go to the page you want to modulate.
2. Hold one of `A1..A7` to choose `M1..M7`.
3. While holding it, turn one or more knobs to set modulation amount for those parameters.
4. Release the A-row key to exit modulation-tracking mode.

The display shows `M#` badges and tracking indicators for assigned parameters.

## Mod Sources (`M1..M7`)

- `M1..M4`: external CV inputs
- `M5`: VCO-derived control signal (non-audio-rate, compressed/smoothed)
- `M6`: Marbles output 1
- `M7`: Marbles output 2

### External CV auto-bypass behavior

If an external CV source (`M1..M4`) is assigned but no active CV is detected, modulation for that source is bypassed so the parameter falls back to knob-only behavior.

## Page Overview

## Audio Page

Primary synth controls (8 knobs, with `FUEG` in slot 8):

- `V1VO`: VCO1 tuning (exp mapping)
- `V2VO`: VCO2 tuning (exp mapping)
- `V3VO`: VCO3 tuning (exp mapping)
- `XCPL`: single cross-coupler
  - CCW from noon: emphasizes 1->2 coupling
  - noon: independent oscillators
  - CW from noon: emphasizes 2->3 coupling
- `PM1A`: PM depth 1
- `PM2A`: PM depth 2
- `OLVL`: oscillator injection level
- `FUEG`: page fuegoizer (also used as PM3 depth input)

Waveforms:

- VCO1 wave via `A8`
- VCO2 wave via `B8`
- both cycle `sine -> saw -> square -> sine`

## Reverb Page

- `RVMX`: wet/dry mix
- `RSIZ`: room size
- `RDEC`: decay
- `RPRE`: pre-delay
- `RDMP`: damping
- `RMOD`: modulation depth
- `RRAT`: modulation rate
- `FUEG`: page fuegoizer

## Filter/Drive/Marbles Pages

Legacy pages remain available and navigable via `SW1/SW2`.

## Safest Build + Flash Workflow

From `src/FroggersTiga`, use:

```sh
make clean
make
make program-dfu
```

Why this is safest:

- `make clean` prevents stale object/header issues.
- `make` forces a fresh firmware build.
- `make program-dfu` flashes the exact built binary.

After flashing, press reset (or power-cycle) to run the new firmware.

## Troubleshooting

- If controls do not behave as expected after a code change, always run the full safe workflow above (`clean -> build -> flash`) before debugging behavior.
- If modulation feels capped when using external CV, verify CV cable/source is actually active; inactive external CV is intentionally bypassed to knob-only behavior.
