# Frogg3rs

This is a synthesizer with three oscillators, an envelope per oscillator, a
filter, a drive stage, a delay and a reverb. It runs as a desktop app, as a VST3
or Audio Unit plugin, and in a browser. Every parameter is a knob on one of six
banks of sixteen.

Two things here work differently from other modular synthesizers.

**Modulation goes two levels deep.** Any parameter can be modulated by any of
fifteen sources. Six of those sources are random generators, and the rest are
taken from the instrument itself: each oscillator's raw audio-rate output, each
oscillator's envelope follower, a noise source, and external audio with its own
envelope follower. So one oscillator can drive any parameter in the instrument
at audio rate. Each modulation depth is itself a knob, so it can be modulated by
those same fifteen sources — the amount by which one thing moves another is a
thing you can move. Most parameter ranges are exponential, so a source pushes a
parameter further at one end of its travel than the other.

**Randomization is weighted to stay playable.** Randomize rolls new values and
new modulation depths across a bank or the whole instrument. The number of
sources a parameter picks up is a weighted draw: two is the most common result,
one parameter in five comes out with no modulation at all, and four or more is
rare. A randomized patch comes out with some things moving and some holding
still.

There is also a bit-scramble called fuego, on two knobs named Crispy and
Crunchy, that corrupts parameter values on their way to the DSP. At zero it does
nothing. Turned up, knob moves stop being smooth and values snap between
islands.

Full parameter reference: [`MANUAL.md`](MANUAL.md).

**Current development is the Sheaf-based app under [`app/`](app/README.md)** — see
[Frogg3rs — Sheaf app](#frogg3rs--sheaf-app-current-development) below. The original **Daisy Field**
hardware firmware under `src/` is **frozen** — kept in the tree and buildable, not under active
development — while that work continues. The pre-Sheaf desktop and browser simulators are gone:
`desktop/`, `desktop-v2/`, `sim/`, `wasm/`, and `web/` were deleted once the Sheaf app superseded
them, and `vcv/` was never populated with tracked files.

- **Sheaf app docs:** [`app/README.md`](app/README.md) — build instructions, Sheaf submodule pin, status
- **Manual:** [`MANUAL.md`](MANUAL.md) — global controls and all six parameter banks for the current
  Sheaf app
- **Daisy Field manual:** [`DAISY_MANUAL.md`](DAISY_MANUAL.md) — the frozen Eurorack firmware (pages,
  buttons, modulation workflow, safe flash sequence)
- **Quick Dict:** [`QUICK_DICT.md`](QUICK_DICT.md) — terse, slot-ordered parameter glossary for the
  current Sheaf app
- **License:** MIT — see [`LICENSE`](LICENSE) (copyright JoYoFresh and Diego Aguilar-Canabal)

## Frogg3rs — Sheaf app (current development)

The active line of development: Froggers' DSP ported onto [Sheaf](https://github.com/jvictor0/Sheaf)
(`External/Sheaf`, a pinned submodule — see [`app/README.md`](app/README.md) for the exact commit and
why). This is a from-scratch app on Sheaf's parameter/UI framework, not a continuation of the Daisy
Field firmware or the `desktop-v2` bridge spike (`desktop-v2` was cleared of that spike and frozen
once this app superseded it).

Build and test from the repo root:

```sh
./app/build-launcher.sh
```

```sh
cd app && make test
```

`make test` has no `-k` and stops at the first failing binary of ten — check that all ten ran before
reading "green" into a partial result.

Planning and design history for this app live under [`openspec/`](openspec/) (change proposals,
specs, design docs, handoffs).

Parameter reference for this app: [`MANUAL.md`](MANUAL.md) (global controls, then all six banks —
Audio, Envelope, Filter, Drive, Delay, Reverb — parameter by parameter) and [`QUICK_DICT.md`](QUICK_DICT.md)
(the same six banks, one line per parameter).
