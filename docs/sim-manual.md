# FroggersTiga Simulator Manual

**Release v1.0.3** — desktop app, web sim, and VST/AU plugin share this operator guide.

This guide covers the **desktop**, **web**, and **plugin** simulators. On-screen knob names match this manual. For Daisy Field hardware, see `MANUAL.md` in the repository.

## Quick start

1. Wait for **Engine ready — click Play** (web) or open the desktop app.
2. Click **Play** — VCO output is audible with **External / Ext. In.** off.
3. Use page pills or **◀ ▶** to switch pages (Audio → Random → Reverb → Filter → Drive → Delay).
4. Turn knobs 1–7 for parameters; knob 8 is **Crispy** on every page (see Global controls).
5. To modulate a knob, expand **Mod sources** (web) or use the mod rack (desktop) — covered in Mod bay below.

## Layout

Six pages — **Audio**, **Random**, **Reverb**, **Filter**, **Drive**, and **Delay** — each with eight knobs. Rows 1–7 are page parameters; row 8 is always **Crispy**.

| Host | How you navigate |
|------|------------------|
| **Web** | One page at a time; swipe or use pills |
| **Desktop** | Five adjacent panels plus a **Delay** overlay |
| **VST / AU** | Same layout as desktop; the DAW runs transport |

## Global controls

### Crispy

**Crispy** (knob 8 on every page) scrambles knobs 1–7 on that page. When external input is on, it also blends external ring-mod topology into the mix.

Modulation is applied **first**; Crispy then scrambles the low bits of the modulated result. Crispy itself can be modulated — scramble intensity follows the **effective** Crispy level. **Pair-AR** knobs (Audio page) accept mod CV but are **not** fuegoized.

### Transport

| Control | Action |
|---------|--------|
| **Play** | Start audio processing |
| **Stop** | Halt audio output |
| **External / Ext. In.** | Ring-mod external input (see Host guide) |
| **Randomize** (page) | Randomize knobs 1–7 on the current page |
| **Rand mod** (page) | Randomize mod sources and depths on the current page |
| **Rand All** | Randomize all pages + Delay |
| **Rand Mods** | Randomize all mod routes |
| **Rand Resample** | Resample both random S&H channels (draws from bags) |
| **Rand waveforms** | Randomize VCO waveform morph (Audio page) |

## Mod bay

Five mod sources drive per-knob modulation.

### Sources

- **MIDI CC 1** — Hardware or Web MIDI CC latched to CV (default: channel 1, CC 1).
- **MIDI CC 2** — Same for channel 1, CC 2.
- **VCO Envelope** — Slow level from the VCO mix; shown as a scope trace.
- **Random 1 S&H** — Stepped random CV on channel 1; see Random S&H below.
- **Random 2 S&H** — Stepped random CV on channel 2; see Random S&H below.

### Routing

On **web**, pick a source with the **Mod source** dropdown under each knob. When a source is selected, the knob controls **mod depth** instead of the base parameter.

On **desktop** and **plugin**, drag a patch cable from a mod rack jack to a knob input.

### Mod depth & blend

Mod depth is a **crossfade** between the stored knob value (base) and the mod source — not `knob × CV` (VCV attenuator style).

| Depth | Result |
|-------|--------|
| 0 | Base only |
| 1 | Mod source only |
| Between | Linear mix: `base × (1 − depth) + mod × depth` |

While a mod route is active, the on-screen knob shows the **live effective value** when idle. Dragging edits **mod depth**; the stored base is unchanged until you clear the route.

**MIDI CC 1** and **MIDI CC 2** are ignored when that CC input is disabled (grey column). Other mod sources are always active when patched.

### MIDI CC enable

Disable a CC input to grey its mod column, block new routes, clear existing ones, and exclude it from random mod.

- **Desktop / plugin:** MIDI Settings **On** toggle per CC pair.
- **Web:** **External MIDI** is the sole CC gate — CC 1 only (default channel 1, CC 1). MIDI CC 2 is not available on web.

**QWERTY keyboard** (desktop) drives MIDI CC 1 only and respects the CC 1 enable flag.

### Mod indicators

- **MIDI CC 1** and **VCO Envelope** show live CV scope traces on web and desktop.
- **MIDI CC 2** scope appears on desktop and plugin only.
- **Random 1 S&H** and **Random 2 S&H** show a green LED while **Play** is on: **green** when held CV is above **55%** of full scale; **dim** at or below 55%, or when audio is stopped.

### Random S&H

Each random channel draws a value from its bag when you press **Rand Resample**. There is no internal clock — values change only on a resample. **Slew** smooths glides between held values; it does not trigger new steps. Held voltage is normalized **0–100%** on the mod bus (the same CV you patch to knobs).

## Page reference

### Page 1 — Audio

