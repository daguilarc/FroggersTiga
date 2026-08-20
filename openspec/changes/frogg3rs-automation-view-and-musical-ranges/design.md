# Design — frogg3rs-automation-view-and-musical-ranges

Anchors below were re-read 2026-08-20 against the current tree. Where a
reference names a function, constant or class it cites the SYMBOL, not a line
number: the predecessor change carried six wrong line citations, one of them
400 lines past the end of its file, and three more rotted during execution.

## A — Automation must not move the visible page

**Decided (operator):** the editor's page follows operator selection only.
Automation writes land on their own bank's parameter and are silent. Two lanes
in different banks must not oscillate the view.

**Why it is not free.** `MessageIn::ParamSetAbsolute` is slot-addressed;
`MessageIn` carries a `bankIx` field but `MessageInBus::Apply` passes it only
for `SelectParamBank`. `ParameterManager::HandleSetAbsolute` resolves through
`BankSlot::ResolvePosition` and requires `BankSlot::Owns`, so a cross-bank
write needs the shared slot pointed at the target bank first.

**The primitive already exists.** `Bank::HandleSetAbsolute(PhysicalEncoderId,
const SceneState&, float)` is public and operates only on that `Bank`'s own
cells, never touching `BankSlot` or `selectedBank_`, and
`ParameterManager::BankAt(std::size_t)` is public. What is missing is a
slot-agnostic wrapper.

**Why the app-side route was rejected.** select→write→restore works
mechanically — `Engine::DrainMessageBus` applies non-realtime messages in
strict FIFO within one block, so the triple lands before any repaint. But
`BankSlot::SelectBank` calls `Deselect()` on the outgoing bank, which resets
`visible_` to `topLevel_` and clears `selected_`, closing any drill-down the
operator has open; `FroggersModulationDrillIn::Level()` returns a cached
counter that is never re-derived, so it goes stale. Today a visible page flip
hints that something happened. Under the new behavior it is silent.

**The framework change must be general.** Any Sheaf app layering external
automation over a shared `BankSlot` hits this gap. So: additive only (a new
`MessageIn` type and a new `ParameterManager` method alongside the existing
slot-addressed path), no frogg3rs in the name or rationale, its own tests in
Sheaf's own suite. Adding an enumerator to `MessageIn::Type` means auditing
exhaustive switches over it in both repositories.

Also required: `RandomizePage`/`ResetPage` must target the operator's bank,
not the last automated one; and the existing test asserting the old behavior
is rewritten to the new contract, not deleted.

**Open decision:** whether the visible bank persists in plugin session state.
The predecessor added a `sessionExtras` key to the plugin's saved blob for the
Freeze latch; under the new semantics the visible page is purely operator-set
state, which is the argument that made Freeze worth persisting.

## B — Plugin audio input bus

The plugin constructs with `BusesProperties().withOutput(...)` and no input.
The core now requests one input channel and derives the external sources'
connected state from the host's routed signal. In a DAW the routing question
answers itself: a bus the user connected is explicit routing.

Trace before building: the JUCE layout API for an optional instrument input
bus, what `processBlock` does with input buffers today, and how to derive
connected from bus-enabled-with-nonzero-channels through the existing
`SetExternalAudioConnected` writer — once per layout change, never per block.

## C — Envelope times map exponentially

`VoiceEnvelope`'s `mapAttack`, `mapDecay` and `mapRelease` are linear over
`[kMinTimeSeconds, kMax*Seconds]`; `kMinTimeSeconds` is 0.0005,
`kMaxAttackSeconds` 0.5, `kMaxDecaySeconds` 1.0, `kMaxReleaseSeconds` 2.5.
Every other time or frequency control in the instrument uses
`dsp::ExpMapCompute`.

Move the three to `ExpMapCompute` and raise their floors, because a geometric
median taken from 0.5 ms is too clicky:

| control | floor now | floor proposed | median draw now → proposed |
|---|---|---|---|
| Attack | 0.5 ms | 1 ms | 250 ms → ~22 ms |
| Decay | 0.5 ms | 5 ms | 500 ms → ~71 ms |
| Release | 0.5 ms | 5 ms | 1.25 s → ~112 ms |

**`mapGrace` stays linear — deliberate.** It maps from zero, and zero is a
real setting: no minimum hold. Exponential cannot reach zero. This is the
same split that let the phase-modulation rate take a floor precisely because
phase-modulation depth owns the off position.

**Sustain.** `mapSustain` is linear over `[kMinSustainLevel, 1.0]` with
`kMinSustainLevel = 0.10`. Straight exponential over the same range makes
randomization quieter, not louder — the mean falls from 0.550 to 0.391.
Exponential over `[0.25, 1.0]` gives an even 3 dB per quarter turn AND a
random mean of 0.541, statistically unchanged, while raising the worst-case
random sustain from −20 dB to −12 dB. Existing patches move by under 1 dB
except at the very bottom.

Deliberate divergence from the frozen `src/core` reference, which is linear.
The parity tests live in `app/` and reference these constants symbolically.

## D — Control bounds

