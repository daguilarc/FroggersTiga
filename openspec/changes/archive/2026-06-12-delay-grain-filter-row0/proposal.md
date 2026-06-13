## Why

Delay page row 4 is labeled **Tone** but the DSP is a one-pole smooth on the delay tap (`dton` as alpha; unused `m_toneAlpha`). Quick Dict says “LP in feedback path” — a darken-on-repeats smear, not tone in any useful sense. It landed in `stereo-delay-page` as the fifth of eight knobs (panel parity with Field + **FUEG**), not from a musical requirement.

Operators want **detune** on repeats — stereo pitch offset on the echo — which is a natural fit for that slot and distinct from **Mod depth** (LFO wobble on delay time). **Mod depth** = moving delay; **Detune** = pitched-shifted read on L/R.

Filter row 0 (`DELF` / `PureDelay`) stays a **short line before the comb** in core DSP. Sim UI shall **not** call it Filter delay or grain; rename to **Comb line** (line length before comb filter). Grain/detune does **not** move to Filter — detune lives on Delay only.

**Bump** on Filter rows 1–3 is implementation slang for a **peaking EQ biquad** (`ResonantBump` — freq, gain, Q). Sim labels shall use standard EQ words (**Peak freq**, **Peak gain**, **Peak Q**), not “Bump.”

**Reorganizer** on Drive row 4 is the XOR flip mask (`DIGR`); row 5 (`HASH`) is bit-scramble depth. Sim labels: **XOR**, **Bit depth** (matches what the knobs do).

**Reverb** rows 5–6 (`RMOD`, `RRAT`) are an LFO on delay-line lengths — chorus motion that reads poorly on a dual-line tank summed to mono. Replace with **Stereo width** (separate the two line outputs) and **Diffusion** (cross-feed in the feedback matrix). Desktop and web sim; core `FroggersEngine` DSP. Firmware OLED keeps `RMOD`/`RRAT` 4-char names.

## What Changes

- **Replace Tone with Detune** on Delay page row 4: display **Detune**; `DelayParams` field `dton` → semantic **detune** (keep index 4 / fuego row unchanged). DSP: stereo cents offset on wet read times (see design).
- **Filter row 0 display** — **Pure delay** → **Comb line**; Quick Dict `Short delay line before comb`.
- **Stereo max** — `kMaxDelaySeconds` **2.0** s, `kMaxDelaySamples` **96000**.
- **Remove** dead `m_toneAlpha` / tone smooth code from `StereoDelay`.
- **Filter rows 1–3** — **Peak freq**, **Peak gain**, **Peak Q** (was Bump center/gain/width).
- **Drive rows 4–5** — **XOR**, **Bit depth** (was Reorganizer, Bit depth display drift).
- **Reverb rows 5–6** — remove LFO; **Stereo width**, **Diffusion** DSP + display (was LFO depth/rate).
- **Supersedes** Filter-delay naming in `web-sim-core-fix` §1.

**Unchanged:** firmware 4-char OLED names (`RMOD`, `RRAT`, `DIGR`, `HASH`, …); Reverb rows 0–4; Delay page send/feedback/width/mod/mix.

## Capabilities

### New Capabilities

- `stereo-delay-detune`: Detune on Delay row 4 replaces Tone DSP and label.
- `reverb-stereo-diffusion`: Reverb rows 5–6 stereo width + diffusion replace LFO.

### Modified Capabilities

- `stereo-delay`: Row 4 role, max time 2 s, buffer 96000 @ 48 kHz.
- `sim-parameter-display-names`: full dictionary sweep (Detune, Comb line, Peak EQ, XOR/Bit depth, Reverb width/diffusion).

## Impact

- `src/core/FroggersEngine.hpp` — reverb tank (remove LFO; width + diffusion)
- `sim/StereoDelay.hpp`, `sim/ParamDisplayNames.hpp`, `QUICK_DICT.md`, `web/public/manual.md` (Reverb section)
- `web-sim-core-fix` — drop Filter-delay / Tone strings; stereo cap owned here
