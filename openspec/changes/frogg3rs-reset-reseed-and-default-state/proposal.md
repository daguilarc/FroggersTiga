# Proposal — `frogg3rs-reset-reseed-and-default-state`

**Created 2026-08-30. Supersedes `frogg3rs-default-state-single-source`,** whose
investigation plan rested on two premises that measurement falsified. That change
is archived unimplemented; its spec delta is carried forward here unchanged.

Three controls claim to return the instrument to its default state — launch,
Reset All, and New. Two of them are wrong, in unrelated ways, and one is governed
by no requirement at all.

`openspec/specs/froggers-transport-and-reset-controls/spec.md:39-52` already
requires "a single definition shared by launch and reset, so the two can never
drift apart," and its scenario at `:53-59` requires that after Randomize All,
Reset All leaves the state equal to a fresh launch "field for field" (`:57`).
That requirement governs launch and reset, does not govern New, and was not
backed by a check strong enough to hold it.

## What the superseded change got wrong

Recorded because both errors were in its METHOD, and the method is reusable.

- **It planned to re-arm the audible reproduction at Grace/Curve knob values.**
  That cannot work. Grace and Curve govern how long ANY patch sustains, so
  raising them holds a pristine arm up exactly as much as a reset arm. Measured
  across a 3x5 grid: zero usable points, every point either decaying in both
  arms or holding in both. The separation was never a function of the knobs.
- **Its fallback was a parameter-state diff via `ParameterValuesToJSON()`.**
  That serialises `sceneCenters`, `gestureValues`, `gestureActive`
  (`ParameterModulation.cpp`, `Parameter::ToValueJSON`) — the COMMANDED state,
  which Reset All already sets correctly. It cannot observe this defect at all.

Both errors share a shape: a check chosen for being available rather than for
being able to fail.

## Defect A — Reset All did not reseed. FIXED.

