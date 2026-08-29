# Frogg3rs

This is a synthesizer with three oscillators, an envelope per oscillator, a
filter, a drive stage, a delay and a reverb. It runs as a desktop app, as a VST3
or Audio Unit plugin, and in a browser. Every parameter is a knob on one of six
banks of sixteen.

These are some of the unique features in this digital synthesizer:

**Modulation goes three levels deep.** Any parameter can be modulated by any of
fifteen sources, and for the oscillators that includes themselves: VCO1's audio
output can drive VCO1's own pitch. Six of the sources are random sample and
hold lanes. The rest are taken from the instrument: each oscillator's raw
audio-rate output, each oscillator's envelope follower, white noise, and
external audio with its own envelope follower. The envelope followers run at
10 ms attack and 50 ms release, so they move at a few Hz rather than at audio
rate, and one oscillator can drive any parameter either at audio rate or at that
slower envelope rate depending on which source you pick.

Each modulation depth is itself a knob carrying those same fifteen sources, and
so is the depth of that one. The amount by which one thing moves another is a
thing you can move, and so is the amount by which that moves.

That compounds quickly. One parameter has fifteen depth knobs. Each of those
fifteen has fifteen of its own, which is 225. Each of those 225 has fifteen
more, which is 3,375. There is no fourth level, so a single parameter sits on
top of 3,615 knobs.

Most parameter ranges are exponential. A source pushes a parameter further at
one end of its travel than at the other, so the same modulation depth reads as a
different amount of movement depending on where the knob is sitting.

**Randomization is weighted to stay playable.** Randomize All draws new values
and new modulation depths across the whole instrument. Randomize Page draws
exactly what is on screen: a bank's values on a parameter page, or that view's
depths inside a modulation view. The number of sources a parameter picks up is a
weighted draw: two is the most common result, one parameter in five comes out
with no modulation at all, and four or more is rare. A randomized patch comes out
with some things moving and some holding still.

You may fork this repo if you wish to bias the randomization differently. I
weighted this after testing and adjusting to taste; your taste may differ.

There is also a bit-scramble on two knobs named Crispy and Crunchy, which
corrupts parameter values on their way to the DSP. At zero it does nothing.
Turned up, knob moves stop being smooth and values snap between newly fried
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

Froggers' DSP on [Sheaf](https://github.com/jvictor0/Sheaf)'s parameter and UI framework. Sheaf is a
pinned submodule at `External/Sheaf`; [`app/README.md`](app/README.md) records the pinned commit.

Build the playable app from the repo root:

```sh
./app/build-launcher.sh
```

It writes `app/build-launcher/Frogg3rs.app` and signs it. The script caps itself at `-j2` and runs
`nice`, which an 8-core machine needs; raising it can freeze the host.

Run the tests:

```sh
cd app && make test
```

`make test` has no `-k` and stops at the first failing binary of ten — check that all ten ran before
reading "green" into a partial result.

Change proposals and specs live under [`openspec/`](openspec/).

Parameter reference for this app: [`MANUAL.md`](MANUAL.md) (global controls, then all six banks —
Audio, Envelope, Filter, Drive, Delay, Reverb — parameter by parameter) and [`QUICK_DICT.md`](QUICK_DICT.md)
(the same six banks, one line per parameter).
