# FroggersTiga Quick Dict

Short glosses for sim knob and mod labels. Full guide → in-app **Manual**. Daisy Field hardware → `MANUAL.md` in the repository.

## Sim mod sources

MIDI CC 1 — Hardware/Web MIDI CC → latched CV (default ch 1, CC 1); disable greys column and clears routes
MIDI CC 2 — Hardware/Web MIDI CC → latched CV (default ch 1, CC 2); disable greys column and clears routes
VCO Envelope — Slow level from VCO mix; scope trace
Random S&H 1 — S&H random mod CV ch.1 — resample with Rand Resample; see Mod indicators in Manual
Random S&H 2 — S&H random mod CV ch.2 — resample with Rand Resample; see Mod indicators in Manual

Mod depth — Crossfade amount between stored knob (base) and mod source; 0 = base only, 1 = mod only

## Transport

Engine — Top row: start/stop audio processing; with Engine on, internal VCOs drive sound by default (no MIDI or running sequencer required)
Stop — Top row: stop audio processing
Start Sequence — Performance band: toggles **Start Sequence** / **Stop Sequence** for pattern playback (not the same as Engine); step gates shape envelopes only while it runs
Record — Performance band: arm sequencer step capture
BPM — Performance band: sequencer tempo (slider beside numeric readout)
Steps — Performance band: pattern length in steps (4–64)
Ext. In. — Optional line/mic; parallel ring mod when gate open; VCO-only when off or silent
Randomize — Per module: randomize all three scene slots (S1/S2/S3 stored positions) for every musical row on that page (not Crispy); control-core authority, then syncToHost. Does not change S1/S2/S3 endpoint selection or scene blend. Use **Rand mod** for mod depths on that page.
Rand mod — Mod sources + depths on current page
Rand All — All modules: all three scene slots per musical row (not Crispy) + mod depths + global Crunchy scene slots; also randomizes L/R endpoint assignment and scene blend; clears gesture selection first
Rand-seq — Sequencer toolbar dice: same scene-slot policy as Rand All (includes Crunchy slots); writes step buffer(s) per Step/Pattern scope; also randomizes live L/R endpoints and blend once per press; does not write mod depths
Rand Mods — All mod routes
Rand Resample — Resample both S&H channels (draws from bags)
Rand waveforms — Randomize VCO morph (sine/saw/square blend)
Crunchy — Global fuego on all pages and all Crispy instances (web global strip)

## Scenes (desktop v2 performance band)

Cold start — Each row's three scene slots seed from factory defaults on launch; Audio VCO1–VCO3 default to **30 Hz** with **sine / square / saw** morphs respectively; audible baseline without editing rings.

Scene S1 / S2 / S3 — Pick L/R morph endpoints (global across all modules): first press sets the left endpoint scene, second press sets the right. Active endpoints show **·L** / **·R** on buttons. Does not store current knob positions on button press.
Scene blend — Morph between those two endpoints (slider ends labeled **L** / **R**; 0 = left scene ordinal, 1 = right)
Encoder rings — With no gesture lane selected, turns edit the scene slot selected by blend; concentric L/R arcs show stored scene centers; blended dot shows morph position

Scenes vs gestures vs sequencer — Scenes hold per-knob stored positions; gestures hold per-knob performance offsets; sequencer steps recall scenes/gestures on a timed grid (see Sequencer section).

## Gestures (desktop v2 performance band)

G1 / G2 — Short labels for **Gesture 1** / **Gesture 2** toggles in the performance band
Gesture 1 (G1) — Select gesture lane 1, then turn an encoder ring to write offset for that lane; badge appears on affected encoders
Gesture 2 (G2) — Select gesture lane 2, then turn an encoder ring to write offset for that lane; badge appears on affected encoders
Gesture weight — Horizontal slider (0–1) per lane scales how much the lane affects rings
Rand All — Clears gesture selection before randomizing

## Shift (desktop v2)

Shift — Performance modifier from the global strip toggle, computer keyboard, or MIDI **Shift button** row (standalone)
Shift held — Blocks encoder ring drags; does not erase stored scenes by itself
Shift + press (module ring) — Factory reset **that row only**: all three scene slots (S1/S2/S3) to inventory default, mod depths to default, gesture depths to 0; scene L/R endpoints and blend unchanged; other rows untouched
Shift + press (Crunchy) — All three Crunchy scene slots to **0** — not the same as turning Crunchy down (a turn edits only the **active** scene slot selected by blend)
Shift + press (Crispy row) — Full row reset like any module knob (Crispy factory default is 0)
Mod-depth view — Shift held blocks drags (same as normal view); Shift + press resets the **underlying parameter** (all three scene slots + mods/gestures), not a single depth slider

