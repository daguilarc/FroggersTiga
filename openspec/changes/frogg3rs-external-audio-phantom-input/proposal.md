# Proposal — `frogg3rs-external-audio-phantom-input`

**Created 2026-08-09. Supersedes `frogg3rs-parametric-slew-and-stop-root-cause`**, archived at
`../archive/2026-08-09-frogg3rs-parametric-slew-and-stop-root-cause/` — implementation COMPLETE,
not a failure and not a partial delivery (see `SUPERSESSION-RECORD.md`). This change picks up the
one item that directory carried forward, unexecuted, across three change generations — `W4.2` — and
corrects the plan attached to it, which turns out to have been wrong the entire time.

**This document is self-contained.** Read `SUPERSESSION-RECORD.md` only for the history of how
`W4.2` was carried forward and why its framing changed; every binding fact is repeated here.

---

## 0. Why this is not simply "do W4.2 as written"

`W4.2` has read the same way since `frogg3rs-modulation-truth-and-voicing` (2026-08-05): *remove
`kExternalAudioOptedIn`, after the Sheaf pin bumps `77a3019e` → `508d9d68`, because upstream ask #8
lands the requested-vs-active distinction the app needs.* Carried untouched through
`frogg3rs-blowout-and-drilldown-repair` and `frogg3rs-parametric-slew-and-stop-root-cause`, always
listed only in each generation's "Deferred, untouched" footer — never re-derived, never re-verified.

**Both premises behind it are wrong, verified by reading — not by re-deriving the old plan:**

| the plan assumed | reading found |
|---|---|
| Ask #8 landed at `508d9d68`; `kExternalAudioOptedIn` "can go" (`UPSTREAM-SHEAF-ASK.md` item 8, as it read before this change) | At `508d9d68`, `AudioInputView::HasActiveChannel(ch)` is `channel < activeChannelCount_ && inputs_ != nullptr && inputs_[channel] != nullptr`, with `activeChannelCount_ = clamp(numInputChannels, 0, requested)` — the **same three conditions** as the app's own permanently-true check. What actually landed is `RequestedChannelCount()` vs `ActiveChannelCount()`, which answers "did the device give us what we asked for" — a different question from "did the operator route something in". `AppContext` still carries no audio-device state at `508d9d68` either. **`UPSTREAM-SHEAF-ASK.md` item 8 is corrected in place as part of this change's own citation-sweep pass**, ahead of any of the rest of this change landing. |
| The fix needs the pin bump first | The defect lives entirely in `FroggersApp::Config()`, at the CURRENT pin (`77a3019e`), and has nothing to do with which Sheaf commit is checked out. `config.numAudioInputs = 1` causes JUCE to open a default input device at either pin; that call shape is unchanged between them. |
| Removing `kExternalAudioOptedIn` is safe once the ask lands | Removing it **today**, at either pin, would immediately make both external-audio modulation sources permanently "connected" — `block.numInputChannels` is already `1` (the built-in mic opens on every launch, unasked), so the flag is the *only* thing standing between the current build and the exact bug `UPSTREAM-SHEAF-ASK.md` item 8 describes. |

This is a live landmine, not a deferred nicety. The flag's own comment invites exactly the wrong
edit — *"Flip `kExternalAudioOptedIn` when there is a real opt-in signal to gate on"*
(`app/FroggersAppCore.hpp:621-623`) — and the (now-corrected) upstream doc told a future reader that
signal had already arrived. Nothing but four generations of nobody picking up a footer item kept
that edit from being made.

## 1. Objective

**Stop opening the operator's microphone without being asked, and stop compensating for it with a
hardcoded lie.** `FroggersApp::Config()` SHALL request zero audio input channels. The two
external-audio modulation sources — `kModSlotExternalAudio` (13) and `kModSlotExternalAudioEf` (14)
— SHALL be disconnected **by construction**, because no input channel exists to derive a connected
state from, not by a separate opt-out flag that a future edit (upstream or app-side) could silently
invalidate.

