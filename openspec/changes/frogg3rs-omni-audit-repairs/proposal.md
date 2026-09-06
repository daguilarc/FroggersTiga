# Proposal — `frogg3rs-omni-audit-repairs`

**Created 2026-09-05. Preflight revision 2026-09-06 (inline, this session).
Execution approved by the operator 2026-09-06 with this revision.**

Paths are repo-root relative. Line numbers are 2026-09-06 working-tree reads at
`08b5fd3` plus the uncommitted edits listed under Overlaps. The source of every
numbered finding is the fact-checked audit,
`frogg3rs-omni-rule-audit-2026-09-05.md` (scratchpad of session
`77b2bd60`), whose numbering this proposal keeps. Finding 5 of that audit was
false and is not here. Findings the preflight added are numbered P1–P21 in the
"Preflight 2026-09-06" section and folded into the design and tasks.

## What the operator asked for

Repair the confirmed findings of the omni-rule audit of this repository.

## Impact

Directories this change edits, each swept for hygiene in the preflight
(§8.0); what the sweep found is in the Preflight section and in the tasks.

- repo root: `Makefile`, `.gitignore`, `.gitmodules`, `README.md`,
  `DAISY_MANUAL.md`, `publish/` (deleted), `External/theallelectricsmartgrid`
  (deleted). Sweep found: stale ignore lines for six deleted trees, one task
  label in `.gitignore`.
- `.github/workflows/`: one new workflow. Sweep: clean.
- `app/` (headers, tests, `Makefile`, `check_no_frozen_includes.sh`): sweep
  found "frozen" wording and a script/target name describing `src/` as
  frozen; six `RequiredHeight()` mentions naming a deleted function.
- `app/dsp/`: sweep found the same "frozen" wording in port comments.
- `app/vst/`: two comments citing the transport branches by line and by
  quoted code. Sweep: otherwise clean.
- `app/browser/`: one comment describing `publish/`. Sweep: otherwise clean.
- `src/core/`: sweep found the deleted simulator's host hooks with zero
  setters (P3), two zero-caller methods (P12).
- `src/common/`: sixteen forwarding shims, one of them (`Include.hpp`) with a
  single consumer that uses nothing it forwards (P6).
- `src/FroggersSolo/`, `src/FroggersGuitar/`: not edited. `src/TestControl/`:
  deleted (P22).
- `test/firmware/`: sweep found a restated struct with a promised assertion
  that does not exist (4c); test lines setting deleted members (P3).
- `openspec/changes/`, `openspec/specs/`: the archive of finding 14, the spec
  deltas, and one validate failure in a sibling change's delta (P14).
- `External/Sheaf`: read only. Nothing in it is edited.

## Findings and what the tree does now

### Tests that prove nothing

**2 — nothing invokes the firmware tests.** `test/firmware/CMakeLists.txt:33-37`
registers five ctest binaries over `src/core` only (`:7`, `:11`). No Makefile,
CMake file or workflow references `test/firmware`; `.github/workflows/` has
three files and none mentions the directory. The binaries in
`test/firmware/build/` date from a manual run on 2026-08-28 (`CMakeCache.txt`:
Unix Makefiles, `/usr/bin/c++`, Release).

**4a — `stereo_delay_clear_buffers_resets_to_silence`**
(`app/FroggersDspParityTests.cpp:4338-4352`) feeds 100 samples of a sine,
calls `ClearBuffers()`, processes one zero sample and asserts only
`std::isfinite` on the wet pair. A `ClearBuffers()` that does nothing passes.
Preflight (P7): at `dtim = 0.4` the base delay is
`ExpMapCompute(0.001, 2.0, 0.4)` (`app/dsp/Delay.hpp:863`) = 0.0209 s, about
1003 samples at 48 kHz, so after 100 samples the read head has not reached the
signal and the wet pair is still zero. A positive control on the pre-clear wet
pair needs at least that many samples; the repair feeds 4800.

**4b — `envelope_followers_track_abs_value_with_attack_release_asymmetry`**
(`:1084-1097`) computes `expectedFall` at `:1094`, discards it at `:1095`, and
asserts at `:1096` only that `out[0] < ef.attackCoeff`. The follower's update
is `level += (target - level) * coeff` with `coeff = releaseCoeff` when the
target is below the level (`app/dsp/EnvelopeFollowers.hpp:55-56`), so after
the attack step to `attackCoeff` and one release step toward 0 the exact value
is `attackCoeff * (1 - releaseCoeff)`. Any `releaseCoeff` in (0, 1] passes the
current assertion.

**1 — `src/core` paths no test executes.** `test/firmware/` never names
`VcoAdsrState`, `VcoWaveMorph`, `VcoWaveEval`, `BiquadSection`, `Comb`,
`OPLowPassFilter`, `PolynomialDrive`, `RGen`, `SDDSine`, `SampleRateReducer`,
`TanhSaturator` or `Marbles`; its only `FUEG` mention is a comment
(`VariantMix_test.cpp:78`). `Parameter.hpp:129-151` is the fuegoization
scramble; at knob 0 the mask is 0 and the scramble is the identity. The app's
parity suite re-derives that formula for the `app/dsp` port
(`FroggersDspParityTests.cpp:1251-1258`) and never includes the firmware
header, so the firmware `Parameter` has never been executed at a non-zero mask
by any test.

