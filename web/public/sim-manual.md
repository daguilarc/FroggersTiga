# FroggersTiga Simulator Manual

**Release v1.0.4** — desktop app and web sim share this guide.

On-screen knob names match this manual. For Daisy Field hardware, see `MANUAL.md` in the repository.

## Getting sound

1. **Web:** wait for **Engine ready — click Play**, then click **Play**.
2. **Desktop v1:** open the standalone app and click **Play**.
3. **Desktop v2:** open **FroggersTigaV2** and click **Engine** (top row). Internal VCOs drive sound immediately — no MIDI note or running sequencer required.
4. **Stop** / **Engine** off halts audio output.

You hear the three VCOs with **External / Ext. In.** off. On desktop v2 cold start, VCO1–VCO3 default to **30 Hz** with **sine / square / saw** morphs. Turn knobs on the visible page to shape the sound.

## External input (optional)

**Ext. In.** routes line, mic, or interface input into the engine.

| State | What you hear |
|-------|----------------|
| **Off** or input silent | VCO mix only |
| **On** and input loud enough | Parallel ring mod — each VCO multiplies the external signal; the engine averages the three products |

A peak meter beside **Ext. In.** (desktop/web) shows input level when External is on and audio is playing. Ring mod opens above the Schmidt gate (~−40 dBFS).

**Crispy** (knob 8) does **not** change how external audio is ring-modded.

## Global Crunchy (web + desktop v2)

**Crunchy** in the global strip applies fuego to **every knob on every page**, including all **Crispy** instances. Turn it up and all parameter moves get grittier across the whole sim.

- **Crunchy** is global; **Crispy** (last row on each page) is page-local and stacks on top.
- Modulation applies first; fuego scrambles the modulated result.
- On **web**, pair-AR knobs on Audio accept modulation; **Audio-page Crispy** and **global Crunchy** apply fuego to pair-AR on web (v2 fuego host).
- On **v1 desktop**, pair-AR has no global Crunchy control; Crunchy does not apply to pair-AR.
- On **desktop v2**, pair-AR moves to carousel page 7 (**Pair-AR**); per-VCO attack/release rows receive global Crunchy like other musical rows. Global **Crunchy** uses a scene encoder ring (S1/S2/S3 + blend) like module rows — not a single unscened rotary.

## Randomize buttons

Use these to explore without turning every knob by hand.

| Control | What it does |
|---------|--------------|
| **Randomize** (on each page) | **Desktop v2:** all three scene slots per musical row on that module (not Crispy). **v1/web:** randomize knobs 1–7 on that page (not Crispy) |
| **Rand mod** (on each page) | Randomize mod sources and depths on that page |
| **Rand All** (global strip) | **Desktop v2:** all modules — scene slots (not Crispy) + mod depths + global Crunchy scene slots; also randomizes L/R endpoint assignment and scene blend. **v1/web:** all pages + pair-AR + Delay |
| **Rand-seq** (sequencer dice) | **Desktop v2:** scene slots into step buffer(s) per Step/Pattern scope (same scene policy as Rand All); also randomizes live L/R endpoints and blend once per press; no mod depths |
| **Rand Mods** (global strip) | Randomize all mod routes |
| **Rand Resample** (global strip) | Draw new values from both Random bags → mod S&H outputs |
| **Rand waveforms** (global strip) | Randomize VCO1/VCO2 waveform morph |
| **Crunchy** (global strip) | Global fuego on all pages and all Crispy instances |

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

**Delay** is a separate stereo wet effect on the desktop app and web sim (sixth column on desktop, last page on web).

## Audio

Three oscillators. Click the waveform icon beside VCO1–VCO3 to cycle sine ↔ saw ↔ square morph (VCO3 is sine only on Field firmware; sim morphs all three). **Desktop v2 cold start:** VCO1 **30 Hz sine**, VCO2 **30 Hz square**, VCO3 **30 Hz saw** — seeded into all three scene slots per row.

**Cross-coupler (row 4):** noon = off. Turn **CCW** for VCO1↔VCO2 coupling. Turn **CW** for VCO2↔VCO3.

**Phase mod** rows set how hard each coupled path pushes oscillator phase:

| Row | Label | Routing |
|-----|-------|---------|
| 5 | Phase mod 1 | VCO2 → VCO1 when cross-coupler is CCW (1→2) |
| 6 | Phase mod 2 | VCO1 + VCO3 → VCO2 |
| 7 | Phase mod 3 | VCO2 → VCO3 when cross-coupler is CW (2→3) |

