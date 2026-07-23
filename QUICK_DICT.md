# FroggersTiga Quick Dict

Short glosses for sim knob and mod labels. Full guide → in-app **Manual**. Daisy Field hardware → `MANUAL.md` in the repository.

## Sim mod sources

v1 desktop and web sim use the sources below. Desktop v2 uses the **Permanent mod sources** section (fifteen lanes; no MIDI CC lanes).

MIDI CC 1 — Hardware/Web MIDI CC → latched CV (default ch 1, CC 1); disable greys column and clears routes
MIDI CC 2 — Hardware/Web MIDI CC → latched CV (default ch 1, CC 2); disable greys column and clears routes
VCO Envelope — Slow level from VCO mix; scope trace
Random S&H 1 — S&H random mod CV ch.1 — resample with Rand Resample; see Mod indicators in Manual
Random S&H 2 — S&H random mod CV ch.2 — resample with Rand Resample; see Mod indicators in Manual

Mod depth — Desktop v2: per-lane signed depth toward that mod source; 0 = lane off; multiple nonzero assignable lanes on a row sum into effective modulation. v1/web: crossfade between stored knob (base) and one mod source (0 = base only, 1 = mod only)

## Transport

Boot (desktop v2 standalone) — Healthy boot keeps the main window open after launch; instant exit indicates a build or runtime fault. Launch the Release build with `./scripts/open-desktop-v2.sh` or open `desktop-v2/build/FroggersTigaDesktopV2_artefacts/Release/FroggersTigaV2.app` directly.
Play — Top transport/signal band: start audio processing; with Play active, internal VCOs drive sound by default (no MIDI or running sequencer required)
Stop — Top transport/signal band: stop audio processing
Record audio — Top transport/signal band: round red circle + **Record audio** label; captures stereo output to file. Requires **Play** first (v1 parity). Export format (WAV / MP3 / FLAC / OGG) is set in the **Audio** menu, not in the transport row. Distinct from sequencer **Write Seq.** (step snapshot capture).
Ext. In. — Optional line/mic; parallel ring mod when gate open; VCO-only when off or silent
Randomize — Per module: randomize all three scene slots (S1/S2/S3 stored positions) for every musical row on that page (not Crispy); control-core authority, then syncToHost. Does not change S1/S2/S3 endpoint selection or scene blend. Use **Rand mod** for mod depths on that page.
Rand mod — Mod depths on current page from the 15-lane catalog
Rand All — All modules: all three scene slots per musical row (not Crispy) + mod depths + global Crunchy scene slots; also randomizes L/R endpoint assignment and scene blend; clears gesture selection first. **All Scenes** / **Current Scene** scope pair under the button selects scene write target.
Rand-seq — Sequencer toolbar dice: same scene-slot policy as Rand All (includes Crunchy slots); writes step buffer(s) per **Step** / **All steps** scope; also randomizes live L/R endpoints and blend once per press; does not write mod depths
Rand Mods — Global-command band: randomize mod depths per **All Steps** / **Current Step** scope pair; when writing sequencer snapshots, respects sequencer toolbar **Step** / **All steps** scope
Rand Resample — Resample both Random S&H channels (draws from bags)
Rand waveforms — Randomize VCO morph (sine/saw/square blend)
Crunchy — Global fuego on all pages and all Crispy instances (web global strip; desktop v2 global-command band scene encoder ring)

## Top chrome (desktop v2)

Transport / signal band — **Play**, **Stop**, **Record audio**, and the **global oscilloscope** (persistent across carousel and runtime pages)
Global-command band — **Rand All**, **Rand Mods**, **Rand waveforms**, **Rand Resample**, **Crunchy** label + ring; scene/step scope radio pairs directly below **Rand All** and **Rand Mods**
Global oscilloscope — Shell-level signal monitor in the transport band; default three color-coded VCO traces; source-group switching for LFO EF, pair buses, EFs, Random S&H, External Audio when inspected; samples come from the shared fifteen-lane CV history (one GetCvOut push per UI tick); separate from per-row CV LEDs and MOD drill-in
Parameter detail underlay — Display-only source-activity waveform under each detail-grid depth encoder (manifest lane color); Target (Back) has none; same history store as the global oscilloscope; does not change ParamTurn / ModDrillIn hit targets
All Scenes / Current Scene — Randomization scope under **Rand All**: write all three scene slots per row vs only the active scene edit target; preserves L/R endpoint picks and blend slider
All Steps / Current Step — Randomization scope under **Rand Mods**: all 16 sequencer indices vs playhead (playing) or edit step (stopped)

