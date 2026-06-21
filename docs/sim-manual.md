# FroggersTiga Simulator Manual

**Release v1.0.4** — desktop app, web sim, and VST/AU plugin share this operator guide.

This guide covers the **desktop**, **web**, and **plugin** simulators. On-screen knob names match this manual. For Daisy Field hardware, see `MANUAL.md` in the repository.

## Quick start

1. Wait for **Engine ready — click Play** (web) or open the desktop app.
2. Click **Play** — VCO output is audible with **External / Ext. In.** off.
3. **Web:** use page pills or **◀ ▶** to switch pages in host page order (Audio → Random → Reverb → Filter → Drive → Delay). **Desktop:** all five core panels are visible at once — output FX columns read Drive → Filter → Reverb left-to-right (see [Desktop host guide](#desktop-standalone)).
4. Turn knobs 1–7 for parameters; knob 8 is **Crispy** on every page (see Global controls).
5. To modulate a knob, expand **Mod sources** (web) or use the mod rack (desktop) — covered in Mod bay below.

## Layout

Six host pages — **Audio**, **Random**, **Reverb**, **Filter**, **Drive**, and **Delay** — each with eight knobs. Rows 1–7 are page parameters; row 8 is always **Crispy**. Host **page index** order (web pills, Field hardware) is fixed; desktop standalone **column** order permutes pages 4/3/2 to match output FX signal flow.

| Host | How you navigate |
|------|------------------|
| **Web** | One page at a time; swipe or use pills (page index order above) |
| **Desktop** | Five adjacent panels (**Drive → Filter → Reverb** among output FX columns) plus a **Delay** overlay |
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
| **Randomize** (page) | Randomize knobs 1–7 on the current page (Audio page also randomizes pair-sum A/R) |
| **Rand mod** (page) | Randomize mod sources and depths on the current page (Audio page includes pair-sum A/R) |
| **Rand All** | Randomize all pages + pair-sum A/R + Delay |
| **Rand Mods** | Randomize all mod routes |
| **Rand Resample** | Resample both random S&H channels (draws from bags) |
| **Rand waveforms** | Randomize VCO waveform morph (Audio page) |

## Mod bay

Five mod sources drive per-knob modulation.

### Sources

Internal mod sources are shared; MIDI/CC availability depends on the host (see **Host input boundaries** below).

- **MIDI CC 1** — Desktop standalone: hardware or QWERTY CC latched to CV (default channel 1, CC 1). Web: Web MIDI CC 1 when **External MIDI** is on (same default). VST/AU and VCV: not available — use DAW parameter mapping or Rack CV instead.
- **MIDI CC 2** — Desktop standalone only: second hardware CC pair (default channel 1, CC 2). Not exposed on web, VST/AU, or VCV.
- **VCO Envelope** — Slow level from the VCO mix; shown as a scope trace (desktop, web, VST/AU).
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

**MIDI CC 1** and **MIDI CC 2** (desktop standalone only) are ignored when that CC input is disabled (grey column). Other mod sources are always active when patched.

### MIDI CC enable

Disable a CC input to grey its mod column, block new routes, clear existing ones, and exclude it from random mod.

- **Desktop standalone:** MIDI Settings **On** toggle per CC pair (MIDI CC 1 and MIDI CC 2).
- **Web:** **External MIDI** is the sole CC gate — CC 1 only (default channel 1, CC 1). No CC 2 UI, scope, or ingestion path.
- **VST / AU:** No fixed CC pairs, CC enable toggles, or CC mod cells — map MIDI in the DAW to the **107** exposed host parameters instead.
- **VCV Rack:** No MIDI boundary — patch external Rack MIDI-to-CV modules into per-parameter CV jacks.

**QWERTY keyboard** (desktop standalone only) drives MIDI CC 1 and respects the CC 1 enable flag.

### Mod indicators

- **Web:** MIDI CC 1 and VCO Envelope scopes; Random 1/2 LEDs.
- **Desktop standalone:** MIDI CC 1, MIDI CC 2, and VCO Envelope scopes; Random 1/2 LEDs.
- **VST / AU:** VCO Envelope scope only (mod indices 4/5/6); Random 1/2 LEDs — no CC scopes.
- **VCV Rack:** Random 1/2 LEDs only — no scopes or MIDI widgets.
- **Random 1 S&H** and **Random 2 S&H** show a green LED while audio runs: brightness tracks held CV proportionally (quadratic curve, full green at **55%** and above); dark when audio is stopped.

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

## Host input boundaries

**OpenSpec master contract (per-host differences + spec index):** [`openspec/specs/froggers-host-master/spec.md`](openspec/specs/froggers-host-master/spec.md)

| Host | External MIDI / CC | Mod rack | Parameter control |
|------|-------------------|----------|-------------------|
| **Web** | **External MIDI** → CC 1 only (no CC 2 UI) | Four entries: CC 1, VCO Env, Random 1/2 | On-screen knobs + mod dropdowns |
| **Desktop standalone** | Two hardware CC pairs (MIDI Settings) + QWERTY → CC 1 | Five entries: CC 1, CC 2, VCO Env, Random 1/2 | Knobs + patch cables |
| **VST / AU** | None — DAW maps any MIDI channel/CC to parameters | Three entries: VCO Env, Random 1/2 (indices 4/5/6) | **107** DAW-automatable host parameters + patch cables |
| **VCV Rack** | None — use Rack MIDI-to-CV → parameter jacks | Three entries: VCO Env, Random 1/2 (indices 4/5/6) | Knobs + per-parameter CV inputs (voltage adds to internal route) |

## Host guide

### Desktop (standalone)

- Five adjacent submodule panels (no page switching) plus a **Delay** overlay page. Left-to-right columns are **Audio → Random S&H → Drive → Filter → Reverb** (host pages 0, 1, 4, 3, 2) so the output FX segment matches signal order (Drive in `FrogBlock`, then filter stages, then reverb wet/dry); host page indices and Field hardware page order are unchanged.
- **Mod rack** with patch cables — drag from a mod source to a knob to assign modulation (including the four pair-sum AR jacks on the Audio panel bottom band).
- **MIDI Settings** — two CC→CV inputs (MIDI CC 1 and MIDI CC 2, each Channel + CC + **On** enable toggle) plus hardware MIDI Out for VCO envelope export. CC 1 defaults **On**; CC 2 defaults **Off**.
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

- Same six-panel UI as desktop; **mod rack** shows VCO Envelope, Random 1, and Random 2 only (no CC mod cells or scopes).
- **107 host parameters** — every persistent page knob, Delay knob, pair-AR knob/depth, mod depth, and continuous morph control is exposed to the DAW for automation and MIDI learn. There is no hosted CC ingest, no MIDI Settings dialog, and `acceptsMidi()` is false; map any MIDI channel/CC in the DAW to these parameters without a two-pair limit inside Froggers.
- **Transport:** the DAW runs audio — Play/Stop, Record, Audio/MIDI settings, and QWERTY capture are hidden. Use the host's sidechain/mono input bus when **Ext. In.** is available.
- Build: `cd desktop && cmake -B build -DBUILD_VST=ON && cmake --build build --config Release`. Artifacts under `FroggersTigaPlugin_artefacts/Release/`.

### VCV Rack module (local-only)

The VCV plugin is **local-only** (`vcv/` in `.gitignore`; not built on CI).

- **CV-only** — no MIDI In/Out widgets, no CC enable switches, and no CC mod sources. Patch a Rack MIDI-to-CV module into Froggers' per-parameter CV jacks when you need MIDI control.
- **Mod rack** — VCO Envelope, Random 1, and Random 2 at indices 4/5/6. Random LEDs only (no scope cells).
- **Per-parameter CV jacks** — connected voltage adds to the value from any stored internal mod route (`clamp(internal + V/10, 0, 1)`); disconnected jacks use the internal route alone.

---

**Daisy Field hardware:** see `MANUAL.md` in the repository for firmware operation, OLED symbols, pickup badges, and flash procedure.

## Version history

### v1.0.4

- Desktop standalone output FX columns read **Drive → Filter → Reverb** left-to-right to match signal flow (host page indices unchanged)
- Random mod rack LEDs (indices 5/6) use level-proportional brightness on desktop, web, VST/AU, and VCV — shared curve, full green at ~55% CV
- Global **Randomize / Randmod / Rand All** strip moved below **External MIDI** on web and desktop (all viewports)

### v1.0.3

- Audio page **pair-AR** controls (Attack/Release 1+2 and 2+3) with mod CV on desktop and web
- Pair-AR knobs span **1 ms – 10 s** (VCV Fundamental ADSR time range); level-follower behavior unchanged
- Pair-AR knobs track live modulation like other assignable knobs
- **Delay Crispy mod** routes now work (engine + web display parity with page rows)
- **Mod depth & blend** documented — crossfade semantics, mod-then-fuego pipeline
- Audio **Randomize / Randmod / Rand All** now include pair-sum A/R knobs (web + desktop)
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