**External input:** with Ext. In. on and signal above the gate, you get parallel ring mod on the raw per-VCO samples. Pair-AR (below) shapes the VCO-only path when external is off — not the external ring-mod path.

**Pair-sum AR (Audio only):** four extra controls shape how (VCO1+VCO2) and (VCO2+VCO3) sums rise and fall in the **VCO-only** mix. On desktop they sit in a band below the eight rows; on web they are a third row of four knobs. **Attack** / **Release** labels (1 ms – 10 s).

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

Dual stepped random S&H — two independent bags of held random values.

Two random CV channels live here. Press **Rand Resample** on the global strip to draw new values from each bag. Outputs appear in the mod bay as **Random S&H 1** and **Random S&H 2**. There is no internal clock — you trigger steps.

| Row | Parameter |
|-----|-----------|
| 1 | Step chance — odds each channel resamples on Rand Resample |
| 2 | Deja vu 1 — channel 1 bag walk / re-roll |
| 3 | Bag size 1 — channel 1 values (2–8) |
| 4 | Slew 1 — glide between held values |
| 5 | Deja vu 2 — channel 2 bag walk / re-roll |
| 6 | Bag size 2 — channel 2 values (2–8) |
| 7 | Slew 2 — channel 2 glide |
| 8 | Spread — random channel spread |
| 9 | Bias — random channel bias |
| 10 | Crispy |

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
| 8 | Blend — drive blend |
| 9 | Phase — drive phase offset |
| 10 | Crispy |

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
| 8 | Comb/Peak — parallel comb and peak mix |
| 9 | Scoop — filter scoop |
| 10 | Crispy |

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
| 8 | Mod depth — reverb modulation depth |
| 9 | Hold — reverb hold |
| 10 | Crispy |

## Delay

Stereo delay effect for the desktop app and web sim only. Sixth column on desktop; page 6 on web.

| Row | Parameter |
|-----|-----------|
| 1 | Delay time (~0–2 s) |
| 2 | Send |
| 3 | Feedback |
| 4 | Stereo width |
| 5 | Detune |
| 6 | Mod depth |
| 7 | Wet mix |
| 8 | Color — delay tone color |
| 9 | Halo — delay halo width |
| 10 | Crispy |

## Mod bay

Mod sources can push any knob (including pair-AR and Crispy). Desktop shows five sources; web shows four.

**Web:** pick a source in the dropdown under each knob; the knob then sets **mod depth**.

**Desktop:** drag a cable from the mod rack to a knob jack.

**Mod depth** crossfades between the stored knob value and the mod source — not `knob × CV`. Depth 0 = base only; depth 1 = mod only.

| Source | Notes |
|--------|-------|
| MIDI CC 1 | Desktop + web (web: **External MIDI** gate) |
| MIDI CC 2 | Desktop standalone only |
| VCO Envelope | Slow level from VCO mix (scope trace) — not Phase mod 3 |
| Random S&H 1 | From Random page bags; resample with Rand Resample |
| Random S&H 2 | Same |

**Phase mod 3** (Audio row 7) is a dedicated knob on sim hosts. **Crispy** (row 8) is the fuegoizer only — not PM3, not external mix.

## Desktop v2 (FroggersTigaV2)

The v2 standalone app (`FroggersTigaV2`) replaces the six-column layout with a **module carousel**, encoder rings, scenes, two gesture lanes, a step sequencer, and a **Pair-AR** page. Web sim keeps page pills and does **not** include v2 chrome (rings, scenes, gestures, sequencer, or Pair-AR carousel page).

**Default audio:** With **Engine** on, internal VCOs drive output at current knob and scene values. You hear knob edits without MIDI and without running the step sequencer — including when **Start Sequence** is off or the pattern is empty. Step gates in the grid are pattern data only until **Start Sequence** runs; lit gate cells render dimmed when stopped.

### Module vs Scene

Two different navigation concepts:

| Control | Label | What it does |
|---------|-------|--------------|
| **Module carousel** | **Module:** | Switches the active FX block: Audio → Random → Drive → Filter → Reverb → Delay → **Pair-AR**. Left/right arrows wrap 0↔6. |
| **Scene strip** | **Scene:** | Stores preset endpoints **S1**, **S2**, **S3** for every parameter. Scene storage is **global** — all modules share the same scene endpoints per knob. |