Three VCOs, cross-coupling, and phase modulation. Click the waveform icon beside VCO1–VCO3 to cycle sine ↔ saw ↔ square morph.

**Phase mod 3** (row 7) sets VCO2 → VCO3 phase-mod depth when the cross-coupler is toward 2→3. **VCO Envelope** in the mod bay is a separate mod source — a slow CV trace from the VCO mix, not this knob.

| Row | Parameter | What it does |
|-----|-----------|--------------|
| 1 | VCO1 | Frequency + morph |
| 2 | VCO2 | Frequency + morph |
| 3 | VCO3 | Frequency (sine) |
| 4 | Cross-coupler | CCW 1→2, CW 2→3 from noon |
| 5 | Phase mod 1 | VCO2 → VCO1 when coupled |
| 6 | Phase mod 2 | VCO1+VCO3 → VCO2 |
| 7 | Phase mod 3 | VCO2 → VCO3 when cross-coupler is CW (2→3) |
| 8 | Crispy | See Global controls |

**Pair-sum AR (Audio only):** Four extra controls shape how the (VCO1+VCO2) and (VCO2+VCO3) sums rise and fall in the mix. On **desktop**, they appear as a horizontal band below the eight vertical rows (jack → knob → label). On **web**, they are a third row of four knobs on the Audio page only. Panel labels abbreviate **Attack** as **Att.** and **Release** as **Rel.** (**Rel.** is combined decay+release for the pair sum — not reverb row **Decay**). Each Att./Rel. knob spans **1 ms – 10 s** (exponential). The envelope **follows pair-sum level** (attack when the sum rises, release when it falls) — not a gate-triggered ADSR. The knob→time mapping matches VCV Fundamental ADSR Attack/Release **time range** only. Delay time (Page 6) is separate (~0–2 s).

| Control | Range | What it does |
|---------|-------|--------------|
| Att. 1+2 | 1 ms – 10 s (exponential) | Attack — rise time when the VCO1+VCO2 pair level increases |
| Rel. 1+2 | 1 ms – 10 s (exponential) | Release — fall time when the pair level decreases |
| Att. 2+3 | 1 ms – 10 s (exponential) | Attack — rise time for the VCO2+VCO3 pair |
| Rel. 2+3 | 1 ms – 10 s (exponential) | Release — fall time for the VCO2+VCO3 pair |

### Page 2 — Random

Dual sample-and-hold random CV. Knobs configure two independent bags; **Random 1 S&H** and **Random 2 S&H** in the mod bay are the outputs. Press **Rand Resample** to resample both channels. For stepping, slew, and LED behavior, see **Random S&H** in Mod bay.

| Row | Parameter | What it does |
|-----|-----------|--------------|
| 1 | Step chance | Probability each channel resamples on **Rand Resample** press |
| 2 | Deja vu 1 | Channel 1 bag walk / re-roll |
| 3 | Bag size 1 | Channel 1 values (2–8) |
| 4 | Slew 1 | Channel 1 glide between held values (not a clock) |
| 5 | Deja vu 2 | Channel 2 bag walk / re-roll |
| 6 | Bag size 2 | Channel 2 values (2–8) |
| 7 | Slew 2 | Channel 2 glide between held values (not a clock) |
| 8 | Crispy | See Global controls |

### Page 3 — Reverb

| Row | Parameter | What it does |
|-----|-----------|--------------|
| 1 | Wet/dry | Reverb mix |
| 2 | Room size | Delay line lengths |
| 3 | Decay | Feedback / tail length |
| 4 | Pre-delay | Time before reverb tank |
| 5 | Damping | HF loss in feedback |
| 6 | Stereo width | Reverb L/R spread |
| 7 | Diffusion | Cross-feed between reverb lines |
| 8 | Crispy | See Global controls |

### Page 4 — Filter

| Row | Parameter | What it does |
|-----|-----------|--------------|
| 1 | Comb offset | Short line before comb — smears strike, not pitch |
| 2 | Peak freq | Peaking EQ frequency |
| 3 | Peak gain | Peaking EQ gain |
| 4 | Peak Q | Peaking EQ Q |
| 5 | Comb delay | Comb pitch |
| 6 | Comb feedback | Comb resonance |
| 7 | Comb LP | Darken comb feedback |
| 8 | Crispy | See Global controls |

### Page 5 — Drive

| Row | Parameter | What it does |
|-----|-----------|--------------|
| 1 | Drive | Polynomial drive amount |
| 2 | Shape | Drive curve |
| 3 | SRR 1 | Sample-rate reducer 1 |
| 4 | SRR 2 | Sample-rate reducer 2 |
| 5 | XOR | XOR bit mask on samples |
| 6 | Bit depth | Low-bit scramble depth |
| 7 | Fuzz | Sine/tanh blend |
| 8 | Crispy | See Global controls |

### Page 6 — Delay