**Explicitly not restoring external audio.** It stays unavailable until upstream exposes a routed-
input signal (`UPSTREAM-SHEAF-ASK.md` item 8's own ask, still open — asks 1/2 there). That is the
status quo today; this change does not change it, and does not regress it either.

## 2. Data flow — the actual mechanism, traced

**Verified by reading** `app/FroggersAppCore.hpp`, `app/FroggersModulation.hpp`, and (read-only —
`External/Sheaf` is frozen and unpatchable under this task's constraints)
`External/Sheaf/projects/synth/runtime/Runtime.hpp`:

```
FroggersApp::Config()                    config.numAudioInputs = 1   (app/FroggersAppCore.hpp:186)
  |
  v
Runtime.hpp:237                          deviceManager_.initialiseWithDefaultDevices(
                                            config.numAudioInputs, config.numAudioOutputs)
                                          -- JUCE opens the DEFAULT input device. Chosen by nobody.
  |
  v
Runtime.hpp:260-270                      persisted input device applied ONLY if
                                          wantedInputName.isNotEmpty() &&
                                          IsEnumeratedInputDevice(...)
                                          -- until the operator explicitly picks a device in the
                                             Audio page, the default mic simply stays open.
  |
  v
ProcessBlock, app/FroggersAppCore.hpp:625-626
                                          externalInputHasChannel =
                                            block.inputs != nullptr && block.numInputChannels > 0
                                            && block.inputs[0] != nullptr
                                          -- permanently true: numInputChannels is permanently 1.
  |
  v
app/FroggersAppCore.hpp:624,627          constexpr bool kExternalAudioOptedIn = false;
                                          externalInputConnected =
                                            kExternalAudioOptedIn && externalInputHasChannel;
                                          -- kExternalAudioOptedIn = false is the ONLY thing keeping
                                             this honest today.
  |
  v
app/FroggersModulation.hpp:415,423-425   SetExternalAudioConnected(externalInputConnected) sets
                                          kModSlotExternalAudio(13) / kModSlotExternalAudioEf(14)
                                          .connected -- exactly the metadata Randomize consults
                                          (src/ParameterModulation.cpp:2886-2895, upstream,
                                          correctly implemented -- it picks only among sources whose
                                          metadata says connected; the app was lying to it).
```

**Consequence, stated plainly:** one hand-set boolean is the entire reason Randomize does not
currently assign modulation depth to room noise. Delete it without also fixing `numAudioInputs`,
and the exact bug `UPSTREAM-SHEAF-ASK.md` item 8 describes — which this app already diagnosed and
worked around once — returns immediately, with zero other changes required to reintroduce it.

**The fix removes the landmine at its root, not just its trigger.** `config.numAudioInputs = 0`
means `initialiseWithDefaultDevices(0, numAudioOutputs)` opens no input device at all,
`numInputChannels` is `0` by construction, `externalInputHasChannel` is `false` by construction, and
`kExternalAudioOptedIn` — and the branches that read it — have nothing left to do. Delete them
together; there is no remaining call site for the flag once the channel it gates never exists.

**Downstream, also verified by reading:** Sheaf gates the Audio page's own input-device selector on
`numAudioInputs > 0` (`RuntimePagesJuce.hpp:71`, `JuceRuntimeMainServices.hpp:83`). A selector that
currently does nothing useful — there is no code path that routes a chosen device's audio anywhere
once `kExternalAudioOptedIn` is false — correctly disappears rather than continuing to imply a
working control the operator could reach for.

**`numAudioInputs = 0` is not a novel value in this codebase.** `FroggersMonoValidationTests.cpp`'s
own `Config()` (a separate, unshipped validation app, task 2.5) already sets
`config.numAudioInputs = 0` (`:65`) for its own unrelated minimal-rig reasons. It is not evidence
this fix is correct, only evidence `0` is already an exercised, working value for this field
elsewhere in the tree — the field is not somehow required to be nonzero to build or run.

## 3. Constraints

- **Sheaf is pinned at `77a3019e` and unpatchable.** This fix needs no Sheaf change and no pin bump
  — the defect and its fix are both entirely app-side, at the current pin. Do not gate this change
  on `W4.1` (the pin bump); it was never a real dependency, only a miscategorized one.
- **Frozen trees stay byte-identical:** `desktop-v2/ desktop/ src/ sim/ wasm/ vcv/ web/`. No change
  to `External/Sheaf`.
- **`app/FroggersHeadlessTests.cpp:111-113`, test `froggers_config_requests_exactly_one_audio_input_channel`,
  asserts `config.numAudioInputs == 1`, with an 11-line comment (`:101-110`) arguing FOR that value
  using the same `Runtime.hpp:237,260-261` contract this proposal traces to the opposite
  conclusion.** This test's name and body must be rewritten together, in the same commit as the
  `Config()` change — not left standing as a stale regression for a later pass to rediscover.
- **A second test in the same file, `froggers_init_process_block_produces_finite_stereo_output`
  (`:72-99`), has a comment (`:77-86`) claiming it "doubles as proof that FroggersApp tolerates an
  actually-present input channel" — true only because `SynthRig` currently allocates a real input
  buffer when `numAudioInputs == 1` (`tests/support/SynthRig.hpp:62,72,76,454,456`).** Once
  `numAudioInputs` is `0`, `SynthRig` allocates no input buffer and this test's own assertions
  (no NaN, finite, stereo) keep passing, but trivially — the coverage the comment claims is gone.
  Named here so it is not mistaken for a passing, unaffected test.
- **Subagents: Sonnet or Haiku, never Opus**, model set explicitly on every dispatch.
- **`nice make -j2`, never higher** (8-core/16 GB).
- **Code changes sequential. Parallel dispatch only for read-only analysis.**
- **No AI attribution on commits.**
- **Cite by symbol, not by line** — every line number in this directory goes stale on the next edit;
  the ones above are a snapshot from this proposal's own reading, 2026-08-09.

## 4. Structure plan

**T1 — the `Config()` fix.** One line, `app/FroggersAppCore.hpp`: `config.numAudioInputs = 1` →
`config.numAudioInputs = 0`. Rewrite the "Task 2.6" comment above it (currently argues carefully FOR
raising it to 1, and never follows the request through to what `initialiseWithDefaultDevices`
actually does with a nonzero count) to record why the value is back to 0 and what would justify
raising it again — a real routed-input signal from upstream, not a pin bump.

**T2 — delete the compensating flag, not just flip it.** `app/FroggersAppCore.hpp`'s `ProcessBlock`:
delete `kExternalAudioOptedIn` and its "flip me" comment, and fold `externalInputHasChannel` /
`externalInputConnected` into whichever single expression leaves the fewest names for a future
reader to reconcile.

**T3 — the two test consequences named in §3.** Land together with T1, in the same commit.

**T4 — `UPSTREAM-SHEAF-ASK.md` item 8 correction.** Already done, ahead of the rest of this change,
as part of the citation-sweep pass that archived this change's predecessor. Recorded as a checked
task in `tasks.md` rather than left as an undocumented fait accompli.

**T5 — spec delta.** `specs/froggers-modulation-slate/spec.md`, tightening "External-audio sources
stay present but inert when unavailable" to define "unavailable" precisely enough that this bug
class cannot recur silently behind a single untested boolean again.

## 5. Dependencies

- T1 and T2 land together. T2 without T1 deletes a landmine's pin while leaving the grenade live
  (§0's table, row 3, run in reverse). T1 without T2 leaves dead code — `kExternalAudioOptedIn` and
  `externalInputHasChannel` both become provably always-false, worth removing in the same pass
  rather than left for a later linter pass to flag.
- T3 has no code dependency on T1, but MUST land in the same commit (§3) — that is a process
  constraint, not a build dependency.
- T4 has no dependency on the others; it is independent documentation-only work, and is already
  done.
- T5 can land independently of T1–T3 (it documents the target behavior), but is more honest to
  write after T1/T2 actually land, so its scenarios describe verified behavior rather than a plan
  that might drift from what gets built.

## 6. Non-goals

- **Does not restore external audio.** The two sources stay unavailable; this change only makes
  their unavailability honest and construction-guaranteed instead of flag-guaranteed. Restoring the
  capability needs upstream to expose a routed-input signal (`UPSTREAM-SHEAF-ASK.md` item 8, asks 1
  or 2) — the app-side change at that point is one line (raise `numAudioInputs` back to 1 or more,
  and derive `externalInputConnected` from the real signal instead of a channel-exists check).
- **Does not bump the Sheaf pin.** `W4.1` (`77a3019e` → `508d9d68`) is unrelated to this fix and
  stays independently deferred.
- **No change to `External/Sheaf`, and no change to any other frozen tree.**
- **Does not revive S2 (the slew).** Dropped by the operator in the predecessor change
  (`../archive/2026-08-09-frogg3rs-parametric-slew-and-stop-root-cause/tasks.md` §S2); out of scope
  here and must not be reintroduced as a side effect of touching neighboring code in
  `FroggersAppCore.hpp`.
- **Does not touch any of the predecessor's other landed work** (drive-stage DC fix, transport-gated
  modulation, reverb saturator, VCO refactor, drilldown/header UI). Different mechanism, different
  file region, orthogonal to this change entirely.