The governing reasoning is already recorded in the code, on the resonant
peak's own ceiling: the knob is a modulation target, so a randomized depth
visits its bounds regularly rather than only when the operator dials there.
That comment also names the counterweight — past a point, pulling a bound in
makes the effect inaudible rather than tamer, and the next lever is the comb
feedback that feeds it, not the ceiling.

**Bounds producing quiet draws:** Sustain's floor (see C); Attack's ceiling,
where the top tenth of draws still exceed 269 ms even after C — halving it to
250 ms moves that to 144 ms. Comb drive floors at 0.25 but its median is
unity and it is symmetric in log space; probably leave it.

**Bounds producing inert draws** — all floor at 20 Hz, where the effect sits
below audibility and randomization visibly does nothing: Peak freq (Filter
slot 1, drives a `ResonantBump` — traced, it is a peak, not a cutoff, so a low
draw idles rather than mutes), Scoop freq (Filter slot 10, an independent
knob), and Comb delay (Filter slot 4). Raising these to about 100 Hz moves
the bottom decile from 40 Hz to 170 Hz.

**Bounds that must not move — the zero is meaningful:** Peak gain (floor 1.0,
no boost), Fold, Scoop depth, phase-modulation depth, ring-mod depth, Grace.

Every number here is derived, not heard. The floors and the attack ceiling
are what operator smoke is for.

## E — Comment sweep, remaining scope

Carried from the predecessor, which swept the live `app/` tree under a
narrowed token set and excluded `app/vst/` because another task held it.
Remaining: `app/vst/` and any Sheaf files this change touches.

The narrowed set also leaves classes it cannot match, recorded rather than
forgotten: bare dates, packet codenames, `task F.#` and `design E#` forms,
and — in `FroggersAudioRoutingTests.cpp` — bare letter-number labels
deliberately kept because that file uses them as running cross-reference
shorthand, including inside runtime diagnostic strings. Renaming that scheme
is its own decision.

Method that worked and should be reused: pre-compute the hit list per file so
agents skip discovery; classify every hit before changing any; report found
versus changed per file; and prove behavior-neutrality by stripping comments
from both versions and diffing.

## F — Documentation

Three documents exist where two should. `MANUAL.md` is already the current
app's manual. `DAISY_MANUAL.md` covers the frozen Daisy Field firmware.
`SIM_MANUAL.md` documents the frozen web and desktop simulators and is the
redundant one; the branch merge makes the current app the only app, and the
only history worth keeping is the Daisy Field hardware, which has its own
manual and stays untouched.

`MANUAL.md` must cover exactly: what frogg3rs is and what it runs in
(standalone, browser build, VST3 and AU); every global control **including
audio and MIDI configuration**, which is documented nowhere today and must be
traced against what each host actually exposes; and every bank, parameter by
parameter. `QUICK_DICT.md` is the terse counterpart, and its claim that the
external-audio sources are permanently unavailable is now false.

**Voice.** These are functional documents for someone holding the
instrument. Say what a control does, give its range, units and default, and
stop. Do not define by negation, do not compare to versions the reader never
saw, and do not sell the product back to them. A comparison to the Daisy Field
firmware IS legitimate — it exists, it has a manual in this repository, and a
reader arriving from it needs to know which features are absent here.

## Gates

`cd app && nice make -j2 test`; plugin builds VST3 + AU; browser build and
e2e green; Sheaf's own suite for the framework change. Never above `-j2`,
always `nice`. Baseline at this change's open: 290/290.

## G — Known defect, NOT in this change's scope

Found while tracing a dangling comment citation during the predecessor's
close-out. Recorded here because this is the live change someone will read
next, and because the fix belongs to work that has not been opened yet.

**`sim/Fuegoize.hpp` invokes undefined behavior at full fuego.** It computes
the modulo divisor as `static_cast<uint8_t>((mask + 1u) ? (mask + 1u) : 1u)`.
The mask is `(1u << round(fuegKnob * 8)) - 1`, so a fuego knob at maximum
gives 255; `mask + 1u` is then 256, and the `uint8_t` cast wraps it to zero.
The next expression divides by it.

Verified unaffected:

- **The Daisy firmware.** `src/core/Parameter.hpp`'s inline fuegoize casts
  the RESULT rather than the divisor, so `mask + 1` keeps int width and the
  division is defined. There is no other fuegoize implementation anywhere
  under `src/`, and the firmware includes neither `sim/` nor anything that
  reaches it.
- **The current app.** `app/dsp/Fuegoize.hpp` uses a `uint32_t` divisor, and
  its header already documents this discrepancy — it deliberately ported the
  firmware's formula rather than the simulator's.

Reachable from the wasm build: `wasm/CMakeLists.txt` puts `sim/` on the
include path, and `bindings.cpp` reaches `Fuegoize.hpp` through
`WasmSimHost.hpp` and `DelayState.hpp`. The code path is traced; a crash has
not been observed.

**Disposition: fix it when the v2 branch merges into main**, since desktop
and wasm are updated as part of that work. Nothing in the specs prevents
editing those trees — `froggers-web-host`'s "superseded, not edited"
requirement is scoped to the change that introduced it, not standing. The
fix is to move the cast off the divisor, matching the firmware, and to add a
test that drives fuego to maximum: nothing exercises that path today, because
the app's parity tests cover `app/dsp/Fuegoize.hpp` rather than the
simulator's copy.
