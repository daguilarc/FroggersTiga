# Tasks — frogg3rs-automation-view-and-musical-ranges

Gate after every group: `cd app && nice make -j2 test` green (baseline
290/290 at this change's open; NEVER above `-j2`, always `nice`). One commit
per group. Subagent dispatch: lightest capable model, one file or one
concern per agent, pre-computed hit lists where a sweep is involved.
Anchors in design.md cite symbols, not line numbers — locate by symbol.

Standing rule: anything found during execution is fixed inside this change.
A finding that contradicts a recorded decision stops for the operator.

## 0. Repo hygiene (design H) — STEP ZERO, before any other group

Hygiene is step 0 of every change (omni-rule §13.0). Nothing here is deferred
to a follow-up; what the sweep finds is fixed in this change, except where an
item is still load-bearing, which sequences it to group 10 rather than
excusing it.

- [ ] 0.1 Delete the gate scripts nothing invokes. Confirmed orphan by
      searching for the INVOCATION — bare name AND path, across workflows,
      Makefiles, package.json and other scripts — not by the file existing:
      `scripts/check_subagent_packet_gates.sh` plus the three
      `scripts/check_desktop_v2_*` scripts it is the only caller of;
      `scripts/check_host_artifact_hygiene.sh`;
      `scripts/check_openspec_hygiene.sh`; `scripts/verify_clean_rebuild.sh`;
      `sim/check_common_core_wrappers.sh`;
      `sim/check_firmware_toolchain_parity.sh`; and the four
      `sim/check_vcv_*` scripts. About 1,017 lines, over half of it guarding
      frozen trees. RE-VERIFY each one's orphan status at execution time
      before deleting it — this list was traced on 2026-08-20 and a call site
      added since would make it wrong. Report found versus changed.
- [ ] 0.2 `app/browser/check-renamed-origin.sh` is NOT an orphan — it runs
      from `app/browser/Makefile`, `app/browser/local-smoke.sh` and
      `.github/workflows/pages.yml`. Keep it as the regression guard against
      old-name URLs returning to published bytes, but trim it: 95 lines, of
      which roughly 45 are commentary about a rename that has now happened.
      Keep the three-tier policy, drop the history.
- [ ] 0.3 Root-level correspondence artifacts, all tracked and none of them
      repo content: `upstream-email-external-audio-draft.md`,
      `upstream-email-jvictor0-draft.md`,
      `upstream-email-jvictor0-2-draft.md`,
      `upstream-email-jvictor0-3-drilldown-answer-draft.md`, and
      `sheaf-audioconfig-labels.patch`. Confirm each has been sent or landed
      upstream, then remove it; anything still pending moves under `docs/`
      rather than staying at the root.
- [ ] 0.4 Machine-local, untracked, not repo bloat but worth clearing while
      here: `node-v22.16.0-darwin-arm64/` (178 MB) and its `.tar.gz`,
      `Rack-SDK/` (22 MB), and `.emsdk/` (1.6 GB). `.emsdk`'s cache still
      records the pre-rename absolute path, which the next wasm build is the
      first thing to notice — clear or regenerate it rather than waiting to
      be surprised.
- [ ] 0.5 Report the sweep: found versus changed per item, and name anything
      deliberately kept with the reason. A partial cleanup that reads as
      complete is worse than none.

## 1. Bank-addressed parameter write (design A) — the framework half

- [ ] 1.1 Trace and confirm design A's claims against current source: that
      `ParamSetAbsolute` is slot-addressed, that `bankIx` is consumed only
      by `SelectParamBank`, that `Bank::HandleSetAbsolute` and
      `ParameterManager::BankAt` are public, and that `Deselect()` closes an
      open drill-down. Report anything that has moved.
- [ ] 1.2 Add the additive framework API in `External/Sheaf`: a new
      `MessageIn` type carrying `bankIx`, its `MessageInBus::Apply` case, and
      a `ParameterManager` method that resolves `position` to a
      `PhysicalEncoderId` through the addressed slot's encoder list and then
      writes to `BankAt(bankIx)` DIRECTLY, bypassing `BankSlot::Owns`'s
      visible-bank requirement. The write MUST address the target bank's
      top-level mapping, not its visible page — `Bank::HandleSetAbsolute`
      resolves through `visible_`, which is the modulation drill-down page
      when that bank is drilled in, so reusing it would land an automated
      value on a modulation depth (design A). `Bank` has no top-level-addressed
      write today, so add that primitive too. No existing signature or
      behavior changes. Nothing frogg3rs-specific in the name, comments, or
      rationale.
- [ ] 1.3 Decide, and record, what the new path does about the two behaviors
      the slot path performs and this one does not inherit: the
      `GetCurrentModifier() == Modifier::None` gate, and
      `RecordProcessedAbsoluteEpoch`. Neither is optional-by-default; both are
      a choice.
- [ ] 1.4 Audit every surface in BOTH repositories that enumerates message
      types, not only exhaustive `switch`es: the parallel `UISystemMessage`
      enum and the arg1/arg2 system-message encoding are the two known ones.
      Classify each as needing the new type or deliberately not carrying it,
      and report found versus changed.
- [ ] 1.5 Record Sheaf's test baseline BEFORE the first Sheaf edit (design's
      Gates: two braid-4 96 kHz deadline tests fail deterministically on this
      machine and are not this change's). Counts, not adjectives.
- [ ] 1.6 Tests in Sheaf's own suite, in Sheaf's own style: a bank-addressed
      write lands on the target bank and leaves the shared slot's selection
      and the visible bank's drill-down untouched; and a bank-addressed write
      to a bank that is ITSELF drilled into modulation still lands on the
      top-level parameter, not on a depth cell.

## 2. Automation stops moving the visible page (design A) — the app half

- [ ] 2.1 Use the new write in the plugin bridge for ALL host automation, not
      only cross-bank writes — a write to the visible bank must not depend on
      that bank's drill-down state either. Drop the `MessageIn::SelectParamBank`
      push and the `RequestBankSelect` call from that path, and remove the
      state that existed only to gate them (`needsBankSelect`,
      `lastSelectedBankIxForHostWrites_`) rather than leaving it dead.
- [ ] 2.2 `RandomizePage`/`ResetPage` must target the operator's bank, not
      the last automated one. Verify and test.
- [ ] 2.3 Rewrite the existing test that asserts the OLD contract (visible
      page follows automation, page actions hit the automated bank) to the
      new one — it is
      `host_automation_in_a_non_visible_bank_keeps_active_bank_and_page_actions_in_sync`
      in `app/vst/FroggersVstHostTests.cpp` — name, header comment, and
      assertions. Do not delete it.
- [ ] 2.4 Tests: a write to a non-visible bank lands correctly and the page
      does not move; two banks automated at once do not oscillate it; **an
      open modulation drill-down survives a cross-bank write** — this is the
      defect the framework route was chosen to prevent, so prove it.
      Positive control: the same assertion must show the page DOES move on
      an operator-driven selection. Also: automation of a parameter in the
      bank the operator is ALREADY viewing and has drilled into lands on the
      top-level parameter, not on a modulation depth — that is the case the
      old `SelectParamBank` push was incidentally covering, and removing it
      is what puts the case at risk.
- [ ] 2.5 DECIDED — yes, the visible bank persists in plugin session state.
      Add it as a second key alongside the Freeze latch in the existing
      `sessionExtras` object; the mechanism is already there and this is one
      key plus one restore call. Restore it through the operator-selection
      path, which this change makes the sole authority over the visible page,
      so a restored session enters the same way an operator selection does.
      The saved index comes from a host project file and can name a bank that
      no longer exists, so bounds-check it on restore and fall back to the
      default page rather than trusting the document.

## 3. Plugin audio input bus (design B)

- [ ] 3.1 Trace: JUCE's optional-input-bus layout for an instrument, what
      `processBlock` does with input buffers today, and where `connected`
      should be derived.
- [ ] 3.2 Declare the bus; derive connected from bus-enabled-with-nonzero-
      channels through the existing `SetExternalAudioConnected` writer, once
      per layout change, never per block.
- [ ] 3.3 Tests: instantiates with the bus absent and with it present;
      routing into it connects the sources; disconnecting returns them to
      inert. Positive control both directions.

## 4. Envelope times map exponentially (design C)

- [ ] 4.1 Move `mapAttack`, `mapDecay`, `mapRelease` to `dsp::ExpMapCompute`
      and raise their floors per design C. **Leave `mapGrace` linear** — its
      zero is a real setting.
- [ ] 4.2 Sustain: exponential over `[0.25, 1.0]`. Report the measured
      random mean and confirm it lands near today's 0.550.
- [ ] 4.3 Tests: each mapping's floor, midpoint and ceiling pinned against
      literal expected values, not re-derived from the same constants the
      production code reads — a symbolic re-derivation cannot detect an
      endpoint change. Prove each new assertion red by moving a constant,
      live, then restore.
- [ ] 4.4 Re-run the parity tests that reference these constants; report
      results rather than assuming. Record the divergence from `src/core` in
      the code, in plain terms.

## 5. Control bounds (design D)

- [ ] 5.1 Attack ceiling to 250 ms. Report the resulting top-decile draw.
- [ ] 5.2 Raise the 20 Hz floors on Peak freq, Scoop freq and Comb delay to
      about 100 Hz. Confirm from source which parameter each drives before
      changing it. Report the comb low-pass knob's resulting floor too: its
      range starts at `4 * combFreq`, so this change moves it from 80 Hz to
      400 Hz at minimum comb frequency (design D).
- [ ] 5.3 Enumerate EVERY bounded control mapping in the live `app/` tree —
      by operand (`ExpMapCompute` and the envelope maps), not by the shape of
      the three already found — and classify each bound as changed or
      deliberately kept. Report found versus changed. VCO pitch shares the
      20 Hz floor and is a known KEEP (design D); the point is to prove the
      list is complete, not to widen it.
- [ ] 5.4 Leave the meaningful no-effect positions alone: Peak gain, Fold,
      Scoop depth, phase-modulation depth, ring-mod depth, Grace. Assert in
      tests that each still reaches a true no-effect setting at its minimum —
      note that is 0.0 for the depths and Grace, but 1.0 (unity, no boost, no
      fold) for Peak gain and Fold, so a "reaches zero" assertion would be
      wrong for those two.
- [ ] 5.5 OPERATOR SMOKE: every number in designs C and D is derived, not
      heard. Play it. The floors, the attack ceiling and the sustain floor
      are the values to argue with.

## 6. Comment sweep, remaining scope (design E)

- [ ] 6.1 `app/vst/` under the same method: pre-computed hit list, classify
      before changing, found-versus-changed per file, behavior-neutrality
      proven by comment-stripped diff. Known targets: a `(group 5 brief,
      binding)` reference, a stale comment describing Reset's old
      hardcoded-zero mechanism, four omni-rule citations, and general
      "Group 5"/"Group 8" saturation.
- [ ] 6.2 Any `External/Sheaf` file this change touches, to Sheaf's own
      standards.
- [ ] 6.3 Strip `FroggersAudioRoutingTests.cpp`'s bare letter-number labels
      (`D16`, `B7.5.0`, `M1` and the rest — 81 occurrences), in comments and
      in runtime diagnostic strings alike. These are planning-doc labels, and
      the standing rule is that comments explain behavior rather than the
      history of how it was planned: keep every rationale, drop the label.
      Say what the thing IS — `D16` is the default patch, `M1` is the route
      through `FroggersAppCore`'s private context — so a reader who never saw
      the planning documents loses nothing.

## 7. Documentation (design F)

- [ ] 7.1 DECIDED — the deletion lands at cutover (group 10), not here.
      `SIM_MANUAL.md` is a build and CI input, not only prose:
      `SIM_MANUAL.md` is a build and CI input, not only prose. It feeds
      release-note rendering and a release-metadata CI check, is the source of
      four generated mirrors enforced by a pre-commit hook and a sync check,
      is embedded as a resource by two CMake targets in trees this change
      declares out of scope, is linked from the public site, and is required
      BY NAME by three live specs (`sim-operator-doc-parity`,
      `froggers-host-master`, `global-strip-marbles-label`) that this change
      has no deltas for. Full trace in design F. Either this change takes on
      those deltas and re-points the release-notes and mirror inputs at
      `MANUAL.md`. That work is sequenced to group 10, where the release
      tooling and the frozen trees are open anyway. This group delivers the
      `MANUAL.md` content it does not depend on: 7.2 through 7.5 proceed now.
- [ ] 7.2 `MANUAL.md`: confirm it covers what frogg3rs is and what it runs
      in; every global control; every bank parameter by parameter.
- [ ] 7.3 Add the audio and MIDI configuration section — absent today.
      TRACE what each host actually exposes: device selection, MIDI CC
      mapping, and what differs in the plugin where the DAW owns transport
      and tempo. Do not describe it from assumption.
- [ ] 7.4 `QUICK_DICT.md`: correct the claim that the external-audio sources
      are permanently unavailable. Re-check every other parameter line
      against current behavior while there.
- [ ] 7.5 Apply design F's voice rule to whatever you rewrite: say what the
      control does, give range, units and default, stop.

## 8. Carried from the predecessor

- [ ] 8.1 Assert the plugin editor renders parameters before the host ever
      calls `processBlock` — the site half is covered, this half was never
      asserted.
- [ ] 8.2 Correct `UPSTREAM-SHEAF-ASK.md`'s stale ask-8 rows: the routed
      signal landed at the pinned commit.
- [ ] 8.3 OPERATOR-COORDINATED, machine-local: rename the working folder
      `~/Desktop/FroggersTiga` to `~/Desktop/frogg3rs` at a session
      boundary; it invalidates a running session's absolute paths.
      Afterwards update ledger and memory entries citing the old path.

## 9. Verify and hand off for testing

- [ ] 9.1 Full suite green; browser build and e2e green; plugin builds VST3
      and AU; Sheaf's suite green. Counts reported.
- [ ] 9.2 Change-level postflight: implementation versus proposal
      divergence, plus a fresh duplication pass against the WHOLE diff for
      every new named concept this change introduces — a new name
      retroactively makes previously-fine code redundant, which no
      plan-time pass can catch.
- [ ] 9.3 OPERATOR GATE: DAW smoke, still never performed. Load the plugin
      in a real DAW; confirm host transport and tempo drive it, a parameter
      automates, a DAW-side MIDI mapping moves a parameter, session state
      round-trips, automation does NOT move the visible page, and routing
      into the input bus connects the external sources.
- [ ] 9.4 Push Sheaf BEFORE the superproject — the superproject records only
      the pin, and CI builds from it. Sheaf commits append to the existing
      open pull request.
- [ ] 9.5 Hand off for operator testing. Groups 1–9 are testable here; the
      change is NOT archived at this point, because group 10 has not run.

## 10. Cutover (design G) — GATED on operator testing

Same hygiene principle as group 0, applied where the thing being cleaned up
is still load-bearing until now. Every item here removes or renames something
a shipping path currently depends on, so nothing starts until the operator has
PLAYED IT and LOADED IT IN A DAW — task 5.5 (the musical ranges, every number
of which is derived rather than heard) and task 9.3 (DAW smoke, never once
performed). Both, not either.

- [ ] 10.0 BEFORE ANY OTHER ITEM IN THIS GROUP: declare the spec deltas this
      group needs. Retiring `SIM_MANUAL.md` touches three capabilities that
      name it — `sim-operator-doc-parity`, `froggers-host-master`,
      `global-strip-marbles-label` — and this change declares deltas for
      neither of them today. Deltas are what preflight validates, so they are
      written before the group runs, not during it.
      OPERATOR DECISION inside this task: `sim-operator-doc-parity` exists
      entirely to keep the manual and its four generated mirrors in sync.
      Does that capability RETIRE with the document, or get rewritten around
      `MANUAL.md`? The mirrors and the sync check are still live and still
      useful, which argues for rewriting rather than removing — but that is a
      call, not an inference.
- [ ] 10.1 Merge v2 into main.
- [ ] 10.2 Rename the desktop release product. The asset filenames come from
      the build, not from GitHub, so there is no rename step as such: the
      product name in `desktop/`'s CMake and Inno packaging produces
      `FroggersTiga.dmg` / `FroggersTiga-Setup.exe`, the workflow uploads
      those exact paths, and `web/index.html`'s two download links must
      change in the SAME commit or both downloads 404 the moment the release
      is replaced. Trace all four before touching any.
- [ ] 10.3 OPERATOR DECISION: the `froggerstiga-v1` tag. `AGENTS.md` permits
      exactly one desktop channel under that name and forbids creating other
      tags, so renaming the tag means amending that rule. Keep the legacy tag
      name, or amend — this is a decision, not an executor's call.
- [ ] 10.4 Move the release-notes source and the release-metadata version
      heading off `SIM_MANUAL.md` and onto `MANUAL.md`
      (`desktop/scripts/render-release-notes.sh`,
      `desktop/scripts/verify-release-metadata.sh`).
- [ ] 10.5 Re-point every remaining `SIM_MANUAL.md` consumer at `MANUAL.md`:
      the mirror sync and its pre-commit hook and parity check, the desktop
      Help menu, the website's Manual modal, and the two CMake resource
      embeds. Then delete `SIM_MANUAL.md`, against the deltas declared in
      10.0.
- [ ] 10.6 Retire the frozen trees the merge makes redundant — `desktop-v2/`
      alone is 165 tracked files with no consumer. Trace each tree's
      consumers first and report found versus changed; "frozen" is not
      "removable" until nothing builds from it.
- [ ] 10.7 Fix `sim/Fuegoize.hpp`'s divide-by-zero at full fuego (design G):
      move the cast off the divisor so it matches the firmware's form, and
      add a test that drives fuego to maximum. Nothing exercises that path
      today, so it needs its own coverage.
- [ ] 10.8 Update the desktop and wasm trees as part of the merge, per
      design G. Re-run group 0's sweep afterward: a merge that opens frozen
      trees is exactly when new orphans appear.

## 11. Close

- [ ] 11.1 Full suite green once more after the merge; browser build and e2e
      green; plugin builds VST3 and AU. Counts reported.
- [ ] 11.2 Archive with spec sync. Sync only what is delivered — across all
      eleven groups, not only the ones this session remembers.