`ProcessFrame()` ran `ComputeAllParameters()` only under `if (randomizeRan)`, so
a reset draining on a later block than the randomize never reseeded. Reset writes
`sceneCenters_` and returns; the per-sample path then WALKS the computed values
there at ~81% of the remaining distance per block — a rate the test file already
documented (`app/FroggersAudioRoutingTests.cpp:96` in this change's tree, "only ~81%
applied one block after it is written"). During that walk the DSP is driven with values partway
between what Randomize drew and what Reset commanded.

Measured, both arms identical except whether the reseed fires:

| blocks after reset | parameters differing | worst drift |
|---|---|---|
| +0 | 84 | 0.1879 |
| +1 | 84 | 0.0352 |
| +2 | 84 | 0.0066 |
| +4 | 33 | 0.00023 |
| +8 | 0 | 0 |

The per-block ratio is 0.187, matching the documented 81% independently.

Fix: one reseed covering both drains, `if (randomizeRan || resetRan)`
(`app/FroggersAppCore.hpp`). A second call site was rejected — the two paths owe
the same call for different reasons, and the reasons are recorded there.

Evidence chain, each step measured in the configuration where it means something:

| step | result |
|---|---|
| symptom reproduces (Grace/Curve at `HEAD`) | split 12/12, same-block 0/12, pristine 0/12, x5 runs |
| check fails before fix | 84 differing at +0 |
| check passes after fix | 0 differing at every probe |
| audible symptom gone, same reverted config | 12/12 -> 0/12, x3 runs |
| nothing else broke | 320 passed, 0 failed |

## Defect B — New did not restore the startup state. FIXED.

Before, bit-identical across every run:

    fresh          : 0.51 0.51 0.49 0.49 0.51 0.51
    after New      : (not materialized) x6
    after Reset All: 0.51 0.51 0.49 0.49 0.51 0.51

`New` reclaimed the six cross-VCO pitch depth parameters outright, leaving the
three oscillators in unison. After:

    fresh          : 0.51 0.51 0.49 0.49 0.51 0.51
    after New      : 0.51 0.51 0.49 0.49 0.51 0.51
    after Reset All: 0.51 0.51 0.49 0.49 0.51 0.51

Fix: Sheaf gains an optional `HasRestoreStartupState` hook, invoked when a patch
message reverts the manager to registered defaults, through a single
`ApplyPatchMessageAndNotifyApp()` wrapper so it cannot be wired at three of the
four `ApplyPatchMessage` call sites and missed at the fourth. Frogg3rs
implements it by re-invoking `ApplyFroggersDefaultPatch` and reseeding.

The hook re-invokes the app's own definition rather than restoring a snapshot
of what it produced. An earlier draft of this proposal preferred the snapshot
shape on the grounds that it single-sources the default; that was backwards. A
snapshot is a SECOND representation of the default patch and can drift from the
function that produces it, while the hook makes launch, Reset All and New all
reach the one definition, which is what the spec clause requires.

Sheaf's entire configurable default surface is one scalar per parameter —
`ParameterConfig` (`ParameterModulation.hpp:260-269`) carries a single
`float defaultValue` and no field that can express a modulation DEPTH, which is a
relationship between a target parameter and a source slot. Frogg3rs' default
patch writes those same scalars (`app/FroggersModulation.hpp:1379-1385`) and then
adds what registration cannot hold: `ApplyAudioBankOverlay` materialises
`detail::kAudioPitchDetents` (`:1297-1302`, applied `:1390-1394`).

`New` goes `PatchManager::NewPatch()` (`PatchPersistence.cpp:673-674`) ->
`RevertAllToDefault` -> `ParameterManager::RevertAllToDefaults()` (`:545-546`) ->
`Parameter::RevertAllToDefault()` (`ParameterModulation.cpp:1772` onward), which
rebuilds centers from `config_.defaultValue` and zeroes every depth; neutral
depths are then reclaimed.

Sheaf snapshots a default at the right moment and captures the wrong subset.
`Engine::Initialize()` calls `app_.Init(&context_)` (`Engine.hpp:231`) then
`manager_.CaptureDefaultControlState()` (`:260`) — at that instant the model
holds Frogg3rs' complete default patch, overlay included. What is captured
(`ParameterModulation.hpp:906-914`) is scene, three held flags, gesture values
and selection, and active page ordinal. No parameter centers, no depths.

No app hook exists to reconcile them: `AppConcepts.hpp:27-51` offers
`HasPrepareToPlay`, `HasProcessFrame`, and one sidebar-page registration.

Reset All does not have this problem: `ResetBankToDefaultPatch` zeroes existing
depths then re-applies `ApplyBankDefaultPatch` on top, so the overlay is the last
write (`app/FroggersModulation.hpp:1409-1422`).

## Defect C — the DSP does not latch on a transient. REFUTED.

Raised as a suspicion when Defect A's fix was applied: the reseed removes a
TRIGGER, and it was not established that an ~8-block transient could not drive
the instrument somewhere that outlives it. If a unit latched, every other fast
sweep would be an unprotected trigger -- patch load, New, a scene blend, host
automation -- since none of those pass through the reseed.

Measured. Every page parameter in all six banks driven to its ceiling, held 8
blocks (the same order as the reset transient's walk), then restored through
`ApplyFroggersDefaultPatch` -- the same single definition launch uses. No
Randomize and no Reset anywhere in it, so nothing reseeds and the smoothed path
walks exactly as it did before the fix.

| arm | audible band |
|---|---|
| pristine | 5.73e-11 |
| after sweep + restore | 9.39e-13 |
| frozen (control) | 0.509 |
| floor | 1.0e-3 |

Both controls are required and both hold. The pristine arm proves the
measurement can report silence; the Freeze arm -- a deliberate, documented hold
driven through the real UI action path -- proves it can report a hold. Without
the second, a quiet swept arm would be a property of the rig rather than of the
instrument, and an earlier version of this probe recorded exactly that
uninformative pass before the control was added.

So the transient explanation is complete and Defect A's fix is a cure rather
than a mitigation. An earlier write-up of this investigation described the
latch as a live risk to patch load, New, scene blend and host automation; that
framing rested on this unverified premise and does not survive the measurement.

## Grace/Curve

`app/dsp/VoiceEnvelope.hpp` and `app/FroggersDspParityTests.cpp` are uncommitted
and land inside this change, first, per the operator's ordering. Grace's knob map
moves from linear to `ZeroedExpCompute` base 25; Curve gains `mapCurve()` so ramp
duration moves linearly with the knob. It amends
`openspec/specs/froggers-vco-topology/spec.md`.

It suppresses Defect A's AUDIBLE symptom without touching the defect — measured,
ten runs, no overlap:

| Grace/Curve | randomize-then-reset holds |
|---|---|
| reverted to `HEAD` | 12, 12, 12, 12, 12 |
| applied | 0, 0, 0, 0, 0 |

This is why Defect A's check asserts on the parameter transient at +0 blocks
rather than on audio: the transient check is independent of the mapping, and a
check taken after convergence passes whether or not the fix is present — which is
how all 314 pre-existing tests missed this.

## Other active changes

- `frogg3rs-guitar-and-solo-variants` — `src/` firmware, uncommitted. No overlap.
- `frogg3rs-browser-microphone-permission-path` — touches
  `BrowserRuntimeMainServices.hpp` at `:136-137` and `:198-204`. The File page's
  patch callbacks are at `:343-354`. No overlap.
- `frogg3rs-microphone-path-delivery` — **overlaps and is in flight** (14 open
  tasks of 37). It edits `app/FroggersAudioRoutingTests.cpp` at `:1221` and
  `:2274,2284`. This change adds `#include <optional>` and `#include <utility>`
  near the top of that file, shifting every later line by +2: those anchors
  become `:1223` and `:2276,2286`.