## Sequencer (desktop v2 performance band)

BPM — Pattern tempo (performance band slider labeled BPM)
Pattern length / Steps — Steps per pattern (4–64)
Start Sequence — Begin sequencer pattern playback (performance band; button reads **Stop Sequence** while playing)
Record arm — Capture per-step scene/gesture snapshots while advancing steps
Edit step — Selected step for authoring (toolbar arrows, dice, context menu); distinct from playback **playhead**
Playhead — Current playback position during **Start Sequence**; both highlights visible when edit step and playhead differ
Step gates — **Double-click** a step cell toggles gate lit/rest. Gates drive per-VCO AR envelopes **only while Start Sequence runs**. When stopped, lit gates are stored pattern data only (cells render dimmed); internal VCOs continue at full level.
Single-click step — Select edit step; gate unchanged
Double-click step — Toggle step gate; edit step becomes that step
Right-click step — Context menu **Reset** (factory cold-start snapshot into that step) or **Randomize** (full step snapshot for that step only; does not change live L/R/blend)
← / → — Previous/next edit step within pattern length (wrap)
Dice (Rand-seq) — Randomize scene slots into step buffer(s); **Step** scope = edit step only; **Pattern** scope = blank steps only; also randomizes live L/R endpoints and blend once per press (see Rand-seq above)
Step / Pattern — Scope toggle beside dice: **Step** targets edit step; **Pattern** fills steps with `hasData == false` only

### VST v2 (Quick Dict only)

No Engine row — DAW owns audio transport; internal VCOs drive sound when the DAW is playing audio.
Start Sequence — Same step grid and edit-step UX as standalone once audio is processing.
DAW MIDI Start/Stop — May toggle sequencer playback (**Start Sequence** / **Stop Sequence**).
Step gates — Same policy: affect envelopes only while sequencer is playing.

## MIDI CV (desktop v2 standalone)

Performance-first layout — One primary MIDI input (groovebox-style): dedicated Note/CC **performance triggers** for Shift and Scene S1–S3, separate from envelope **Gate** and from **Pitch** on one knob; CC A/B are live mod sources in module menus. VST uses DAW MIDI/automation instead — no MIDI CV settings dialog.

MIDI In — Pick one input stream: computer keyboard (QWERTY notes on virtual channel), none, or a hardware port
CV Assignments — Map messages from that input to pitch, gate, CC modulators, and performance triggers
Pitch — Page + row target (read-only parameter name); incoming notes bend that knob
Gate — **Any** held note on/off drives Pair-AR envelope gates when enabled; OR-combines with **step gates** while **Start Sequence** runs — not assignable on the Shift row
MIDI CC A / MIDI CC B — Incoming CC routes assignable as mod sources in module mod menus (Ch **Any** = all channels)
Shift button — Dedicated Note or CC + Ch for the **Shift modifier** only (hold = block turns; press while held = reset targeted ring)
Scene S1 / S2 / S3 — MIDI triggers for scene L/R endpoint selection (Note or CC + Ch)
QWERTY Ch — Virtual MIDI channel for computer keyboard notes (1–16)

## Gates (desktop v2 — three meanings)

Step gate — Per-step pattern cell (double-click in sequencer grid); shapes Pair-AR envelopes **only while Start Sequence runs**; stored pattern data when stopped (dimmed cells)
MIDI CV Gate — Global row: any held input note drives envelope gates when enabled; combines with step gates during playback
Ext. In. gate — Schmidt level gate (~−40 dBFS) for external ring mod — unrelated to sequencer or MIDI CV Gate

## Pair-AR (desktop v2 module)

Carousel page **Pair-AR** (page 7): per-VCO attack/release + page Crispy — **no sustain knobs** (web AR parity).

| Row | Desktop v2 | Web Audio (pair-sum) |
|-----|------------|----------------------|
| Atk1 / Rel1 | VCO1 attack / release | Attack 1+2 / Release 1+2 |
| Atk2 / Rel2 | VCO2 attack / release | Attack 2+3 / Release 2+3 |
| Atk3 / Rel3 | VCO3 attack / release | — |
| Crispy | Page fuego for rows 0–5 | Audio page Crispy (shared fuego stack on v2 hosts) |

