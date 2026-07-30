# Sheaf Adoption Inventory (Packet 13.5)

Inventory of how desktop-v2 relates to the upstream Sheaf synth project
(`github.com/jvictor0/sheaf`, `openspec/specs/synth-*`, reviewed 2026-07-09).
Sheaf is the reference for the portable-UI / device-neutral synth architecture.

**No-network-fetch constraint (13.5):** SATISFIED. `rg -ni sheaf` across
`desktop-v2/Source`, `src/`, `sim/` shows no HTTP/fetch/clone/download of Sheaf
at build or runtime. Sheaf is a design reference, not a runtime dependency.

## Adoption status by capability

| Sheaf capability | Spec | desktop-v2 status | Notes |
|---|---|---|---|
| Scene interpolation (`left*(1-blend)+right*blend`) | `synth-parameter-modulation` spm-8 | **ADOPTED / consistent** | `FroggersV2ControlCore::blendedSceneCenter` matches spm-8 exactly; verified during the GlobalCrunchy investigation (crunchy is a scene-morphed param; "global" = all modules, not all scenes; local crispy is a crunchified attenuator via the same `Fuegoize` path — `sim/V2FuegoStack.hpp::ApplyMusicalRow`). |
| Device-neutral input as portable *actions* (backends translate events → semantic actions) | `synth-runtime-ui` sru-14, sru-20 | **PARTIALLY aligned** | Clear-step is a device-neutral action: manifest declares `deviceNeutralClearStep {longPress:true}`, `heldGestures:false`; `SequencerPanelComponent::clearStep()` is the single action, dispatched by mouse/touch long-press. This is why 5.6's "MIDI leg" is N/A — MIDI is not a per-device target (see below). desktop-v2 does NOT yet have Sheaf's full JUCE-free portable-action boundary; UI is JUCE components directly. |
| Controller mapping via JUCE-free view model (controllers → params / message-ins / gestures) | `synth-runtime-ui` sru-11, `synth-midi-instrument` | **Aligned in principle** | `MidiCvAssignmentTable` maps controllers to manifest-owned target IDs (`kControllerTargetDeclarations`), data-driven `MidiCvSettingsComponent` (Packet 10). Not Sheaf's exact `MidiConfigViewModel`, but same single-authority, no-MIDI-learn principle. Controllers map to params/messages/gestures — NOT to UI actions like clear-step. |
| Patch persistence (`PatchManager`, command + version-file model, **no dirty flag**) | `synth-patch-persistence` spp-4, spp-6 | **DIVERGENT** | desktop-v2 uses `FilePatchRuntimeState` (a `dirty` boolean, just corrected in this change) + `AudioEngine` save/load/revert, not a JUCE-free `PatchManager` with per-save version files. Sheaf has no dirty flag (state is command/version-oriented). Reconciliation target: replace the boolean with a computed "current values differ from last saved version". |
| Portable UI model (JUCE-free semantic control tree + backend adapters) | `synth-runtime-ui` sru-14, sru-16 | **NOT adopted** | desktop-v2 renders through JUCE components (`MainComponent`, `PageCarouselComponent`, runtime pages). Largest future adoption gap; would make a browser/DOM backend possible. |
| Randomization via held modifiers + shift-press (`ToggleRandom`/`ToggleRandomMod`, `random-held`/`random-mod-held`) | `synth-parameter-modulation` spm-14, spm-15 ("press, shift-press, and tick routing"), spm-19/20 | **DIVERGENT — deliberate** | Froggers v2 uses discrete scope buttons (Rand All, Rand Mods, per-module Randomize) instead. See "Departure: randomization model" below. This is why the `Shift` toggle / `MessageIn::ShiftHeld` are inert and being removed (design D16). |

## 5.6 resolution grounded in this inventory

The sequencer long-press clear-step is **device-neutral by design** (sru-14/sru-20
pattern + manifest `deviceNeutralClearStep`). Adding a per-device
`midi_seq_step_clear` controller target would contradict the Sheaf model
(controllers map to params/messages/gestures, not UI actions). Resolution:
`clearStep()` is the single device-neutral action; mouse/touch dispatch it today,
any future input backend dispatches the same action. No manifest target added.

## Departure: randomization model (held modifiers → discrete scope buttons)

**Recorded 2026-07-09 — deliberate, principled departure from Sheaf.**

**Sheaf's model** (`synth-parameter-modulation` spm-14/15/19/20): randomization is a
**held modifier + press**. The operator holds a modifier — `Random` (`random-held`)
or `Random-mod` (`random-mod-held`) — and *presses an encoder*; the modified
("shift-press") action randomizes **that single visible/selected target** (the
pressed knob, or the pressed target's modulation subtree). `GetCurrentModifier`
resolves one effective modifier by precedence. Modifiers are `MessageIn` booleans
(`ToggleRandom`/`SetRandom`, …) so a hardware button can hold them.

**Why Froggers departs:** the held-modifier + per-press model has **no clear
affordance to randomize a whole parameter group (module/page) or every parameter
at once from a MIDI controller.** A modified press targets only the one encoder
under the finger; there is no "select this whole module" or "select all" scope that
a modifier-press composes with. Froggers' core randomization affordances are exactly
those group/all scopes — **Rand All** (everything, scene-scoped), **Rand Mods** (all
live mod depths), and **per-module Randomize** (a whole page) — which map cleanly to
single discrete MIDI-mappable buttons (`kControllerTargetDeclarations`), whereas
Sheaf's model would require the operator to hold a modifier and then press every
encoder in the group one by one. Discrete scope buttons are therefore the correct
model for Froggers, and the product contract's **"Held gestures are not part of
Froggers v2"** encodes this.

**Consequence:** the `Shift` toggle is a fossil of Sheaf's held-modifier paradigm —
every input path (toggle, keyboard Shift key, MIDI shift button) dead-ends at
`FroggersV2ControlCore::applyMessage` `case ShiftHeld: break;`. Nothing reads a
shift/held state to gate behavior. It is removed (design D16 / Packet 18.4), and no
mappable shift target is retained (a mappable-but-inert modifier is the same trap).

**Re-evaluation trigger:** revisit this departure **only if Sheaf later adds a
group-scope / page-scope / all-parameters selection** that its modifier-press
randomization composes with (e.g. "select bank/all, then random-press randomizes the
whole selection"). If that lands upstream, the held-modifier model would then cover
Froggers' Rand-All / per-module scopes and adoption could be reconsidered. Absent
that, discrete scope buttons stay.

## Recommended future adoption order (post-change)

1. Patch persistence → Sheaf `PatchManager` semantics (retire the dirty boolean).
2. Portable UI action boundary for input (formalize the device-neutral action
   layer desktop-v2 already gestures at with `clearStep`/`onPageChanged`).
   **Packet 15 adds `ModDrillIn`** as the enter-mod portable action (mouse MOD
   LED, future MIDI encoder press); `ParamTurn` remains turn-only.
3. Full JUCE-free portable UI model (enables non-JUCE backends).
4. **Packet 19** — per-row MIDI encoder turn/press manifest targets on Controllers page
   (after `ModDrillIn` boundary lands in 15.2). See `tasks.md` Packet 19, `design.md` D18.