| Row | Parameter | What it does |
|-----|-----------|--------------|
| 1 | Delay time | ~0–2 s exponential |
| 2 | Send | Output to delay |
| 3 | Feedback | Delay feedback |
| 4 | Stereo width | L/R separation |
| 5 | Detune | Stereo pitch offset on repeats |
| 6 | Mod depth | LFO on delay time |
| 7 | Wet mix | Delay wet level |
| 8 | Crispy | See Global controls |

## Host guide

### Desktop

- Five adjacent submodule panels (no page switching) plus a **Delay** overlay page.
- **Mod rack** with patch cables — drag from a mod source to a knob to assign modulation (including the four pair-sum AR jacks on the Audio panel bottom band).
- **MIDI Settings** — two CC→CV inputs (MIDI CC 1 and MIDI CC 2, each Channel + CC + **On** enable toggle) plus hardware MIDI Out for VCO envelope export. VST/AU inherits the same core gating (MIDI CC 1 on by default, MIDI CC 2 off).
- **Ext. In.** — requires **Ext. In. on + Play**; routes mic, line-in, or USB interface input to the engine. macOS may prompt for audio input access on first capture.
- **Audio Settings** — choose output and input devices.

### Web

- Paged Field-style UI — one page at a time, swipe or use pills to navigate.
- Mod assignment via dropdowns below each knob (no patch cables).
- **External MIDI** — Web MIDI access is requested when you turn **External MIDI** on. Default is **Off**; Play alone does not prompt for MIDI. When External MIDI is off, the **MIDI CC 1** scope is greyed in the mod bay. When on, matching CC 1 messages update the scope (default: channel 1, CC 1). Web does not expose MIDI CC 2.
- **External** — microphone permission is requested when you turn **External** on. Default is **Off**; Play alone does not prompt for mic access. A peak meter beside **External** shows input level when External is on and audio is playing. If the meter stays empty for about a second, check mic permission and input level in the status line. If permission is denied, allow microphone in browser site settings and click **External** again.
- **Mobile browsers** — External + mic uses a play-and-record audio session. **Without headphones**, iPhone Safari often routes synth output to the **earpiece** (top speaker), not the bottom loudspeaker. **With headphones**, output in the headset is normal. Turn **External** off or reload the page to restore built-in speaker playback. Desktop browsers are not affected; use the desktop app for full output routing control.

### VST3 / AU plugin (local-only)

VST/AU sources are **not published** on the public GitHub repo. Keep plugin sources locally.

- Same six-panel UI and mod rack as desktop (VCO Envelope CV scope + Random LEDs).
- **Transport:** the DAW runs audio — there is no standalone Play/Stop bar. Use **Ext. In.** to route the plugin sidechain/mono input bus when the host provides one.
- **Audio Settings** is hidden; pick buffer size and devices in the DAW.
- **MIDI Settings** opens a CC-only dialog (channel, CC number, and **On** enable toggles for MIDI CC 1 and MIDI CC 2). Route MIDI from the DAW track to the plugin input; disabled CC pairs are ignored and grey out the matching mod rack columns.
- Build: `cd desktop && cmake -B build -DBUILD_VST=ON && cmake --build build --config Release`. Artifacts under `FroggersTigaPlugin_artefacts/Release/`.

---

**Daisy Field hardware:** see `MANUAL.md` in the repository for firmware operation, OLED symbols, pickup badges, and flash procedure.

## Version history

### v1.0.3

- Audio page **pair-AR** controls (Attack/Release 1+2 and 2+3) with mod CV on desktop and web
- Pair-AR knobs span **1 ms – 10 s** (VCV Fundamental ADSR time range); level-follower behavior unchanged
- Pair-AR knobs track live modulation like other assignable knobs
- **Delay Crispy mod** routes now work (engine + web display parity with page rows)
- **Mod depth & blend** documented — crossfade semantics, mod-then-fuego pipeline
- **MIDI CC 2** defaults **Off** on desktop/VST (CC 1 on); web unchanged
- Mobile browser audio routing hints and session handling when **External Audio** uses the mic
- Global strip **Rand Resample** label (marbles step); web Playwright e2e harness

### v1.0.1 (initial sim release)

- Dual MIDI CC→CV mod inputs on desktop mod rack (**MIDI CC 1** / **MIDI CC 2**); web mod bay exposes **MIDI CC 1** only
- Web **External MIDI** (permission-gated Web MIDI CC ingest)
- CC-only MIDI path — hardware CC + **Computer keyboard** (QWERTY → **MIDI CC 1**); note queue removed
- Audio page row 7 labeled **Phase mod 3** (PM3 knob parity)
- Web mod-source labels and dropdown options from wasm (`ParamDisplayNames` authority)
- Five-column desktop mod rack; four-entry web mod bay (MIDI CC 1, VCO Envelope, Random 1/2)
