# FroggersTiga Simulator Manual

Operator guide for the **desktop** and **web browser** simulators. Knob names match the on-screen labels (`ParamDisplayNames`). This is not the Daisy Field hardware manual.

## Quick start

1. Wait for **Engine ready — click Play** (web) or open the desktop app.
2. Click **Play** — VCO output is audible with **External / Ext. In.** off.
3. Use page pills or **◀ ▶** to switch pages (Audio → Random → Reverb → Filter → Drive → Delay).
4. Turn knobs 1–7 for parameters; knob 8 is always **Crispy** on every page.
5. Expand **Mod sources** to assign VCO Envelope, Random 1 S&H, or Random 2 S&H to any knob via the dropdown below each slider.

## Transport

| Control | Action |
|---------|--------|
| **Play** | Start audio processing |
| **Stop** | Halt audio output |
| **External / Ext. In.** | Ring-mod external input (see host notes below) |
| **Randomize** (page) | Randomize knobs 1–7 on the current page |
| **Rand mod** (page) | Randomize mod sources and depths on the current page |
| **Rand All** | Randomize all pages + Delay |
| **Rand Mods** | Randomize all mod routes |
| **Random** | Step both random bags |
| **Rand waveforms** | Randomize VCO waveform morph (Audio page) |

## Mod bay

Three internal mod sources drive per-knob modulation:

- **VCO Envelope** — slow level from the VCO mix (continuous CV scope trace)
- **Random 1 S&H** — sample-and-hold random CV on mod channel 1 (see **Random outputs** below)
- **Random 2 S&H** — sample-and-hold random CV on mod channel 2

**Random outputs (S&H):** Each channel draws a random value from its random bag when you press **Random** (or the gate). There is no internal clock — values change only on a step. **Slew** smooths glides between held values; it does not trigger new steps. The held voltage is normalized **0–100%** on the mod bus (the same CV you patch to knobs).

**Green LED (Random 1 / Random 2):** Shows the held mod CV for that channel while **Play** is on. **Green** = CV above **55%** of full scale. **Dim** = CV at or below 55%, or audio stopped. VCO Envelope uses a scope trace instead of an LED.

On **web**, assign a source with the **Mod source** dropdown under each knob. When a source is selected, the knob controls **mod depth** instead of the base parameter. On **desktop**, drag a patch cable from the mod rack jack to a knob input.

## Page 1 — Audio

| Row | Parameter |
|-----|-----------|
| 1 | VCO1 |
| 2 | VCO2 |
| 3 | VCO3 |
| 4 | Cross-coupler |
| 5 | Phase mod 1 |
| 6 | Phase mod 2 |
| 7 | Phase mod 3 |
| 8 | Crispy |

Click the waveform icon beside VCO1–VCO3 (between the label and knob on the Audio page) to cycle sine ↔ saw ↔ square morph.

**Phase mod 3** (row 7) sets VCO2 → VCO3 phase-mod depth when the cross-coupler is turned toward 2→3. **Crispy** scrambles knobs 1–7 and blends external ring-mod topology when external input is present; it no longer controls PM3 on sim hosts.

## Page 2 — Random

Random is a dual **sample-and-hold** random CV source. Knobs configure two independent bags; **Random 1 S&H** and **Random 2 S&H** in the mod bay / mod sources panel are the outputs (patch or assign them like any mod source). Press **Random** to step both channels.

| Row | Parameter |
|-----|-----------|
| 1 | Step chance |
| 2 | Deja vu 1 |
| 3 | Bag size 1 |
| 4 | Slew 1 |
| 5 | Deja vu 2 |
| 6 | Bag size 2 |
| 7 | Slew 2 |
| 8 | Crispy |

**Slew 1** / **Slew 2** smooth the output toward each new held value; they do not clock the module. After a step, watch the mod indicator: green when that channel's held CV is above **55%**, dim otherwise (see **Mod bay**).

## Page 3 — Reverb

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

## Page 4 — Filter

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

## Page 5 — Drive

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

## Page 6 — Delay

| Row | Parameter |
|-----|-----------|
| 1 | Delay time |
| 2 | Send |
| 3 | Feedback |
| 4 | Stereo width |
| 5 | Detune |
| 6 | Mod depth |
| 7 | Wet mix |
| 8 | Crispy |

## Desktop vs web

### Desktop

- Five adjacent submodule panels (no page switching) plus a **Delay** overlay page.
- **Mod rack** with patch cables — drag from a mod source to a knob to assign modulation. **VCO Envelope** shows a CV scope; **Random 1 S&H** / **Random 2 S&H** show a green LED (on when held mod CV > **55%** while playing).
- **MIDI Settings** — QWERTY piano keyboard or hardware MIDI device for pitch CV.
- **Ext. In.** — requires **Ext. In. on + Play**; routes mic, line-in, or USB interface input to the engine. macOS may prompt for audio input access on first capture.
- **Audio Settings** — choose output and input devices.

### Web

- Paged Field-style UI — one page at a time, swipe or use pills to navigate.
- Mod assignment via dropdowns below each knob (no patch cables).
- No MIDI in the browser build.
- **External** — microphone permission is requested when you turn **External** on. Default is **Off**; Play alone does not prompt for mic access. A peak meter beside **External** shows input level when External is on and audio is playing. If the meter stays empty for about a second, check mic permission and input level in the status line. If permission is denied, allow microphone in browser site settings and click **External** again.

---

**Daisy Field hardware:** see `MANUAL.md` in the repository for firmware operation, OLED symbols, pickup badges, and flash procedure.
