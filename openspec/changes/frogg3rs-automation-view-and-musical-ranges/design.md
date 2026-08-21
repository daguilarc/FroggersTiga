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

**The primitive is close, but it addresses the wrong page.**
`Bank::HandleSetAbsolute(PhysicalEncoderId, const SceneState&, float)` is
public and never touches `BankSlot` or `selectedBank_`, and
`ParameterManager::BankAt(std::size_t)` is public. But that write resolves
through `FindVisibleCell`, which walks the bank's `visible_` cells — and
`visible_` is the modulation drill-down page whenever that bank is drilled in
(`Bank::OpenModulationView` clears `visible_` and refills it with depth
parameters plus the selected parameter on the last encoder). It is `topLevel_`
only when the bank is not drilled in, which `AddMapping`, `RegisterParameters`
and `Deselect` each restore.

**Consequence, and it is this change's to prevent.** Today the bridge's
`SelectParamBank` push calls `Deselect()` on the outgoing bank, forcing
`visible_ = topLevel_` before the write, so an automated value lands on the
top-level parameter. Remove that push (task 2.1) and a lane automating a
parameter in the bank the operator has drilled into resolves against the
drill-down page instead: the value lands on a MODULATION DEPTH cell, not the
automated parameter. The narrow form of this already exists — when the target
bank is the one the bridge last automated, no `SelectParamBank` is pushed and
the slot path resolves against the same drill-down page — but the change turns
a narrow case into every case, and it is the same defect class the change
exists to fix, inverted: the automated value's destination would depend on
what the operator is looking at.

**So the new write addresses the bank's top-level mapping, not its visible
page.** That is what an automation lane means by "this parameter": a fixed
destination that no view state can move. `Bank` exposes no top-level-addressed
write today (`ApplyModifierToTopLevel` is a bulk modifier op, not a set), so
the framework half is a small new `Bank` primitive plus the wrapper, not a
wrapper alone. Trace `Bank`'s private `topLevel_` cell lookup before writing
it; `FindVisibleCell`'s shape is the model.

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
every surface that enumerates message types in both repositories — not only
exhaustive switches. There is at least one parallel enum, `UISystemMessage`
(`projects/synth/include/synth/MidiConfigViewModel.hpp`), and a documented
arg1/arg2 encoding for system messages (`MidiConfigBlocks.hpp`). Classify each
as needing the new type or deliberately not carrying it; do not assume either.

**Two behaviors the existing path performs that the new one must decide.**
`ParameterManager::HandleSetAbsolute(slotIx, position, ...)` gates the write on
`GetCurrentModifier() == Modifier::None` and then calls
`BankSlot::RecordProcessedAbsoluteEpoch(position, absoluteEpoch)`; the
`MessageInBus::Apply` case carries a comment saying both are deliberately
downstream of the apply-or-reject decision. The bank-addressed path inherits
neither by construction. Decide each explicitly and record the reasoning: a
held modifier is operator state, and the epoch acknowledgement is per-slot
bookkeeping the plugin bridge does not currently populate (it pushes
`ParamSetAbsolute` with the default epoch 0).

Also required: `RandomizePage`/`ResetPage` must target the operator's bank,
not the last automated one; and the existing test asserting the old behavior
is rewritten to the new contract, not deleted.

**Decided (operator): the visible bank persists in plugin session state.** The
predecessor added a `sessionExtras` key to the plugin's saved blob for the
Freeze latch; under the new semantics the visible page is purely operator-set
state, which is the same argument that made Freeze worth persisting, so it
gets the same treatment — a second key in the object that already exists.
Restore runs through the operator-selection path, since that is the only
authority over the visible page this change leaves standing.

## B — Plugin audio input bus

The plugin constructs with `BusesProperties().withOutput(...)` and no input.
The core now requests one input channel and derives the external sources'
connected state from the host's routed signal. In a DAW the routing question
answers itself: a bus the user connected is explicit routing.

**Connected is consent, never channel presence.** This is already settled
doctrine in the core and the plugin does not get to re-litigate it.
`config.numAudioInputs = 1` carries a warning in its own comment: requesting
any nonzero count opens a platform-default input device at launch, before any
device selection is made — the built-in microphone, unasked. So the
external-audio sources never read channel presence as permission. They
subscribe to `AppContext::InputRouted()`, which is true only while an
OPERATOR-SELECTED input device is open, and false for a default device nobody
chose.

