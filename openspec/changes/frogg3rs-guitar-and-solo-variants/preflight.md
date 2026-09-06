# Preflight audit — `frogg3rs-guitar-and-solo-variants`

Run 2026-08-28 against the working tree. Every claim below was checked by
reading the cited file, not by reasoning from the proposal's own framing.

Verdict: **REJECT AS WRITTEN, APPROVE AFTER THE FIXES IN §F.** The change's
central trace is sound and its goal is buildable. Three of its tasks rest on
claims the source contradicts, and one rests on a premise that measurement
refutes.

---

## A. Scope trace — who compiles the engine

`src/core/FroggersEngine.hpp` is `#include`d by exactly one file:
`src/FroggersTiga/FroggersTiga.hpp:3`. Every other hit in the repo is a COMMENT
citing it as a parity reference, never an include — 20 such comments in
`app/FroggersDspParityTests.cpp` and `app/FroggersAppCore.hpp`.

So the engine compiles only into the Daisy Field firmware, and editing it cannot
reach the Sheaf app, desktop, VST or browser hosts. The proposal's non-goals hold
structurally, not just by intent.

**Accepted collateral, stated rather than discovered later:** those 20 comments
cite `FroggersEngine.hpp:LINE`. Every edit to that file shifts the lines they
name and makes them stale. They are in `app/`, which this change may not touch,
so they are left stale deliberately. Naming it here is the whole of the
mitigation available.

## B. Verified as written

| claim | cited | verdict |
|---|---|---|
| `MixExternalAndOsc` body quoted verbatim | `FroggersEngine.hpp:811-820` | VERIFIED |
| one definition, one call site | grep, whole repo | VERIFIED (call site `:871`) |
| gate is `m_extGate` over smoothed `\|extIn\|` | `:868-870` | VERIFIED |
| `chainIn` → `m_frogBlock.Process` → `ApplyOutputFx` | `:871-873` | VERIFIED |
| `UpdateParams()` called from `ProcessSample` | `:852` | VERIFIED |
| `ProcessBlock` loops per sample | `:653-660` | VERIFIED (loop proper is `:656-659`) |
| `ProcessReverb(output)` runs every sample | `:844` | VERIFIED, blended at `:847` |
| `SwapBuffersAndTransmit()` unconditional every poll | `DaisyIO.hpp:137` | VERIFIED |
| screen IS throttled, LEDs are not | `DaisyIO.hpp:14,218` vs `:137` | VERIFIED |
| B1/B3 immediate, B2/B4 queued | `DaisyIO.hpp:52-74` | VERIFIED |
| `DrainOne` runs `RandomizeAllPages()` whole | `FieldMutationQueue.hpp:42-58` | VERIFIED |
| coalescing exists in `Enqueue` | `FieldMutationQueue.hpp:26-33` | VERIFIED |
| six biquad recomputes to reach two states | `:558-564` + `ResonantBump.hpp:24-40` | VERIFIED as counted — but see D1 |
| `m_scoopNotch` height from its own smoother | `:563-564` | VERIFIED |
| ring-mod term uses raw per-VCO samples | `:819` vs `MixOscVoices` `:772-809` | VERIFIED |
| one firmware target, `TARGET := FroggersTiga` | `src/FroggersTiga/Makefile:1` | VERIFIED |
| `src/mk/daisy.mk~` is a stale backup nothing includes | diff vs `daisy.mk`; grep | VERIFIED, tracked by git |
| `external-ring-mod-mix` overreaches to all hosts | `openspec/specs/external-ring-mod-mix/spec.md:5,40` | VERIFIED |
| Sheaf ring mod is a different mechanism | `app/dsp/Vco.hpp`, `app/FroggersAppCore.hpp:1108` | VERIFIED |

## C. Wrong citations — substance holds, line numbers do not

Task 0.5.3 says `m_bumpFreq.Process()` is at `:557` and `:560`, and
`m_bumpWidth.Process()` at `:559` and `:561`. Actual:

