# FroggersTiga Simulator Manual

**Release v1.0.4** — desktop app and web sim share this guide.

On-screen knob names match this manual. For Daisy Field hardware, see `MANUAL.md` in the repository.

## Getting sound

1. **Web:** wait for **Engine ready — click Play**, then click **Play**.
2. **Desktop v1:** open the standalone app and click **Play**.
3. **Desktop v2:** open **FroggersTigaV2** (Release build via `./scripts/open-desktop-v2.sh`) and click **Play** in the top transport row. Internal VCOs drive sound immediately — no MIDI note or running sequencer required.
4. **Stop** halts audio output.

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
| **Rand All** (global-command band) | **Desktop v2:** all modules — scene slots (not Crispy) + mod depths + global Crunchy scene slots; also randomizes L/R endpoint assignment and scene blend. Scope: **All Scenes** / **Current Scene** pair below the button (scene endpoints only — does not change L/R picks or blend position). **v1/web:** all pages + pair-AR + Delay |
| **Rand-seq** (sequencer dice) | **Desktop v2:** scene slots into step buffer(s) per **Step** / **All steps** scope; also randomizes live L/R endpoints and blend once per press; no mod depths |
| **Rand Mods** (global-command band) | **Desktop v2:** randomize mod depths on eligible rows per **All Steps** / **Current Step** scope pair below **Rand Mods** (and sequencer toolbar scope when writing step snapshots). **v1/web:** randomize all mod routes |
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

Dual stepped random S&H — two independent bags of held random values. Inspired by Mutable Instruments Marbles (UI labels are **Random S&H 1/2**).

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

## Mod bay (v1 desktop and web)

Mod sources can push any knob (including pair-AR and Crispy). Desktop v1 shows five sources; web shows four.

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

The v2 standalone app (`FroggersTigaV2`) uses a **module carousel**, encoder rings, scenes, a **fixed 16-step sequencer**, **runtime pages** (File, Audio, MIDI/Controllers), and a converged **top chrome stack**. Web sim keeps page pills and does **not** include v2 chrome (rings, scenes, sequencer, runtime pages, or global oscilloscope).

**Default audio:** With **Play** on, internal VCOs drive output at current knob and scene values. You hear knob edits without MIDI and without running the step sequencer — including when **Start Sequence** is off or every step is unwritten. Step gates in the grid are pattern data only until **Start Sequence** runs; lit gate cells render dimmed when stopped.

### Top chrome stack

One chrome region above the carousel with two bands:

| Band | Controls |
|------|----------|
| **Transport / signal** | **Play**, **Stop**, **Record audio**, and the **global oscilloscope** (three color-coded VCO traces by default; switches to other manifest source groups when inspected; fed from the same fifteen-lane CV history used by detail underlays) |
| **Global command** | **Rand All**, **Rand Mods**, **Rand waveforms**, **Rand Resample**, **Crunchy** (scene encoder ring); **All Scenes** / **Current Scene** under **Rand All**; **All Steps** / **Current Step** under **Rand Mods** |

The global oscilloscope stays visible across carousel modules and runtime pages. The Audio runtime page shows device/status only — it does not duplicate the scope.

**Record audio** requires **Play** first (v1 parity). Export format is set in the **Audio** menu.

### Module carousel and runtime pages

| Control | What it does |
|---------|--------------|
| **Module carousel** | **Module:** header switches FX blocks: Audio → Random S&H → Reverb → Filter → Drive → Delay → **Pair-AR** (Envelope). Left/right arrows wrap 0↔6. Audio/VCO is the default launch page. |
| **Runtime rail** | Right-side **File**, **Audio**, and **MIDI** buttons toggle setup pages without replacing carousel arrows. **File/Patch** — save/load/revert and patch identity. **Audio** — devices, channels, sample rate, external input, meters. **MIDI** — controller mappings, connection state, fan-out summary. |

**Audio/VCO** exposes VCO1–VCO3 frequency and **waveform morph** controls (click waveform icons), **cross-coupler** routing (product contract: separate VCO 1/2 and VCO 2/3 couplers; legacy single **Cross-coupler** row remains on web), and phase-mod rows.

### Module vs Scene

| Control | Label | What it does |
|---------|-------|--------------|
| **Performance band** | **Scene:** | **S1**, **S2**, **S3** pick L/R morph endpoints (global across modules). Active endpoints show **·L** / **·R**. |
| **Blend slider** | **L** / **R** | Morphs between the two selected scene ordinals. |

Scene storage is **global** — all modules share the same scene endpoints per parameter row.

### Encoder rings and MOD drill-in

Each knob row shows a Sheaf-style encoder ring:

- **Outer ring** — scene L effective value
- **Inner ring** — scene R effective value
- **Center dot** — blended value sent to the engine
- **CV LED / MOD** — fixed-center affordance; click opens parameter-detail modulation for that row
- **Ring annulus** — drag-turn edits the parameter (or an open detail-lane depth); turn does **not** open detail