The **blend slider** morphs between scene **L** and **R** endpoints (ends labeled **L** / **R**). Active endpoints show **·L** / **·R** on S1–S3 buttons. Scene buttons update the less-selected endpoint when you store a new value.

### Encoder rings

Each knob row shows a Sheaf-style encoder ring instead of a plain rotary:

- **Outer ring** — scene L effective value
- **Inner ring** — scene R effective value
- **Center dot** — blended value sent to the engine
- **Upper badges** — active mod sources on that row
- **Lower badges** — gesture lanes (G1/G2) affecting that row
- **Min/max arcs** — reachability from modulation depth (matches audible Smart Grid math)

**Interaction:**

| Action | Normal view | Mod-depth view |
|--------|-------------|----------------|
| Drag ring | Edit scene-blended center (or depth when mod route active) | Edit assigned depth |
| Press | Open mod-depth view | Close view |
| **Shift** + press | Revert param + depths to default | Revert target param |
| **Shift** held + drag | No turn | No turn |

**Shift held** is a modifier only — it does not erase scenes until you **press** a ring while Shift is down.

**Shift + press** scope:

| Target | What resets |
|--------|-------------|
| Module encoder ring | **That row only** — all three scene slots (S1/S2/S3) to inventory factory default, mod depths to default, gesture depths to 0 |
| **Crunchy** ring | All three Crunchy scene slots to **0** (turning Crunchy down edits only the **active** slot selected by blend) |
| **Crispy** row on a page | Same full row reset as any module knob (Crispy defaults to 0) |

Scene **L/R** endpoint buttons and the **blend** slider are unchanged. Other parameters keep their scene memories unless you Shift+press their rings too.

### Mod-depth view

**Normal view** — rings edit scene-blended knob centers (or gesture offsets when G1/G2 is selected).

**Enter mod-depth view** — press an encoder ring whose row has at least one mod source assigned. The carousel remaps visible slots to that row's mod-depth lanes, ending with a **target** slot for the knob itself.

**In mod-depth view** — drag a lane ring to edit that mod route's depth (not the scene center). Press the **target** slot (last lane) to close and return to normal view.

**Shift in mod-depth view** — held drag still does nothing. **Shift + press** on any slot resets the **underlying parameter** (all three scene slots + mod depths + gestures for that row), same as normal view — not "reset this one depth slider."

### Gestures (G1 and G2)

Two independent gesture lanes in the global strip. Toggles read **G1** / **G2** (full names: **Gesture 1** / **Gesture 2**). Select a lane, then turn a ring to store a gesture offset for that parameter. **Gesture weight** sliders (0–1) scale lane influence. Gesture badges appear on affected encoders. Gestures are separate from scene storage. **Rand All** clears gesture selection first.

### Global strip (v2)

Keeps v1 randomize buttons plus:

| Control | Role |
|---------|------|
| **Crunchy** | Global fuego scene encoder ring (S1/S2/S3 + blend) on all rows all pages, including every Crispy instance |
| **Shift** | Keyboard or MIDI-assignable; interaction matrix above |
| **Gesture 1 / G1** | Gesture lane 1 selector |
| **Gesture 2 / G2** | Gesture lane 2 selector |
| **LFO / VCO** | Mod source shortcuts |
| **Scene S1–S3** | Scene endpoint storage |
| **Blend** | Morph scene L ↔ R |
| **Sequencer** | BPM, pattern length, play/stop, record arm |

### Pair-AR module (page 7)

Replaces v1 pair-sum band on desktop v2. **Per-VCO AR** (attack → hold at full level while gate high → release on gate off). **No sustain knobs** — seven rows (Atk1–Rel3 + Crispy). With **Engine** on and **Start Sequence** off, per-VCO gates are open — knob and scene edits are audible immediately. While **Start Sequence** runs, playhead step gates and live MIDI/gate CV OR-combine to shape envelopes. When **Start Sequence** stops, gates return open.

| Row | Parameter |
|-----|-----------|
| 1 | Atk1 (VCO1 attack) |
| 2 | Rel1 (VCO1 release) |
| 3 | Atk2 (VCO2 attack) |
| 4 | Rel2 (VCO2 release) |
| 5 | Atk3 (VCO3 attack) |
| 6 | Rel3 (VCO3 release) |
| 7 | Crispy |

Web keeps four **pair-sum** A/R knobs on the Audio page (Attack 1+2, Release 1+2, Attack 2+3, Release 2+3). Desktop v2 keeps six **per-VCO** knobs on the Pair-AR carousel page.