```
558  m_resonantBump.SetFreq(m_bumpFreq.Process());
559  m_resonantBump.SetHeight(m_bumpResonance.Process());
560  m_resonantBump.SetWidth(m_bumpWidth.Process());
561  m_scoopNotch.SetFreq(m_bumpFreq.Process());
562  m_scoopNotch.SetWidth(m_bumpWidth.Process());
563  const float scoop = m_filterScoop.Process();
564  m_scoopNotch.SetHeight(std::max(0.05f, 1.0f - 0.95f * scoop));
```

`m_bumpFreq` is `:558` and `:561`; `m_bumpWidth` is `:560` and `:562`; `:559` is
`m_bumpResonance`, which is read once. The double-read is real. The line numbers
are not. Also `ResonantBump`'s coefficient block runs `:42-72`, not `:42-62`.

## D. Findings that change the work

### D1. The parallel filter branch is dead, so half the cost the plan attacks is not real cost — it is unreachable code

`SetUseV2FilterParallel` (`FroggersEngine.hpp:263`) has **zero callers**. The
only occurrences of the flag in the whole repo are its default `false`
(`:113`), the setter, and the read at `:824`. `app/FroggersAppCore.hpp:1644`
carries a comment saying the app "used to mirror" that call — it no longer does.

Therefore `ApplyOutputFx`'s parallel branch `:825-833` never executes, and:

- `m_scoopNotch` is never processed. Its three per-sample setter calls
  (`:561,:562,:564`) recompute a biquad **no audio ever passes through**.
- `m_filterCombPeak.Process()` (`:828`) and `m_filterScoop.Process()` (`:830`)
  are never called at runtime.

Task 0.5.2's headline is "six biquad recomputations reduced to two." Three of
those six belong to a filter that is inert. The true figure is **six reduced to
one**, and the correct fix is deletion, not consolidation.

Two independent confirmations that the scoop path is inert even if the branch
were reached:

- `m_filterScoop.SetTarget(m_filterParams->GetParam(8))` (`:481`), but the filter
  page only ever `InitParam`s positions 0–6 (`:632-638`). Position 8 is a
  default-constructed `Parameter` with `m_knobValue = 0`
  (`Parameter.hpp`, ctor). The target is permanently 0, so
  `SetHeight(max(0.05, 1 - 0.95*0)) = 1.0` — and `ResonantBump.hpp:44-45`
  documents height 1.0 as transparent.
- `m_filterCombPeak.SetTarget(m_filterParams->GetParam(7))` (`:480`) reads
  position 7, which `SetFuegoization()` initialises as **`FUEG`**
  (`Page.hpp:75,81`, default `crispyRow = 7`, called at `:649`). The comb/peak
  blend is wired to the Crispy knob. Inert today; a live bug the moment the
  branch is reached.

### D2. The double-read family has three members, not two

Counting `.Process()` call sites per smoother across `FroggersEngine.hpp`:
`m_bumpFreq` 2, `m_bumpWidth` 2, `m_filterScoop` 2, everything else 1.

Task 0.5.3 names two and stops. Task 0.5.2 looks straight at the third —
"`m_scoopNotch`'s height is computed from `m_filterScoop.Process()` … check that
before folding it in" — and does not notice it is itself double-read, at `:563`
and `:830`. §7: acting on the named member leaves its siblings in the state that
made the named one worth raising.

Under D1 this resolves by deletion rather than by consolidation.

### D3. `UpdateParams()` at block rate — measured, and refused

Task 0.5.1 required this be settled by measurement before anything else runs.
Block size is **48**, set by `External/libDaisy/src/daisy_field.cpp:79`
(`seed.SetAudioBlockSize(48)`); `DaisyIO::Init` never overrides it. Sample rate
is **48 kHz** (`App.hpp:15`).

Compiling the real `RuntimeParam.hpp` / `OPLowPassFilter.hpp` on the host and
measuring time to 90% of a unit step:

| call rate | alpha | calls to 90% | time to 90% |
|---|---|---|---|
| per sample, today | 0.122694 | 18 | **0.375 ms** |
| per block, same alpha | 0.122694 | 18 | **18.0 ms** |
| per block, alpha re-derived | 0.956514 | 1 | **1.0 ms** |