Deriving the plugin's connected state from "the bus is enabled with nonzero
channels" would throw that away. Bus enablement is the HOST's choice, not the
operator's: a DAW may enable an optional input bus on instantiation with
nothing routed into it, and the sources would report connected and start
taking randomization from silence. That is the phantom-input defect this
project has already paid to fix once, re-introduced in a new host.

**Decided (operator): an explicit toggle in the plugin editor — which is the
same mechanism the standalone already has, not a second one.** Device
selection and a toggle are the same contract when the default is NO DEVICE:
the input starts off, and any move off that default is the operator's
affirmative act. The standalone's selector happens to offer many devices; the
plugin's offers two states, none and the input bus, because the DAW owns the
devices. Same rule, different arity.

So: connected = the operator opted in AND the bus is actually enabled with at
least one channel. Consent and capability, both required — opting in cannot
invent a signal, and an enabled bus cannot imply consent. Both hosts feed the
one existing `InputRouted` seam; nothing new is introduced for the plugin.

The opt-in is operator-set state that a reopened project should remember, so
it persists in `sessionExtras` beside the Freeze latch and the visible bank,
using that same mechanism rather than a new one. Its default is off, which is
what makes a legacy blob with no such key restore correctly.

**Where it lives is the only real difference from the standalone.** Sheaf's
audio page, the one carrying "Input device", is part of the runtime shell
(`projects/synth/juce/RuntimePages.hpp`) and only the standalone runs it; the
plugin editor renders the portable surface and nothing else. So the control
goes on the portable surface, and it reads as the input selection it is —
off by default, the DAW's input bus the only other choice — rather than as a
new bespoke interaction. Same concept the operator already knows, in the one
surface the plugin has.

Trace before building: the JUCE layout API for an optional instrument input
bus, what `processBlock` does with input buffers today, which JUCE callback
fires on layout change, and how to reach the existing
`SetExternalAudioConnected`/`InputRouted` writer — once per change of either
input, never per block. There is no external-audio toggle anywhere in the
current app surface to copy; the frozen web sim's External button is the
nearest precedent for what it does.

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

**Bounds producing inert draws** — floored at 20 Hz, where the effect sits
below audibility and randomization visibly does nothing: Peak freq (Filter
slot 1, drives a `ResonantBump` — traced, it is a peak, not a cutoff, so a low
draw idles rather than mutes), Scoop freq (Filter slot 10, an independent
knob), and Comb delay (Filter slot 4). Raising these to about 100 Hz moves
peak's and scoop's bottom decile from 40 Hz to 170 Hz; comb's ceiling is
10 kHz rather than 20 kHz, so its own decile moves from 37 Hz to 159 Hz.

**A fourth 20 Hz floor exists and is not one of the three.** VCO pitch
(`app/dsp/Vco.hpp`, `ExpMapCompute(20/sr, 20000/sr, pitchKnob01)`) has the same
floor and the same bottom decile. It is listed here because an enumeration that
stops at three sites and reads as complete is the failure this project has
already paid for once. Pitch is NOT proposed for change: a low oscillator is a
sub-bass or a slow pulse, which is a musical setting rather than an inert one,
where a 25 Hz resonant peak is neither. Classified and kept, not missed.

**Raising the comb floor moves a second control's range.** The comb low-pass
cutoff derives its own floor from the comb frequency —
`ExpMapCompute(4.0f * combFreq, 20000/sr, knob(Filter, 6))` — so a comb floor
of 100 Hz raises that knob's floor from 80 Hz to 400 Hz whenever comb frequency
sits at minimum. That is acceptable on the same reasoning (an 80 Hz low-pass on
a comb is inaudible), but it is a range change to a control nobody asked to
change, so it is stated rather than discovered later.

**Bounds that must not move — the zero is meaningful:** Peak gain (floor 1.0,
no boost), Fold, Scoop depth, phase-modulation depth, ring-mod depth, Grace.

Every number here is derived, not heard. The floors and the attack ceiling
are what operator smoke is for.

**The requirement is universal; this list is not yet.** The spec delta says
every control's bounds sit where the control still does something. The five
bounds above are the ones traced so far, found by reading the filter and
envelope mapping blocks — not by enumerating every `ExpMapCompute` bound in the
tree. Task 5 carries that enumeration, and it reports found versus changed:
a bound deliberately kept is a result, a bound never looked at is not.

## E — Comment sweep, remaining scope

Carried from the predecessor, which swept the live `app/` tree under a
narrowed token set and excluded `app/vst/` because another task held it.
Remaining: `app/vst/` and any Sheaf files this change touches.