### Sequencer

Full step sequencer in the global strip and sequencer panel:

- **BPM** and **Steps** (pattern length 4–64) in the performance band
- **Start Sequence** / **Stop Sequence** toggles pattern playback (distinct from **Engine**)
- **Record arm** captures per-row scene slots and gesture weights into each step as the playhead advances
- **Edit step** (selected for authoring) is distinct from the playback **playhead**; both can highlight when they differ

**Step grid interaction:**

| Action | Result |
|--------|--------|
| **Single-click** step K | Edit step becomes K; gate unchanged |
| **Double-click** step K | Toggle step K gate (lit/rest); edit step becomes K |
| **Right-click** step K | Menu: **Reset** (factory cold-start snapshot into step K) or **Randomize** (full step snapshot for step K only) |

**Edit-step toolbar** (above the grid): **←** / **→** move edit step within pattern length (wrap); **dice (Rand-seq)** randomizes scene slots into step buffer(s) — **Step** scope = edit step only, **Pattern** scope = blank steps only; also randomizes live L/R endpoints and blend once per press. Toolbar dice differs from context-menu **Randomize** (dice updates live L/R/blend; context menu does not).

**Gate policy:** Lit gates drive per-VCO AR envelopes **only while Start Sequence runs**. When stopped, lit gates are stored pattern data (cells render dimmed); internal VCOs continue at full level. Optional external MIDI clock sync (MIDI CV settings).

### Three meanings of "gate"

| Kind | Where | Role |
|------|-------|------|
| **Step gate** | Sequencer grid (double-click step) | Per-step pattern on/off for Pair-AR envelopes while **Start Sequence** runs |
| **MIDI CV Gate** | MIDI CV settings row | Any held note on the input drives envelope gates when enabled; OR-combines with step gates during playback |
| **Ext. In. gate** | External input meter | Schmidt level (~−40 dBFS) opens parallel ring mod — unrelated to sequencer or MIDI CV |

**Shift** and **Scene** MIDI bindings are performance triggers, not the Gate row. The Gate row does not replace step gates — step gates are pattern data you author in the grid.

### Mod grid (v2)

Eight internal mod sources per row (lit cells + dropdown): six envelope followers, **Random S&H 1** and **Random S&H 2**. Scope grid shows six EF traces and two Random S&H LEDs. No patch cables.

### Stereo output default

Desktop v2 initializes **stereo output** by default (`initialiseWithDefaultDevices(0, 2)` — same as v1). The engine renders a mono synth bus; delay and reverb stereo spread applies to L/R. Select a mono output device in **Audio Settings** and the bus downmixes to one channel.

### Rand All (v2)

**Rand All** randomizes all three scene slots per musical row on every module page (skip Crispy), all mod depths, and global **Crunchy** scene slots. It also randomizes L/R endpoint assignment and scene blend. It clears gesture selection first.

Per-module **Randomize** uses the same scene-slot policy on the **current page only** (not Crispy); does **not** change endpoints or blend. Use **Rand mod** separately for mod depths on that page.

### MIDI CV settings (desktop v2)

Desktop v2 replaces the v1 two CC-pair dialog with a unified assignment table on one MIDI input.

**Step 1 — MIDI In:** pick one input stream (computer keyboard, none, or hardware port).

**Step 2 — CV Assignments:** map messages from that input:

| Row | Maps to | Notes |
|-----|---------|-------|
| **Pitch** | Page + row knob | Read-only target name (e.g. VCO1 on Audio row 0) |
| **Gate** | ADSR + sequencer gates | Note on/off |
| **MIDI CC A** | Mod source **MIDI CC A** | Ch (**Any** = all channels) + CC number |
| **MIDI CC B** | Mod source **MIDI CC B** | Ch (**Any** = all channels) + CC number |
| **Shift button** | Shift modifier | Note or CC + Ch |
| **Scene S1–S3** | Scene endpoint triggers | Note or CC + Ch |
| **QWERTY Ch** | Virtual channel for keyboard | 1–16 |

**MIDI CC A** and **MIDI CC B** appear in module mod dropdowns and participate in effective-value math when assigned. **MIDI Out (VCO Env)** optionally sends envelope level to a physical port.

### Live performance MIDI (desktop v2 standalone)