Preflight (P3) traced the gates of the first three: `VcoAdsrState` runs only
when `m_vcoAdsr && m_adsrParams` (`FroggersEngine.hpp:786`), set only by
`SetVcoAdsrState` (`:255`), which nothing in `src/` or `test/` calls;
`VcoWaveMorph`/`EvalWaveMorph` run only when `m_simWaveMorph` (`:111`), set
only by `SetSimWaveMorph` (`:233`), called by nothing; the independent PM LFO
runs only when `m_simIndependentPm` (`:130`), set only by
`SetSimIndependentPm` (`:272`), called by nothing. These are the deleted
simulator's host hooks. The repair for a path nothing can reach is deletion
(§8.0), not a test; see P3. The nine DSP modules are live: `Comb` (`:98`),
`FrogBlock` of `PolynomialDrive` (`:101`), `Marbles` (`:103`),
`OPLowPassFilter` (`:73`, `:105`), `TanhSaturator` (`:107`), `RGen` (`:315`),
`SDDSine` (`:193`), `SampleRateReducer` inside `PolynomialDrive.hpp:169-170`,
`BiquadSection` inside `ResonantBump.hpp:9`.

### Stale references

**3 — citations into deleted files.** `FroggersDspParityTests.cpp:4226`,
`:4258`, `:4464` cite `StereoDelay.hpp:60-64`, `:66`, `:120`; `:1065` and
`:4198` name `V2EnvelopeFollowerBank` and `StereoDelay` as "the retired
simulator's". Both files were deleted by `f2369151` (2026-08-22) and resolve
only as `f2369151^:sim/StereoDelay.hpp` and
`f2369151^:sim/V2EnvelopeFollowerBank.hpp` (verified 2026-09-06: `:58-64` is
the `dsnd <= 0.0001f` early return, `:66` the `ExpParam::Compute` base
seconds, `:118-122` the fractional read).

**13 — stale citations in the app headers.**
- `app/FroggersAppCore.hpp:225` cites `FroggersPageLayout::RequiredHeight()`;
  no such function exists. Preflight (P9): the name appears six times, all as
  history of its own removal: `FroggersAppCore.hpp:225`,
  `FroggersUiSurface.hpp:270`, `:281`, `:339`, `FroggersSurfaceTests.cpp:465`,
  `:515`. The constant `Config().uiHeight` is kept in step with today is
  `FroggersPageLayout::kDefaultHeight` (`FroggersUiSurface.hpp:299`), by hand
  and on purpose: the layout resolves against whatever root extent it is
  given (`RootBounds`), so the two numbers need not agree for the app to work,
  and no check is owed.
- `app/FroggersUiSurface.hpp:188-194` and `:1489-1495` say the two hand-rolled
  `Label` nodes will "collapse into captions" when upstream caption placement
  lands. It landed (`External/Sheaf` `f321147b`, 2026-08-18).
  `Builder::FinishControl` (`PortableUIBuilders.hpp:442-484`) always wraps
  caption and control in one `Row` (`:456`) and `CaptionPlacement::After`
  (`:475`) only orders the label after the control in that row; it never
  stacks, and these labels sit below their sliders. Both comments also cite
  `PortableUIBuilders.hpp:428-465`, which is off by 14–19 lines.
- Sheaf line citations that drifted: `FroggersAppCore.hpp:807` →
  `ParameterModulation.hpp:796` (`ComputeAllParameters` is at `:809`); `:859`
  → `AppContext.hpp:173-201` (no longer `AudioBlock`); `:320` →
  `Engine.hpp:280-301` (the guard is at `:299`). `:408` and `:1233` are exact.
  The full set has not been enumerated; task 3.4 does that.
- `DAISY_MANUAL.md` at HEAD names `src/FroggersTiga/` six times; the
  uncommitted diff of `frogg3rs-guitar-and-solo-variants` already replaces every
  one (`grep -c FroggersTiga DAISY_MANUAL.md` is 0 on 2026-09-06).
- Preflight (P17): `app/vst/FroggersPluginProcessor.cpp:753-758` cites the
  Play/Stop branches as `FroggersUiSurface.hpp:1826-1876` (they are at
  `:2160-2218`) and `:1197-1199` quotes the Freeze branch's code, which task
  4.1 rewrites.
- Preflight (P18): `app/` carries citations into `src/core` files this change
  edits or deletes: `FroggersEngine.hpp:<n>` (AppCore 7, parity tests 12 by
  the variants change's own count at its `tasks.md:221`, `app/dsp/Vco.hpp`
  and others), `Marbles.hpp:<n>` (`app/dsp/RandomShLane.hpp`, parity tests),
  `VcoWaveEval.hpp:7-23` (`FroggersDspParityTests.cpp:80`, `:224`,
  `FroggersModulation.hpp:1347`), `src/core/VcoAdsrState.hpp`
  (`app/dsp/VoiceEnvelope.hpp:7`, `FroggersDspParityTests.cpp:233`). Every
  one is re-resolved after group 5, and citations into deleted files become
  git-object citations at `08b5fd3` (the last `main` commit carrying them,
  and none of the three is touched by the variants diff).