Gate policy — **open** when sequencer is stopped (`!m_playing`); while **Start Sequence** runs, gates follow step gates + MIDI note/CV. Knob edits are audible immediately; A/R times apply on the next gate edge.

## Crunchy, Crispy, and pair-AR

| Control | Page rows 1–7 | Pair-AR (web Audio / desktop v2 page 7) | Notes |
|---------|---------------|----------------------------------------|-------|
| Crispy | Scrambles musical rows | Included (Audio Crispy on web; Crispy row on Pair-AR page) | Mod affects scramble intensity |
| Crunchy (global) | All rows all pages | Included on web + desktop v2 fuego hosts | Desktop v2: scene encoder ring (S1/S2/S3 + blend); web: single rotary |

## Global

Crispy — Scramble knobs 1–7 on any page (mod applied first); moddable for scramble intensity (Field: FUEG — same fuegoizer, not external mix)
Crunchy — Global fuego pass on every row every page including Crispy; stacks before page Crispy. **Desktop v2:** uses S1/S2/S3 scene slots and blend like module encoder rings (not a single unscened rotary). **Web:** single global rotary.

## Audio

VCO1 — Frequency + morph; cold start **30 Hz**, **sine** morph
VCO2 — Frequency + morph; cold start **30 Hz**, **square** morph
VCO3 — Frequency + morph; cold start **30 Hz**, **saw** morph
Cross-coupler — CCW 1→2, CW 2→3 from noon
Phase mod 1 — VCO2 → VCO1 when coupled
Phase mod 2 — VCO1+VCO3 → VCO2
Phase mod 3 — VCO2 → VCO3 when cross-coupler is CW (2→3)
Attack 1+2 — Pair-sum attack (VCO1+VCO2) — web Audio pair-AR
Release 1+2 — Pair-sum release (VCO1+VCO2); not reverb Decay — web Audio pair-AR
Attack 2+3 — Pair-sum attack (VCO2+VCO3) — web Audio pair-AR
Release 2+3 — Pair-sum release (VCO2+VCO3) — web Audio pair-AR

## Random

Step chance — Probability each channel resamples on Rand Resample press
Deja vu 1 — Channel 1 bag walk / re-roll
Bag size 1 — Channel 1 values (2–8)
Slew 1 — Channel 1 glide between held values (not a clock) → Random S&H 1
Deja vu 2 — Channel 2 bag walk / re-roll
Bag size 2 — Channel 2 values (2–8)
Slew 2 — Channel 2 glide between held values → Random S&H 2
Spread — Random channel spread (web v2 row 8)
Bias — Random channel bias (web v2 row 9)

## Reverb

Wet/dry — Reverb mix
Room size — Delay line lengths
Decay — Feedback / tail length
Pre-delay — Time before reverb tank
Damping — HF loss in feedback
Stereo width — Reverb L/R spread
Diffusion — Cross-feed between reverb lines
Mod depth — Reverb modulation depth (web v2 row 8)
Hold — Reverb hold (web v2 row 9)

## Filter

Comb offset — Short line before comb — smears strike, not pitch
Peak freq — Peaking EQ frequency
Peak gain — Peaking EQ gain
Peak Q — Peaking EQ Q
Comb delay — Comb pitch
Comb feedback — Comb resonance
Comb LP — Darken comb feedback
Comb/Peak — Parallel comb and peak mix (web v2 row 8)
Scoop — Filter scoop (web v2 row 9)

## Drive

Drive — Polynomial drive amount
Shape — Drive curve
SRR 1 — Sample-rate reducer 1
SRR 2 — Sample-rate reducer 2
XOR — XOR bit mask on samples
Bit depth — Low-bit scramble depth
Fuzz — Sine/tanh blend
Blend — Drive blend (web v2 row 8)
Phase — Drive phase offset (web v2 row 9)

## Delay

Delay time — ~0–2 s exponential
Send — Output to delay
Feedback — Delay feedback
Stereo width — L/R separation
Detune — Stereo pitch offset on repeats
Mod depth — LFO on delay time
Wet mix — Delay wet level
Color — Delay tone color (web v2 row 8)
Halo — Delay halo width (web v2 row 9)

## Field-only

FUEG — Scramble knobs 1–7 (mod first); also PM3 depth on Audio page; not external ring-mod mix
Pickup badges, M1–M7 CV assign, SW pages, OLED abbreviations → repository `MANUAL.md`.