The narrowed set also leaves classes it cannot match, recorded rather than
forgotten: bare dates, packet codenames, `task F.#` and `design E#` forms,
and — in `FroggersAudioRoutingTests.cpp` — bare letter-number labels used as
running cross-reference shorthand, including inside runtime diagnostic
strings. These are not an open question: they are planning-doc labels in
comments, which the standing rule already retires. Keep the rationale, drop
the label, and name what the thing actually is.

Method that worked and should be reused: pre-compute the hit list per file so
agents skip discovery; classify every hit before changing any; report found
versus changed per file; and prove behavior-neutrality by stripping comments
from both versions and diffing.

## F — Documentation

Three documents exist where two should. `MANUAL.md` is already the current
app's manual. `DAISY_MANUAL.md` covers the frozen Daisy Field firmware.
`SIM_MANUAL.md` documents the frozen web and desktop simulators and is the
redundant one by content; the branch merge makes the current app the only app,
and the only history worth keeping is the Daisy Field hardware, which has its
own manual and stays untouched.

**Redundant by content is not the same as removable, and here it is not.**
`SIM_MANUAL.md` is a build and CI input, not just prose. Traced:

- **Release notes are rendered from it.** `desktop/scripts/render-release-notes.sh`
  reads it, `.github/workflows/desktop-release.yml` runs that, and `AGENTS.md`
  makes it the mandatory desktop release path.
- **A CI check fails without it.** `desktop/scripts/verify-release-metadata.sh`
  greps it for the current-release heading and fails the build when absent.
- **It is the source of four generated mirrors.** `scripts/sync-help-docs.sh`
  copies it to `docs/sim-manual.md` and `web/public/sim-manual.md`;
  `sim/check_operator_docs_sync.sh` enforces the pair; `scripts/hooks/pre-commit`
  re-syncs and re-stages them on every commit.
- **Two CMake targets embed it as a resource.** `desktop/CMakeLists.txt` and
  `desktop-v2/CMakeLists.txt` — trees this change declares out of scope, so
  deleting the file either breaks them or breaks the scope boundary.
- **The public site links to it.** `web/index.html` points at its GitHub blob.
- **Three LIVE specs require it by name.** `sim-operator-doc-parity` is built
  entirely on it and its mirrors; `froggers-host-master` names it as the
  operator-doc mirror and in its baseline index; `global-strip-marbles-label`
  requires a labelled row inside it. None of the three has a delta in this
  change.

So the deletion is not a documentation edit, and it lands at the cutover
(group 10) where those consumers are already being opened. What this change
does unconditionally is the rest of §F: `MANUAL.md`'s coverage, the audio and
MIDI configuration section, and `QUICK_DICT.md`'s false external-audio claim.

**The parity capability retires with it — it protects nothing afterward.**
`sim-operator-doc-parity` exists to keep one manual and four generated mirrors
byte-identical. The current app does not read a mirror: its site links
`MANUAL.md` on GitHub directly and the browser build copies no markdown at
all. `pages.yml` already publishes `app/browser/dist/site` and its own comment
says the pipeline no longer builds the legacy web display authority — so the
sync check that workflow still runs is already guarding a site it stopped
publishing. The three consumers that remain are the v1 help modal, the built
v1 Pages site, and the frozen desktop app's embedded Help, all of which go at
the cutover. So the whole apparatus goes: the sync script, the parity check,
the pre-commit re-sync, the four mirror files, and the two CMake embeds.
`QUICK_DICT.md` stays — it is the terse counterpart to `MANUAL.md`, and only
its copies were ever the redundancy.

**Retiring the mirrors is not the same as taking the docs away, and the app
must gain what the frozen desktop already had.** The frozen desktop bundles
the root `SIM_MANUAL.md`, `QUICK_DICT.md` and `LICENSE` as resource sources in
its own CMake — one source of truth, the copy existing only inside the built
bundle. The current app does not: `app/Resources/` holds two icons, and the
browser site reaches the manual by linking to GitHub. That is acceptable for a
browser, which is already on the network, and wrong for a plugin loaded in a
DAW on a machine that is not. So the standalone and the plugin embed
`MANUAL.md` and `QUICK_DICT.md` the same way the frozen desktop did.

This is why the parity capability can retire without leaving the operator
worse off. The redundancy it policed was four CHECKED-IN duplicates that had
to be re-synced by a hook and verified by a CI check. A build-time embed has
no such duplicate: edit the one document and the next build carries it. The
requirement moves from "keep six files identical" to "there is one file."

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