## Runtime pages (desktop v2)

File — Right rail: patch identity, dirty state, save/load/revert, controller-mapping persistence results
Audio — Right rail: hardware I/O, channels, sample rate, block size, external-input state, meters; no duplicate oscilloscope
MIDI — Right rail: selected input, connection/receiving/error state, explicit mapping rows, multi-target fan-out summary, persistence status, target readback

## Permanent mod sources (desktop v2)

Fifteen lanes in the parameter-detail 4×4 grid (depth 0 = off; multi-lane depths sum when eligible). No module-row mod dropdown. MIDI CC A/B are **not** lanes — they are controller targets.

VCO 1+2 — Audio-rate VCO pair bus (lane 1)
VCO 2+3 — Audio-rate VCO pair bus (lane 2)
VCO 1+3 — Audio-rate VCO pair bus (lane 3)
VCO 1 EF — VCO 1 envelope follower
VCO 2 EF — VCO 2 envelope follower
VCO 3 EF — VCO 3 envelope follower
VCO 1+2 EF — Adjacent-pair envelope follower
VCO 2+3 EF — Adjacent-pair envelope follower
LFO EF 1 — Slow (LFO-rate) VCO envelope-follower tap 1
LFO EF 2 — Slow (LFO-rate) VCO envelope-follower tap 2
LFO EF 3 — Slow (LFO-rate) VCO envelope-follower tap 3
Random S&H 1 — Random stepped source 1 (resample with **Rand Resample**; inspired by Mutable Instruments Marbles)
Random S&H 2 — Random stepped source 2 (resample with **Rand Resample**)
External Audio (audio rate) — External input audio-rate lane; visible but unavailable when input off
External Audio (envelope follower) — External input EF lane; visible but unavailable when input off

MOD / CV LED — Fixed-center click target on module encoder rings; opens 4×4 parameter-detail grid (ModDrillIn). Ring drag stays ParamTurn and does not open detail. Device-neutral: rotate/turn → ParamTurn; press (MOD LED or mapped MIDI encoder press via Controllers `…_encoder_mod_drill_in`) → ModDrillIn.
Parameter detail — 4×4 modulation grid for one row: 15 independent depth encoders (each with a CV activity underlay) + **Target (Back)**; press Target (Back) to return to the module page
Target (Back) — Sixteenth detail cell; ParamPress exits detail; no underlay

## Page carousel (desktop v2)

Module page — Left/right arrow buttons on the carousel header change the active module page. **Rand** / **Rand mod** on the carousel header randomize the current page (see §Transport).

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

## Sequencer (desktop v2)