The re-derivation is not clean. It asks `SetAlphaFromNatFreq(1000/1000 = 1.0)`,
which exceeds `x_maxCutoff = 0.499` (`OPLowPassFilter.hpp:9,28`) and is silently
clamped. A 1 kHz smoother cannot exist at a 1 kHz update rate — the request is
above the block-rate Nyquist. Keeping the alpha smears every knob 48×; clamping
discards 95.7% of the filter state per block, which is smoothing removed, not
preserved.

**Positive control (§8.1):** the controlling quantity moved — 0.375 ms to either
18.0 ms or 1.0 ms — so the instrument was live and this is a real negative, not
a void run.

**Ruling: 0.5.1's own exit condition fires. `UpdateParams()` does not move to
block rate.** Task 0.5.4 already anticipated this outcome and is satisfied by
saying so. The headroom comes from D1's deletion instead.

### D4. There is no boot screen to label

`DaisyIO::Init` (`:180-209`) calls `display.Fill(0); display.Update()`
(`:186-187`) and nothing else — it clears the OLED and shows nothing. Task 1.2
speaks of the variant name reaching "boot screen, manual, and the firmware
artifact's filename" as though the first exists. It has to be written.

### D5. Task 0.4 writes to a file git no longer tracks

`.gitignore:39` now carries `openspec/changes/archive/`, and 599 archive files
are staged for deletion in the working tree. Editing the archived
`2026-07-25-field-button-latency-headroom` banner is still worth doing for a
reader of the working tree, but it will not appear in the repo. Stated so the
edit is not mistaken for a published correction.

### D6. Spec delta corrects the requirements and leaves the Purpose overreaching

The delta rewrites both requirement bodies, and their headers match the existing
ones in `openspec/specs/external-ring-mod-mix/spec.md:9,28`, so the merge is
well-formed. But `:5` — "in the shared `FroggersEngine` across all hosts" — is
the same overreach in the Purpose line and the delta does not touch it.

Task 0.3 also asks whether `field-operator-doc-parity` repeats the overreach. It
does not: grepping it for `any host`, `all hosts`, `desktop`, `WASM` returns
nothing. The check was worth asking for and its answer is no.

### D7. `MixOscVoices` has side effects on the external path

`MixExternalAndOsc` calls `MixOscVoices(v1,v2,v3)` at `:813` before the gate
test and discards the result when the gate is open. That call advances pair-AR
envelope state — `m_pairAr->tickSmoothers()` and two `m_pair12/m_pair23.Step()`
calls (`:791-805`). Any "obvious" early-out that skips it would change pair-AR
evolution on the external path. Guitar must keep calling it for the same reason.
This is a preservation constraint the plan does not state; task 5.5 asserts the
adjacent fact (the ring-mod term uses raw samples) but not this one.

### D8. Numbering

The task list runs 0, 0.5, 1, 2, 3, **5**, 6. There is no section 4.

## E. Other active changes

`frogg3rs-drive-tone-floor` targets `app/dsp/Drive.hpp`;
`frogg3rs-browser-audio-device-selection` targets `app/browser/`. Neither names
any path under `src/`. No overlap with this change.

## F. Required fixes before execution

1. Replace 0.5.2 and 0.5.3 with the deletion in D1: remove the dead parallel
   branch, `m_scoopNotch`, `m_filterCombPeak`, `m_filterScoop`,
   `m_useV2FilterParallel` and `SetUseV2FilterParallel`. Report six biquad
   recomputes per sample reduced to one. Correct the line citations.
2. Resolve 0.5.1 to "does not happen", carrying the measured table from D3, and
   mark 0.5.4 satisfied by that outcome.
3. Say in 1.2 that the boot screen is new.
4. Note in 0.4 that the archive edit is untracked.
5. Extend 0.3 to the Purpose line, and record that
   `field-operator-doc-parity` is clean.
6. Add the D7 preservation constraint to §5.
7. Renumber, or state that section 4 is intentionally absent.
8. Build mechanism (task 1.1) is settled by operator ruling: two app
   directories, two `TARGET`s, two `.bin`s, flashed independently. Neither
   binary carries the other's code. `daisy.mk:98` composes `DEFS` from the app
   Makefile, so a `-D` set before the include is the selector; `BUILD_DIR` is
   per-app-directory (`config.mk:10`), so the two builds cannot collide.