## H — Repo hygiene, step zero

Hygiene opens every change now (omni-rule §13.0), and what the sweep finds is
fixed inside the change that found it. This tree had accumulated enough to make
the point on its own.

**Gates nothing invokes.** Twenty-nine gate scripts exist under `app/`,
`scripts/`, `sim/` and `desktop/scripts/`, totalling about 1,715 lines. About
1,017 of those lines are in scripts NO workflow, Makefile, package.json or other
script calls. The largest single cluster — `check_subagent_packet_gates.sh` and
the three `check_desktop_v2_*` gates it is the only caller of, 325 lines
together — guards desktop-v2, which is frozen. Four `check_vcv_*` gates, 235
lines, guard a frozen VCV tree the same way.

**Orphan status is established by searching for the INVOCATION, not the file.**
A first pass here matched call sites by path prefix and concluded
`check-renamed-origin.sh` was dead. It is not: it runs from
`app/browser/Makefile`, `app/browser/local-smoke.sh` and
`.github/workflows/pages.yml`, and only a bare-name search finds all three.
Deleting on that first answer would have removed a live CI gate. Search by bare
name AND by path, every time — this is §8's operand rule wearing different
clothes.

**Load-bearing hygiene is sequenced, not skipped.** `SIM_MANUAL.md` is the clear
case: redundant as prose, still feeding release-note rendering, a release
metadata check, four generated mirrors, two CMake resource embeds and the
in-app and website Manual viewers. It cannot be deleted where it is redundant;
it is deleted where it stops being load-bearing, which is the cutover. Same for
the desktop release product name, the frozen trees, and the `froggerstiga-v1`
tag. That is what group 10 is: not a separate change, the same principle applied
at the point in the plan where it becomes safe.

**Also swept:** five tracked correspondence artifacts at the repository root
(four upstream email drafts and a patch file), and — machine-local and
untracked, but worth clearing while here — a 178 MB Node distribution with its
tarball, a 22 MB Rack SDK, and a 1.6 GB emsdk whose cache still records the
pre-rename absolute path.

## Gates

`cd app && nice make -j2 test`; plugin builds VST3 + AU; browser build and
e2e green;

**That first command does NOT cover `app/vst/`.** `app/Makefile`'s `test`
target lists ten binaries and not one of them is a VST target, so the plugin
tests — where this change's automation, input-bus and editor coverage all
live — are never executed by it. Treating the plugin as a BUILD check only
would let the whole change read green while every new plugin test sat unrun.
The plugin's own CMake test targets are a separate, required gate:
`FroggersVstHostTests`, `FroggersVstSmokeTest`, `FroggersVstEditorTest`, run
from `app/vst/build`. Baseline for the host tests at this group's close:
27/27.
 Sheaf's own suite for the framework change. Never above `-j2`,
always `nice`. Baseline at this change's open: 290/290 for the app suite,
verified from the predecessor's own close-out record.

**Sheaf's suite has no recorded baseline here, and it is not all-green on this
machine.** `braid4_meets_96000hz_256_frame_deadline_and_continuity` and
`braid4_sparse_modulation_meets_96000hz_256_frame_deadline`
(`projects/synth/tests/braid4_deadline_tests.cpp`) fail deterministically on
this hardware and are not this change's to fix. Record the Sheaf baseline
BEFORE the first Sheaf edit, so the framework work is judged against a known
starting count rather than against an assumed green — a gate with no baseline
either stalls on a pre-existing red or launders one.

## G — For the post-testing merge into main

Once this change is tested and accepted, the v2 branch merges into main and
both the desktop and wasm trees are updated as part of that work. That merge
is now group 10 of this change's own task list rather than a future plan:
it is gated on the operator accepting groups 1–9, and it carries the hygiene
that is load-bearing until the merge opens those trees — the desktop release
product name, the `SIM_MANUAL.md` retirement, the frozen trees, and the item
below.

Found while tracing a dangling comment citation during the predecessor's
close-out, and recorded here because this is the live change someone will
read next.

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

**In the merge plan:** move the cast off the divisor so it matches the
firmware's form, and add a test that drives fuego to maximum. Nothing
exercises that path today — the app's parity tests cover
`app/dsp/Fuegoize.hpp`, not the simulator's copy — so the fix needs its own
coverage rather than relying on an existing test to catch a regression.

Nothing in the specs stands in the way: `froggers-web-host`'s "superseded,
not edited" requirement is scoped to the change that introduced it, not
standing, so the merge is free to update these trees.