Fixed 16 steps — Exactly sixteen slots (indices 0..15)
Written / unwritten — Each slot stores whether a step snapshot exists; unwritten slots are skipped during playback
Direction — Toolbar icon cycles `<`, `>`, `RND` (default `>`)
Speed — Toolbar icon cycles `/2`, `/1.5`, `1`, `x1.5`, `x2` (default `1`)
Long-press clear — Hold a written step (mouse, touch, or mapped controller) past threshold to mark it unwritten and wipe its snapshot; short release cancels; not a general held-gesture route
BPM — Pattern tempo (toolbar slider labeled BPM)
Start Sequence — Begin sequencer pattern playback (toolbar; button reads **Stop Sequence** while playing). Distinct from audio **Play** / **Stop**.
Write Seq. — Arm step snapshot capture into the step buffer (toolbar toggle; not **Record** or **Record audio**). **Stopped + armed:** changing edit step captures live → previous edit step, then recalls new edit step; disarming captures live → current edit step once. **Playing + armed:** pressing **Start Sequence** immediately captures playhead step and sets a one-beat skip so the first advance does not recapture step 0; each subsequent beat advance captures the step being left, then recalls landed step; edit step follows playhead while armed. Blank steps factory-seed on first visit during playback. Capture flash highlights the captured step cell briefly after every capture while armed.
Edit step — Always exactly one step selected (default step 1 / index 0); toolbar arrows wrap within 16 slots; distinct from playback **playhead**
Playhead — Current playback position during **Start Sequence**; both highlights visible when edit step and playhead differ
Step gates — **Double-click** a step cell toggles gate lit/rest. Gates drive per-VCO AR envelopes **only while Start Sequence runs**. When stopped, lit gates are stored pattern data only (cells render dimmed); internal VCOs continue at full level.
Single-click step — Select edit step; gate unchanged
Double-click step — Toggle step gate; edit step becomes that step
Right-click step — Context menu **Reset** (factory cold-start snapshot into that step) or **Randomize** (full step snapshot for that step only; does not change live L/R/blend)
← / → — Previous/next edit step within 16 slots (wrap)
Dice (Rand-seq) — Randomize scene slots into step buffer(s); **Step** scope = edit step when stopped, playhead when playing; **All steps** scope = every index 0..15; also randomizes live L/R endpoints and blend once per press (see Rand-seq above)
Step / All steps — Scope radio pair beside dice: **Step** targets edit step when stopped, playhead when playing; **All steps** randomizes every step index 0..15, including steps that already contain data

### VST v2 (Quick Dict only)

No Play/Stop row — DAW owns audio transport; internal VCOs drive sound when the DAW is playing audio.
Write Seq. — Host parameter and sequencer toolbar toggle (no **Record audio** row).
Start Sequence — Same step grid and edit-step UX as standalone once audio is processing.
DAW MIDI Start/Stop — May toggle sequencer playback (**Start Sequence** / **Stop Sequence**).
Step gates — Same policy: affect envelopes only while sequencer is playing.

## MIDI / Controllers (desktop v2 standalone)

Performance-first layout — One primary MIDI input: explicit mapping rows for **Pitch**, **Gate**, **MIDI CC A**, **MIDI CC B**, **Scene S1–S3**, **QWERTY Ch**, optional **External MIDI clock**, plus inventory-generated **encoder turn** / **mod drill-in** targets per module-row parameter. No MIDI learn or recent-event list. Multi-target fan-out allowed. DAW-hosted build uses host parameters instead — no standalone MIDI picker.

MIDI In — Pick one input stream: computer keyboard (QWERTY notes on virtual channel), none, or a hardware port
Pitch — Page + row target (read-only parameter name); incoming notes bend that knob
Gate — **Any** held note on/off drives Pair-AR envelope gates when enabled; OR-combines with **step gates** while **Start Sequence** runs
MIDI CC A / MIDI CC B — Controller targets mapped to manifest parameter IDs; not permanent mod-rack lanes
Scene S1 / S2 / S3 — MIDI triggers for scene L/R endpoint selection (Note or CC + Ch)
QWERTY Ch — Virtual MIDI channel for computer keyboard notes (1–16)
External MIDI clock — Optional sequencer clock source (timing only; not gesture routing)
Encoder turn — Relative CC → `ParamTurn` for that inventory parameter (stable ID `…_encoder_turn`)
Encoder mod drill-in — Button / note / CC threshold → `ModDrillIn` for that parameter (stable ID `…_encoder_mod_drill_in`); same enter-mod action as the MOD LED (Packet 15)

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

## Random S&H (modulation lanes)

Desktop v2 has **no Random S&H module page** — the Step chance / Deja vu / Bag size / Slew / Spread / Bias knobs were deleted. Random S&H 1/2 survive as modulation-lane sources only: fixed Sheaf-style defaults, resampled with **Rand Resample**; a ganged random-LFO visualizer is vendored and attached to their mod-depth cells in a later step. See the Random S&H 1/2 mod-source entries above.

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
