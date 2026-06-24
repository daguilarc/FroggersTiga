# FroggersTiga Simulator Manual

**Release v1.0.4** — desktop app, web sim, and VST/AU plugin share this guide.

On-screen knob names match this manual. For Daisy Field hardware, see `MANUAL.md` in the repository.

## Getting sound

1. **Web:** wait for **Engine ready — click Play**, then click **Play**.
2. **Desktop / plugin:** open the app (or load the plugin in your DAW) and click **Play**.
3. **Stop** halts audio output.

You hear the three VCOs with **External / Ext. In.** off. Turn knobs 1–7 on the visible page to shape the sound.

## External input (optional)

**Ext. In.** routes line, mic, or interface input into the engine.

| State | What you hear |
|-------|----------------|
| **Off** or input silent | VCO mix only |
| **On** and input loud enough | Parallel ring mod — each VCO multiplies the external signal; the engine averages the three products |

A peak meter beside **Ext. In.** (desktop/web) shows input level when External is on and audio is playing. Ring mod opens above the Schmidt gate (~−40 dBFS).

**Crispy** (knob 8) does **not** change how external audio is ring-modded.

## Randomize buttons

Use these to explore without turning every knob by hand.

| Control | What it does |
|---------|--------------|
| **Randomize** (on each page) | Randomize knobs 1–7 on that page (not Crispy) |
| **Rand mod** (on each page) | Randomize mod sources and depths on that page |
| **Rand All** (global strip) | Randomize all pages + pair-AR + Delay |
| **Rand Mods** (global strip) | Randomize all mod routes |
| **Rand Resample** (global strip) | Draw new values from both Random bags → mod S&H outputs |
| **Rand waveforms** (global strip) | Randomize VCO1/VCO2 waveform morph |

On the Audio page, page randomize also hits the pair-sum Attack/Release knobs.

## Crispy (knob 8 on every page)

**Crispy** scrambles knobs **1–7** on that page.

- Turn it up → knob moves get gritty and jumpy instead of smooth.
- **Modulation applies first**; Crispy scrambles the modulated result.
- You can **modulate Crispy** — scramble intensity follows the effective Crispy level.
- **Pair-AR** knobs on Audio accept mod CV but are **not** scrambled by Crispy.

Crispy is the sim name for the Field’s **FUEG** fuegoizer. It does not control external ring-mod mix.

## How audio flows

```text
VCOs (+ optional external ring mod)
  → Drive
  → Filter (comb + peak)
  → Reverb
  → output
```

**Delay** is a separate stereo wet effect on sim hosts (sixth column on desktop, last page on web).

## Audio

Three oscillators. Click the waveform icon beside VCO1–VCO3 to cycle sine ↔ saw ↔ square morph (VCO3 is sine only on Field firmware; sim morphs all three).

**Cross-coupler (row 4):** noon = off. Turn **CCW** for VCO1↔VCO2 coupling. Turn **CW** for VCO2↔VCO3.

**Phase mod** rows set how hard each coupled path pushes oscillator phase:

| Row | Label | Routing |
|-----|-------|---------|
| 5 | Phase mod 1 | VCO2 → VCO1 when cross-coupler is CCW (1→2) |
| 6 | Phase mod 2 | VCO1 + VCO3 → VCO2 |
| 7 | Phase mod 3 | VCO2 → VCO3 when cross-coupler is CW (2→3) |

**External input:** with Ext. In. on and signal above the gate, you get parallel ring mod on the raw per-VCO samples. Pair-AR (below) shapes the VCO-only path when external is off — not the external ring-mod path.

**Pair-sum AR (Audio only):** four extra controls shape how (VCO1+VCO2) and (VCO2+VCO3) sums rise and fall in the **VCO-only** mix. On desktop they sit in a band below the eight rows; on web they are a third row of four knobs. **Att.** = attack when the pair level rises; **Rel.** = release when it falls (1 ms – 10 s).

| Row | Parameter |
|-----|-----------|
| 1 | VCO1 — frequency + morph |
| 2 | VCO2 — frequency + morph |
| 3 | VCO3 — frequency (sine on Field) |
| 4 | Cross-coupler |
| 5 | Phase mod 1 |
| 6 | Phase mod 2 |
| 7 | Phase mod 3 |
| 8 | Crispy |

## Random

Inspired by **Mutable Instruments Marbles**.

Two random CV channels live here. Press **Rand Resample** on the global strip to draw new values from each bag. Outputs appear in the mod bay as **Random 1 S&H** and **Random 2 S&H**. There is no internal clock — you trigger steps.

| Row | Parameter |
|-----|-----------|
| 1 | Step chance — odds each channel resamples on Rand Resample |
| 2 | Deja vu 1 — channel 1 bag walk / re-roll |
| 3 | Bag size 1 — channel 1 values (2–8) |
| 4 | Slew 1 — glide between held values |
| 5 | Deja vu 2 — channel 2 bag walk / re-roll |
| 6 | Bag size 2 — channel 2 values (2–8) |
| 7 | Slew 2 — channel 2 glide |
| 8 | Crispy |