**Turn vs drill-in (device-neutral):** rotation / ring drag → `ParamTurn`; press / MOD LED click → `ModDrillIn`. Mouse: ring annulus drag stays `ParamTurn`; center **MOD/CV LED** click dispatches `ModDrillIn`. MIDI pressable encoder (Controllers page): map rotation to `…_encoder_turn` → `ParamTurn`; map press to `…_encoder_mod_drill_in` → `ModDrillIn`.

**Normal carousel view** — drag a ring to edit the scene-blended center (or gesture offset when **G1** / **G2** is selected in the performance band). Click the center **MOD/CV LED** to open detail even when every lane depth is still zero (no prior assignment step).

**Parameter-detail view (4×4 grid)** — sixteen cells: fifteen permanent **mod source depth** encoders plus one **Target (Back)** cell. Each source-depth cell paints a display-only **CV activity underlay** from the shared fifteen-lane CV history (same samples that feed the global oscilloscope); **Target (Back)** has no underlay. Underlays do not receive pointer events — ring drag stays `ParamTurn`, center MOD stays `ModDrillIn`. Each lane has an independent signed depth; **depth 0 = that lane off**. Multiple nonzero assignable lanes on the same row **sum** into effective modulation (additive multi-lane depths, not a single-source crossfade). Drag a lane ring to edit that source's depth. Unavailable lanes (external audio with no input; blocked VCO pair-bus self-feedback) stay visible but greyed (underlay included at reduced alpha) and refuse turn/clear-press. Press **Target (Back)** to close detail and return to the module page.

There is **no** general held-gesture model: **Rand All**, **Rand Mods**, **Crunchy**, and related commands fire once from explicit clicks using the visible scope pairs. **Shift** was removed from desktop v2 (no held-modifier path; no **Shift + press** reset).

### Permanent 15-lane mod sources

Every eligible parameter exposes the same fifteen source lanes in parameter detail (depth 0 = off; no patch cables; no module-row mod dropdown):

| Lane | Name |
|------|------|
| 1–3 | **VCO 1+2**, **VCO 2+3**, **VCO 1+3** (audio-rate pair buses; raw per-VCO audio lanes absent) |
| 4–8 | **VCO 1 EF**, **VCO 2 EF**, **VCO 3 EF**, **VCO 1+2 EF**, **VCO 2+3 EF** |
| 9–11 | **LFO 1**, **LFO 2**, **LFO 3** |
| 12–13 | **Random S&H 1**, **Random S&H 2** |
| 14–15 | **External Audio (audio rate)**, **External Audio (envelope follower)** — visible but unavailable when external input is off |

**VCO self-feedback rule:** VCO1-owned rows block VCO 1+2 and VCO 1+3 buses; VCO2-owned rows block VCO 1+2 and VCO 2+3; VCO3-owned rows block VCO 2+3 and VCO 1+3.

**MIDI is not a mod lane.** Hardware MIDI and DAW automation map to **manifest target IDs** through the MIDI/Controllers runtime page (standalone) or host parameters (DAW-hosted build). **MIDI CC A** and **MIDI CC B** are controller targets, not entries in the fifteen-lane catalog.

### Gestures (G1 and G2)

Two performance-offset lanes in the **performance band** (not held modifiers). Select **G1** or **G2**, then turn a ring to store an offset. **Gesture weight** sliders (0–1) scale lane influence. **Rand All** clears gesture selection first.

### Pair-AR / Envelope module (page 7)

Carousel label **Pair-AR**; product name **Envelope**. Per-VCO attack/release (Atk1–Rel3) + **Crispy** — **no sustain** knobs. With **Play** on and **Start Sequence** off, per-VCO gates are open. While **Start Sequence** runs, step gates and MIDI/gate CV OR-combine to shape envelopes.

| Row | Parameter |
|-----|-----------|
| 1 | Atk1 (VCO1 attack) |
| 2 | Rel1 (VCO1 release) |
| 3 | Atk2 (VCO2 attack) |
| 4 | Rel2 (VCO2 release) |
| 5 | Atk3 (VCO3 attack) |
| 6 | Rel3 (VCO3 release) |
| 7 | Crispy |

Web keeps four **pair-sum** A/R knobs on the Audio page.

### Fixed 16-step sequencer

Exactly **16** step slots — no pattern-length or **Steps** slider. Each slot is **written** or **unwritten**. Playback skips unwritten slots; when all sixteen are unwritten, sequencer transport is a clocked no-op and audio continues from live synth state.

**Toolbar** (above the grid): BPM, **direction** and **speed** icon buttons (`<` / `>` / `RND` and `/2` / `/1.5` / `1` / `x1.5` / `x2`; defaults `>` and `1`), **Start Sequence**, **Write Seq.**, edit-step arrows, **Rand-seq** dice + **Step** / **All steps** scope.