Desktop v2 MIDI CV settings are optimized for **hands-on performance** from one controller — similar in spirit to Elektron grooveboxes (Digitakt, Octatrack, Analog Rytm): fixed **performance triggers** on dedicated notes or CCs, separate from pitch and envelope gate.

**Recommended layout:**

| Role | MIDI CV row | Typical use |
|------|-------------|-------------|
| Expression | **Pitch** | One knob target (e.g. filter cutoff or VCO1) — play melodies or sweeps |
| Envelopes | **Gate** | Hold notes for Pair-AR; combines with step gates when the sequencer runs |
| Live modulation | **MIDI CC A / B** | Assign in module mod menus as external mod sources |
| Modifier | **Shift button** | Dedicated Note or CC — hold to arm reset; press a ring while held to factory-reset that target |
| Scene morph | **Scene S1–S3** | Dedicated triggers for L/R endpoint picks (same as performance-band buttons) |

**Why this split matters:**

- **Shift** is not the Gate row — map a pad or key to Shift for "factory reset while held" without tying it to note-held envelope logic.
- **Gate** listens to **any** held note when enabled — use it for playing envelopes, not for Shift or scene buttons.
- **Step gates** live in the sequencer pattern — they are not MIDI-mapped; MIDI Gate OR-combines with them only during **Start Sequence**.
- **QWERTY** + **QWERTY Ch** let you rehearse the same bindings from the computer keyboard before attaching a hardware port.

The **VST** does not include this dialog — the DAW owns MIDI routing and automation. Standalone keeps performance bindings local so you can play without mouse edits.

## Desktop and web

| Host | Layout |
|------|--------|
| **Desktop v1** | Six equal columns: **Audio → Random → Drive → Filter → Reverb → Delay**. Mod rack above; global randomize strip below. Pair-AR band on Audio. |
| **Desktop v2** | One module at a time via carousel (7 modules incl. Pair-AR). Encoder rings, scenes, gestures, mod grid, scope, sequencer. See **Desktop v2** above. |
| **Web** | One page at a time — pills order: Audio → Random → Reverb → Filter → Drive → Delay. Pages **Random through Delay** show **ten knobs** (eight musical rows + two expansion rows + Crispy). Mod sources panel expands per knob. Global **Crunchy** rotary in the strip. Pair-AR stays on Audio. |

**Desktop v1:** **MIDI Settings** for two CC→CV pairs; **Audio Settings** for devices. **Ext. In.** needs Play + toggle on.

**Desktop v2:** **MIDI CV settings** for pitch/gate/CC/shift/scene bindings on one input device; **Audio Settings** for stereo/mono output. **Ext. In.** needs **Engine** + toggle on.

**Web:** **External MIDI** gates Web MIDI CC 1. **External** requests mic permission; use headphones on iPhone Safari when using the mic (see status line if meter stays empty).

**Mobile browsers** — External + mic uses a play-and-record audio session. **Without headphones**, iPhone Safari often routes synth output to the **earpiece** (top speaker), not the bottom loudspeaker. **With headphones**, output in the headset is normal. Turn **External** off or reload the page to restore built-in speaker playback.

---

## Appendix

### Host input boundaries

These boundaries apply to the launched desktop standalone app and the web sim.

| Host | External MIDI / CC | Mod rack |
|------|-------------------|----------|
| **Web** | External MIDI → CC 1 only | CC 1, VCO Env, Random 1/2 |
| **Desktop** | Two CC pairs + QWERTY → CC 1 | CC 1, CC 2, VCO Env, Random 1/2 |

### Version history

#### v2.0.0 (desktop v2 — local preview)

- Desktop v2 module carousel (7 modules), encoder rings, scenes, two gesture lanes
- ADSR page replaces pair-AR on desktop v2
- Global Crunchy + per-page Crispy stack; expanded module rows on pages 1–5
- Step sequencer with BPM, pattern length, per-step scene/gesture capture
- Stereo-default output; eight-source mod grid and scope visualization
- Web: expanded pages 1–5 + global Crunchy only (no v2 chrome)

#### v1.0.4

- Parallel-only external ring mod (product topology removed)
- Sim manual learner-first rewrite; desktop Delay described as sixth column
- Crispy/FUEG no longer documented as external mix control
- Desktop output FX columns Drive → Filter → Reverb left-to-right
- Random mod rack LEDs level-proportional on desktop and web

#### v1.0.3

- Audio pair-AR controls; mod depth & blend documented
- PM3 dedicated knob on sim (row 7)