## Drive

First FX stage after the oscillator mix — polynomial drive, digital grit, sample-rate reduction.

| Row | Parameter |
|-----|-----------|
| 1 | Drive |
| 2 | Shape |
| 3 | SRR 1 |
| 4 | SRR 2 |
| 5 | XOR |
| 6 | Bit depth |
| 7 | Fuzz |
| 8 | Crispy |

## Filter

Comb filter and resonant peak after Drive, before Reverb.

| Row | Parameter |
|-----|-----------|
| 1 | Comb offset |
| 2 | Peak freq |
| 3 | Peak gain |
| 4 | Peak Q |
| 5 | Comb delay |
| 6 | Comb feedback |
| 7 | Comb LP |
| 8 | Crispy |

## Reverb

Algorithmic reverb wet/dry.

| Row | Parameter |
|-----|-----------|
| 1 | Wet/dry |
| 2 | Room size |
| 3 | Decay |
| 4 | Pre-delay |
| 5 | Damping |
| 6 | Stereo width |
| 7 | Diffusion |
| 8 | Crispy |

## Delay

Stereo delay effect (sim hosts only). Sixth column on desktop/VST; page 6 on web.

| Row | Parameter |
|-----|-----------|
| 1 | Delay time (~0–2 s) |
| 2 | Send |
| 3 | Feedback |
| 4 | Stereo width |
| 5 | Detune |
| 6 | Mod depth |
| 7 | Wet mix |
| 8 | Crispy |

## Mod bay

Five mod sources can push any knob (including pair-AR and Crispy).

**Web:** pick a source in the dropdown under each knob; the knob then sets **mod depth**.

**Desktop / plugin:** drag a cable from the mod rack to a knob jack.

**Mod depth** crossfades between the stored knob value and the mod source — not `knob × CV`. Depth 0 = base only; depth 1 = mod only.

| Source | Notes |
|--------|-------|
| MIDI CC 1 | Desktop + web (web: **External MIDI** gate) |
| MIDI CC 2 | Desktop standalone only |
| VCO Envelope | Slow level from VCO mix (scope trace) — not Phase mod 3 |
| Random 1 S&H | From Random page bags; resample with Rand Resample |
| Random 2 S&H | Same |

**Phase mod 3** (Audio row 7) is a dedicated knob on sim hosts. **Crispy** (row 8) is the fuegoizer only — not PM3, not external mix.

## Desktop, web, plugin, VCV

| Host | Layout |
|------|--------|
| **Desktop / VST / AU** | Six equal columns: **Audio → Random → Drive → Filter → Reverb → Delay**. Mod rack above; global randomize strip below. |
| **Web** | One page at a time — pills order: Audio → Random → Reverb → Filter → Drive → Delay. Mod sources panel expands per knob. |
| **VCV Rack** | Local-only module; CV jacks per parameter; no MIDI widgets. |

**Desktop:** **MIDI Settings** for two CC→CV pairs; **Audio Settings** for devices. **Ext. In.** needs Play + toggle on.

**Web:** **External MIDI** gates Web MIDI CC 1. **External** requests mic permission; use headphones on iPhone Safari when using the mic (see status line if meter stays empty).

**Mobile browsers** — External + mic uses a play-and-record audio session. **Without headphones**, iPhone Safari often routes synth output to the **earpiece** (top speaker), not the bottom loudspeaker. **With headphones**, output in the headset is normal. Turn **External** off or reload the page to restore built-in speaker playback.

**VST / AU:** local-only build; DAW runs transport; 107 automatable parameters; map MIDI in the DAW.

---

## Appendix

### Host input boundaries

OpenSpec contract: [`openspec/specs/froggers-host-master/spec.md`](openspec/specs/froggers-host-master/spec.md)

| Host | External MIDI / CC | Mod rack |
|------|-------------------|----------|
| **Web** | External MIDI → CC 1 only | CC 1, VCO Env, Random 1/2 |
| **Desktop** | Two CC pairs + QWERTY → CC 1 | CC 1, CC 2, VCO Env, Random 1/2 |
| **VST / AU** | DAW maps to 107 parameters | VCO Env, Random 1/2 |
| **VCV** | Rack MIDI→CV → parameter jacks | VCO Env, Random 1/2 |

### Version history

#### v1.0.4

- Parallel-only external ring mod (product topology removed)
- Sim manual learner-first rewrite; desktop Delay described as sixth column
- Crispy/FUEG no longer documented as external mix control
- Desktop output FX columns Drive → Filter → Reverb left-to-right
- Random mod rack LEDs level-proportional on all hosts

#### v1.0.3

- Audio pair-AR controls; mod depth & blend documented
- PM3 dedicated knob on sim (row 7)