**Direction / speed strip** sits above the 16-cell grid; all sixteen steps are visible at 1280×920 without scrolling.

| Action | Result |
|--------|--------|
| **Single-click** step K | Edit step becomes K; gate unchanged |
| **Double-click** step K | Toggle step K gate (lit/rest); edit step becomes K |
| **Long-press** written step K | Clear step K (mark unwritten; wipe snapshot). Mouse, touch, or mapped controller hold — no menu or confirm. Short press cancels. |
| **Right-click** step K | Menu: **Reset** or **Randomize** (full step snapshot for K only) |

**Edit-step toolbar:** **←** / **→** move edit step (wrap 0..15). Toolbar dice randomizes scene slots per **Step** / **All steps** scope and randomizes live L/R/blend once per press.

**Gate policy:** Lit gates drive per-VCO AR envelopes **only while Start Sequence runs**. When stopped, lit gates are stored pattern data (dimmed cells).

### Three meanings of "gate"

| Kind | Where | Role |
|------|-------|------|
| **Step gate** | Sequencer grid (double-click step) | Per-step pattern on/off for Pair-AR envelopes while **Start Sequence** runs |
| **MIDI CV Gate** | MIDI/Controllers **Gate** row | Any held note drives envelope gates when enabled; OR-combines with step gates during playback |
| **Ext. In. gate** | External input meter | Schmidt level (~−40 dBFS) opens parallel ring mod — unrelated to sequencer or MIDI CV |

### Randomization scope pairs

| Pair | Used by | **All** choice | **Current** choice |
|------|---------|----------------|-------------------|
| **All Scenes** / **Current Scene** | **Rand All**, **Rand-seq** scene policy | Write all three scene slots per row | Write only the active scene edit target |
| **All Steps** / **Current Step** | **Rand Mods**, sequencer mod snapshots | All 16 step indices | Playhead while playing; edit step while stopped |

Scene-scoped randomization preserves L/R endpoint picks and blend slider position.

### Stereo output default

Desktop v2 initializes **stereo output** by default. Select mono in the **Audio** runtime page to downmix.

### MIDI / Controllers (desktop v2 standalone)

One primary input stream. Explicit mapping rows (no MIDI learn, no recent-event list): **Pitch**, **Gate**, **MIDI CC A**, **MIDI CC B**, **Scene S1–S3**, **QWERTY Ch**, optional **External MIDI clock** for sequencer sync, plus **per-parameter encoder turn** and **encoder mod drill-in** targets generated from the module-row inventory. Duplicate physical messages may fan out to multiple targets.

**Pitch** and **Gate** are performance targets. **MIDI CC A/B** route through the controller model to eligible parameters — not through the fifteen-lane mod source catalog.

**Encoder turn vs mod drill-in:** map relative CC / encoder rotation to a parameter’s `…_encoder_turn` target → control-core `ParamTurn(page, slot, delta)`. Map encoder button / note / CC-threshold to `…_encoder_mod_drill_in` → `ModDrillIn(page, slot)` (same enter-mod semantics as the center MOD LED; see turn vs drill-in above). Persistence keys are inventory stable IDs; unknown IDs are rejected.

DAW-hosted Froggers v2 uses host parameters and read-only status — no standalone MIDI picker or record/export row.


## Desktop and web

| Host | Layout |
|------|--------|
| **Desktop v1** | Six equal columns: **Audio → Random → Drive → Filter → Reverb → Delay**. Mod rack above; global randomize strip below. Pair-AR band on Audio. |
| **Desktop v2** | One module at a time via carousel (7 modules incl. Pair-AR/Envelope). Top chrome (transport + global command + oscilloscope), encoder rings, 15-lane mod sources, fixed 16-step sequencer, runtime File/Audio/MIDI pages. See **Desktop v2** above. |
| **Web** | One page at a time — pills order: Audio → Random → Reverb → Filter → Drive → Delay. Pages **Random through Delay** show **ten knobs** (eight musical rows + two expansion rows + Crispy). Mod sources panel expands per knob. Global **Crunchy** rotary in the strip. Pair-AR stays on Audio. |

**Desktop v1:** **MIDI Settings** for two CC→CV pairs; **Audio Settings** for devices. **Ext. In.** needs Play + toggle on.

**Desktop v2:** **MIDI** runtime page for controller mappings; **Audio** runtime page for stereo/mono output. **Ext. In.** needs **Play** + toggle on.

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

- Top chrome stack: transport/signal band (Play, Stop, Record audio, global oscilloscope) + global-command band (Rand All/Mods, scope pairs)
- Module carousel (7 modules), encoder rings, scenes, performance-band gestures
- Pair-AR / Envelope page; 15-lane permanent mod sources and 4×4 parameter-detail grid
- Fixed 16-step sequencer with direction/speed icons and long-press step clear
- Runtime File, Audio, and MIDI/Controllers pages
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
