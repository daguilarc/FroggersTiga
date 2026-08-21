# Proposal — `frogg3rs-automation-view-and-musical-ranges`

**Created 2026-08-20.** Successor to
`frogg3rs-host-state-and-visibility`, which delivered the site visibility
suite, plugin session state, the external-audio re-enable, the phase-modulation
rate floor, reset-to-defaults, and a comment sweep over the live `app/` tree.
This change carries that change's undelivered scope plus two design items the
operator raised while it was closing.

## Why

- **Automation still moves the operator's visible page.** A host parameter
  write is addressed by SLOT, not by (bank, slot): `MessageIn::ParamSetAbsolute`
  carries no bank, so the bridge must push `MessageIn::SelectParamBank` first
  and the value lands on whatever bank the shared `BankSlot` then points at.
  With a DAW running lanes across banks, the editor's page changes under the
  operator's hands. The decision is made — the page follows operator selection
  only — but nothing is implemented.
- **The route for that fix needs a framework capability.** An app-side
  select→write→restore cannot avoid collateral damage: switching away calls
  `Bank::Deselect()`, which resets the outgoing bank to top level and closes
  any modulation drill-down the operator has open, while
  `FroggersModulationDrillIn`'s cached level counter goes stale. Under the new
  behavior that damage is silent. `ParameterManager::BankAt` already exists and
  `Bank::HandleSetAbsolute` is already a public bank-direct write, but it
  resolves through the bank's VISIBLE cells, which are the modulation
  drill-down page when that bank is drilled in — so the framework half is a
  page-independent bank write plus the slot-agnostic wrapper, not the wrapper
  alone.
- **The plugin declares no audio input bus.** The predecessor's own bus
  requirement reserved external audio for the day the core gained inputs. The
  core now requests a channel and the standalone connects the external
  sources from the host's routed signal, but the plugin still presents output
  only, so a DAW cannot route into it. The requirement was written and never
  satisfied; it moves here unsatisfied rather than shipping as a false promise.
- **The envelope is the only part of the instrument mapped linearly.** Every
  other time or frequency control maps exponentially through
  `dsp::ExpMapCompute` — filter cutoff, comb, scoop, resonant bump, drive
  pre-gains, fold, every modulation rate. `VoiceEnvelope`'s `mapAttack`,
  `mapDecay` and `mapRelease` are linear. A uniform random knob on a linear
  time map puts half its draws above the midpoint, so half of all randomized
  voices get a quarter-second attack and no transient. That is why randomized
  patches sound hushed.
- **Several parameter bounds produce dead draws.** Sustain floors at 0.10
  (−20 dB); attack's ceiling admits half-second fades; the peak, scoop and
  comb frequency controls floor at 20 Hz, where the effect is inaudible and
  randomization visibly does nothing. This is the same reasoning already
  recorded in the code for the resonant peak's own ceiling, cut from 10× to
  4× to 2× because a randomized depth visits a bound far more often than a
  hand-dialed one.
- **The documentation describes a product that no longer ships.**
  `SIM_MANUAL.md` documents the frozen web and desktop simulators.
  `QUICK_DICT.md` states the external-audio sources are permanently
  unavailable, which the predecessor change made false. No document explains
  audio or MIDI configuration at all. Retiring `SIM_MANUAL.md` is the one part
  that is not a documentation edit: it is a release-notes and CI input, a
  generated-mirror source, an embedded resource in two out-of-scope trees, and
  a named requirement in three live specs — see design F.

- **Hygiene is step zero, and this tree had a backlog.** About 1,017 lines of
  gate scripts are invoked by nothing at all — the largest cluster guarding a
  frozen desktop-v2 — alongside five tracked correspondence artifacts at the
  repository root and a retired product name still in the release path. Under
  omni-rule §13.0 that is not a follow-up proposal; it opens this change.

## What Changes

- **Repo hygiene (group 0, first)**: delete the gate scripts nothing invokes,
  trim the one that is still live, clear the root correspondence artifacts and
  the stale machine-local SDK caches.
- **Cutover (group 10, gated on operator acceptance)**: the merge to main, the
  desktop release product rename, the `SIM_MANUAL.md` retirement with its three
  spec deltas, the frozen-tree retirement, and the simulator's fuegoize
  divide-by-zero. The same hygiene principle, applied where the thing being
  removed is load-bearing until then.
- **froggers-vst-host** (delta): ADDED — automation delivers to its own bank
  without moving the visible page; MODIFIED — the plugin presents an optional
  audio input bus feeding the external-audio sources.
- **froggers-sheaf-runtime-app** (delta): ADDED — the manual and quick
  dictionary ship inside the standalone and the plugin, readable offline,
  embedded from the repository's single copy at build time.
- **froggers-vco-topology** (delta): ADDED — envelope times map
  exponentially; ADDED — control bounds stay within their musically useful
  range.
- **External/Sheaf**: an additive bank-addressed absolute parameter write,
  appended to the open upstream pull request.
- Code: `app/vst/` (bank-addressed writes, input bus), `app/dsp/VoiceEnvelope.hpp`
  (exponential maps, sustain floor), `app/FroggersAppCore.hpp` (filter bounds),
  and the documentation set.

## Impact

- Affected specs: `froggers-vst-host`, `froggers-vco-topology`,
  `froggers-sheaf-runtime-app`. Deleting
  `SIM_MANUAL.md` would additionally affect `sim-operator-doc-parity`,
  `froggers-host-master` and `global-strip-marbles-label`, which name it in
  their requirements; that deletion is gated on an operator decision (task
  7.1) precisely because taking it on means taking on those deltas and
  editing trees listed as out of scope below.
- Upstream: this is the first change to edit `External/Sheaf`. Its commits
  append to the existing open pull request on the pinned branch. Push Sheaf
  before the superproject — the superproject records only the pin, and CI
  builds from it.
- Carried from the predecessor, unfinished: the comment sweep over `app/vst/`
  and `External/Sheaf`; the plugin-editor half of pre-audio legibility; the
  machine-local working-folder rename; operator smoke; and the change-level
  postflight.
- Scope boundary, now sequenced rather than absolute: groups 1–9 do not touch
  the frozen `desktop/`, `desktop-v2/`, `web/`, `wasm/`, `sim/`, `src/` trees.
  Group 10 does, because the merge opens them and that is where their cleanup
  becomes safe. `src/` remains the DSP parity reference throughout, and the
  envelope work diverges from it deliberately.