**15 — `desktop-v2` was deleted at `b9a8199` (93 paths) and is still described
as alive.** `openspec/specs/mod-rack-dual-midi-jacks/spec.md:34-43` carries the
requirement `v2-excludes-dual-midi-jack-mod-rack` ("Desktop v2 SHALL NOT
render…") with two scenarios. `.gitignore:6` ignores `desktop-v2/build/`;
`:22` cites an archived change's tasks file. `src/core/FroggersEngine.hpp:122`
and `src/core/Marbles.hpp:50-56` describe desktop-v2 as a host the flag or
method serves. Six `app/` comments name it as a forbidden include path
(`app/Makefile:139`, `app/dsp/DspMath.hpp:7`, `app/dsp/VoiceEnvelope.hpp:8`,
`app/FroggersDspParityTests.cpp:6`, `app/FroggersSurfaceTests.cpp:1398`,
`app/FroggersParameters.hpp:102`); `app/check_no_frozen_includes.sh` is the
check behind the first, and its one pattern (`:14`) greps `src/` only.
Preflight (P11): `.gitignore:2-8` also ignore `wasm/`, `desktop/`, `web/`
build outputs; none of those trees exists (`ls` 2026-09-06). Preflight (P12):
`Marbles::ResetPageToDefaults()` (`:57-71`) and
`PageManager::SanitizeSimModAssignments()` (`Page.hpp:262-274`) have zero
callers in `src/`, `test/`, `app/`; both served desktop-v2.

### Structure

**6 — the freeze-latch / transport sequence is written three times.**
`app/FroggersUiSurface.hpp:2179-2187` (Play), `:2214-2216` (Stop),
`:2248-2252` (Freeze engaging) each write `SetFreezeLatched(x)`, then
`PushMessage(Start|Stop)`, then `SetDesiredTransportRunning(y)`. The
latch-before-message order is a happens-before fix for a data race, explained
at `:2177-2178` and `:2212-2213`. Nothing enforces it. No fourth site exists:
`SetDesiredTransportRunning(` and `SetFreezeLatched(` are called from
`FroggersUiSurface.hpp` only (`app/vst/` mentions them in comments).

**7 — `app/FroggersAppCore.hpp`.**
- `:1027-1028` and `:1079-1080` are the same two statements
  (`ForEachStatefulUnit(... Reset ...)`; `delayReverbClearPending_ = false;`),
  inside the function whose own `runStopTeardown` lambda (`:966`) exists "so
  the two can never drift apart" (`:960-965`).
- `:2359`, `:2367`, `:2378` are three booleans (`wasTransportRunning_`,
  `wasFreezeLatched_`, `delayReverbClearPending_`) that the audit read as one
  state machine. Preflight (P2) read every site: the first two are
  previous-sample copies of two independent inputs, written together at
  `:1083-1084`, so all four combinations are reachable; the third is armed at
  the stop edge (`:1034`), cleared on resume (`:1060`) or on idle (`:1028`,
  `:1080`), independently of the other two; and `wasTransportRunning_` is
  read as a value, not an edge, at `:1659`. Eight reachable states of three
  concepts, not four of one. Not a defect; no enum.
- `RouteAudioSample()` spans `:1506-1980` and its own section comments name
  discrete stages that were never extracted: Audio bank (`:1511`), Envelope
  bank (`:1542`), Drive bank (`:1691`, with slots 9, 11–13 at `:1708`), Filter
  bank (`:1717`), Delay bank (`:1878`, with slots 9–13 at `:1921`), Reverb
  bank (`:1932`). The `knob` lambda (`:1507`), the `stoppedKnob` and
  `releaseKnob` lambdas (`:1658-1663`) and four constants (`:1642-1657`) are
  read by more than one section.
- The Audio-bank slot grouping (slot `i` pitch, `i+3` shape, `i+6` phase-mod,
  `i+9` ring-mod) is hand-written at `:1093-1099` and `:1533-1540`; the second
  site's comment (`:1529-1530`) names the grouping the first site already uses.
  No third site: the `+ 3`/`+ 6`/`+ 9` arithmetic on `FroggersBankId::Audio`
  appears nowhere else in `app/`.
- `:669-671` guards `drillIn_.has_value()` in `ProcessFrame()`. `:1297-1299`
  states the same guard elsewhere is "defensive rather than a reachable
  fallback" because `drillIn_` is emplaced in `Init()` (`:304`) and never
  reset. Every `ProcessFrame()` call in the tests follows an `Init()`
  (`FroggersHeadlessTests.cpp:228→241`, `:269→270`); the Sheaf host calls
  `Init` before any frame.

**10 — `src/common/` is a forwarding layer nothing needs.** Sixteen two-line
shims plus five real files (`Include.hpp`, `App.hpp`, `FieldMutationQueue.hpp`,
`FieldSwitchGuard.hpp`, `DaisyIO.hpp`). Only three files include through
`common/`: `src/FroggersSolo/FroggersSolo.cpp:1`,
`src/FroggersGuitar/FroggersGuitar.cpp:1` (both `App.hpp` only) and
`src/TestControl/TestControl.cpp:1-2` (`Include.hpp`, `App.hpp`).
`Include.hpp:3-14` forwards to nine shims plus `DaisyIO.hpp` and `App.hpp`;
`Comb`, `Marbles`, `ModMgr`, `PolynomialDrive`, `ResonantBump` are included by
nothing (every `"ModMgr.hpp"` include resolves inside `src/core`). The engines
include `../core/FroggersEngine.hpp` directly (`FroggersSolo.hpp:3`,
`FroggersGuitar.hpp:3`), and `test/firmware/CMakeLists.txt:7-11` builds with
only `src/core` on the include path. Preflight (P6): `DaisyIO.hpp:5-6` and
`FieldMutationQueue.hpp:3` include `"Page.hpp"` and `"SchmidtTrigger.hpp"` by
bare name, which today resolve to the shims beside them; `src/mk/daisy.mk:38-39`
puts both `src/core` and `src/common` on the include path, so after the shims
go the bare names would resolve through `-I` instead. They are made explicit
(`../core/`) so the resolution is read off the line, not the flags.
`TestControl.cpp` uses none of the DSP modules `Include.hpp` forwards (grep by
operand 2026-09-06: no hit), so `Include.hpp` is probably dead; the build
decides (task 5.2).

### Dead dependency and stale output

**11 — `External/theallelectricsmartgrid`** is declared in `.gitmodules:1-3`,
uninitialized (`git submodule status` shows `-cb26e84`), an empty directory,
and referenced by nothing in `src/`, `app/`, `test/`, any Makefile,
CMakeLists, workflow, or `External/Sheaf/projects/synth`. `e0ae431`
(2026-03-17) replaced its DSP with local implementations; `efb0f8b` cites it
only as naming precedent.

**9 — root `publish/` is stale build output.** Four files:
`froggers-apps.json`, `catalogs/daguilarc/catalog.json`, and one `.js` and
`.wasm` under `packages/frogg3rs/6ad5f16…/`. The hashes verify; the catalog
declares `abiVersion: 2`, and `app/browser/package-catalog.mjs:66-67` says the
current protocol is 4. `pages.yml:101-103` uploads `app/browser/dist/site`
(gitignored, `.gitignore:9`), which `package-catalog.mjs` regenerates on every
deploy. Nothing in the repo or in `External/Sheaf` reads the root `publish/`
paths (P4: the only Sheaf catalog-source list,
`projects/synth/browser/catalog-sources.json`, names `catalogs/sheaf/catalog.json`
relative to its own deployment; `daguilarc` appears in Sheaf only in the prose
of `openspec/changes/fix-out-of-tree-app-gaps/`); the tree arrived in
`b2e0a54` (2026-08-07), a commit about archiving an openspec change.

### Comment labels

**12 — planning labels in comments.** The audit counted 55 across
`app/FroggersAppCore.hpp`, `app/FroggersDspParityTests.cpp`,
`src/core/FroggersEngine.hpp`, `src/core/VcoAdsrState.hpp`,
`src/core/Marbles.hpp`, `app/FroggersSurfaceTests.cpp`, and five comments
that cite the omni rule by name (`FroggersDspParityTests.cpp:3506`, `:5124`,
`:5525`; `FroggersSurfaceTests.cpp:3085`, `:3305`; verified 2026-09-06). The
55 is a lower bound: the parity file's `D1`–`D10` labels (`:4681-4920`), eight
label lines in `FroggersEngine.hpp`, and `.gitignore:13` ("Task 4.3") were
not all counted. The standing rule: a comment explains behaviour; the
rationale after the label stays, the label goes.

### Documentation and backlog

**8 — `src/` is documented as frozen and is not.** `README.md:64-67` and
`:72`, `DAISY_MANUAL.md:15` ("Frozen — not under active development"), the
last surviving inside the uncommitted diff that adds the Solo/Guitar table
three lines above it. The `frogg3rs-guitar-and-solo-variants` proposal
(`proposal.md:12`) unfreezes the tree. Preflight (P10): the word is the
operand of a family. `app/` carries about ninety comment hits describing
`src/` as "the frozen firmware", "the FROZEN source", "the frozen tree"
(`app/FroggersDspParityTests.cpp` 18, `app/FroggersAudioRoutingTests.cpp` 18,
`app/dsp/Delay.hpp` 9, `Drive.hpp` 7, `Reverb.hpp` 7, `FilterFx.hpp` 6,
`VoiceEnvelope.hpp` 6, `app/Makefile` 6, `FroggersAppCore.hpp` 4, others 1–2),
and the check itself is named for the premise: `app/check_no_frozen_includes.sh`
and the `check-no-frozen-deps` target (`app/Makefile:147`, `:149`, `:175-176`,
`:265`; named in `app/dsp/DspMath.hpp:8` and `FroggersDspParityTests.cpp:4`).
The rule the check enforces — `app/` never includes from `src/`, the port is
a copy — is unchanged and now matters more, since the copied source can move
again; only the name and the wording change. `DAISY_MANUAL.md:262` ("frozen
random level") and the Delay/Reverb "freeze" controls are a different word
and stay.

**4c — an assertion that does not exist.** `test/firmware/RefreshGate_test.cpp:13-15`
restates the `RefreshGate` struct from `src/common/DaisyIO.hpp:15-32` and says
the copy is "kept honest by an assertion that the two bodies agree". No such
assertion exists; the only `assert` match in the file is that comment. The
copy exists because `DaisyIO.hpp` pulls in the Daisy SDK and the struct does
not need it.

**14 — `openspec/changes/frogg3rs-controllers-page-user-story/`** is
superseded (`frogg3rs-controllers-editor-add-and-columns/proposal.md:241`:
"superseded and dead pending archive"), its `proposal.md:313` records tasks
2.1–2.4 executed while `tasks.md:56-75` leaves them unchecked, and the
directory is untracked. Operator note 2026-09-06: archiving it is a hygiene
step of this change.

## Preflight 2026-09-06

Inline, per the tasks' group 1. Each item names what it settled and where the
plan changed.

- **P1** (1.4) `FinishControl` is `PortableUIBuilders.hpp:442-484`, the `Row`
  at `:456`, `CaptionPlacement::After` at `:475`. It never stacks caption and
  control. Task 3.3 stands; its citation is corrected.
- **P2** (1.5) The three booleans are not one state machine (finding 7 above).
  Task 4.5 dropped; the audit's second bullet under finding 7 is recorded as
  not a defect.
- **P3** (1.6, §8.0) The simulator host hooks in `src/core/FroggersEngine.hpp`
  have no setter caller anywhere in `src/` or `test/`: `m_simWaveMorph`
  (`:111`, setter `:233`), `m_simDedicatedPm3Knob` (`:112`, `:238`),
  `m_simFxInsert`/`m_simFxInsertCtx` (`:115-116`, `:243`; the only call is
  `HookIdentity_test.cpp:15` passing `nullptr`), `m_vcoAdsr`/`m_adsrParams`
  (`:118-119`, `:255`; the VariantMix tests set them to `nullptr`),
  `m_simIndependentPm` (`:130`, `:272`). With every flag at its default the
  reachable code is the legacy branch of each ternary and `if`. Everything the
  flags gate is deleted (task 5.6), with the three headers only that code
  included (`VcoAdsrState.hpp`, `VcoWaveMorph.hpp`, `VcoWaveEval.hpp`),
  `m_pm3` (read only under the `m_simDedicatedPm3Knob` gate, `:454`, `:730`),
  and the two zero-caller methods of P12. Tasks 9.2, 9.3, 9.4 are dropped:
  their subjects no longer exist. The gate is a printed checksum of the
  engine's output over a fixed input, captured before and after the deletion
  (task 5.6), plus `firmware-test` and the three firmware builds.
  `IsSimAssignableModIndex`/`IsPermanentModSourceIndex` (`SimModSource.hpp`)
  are live (`Page.hpp:308-309`, `AudioPairArState.hpp:75`, `:140`) and stay.
  Corrected at execution (P24): the output-FX insert hook (`SimFxInsertFn`,
  `m_simFxInsert`, `SetSimFxInsert`, the call in `ApplyOutputFx`) is a live
  test seam — `VariantMix_solo.cpp:29-42` installs a counting lambda through
  it and `VariantMix_test.cpp:114-115` asserts on the count — and is kept.
  The preflight grep for it was case-sensitive (`SimFx`) and missed the
  member spelling (`m_simFxInsert`); the executor's stop caught it.
- **P4** (1.7) No Sheaf source names this repository's `publish/` path. Task
  5.4 deletes.
- **P5** (1.8) `check_no_frozen_includes.sh` greps one pattern, `src/` (`:14`).
  `desktop-v2` occurs only in comments; task 7.3 edits comments and renames.
- **P6** (5.2) See finding 10: two more inbound includes, and `Include.hpp` is
  probably dead.
- **P7** Findings 4a and 4b had no task. Tasks 3.5 and 3.6 added; 4a's
  positive control needs 4800 samples (finding 4a above).
- **P8** The proposal had no Impact section. Added above, with the sweep
  result per directory.
- **P9** `RequiredHeight`: FOUND 6, the plan changed 1. Task 3.2 now edits all
  six.
- **P10** "frozen": FOUND about 100 across `README.md`, `DAISY_MANUAL.md`,
  `app/`; the plan changed 2. Task 8.1 now classifies and edits every hit that
  describes `src/`, and task 7.3 renames the script and target.
- **P11** `.gitignore:2-8` ignore six deleted trees; `:13` carries a label.
  Task 7.1 widened.
- **P12** `Marbles::ResetPageToDefaults()` and
  `PageManager::SanitizeSimModAssignments()`: zero callers. Deleted in 5.6;
  task 7.2 no longer rewords `Marbles.hpp:54` (the comment goes with the
  method).
- **P13** Gates: 4.7 and 5.5 had no positive control; the VST gate would run
  binaries built 2026-08-31 against edited headers; `make -C app test` stops
  at the first red binary. Each gate now names its control and rebuild.
- **P14** `openspec validate --all --strict` is red before this change starts:
  `frogg3rs-guitar-and-solo-variants` fails on
  `external-ring-mod-mix/spec.md` ("MODIFIED ... must contain SHALL or
  MUST"), the validator's first-body-line quirk. Task 8.3 reflows that line
  without changing a word, so the `--all` gate can go green.
- **P15** Delivery order: the pushable groups run and commit first, the
  branch is pushed at that commit, and the local-only groups follow (see
  Delivery). The variants' uncommitted work is committed as one labelled
  snapshot at the start of the local-only run so every later commit's diff is
  this change's own.
- **P16** Overlaps: `frogg3rs-controllers-page-row-controls` and
  `frogg3rs-controllers-page-name-in-the-editor` no longer have uncommitted
  code (`git status` shows no `app/` file modified; their work landed in
  `3fa09c1`). `frogg3rs-drilled-in-randomize-floor` (0/15) plans edits to
  `app/FroggersModulation.hpp:867` only; disjoint.
- **P17** Two `app/vst/FroggersPluginProcessor.cpp` comments cite the transport
  branches by stale line and by quoted code; task 4.1 updates them.
- **P18** Citations from `app/` into `src/core` files this change edits are
  re-resolved after group 5 (task 5.7). This executes the item the variants
  change handed over at its `tasks.md:221`, which is ticked there with a
  one-line pointer.
- **P19 — reported, not executed.** `AudioPairArState`/`m_pairAr`
  (`FroggersEngine.hpp:117`, setter `SetAudioPairArState` `:249`) is set by
  no firmware host either: `src/common/App.hpp`, `src/FroggersSolo/`,
  `src/FroggersGuitar/`, `src/TestControl/` never name it. So on hardware
  `MixOscVoices` (`:784-822`) always returns the plain average at `:798-801`,
  and the pair-AR envelopes `m_pair12`/`m_pair23` never step. The variants
  change's VariantMix test (`VariantMix_body.inl:65-82`) probes this path with
  a state it constructs itself, and `PairArEnvelope_test` tests the envelope
  in isolation. Whether pair-AR gets wired to a firmware page or deleted with
  its tests is a design fork on an in-flight change with three operator tasks
  open; this change leaves the code and the tests as they are and puts the
  fork to the operator in its report.
- **P20** (8.1) `grep -c FroggersTiga DAISY_MANUAL.md` = 0 on 2026-09-06.
- **P22 (found at execution, 5.2)** `src/TestControl/` has never compiled:
  `TestControl` lacks the `SetSampleRate` and `ButtonCallback` members
  `src/common/App.hpp:15`, `:36` call, with or without `Include.hpp`; nothing
  outside its own directory references it (a sibling proposal's prose lists
  it beside a `Blink/` that does not exist either). Deleted. It was
  `Include.hpp`'s only consumer, so `Include.hpp` and all sixteen shims go.
  The preflight's baseline run (1.3) did not include the three Daisy builds
  the gate table lists, which is how a never-live gate reached execution.
- **P23 (found at execution, 2.2)** The workflow's first run failed: the root
  `Makefile` includes `src/mk/config.mk` unconditionally and that file errors
  when the Arm toolchain is absent. The include is now skipped for the
  `firmware-test` goal; the second run passes. Pushed as a follow-up commit
  on the pull request.
- **P24** See P3's correction. Every remaining deletion operand was re-checked
  case-insensitively before the engine edit ran.
- **P21** (1.2) `git status --short` shows no change to
  `app/FroggersUiSurface.hpp` or `app/FroggersAppCore.hpp`. The variants'
  uncommitted diff: `DAISY_MANUAL.md` +86/−, `src/FroggersSolo/*` 15 lines,
  `src/common/DaisyIO.hpp` 70, `src/common/FieldMutationQueue.hpp` 16,
  `src/core/FroggersEngine.hpp` 106, `src/core/Page.hpp` 33,
  `src/core/ResonantBump.hpp` 25, `test/firmware/CMakeLists.txt` 16; untracked
  `src/FroggersGuitar/`, `src/core/FroggersVariant.hpp`,
  `test/firmware/RefreshGate_test.cpp`, `test/firmware/VariantMix_*`.

## Design

Each repair, in the order the tasks run.

**Firmware tests get an invocation (2).** A root `Makefile` target
`firmware-test` configures `test/firmware` into `test/firmware/build` with the
same generator and build type the 2026-08-28 run used, builds, and runs
`ctest --output-on-failure`. A new workflow `.github/workflows/firmware-tests.yml`
runs the same target on `ubuntu-latest` for pushes and pull requests touching
`src/core/**`, `test/firmware/**`, `Makefile` or itself. The `cmake` + `ctest`
invocation is copied from the one that works today in `vst-plugin.yml:80-81`
and diffed against it in task 2.2; the workflow is a first attempt and its
first run is expected to be the discovery.

**Assertions that can fail (4a, 4b).** The delay test feeds 4800 samples of the
sine, records the wet pair of the last pre-clear sample and asserts
`std::abs(l) + std::abs(r) > 0` (the positive control: the buffer held signal
and the read head reached it), then asserts the post-clear wet pair is exactly
`0.0f` on both channels. The follower test keeps
`expectedFall = ef.attackCoeff * (1.0f - ef.releaseCoeff)` and asserts
`REQUIRE_NEAR(out[0], expectedFall, 1e-6)`; the directional assertion goes.

**Firmware coverage (1).** New ctest binaries under `test/firmware/`, each
with the positive control the standing rule requires (a value shown to move
before it is shown to be right):
- `Parameter_fuego_test`: the scramble at knob values giving masks 15 and
  255, asserting output differs from input and equals the formula at
  `Parameter.hpp:129-151`, and that knob 0 is the identity.
- `DspModules_test`: one asserted property per module for the nine named
  under finding 1 (a DC gain, a pole, a saturation bound, a known sequence).
- `Page_test`: `KnobUpdate` (`Page.hpp:193`), mod source/depth
  (`:304`, `:326`, `:294`), `Randomize` (`:367`, never the `FUEG` row,
  `Parameter.hpp:191-194`), page navigation wraps (`:429`, `:438`).

**Citations resolve (3, 13).** Deleted-file citations become git-object
citations (`f2369151^:sim/StereoDelay.hpp:60-64`). The six `RequiredHeight`
mentions lose the name and say what is true now: `Config().uiHeight` is a
plain initial window size, kept equal to `FroggersPageLayout::kDefaultHeight`
by hand. The two caption comments state the mechanism as it is:
`FinishControl` keeps caption and control in one row, so a label below its
slider stays a `Label` node. Every Sheaf `file:line` citation in the three
core headers is re-resolved and corrected in one sweep (task 3.4); every
`app/` citation into an edited `src/core` file is re-resolved after group 5
(task 5.7).

**One transport helper (6).** `FroggersUiSurface` gains one private method,
`LatchThenTransport(bool latched, synth::MessageIn message, bool running)`,
whose body is the three statements in the required order and whose comment
carries the happens-before rationale once. Play, Stop, and Freeze-engaging
call it; Freeze-release keeps its lone `SetFreezeLatched(false)`. The two VST
comments that cite the branches are re-resolved.

**AppCore structure (7).** The duplicated two-statement clear becomes one
lambda beside `runStopTeardown`, called from both sites. The slot grouping
becomes one helper that maps `(vco, role)` to a slot index, used by both
sites. `RouteAudioSample()`'s six stages become private methods named after
its own section comments, each taking what its section reads and returning
what it writes, and the method body becomes the sequence of calls. The `knob`
lambda becomes one private method used by every stage; `stoppedKnob` and
`releaseKnob` become private methods beside it; the four constants at
`:1642-1657` become private `static constexpr` members. No arithmetic
changes. The `:669-671` guard is removed; the `:1297-1299` guard and its
comment go with it. The three booleans stay (P2).

**`src/common/` shrinks to what is used (10, P22).** `src/TestControl/` is
deleted (never compiled, no consumer). With it gone `Include.hpp` has no
consumer, so it and all sixteen shims are deleted. `DaisyIO.hpp:5-6` and `FieldMutationQueue.hpp:3`
include `../core/Page.hpp` and `../core/SchmidtTrigger.hpp` explicitly. The
gate is a firmware build of `src/TestControl`, `src/FroggersSolo` and
`src/FroggersGuitar` with the local `arm-none-eabi-gcc` (present at
`/opt/homebrew/bin/`), plus `firmware-test`.

**The simulator's host hooks go (P3, P12).** Every member, setter, accessor,
branch and header that only a deleted host could reach is deleted from
`src/core/FroggersEngine.hpp`; each collapsed ternary or `if` keeps its
legacy branch verbatim. `VcoAdsrState.hpp`, `VcoWaveMorph.hpp`,
`VcoWaveEval.hpp` are deleted. `HookIdentity_test` loses its one hook call
and becomes `EngineDeterminism_test`, printing a checksum of its output so the
same number can be read before and after the deletion. The VariantMix tests
lose the lines that set deleted members and nothing else.

**Dead dependency and stale output go (11, 9).** `git rm` the submodule path,
remove its `.gitmodules` entry and `.git/modules/External/theallelectricsmartgrid`
if present. Delete root `publish/` and rewrite
`package-catalog.mjs:56-71` so it no longer describes a directory that is
gone.

**Labels leave, rationale stays (12).** Mechanical enumeration with the full
operand list (task 6.1), classification of every hit, then the edit. The five
omni-rule citations are handled the same way: "a silent capture proves
nothing — report the max |sample|" stays; `OMNI 9.1:` goes.

**desktop-v2 is gone everywhere (15).** The spec requirement is removed by the
delta in this change. `.gitignore:2-8` are deleted and `:13`, `:22` reworded.
`FroggersEngine.hpp:122`'s comment goes with the flag it describes (P3);
`Marbles.hpp:50-56` goes with its method (P12). The six `app/` include-guard
comments drop `desktop-v2` (and the other deleted trees the parity file's
`:6` lists) and keep `src/`.

**One `RefreshGate` (4c).** The struct moves to `src/core/RefreshGate.hpp`,
SDK-free, included by `DaisyIO.hpp` and by the test. The restated copy and
the comment promising an assertion are deleted; there is one body, so nothing
has to agree with anything. `test/firmware/CMakeLists.txt` already puts
`src/core` on the include path (`:7-11`).

**Docs and backlog (8, 14).** `README.md:64-67`, `:72` and `DAISY_MANUAL.md:15`
describe `src/` as the firmware tree with two live variants; every `app/`
comment that calls `src/` frozen says "firmware" instead; the check is
renamed `check_no_firmware_includes.sh` / `check-no-firmware-includes`. The
dead change is archived with
`openspec archive frogg3rs-controllers-page-user-story --skip-specs -y`, with
one line added to its `proposal.md` naming the superseding change as the
carrier of its 2.1–2.4 work.

## Overlaps with active changes

Enumerated from `openspec list` and `git status` on 2026-09-06.

- **`frogg3rs-guitar-and-solo-variants` (29/33)** owns the uncommitted edits
  listed under P21. Operator ruling 2026-09-05: this change edits those files
  too (findings 4c, 8, P3, and the comment sweeps), on top of the uncommitted
  variants work as it stands. The variant logic itself (`src/FroggersGuitar/`,
  `src/FroggersSolo/`, `FroggersVariant.hpp`, the `VariantMix_*` test logic)
  is not edited; the VariantMix files lose only lines that name deleted
  members. Its open tasks are three operator checks and the citation handover
  P18 executes. Commits that touch any of those files stay local until the
  operator says to push them; see Delivery.
- **`frogg3rs-controllers-page-row-controls` (27/42)** and
  **`frogg3rs-controllers-page-name-in-the-editor` (27/41)**: their code is
  committed (`3fa09c1`); neither names `FroggersUiSurface.hpp` or
  `FroggersAppCore.hpp` in its tasks.
- **`frogg3rs-drilled-in-randomize-floor` (0/15)**: planned only, edits
  `app/FroggersModulation.hpp` only. Disjoint.
- **`frogg3rs-controllers-editor-add-and-columns` (25/26)**: Sheaf-side and
  controllers page. Disjoint.
- **`frogg3rs-controllers-page-user-story`** is the change finding 14 archives.
- No other change plans any of this work.

## Not in this change

- Audit finding 5 (VST host tests): false; `app/vst/CMakeLists.txt:326-336`
  builds and registers them and `vst-plugin.yml` runs them.
- The Makefile flash and vendor targets (`vendor-libs`, `program-dfu`,
  `program-boot`): operator entry points, documented at their call site.
- Sheaf-side work of any kind.
- The pair-AR fork (P19).
- Tests for the deleted simulator paths (former tasks 9.2–9.4).

## Spec deltas

- `mod-rack-dual-midi-jacks`: REMOVED `v2-excludes-dual-midi-jack-mod-rack`
  (finding 15).
- `frogg3rs-distribution`: ADDED "The repository carries no published artifact
  the deploy does not produce" (finding 9).
- `frogg3rs-firmware-verification` (new): ADDED "The firmware tests have a
  checked-in invocation" and "Every firmware DSP path has a test that can
  fail" (findings 2, 1).

The structural, comment and citation repairs change no behaviour and get no
delta; the gates below are what prove that.

## Gates

Named per the standing rule, with what each one loads and its positive
control. Every build runs under `nice` with `-j2` at most (this machine).

| gate | loads | control | run when |
|---|---|---|---|
| `nice make -C app -j2 test`, then every `app/build/froggers_*_tests` binary by path (the recipe stops at the first red binary) | all of `app/` | 4.7: one arithmetic line in `RouteAudioSample` broken, the audio-routing binary red, restored, binary removed | after every `app/` task group |
| `nice cmake --build app/vst/build -j2 && ctest --test-dir app/vst/build --output-on-failure` | `app/vst/` and the headers it compiles | the rebuild itself (binaries newer than the headers) | after groups 4 and 6 |
| `make firmware-test` | `src/core`, `test/firmware` | 5.6: the determinism checksum before and after; 9.4: each new binary once with an assertion inverted | first in 2.3, then after groups 5, 6, 9 |
| `nice make -C src/FroggersSolo -j2 && nice make -C src/FroggersGuitar -j2` | all of `src/` | a clean build from `rm -rf src/*/build` | after group 5 |
| `openspec validate --all --strict` | `openspec/` | P14's reflow turns the one red item green | after groups 7 and 8 |
| `app/browser` e2e | not loaded by any task here | — | carried forward, not run (red since `c81727b9` for reasons outside this change) |

## Delivery

Everything goes to `main` directly; this repository does not use pull
requests (a pull request opened on 2026-09-06 against the proposal's earlier
wording was closed unmerged). One branch `omni-audit-repairs` from `main`, one
commit per task group, no AI attribution lines, pushed to `main` as a
fast-forward once the final gates are green: the app-side groups first, then,
on the operator's go, the variants' uncommitted work as one labelled snapshot
commit and the firmware-side groups on top of it. `git status` clean at repo
and submodule level. Delete the branch after the push.
