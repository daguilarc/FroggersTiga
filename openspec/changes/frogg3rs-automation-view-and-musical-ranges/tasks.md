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

- [x] 0.1 Delete the gate scripts nothing invokes. Confirmed orphan by
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
- [x] 0.2 `app/browser/check-renamed-origin.sh` is NOT an orphan — it runs
      from `app/browser/Makefile`, `app/browser/local-smoke.sh` and
      `.github/workflows/pages.yml`. Keep it as the regression guard against
      old-name URLs returning to published bytes, but trim it: 95 lines, of
      which roughly 45 are commentary about a rename that has now happened.
      Keep the three-tier policy, drop the history.
- [x] 0.3 Root-level correspondence artifacts, all tracked and none of them
      repo content: `upstream-email-external-audio-draft.md`,
      `upstream-email-jvictor0-draft.md`,
      `upstream-email-jvictor0-2-draft.md`,
      `upstream-email-jvictor0-3-drilldown-answer-draft.md`, and
      `sheaf-audioconfig-labels.patch`. Confirm each has been sent or landed
      upstream, then remove it; anything still pending moves under `docs/`
      rather than staying at the root.
- [x] 0.4 Machine-local, untracked, not repo bloat but worth clearing while
      here: `node-v22.16.0-darwin-arm64/` (178 MB) and its `.tar.gz`,
      `Rack-SDK/` (22 MB), and `.emsdk/` (1.6 GB). `.emsdk`'s cache still
      records the pre-rename absolute path, which the next wasm build is the
      first thing to notice — clear or regenerate it rather than waiting to
      be surprised.
- [x] 0.5 DONE — sweep result, found versus changed:
      **Deleted, 15 scripts.** The 13 planned orphans, plus two the sweep
      itself produced: `scripts/repo_path_policy.sh`, whose only consumer was
      the hygiene gate deleted in 0.1, and `scripts/open-desktop-v2.sh`,
      dead since a 2026-07-28 proposal listed it for removal and never did
      it. Deleting a consumer orphans its dependency, so the sweep was re-run
      against every remaining script in `scripts/` after the first pass;
      everything else is reachable from `web/package.json`, `pages.yml` or
      `build-wasm.sh`.
      **Kept, with reason.** `app/browser/check-renamed-origin.sh` runs from
      three places and was trimmed instead (0.2). The four `check_vcv_*`
      gates were restored on operator instruction and moved into the vcv
      tree's own `vcv/scripts/`, where they now pass against real inputs —
      6 SVG files, 72/36 HP parsed — having previously been run by nothing.
      **Documentation repaired**, not just code: 6 live documents named the
      deleted scripts, plus the `froggers-host-master` verification block and
      `SIM_MANUAL.md`'s launcher instruction. Archived changes keep theirs.
      **Root cleared** of 5 correspondence artifacts and `UPSTREAM-SHEAF-ASK.md`.
      **Machine-local:** the 178 MB Node tarball is gone, a pure duplicate of
      the directory beside it. `.emsdk` (1.6 GB), `node-v22…` (178 MB) and
      `Rack-SDK` (22 MB) are KEPT — untracked build inputs whose removal
      forces re-downloads that need install approval and would take the
      browser build gate red. `.emsdk`'s cache holds a pre-rename path;
      emscripten re-derives it on next run.

## 1. Bank-addressed parameter write (design A) — the framework half

- [x] 1.1 DONE during the change audit, all against `External/Sheaf/projects/
      synth`. Confirmed: `MessageInBus::Apply` passes `ParamSetAbsolute` as
      (slotIx, position, value, epoch) with no bank, and `bankIx` reaches
      only `SelectParamBank`; `Bank::HandleSetAbsolute` and
      `ParameterManager::BankAt` are both public; `BankSlot::SelectBank`
      calls `Deselect()` on the outgoing bank, which resets `visible_` to
      `topLevel_` and clears `selected_`; `FroggersModulationDrillIn::Level()`
      returns a counter mutated only by `PressEncoder`/`Back`, never
      re-derived, so it goes stale exactly as design A says.
      MOVED, and design A was corrected for it: `Bank::HandleSetAbsolute`
      resolves through `FindVisibleCell`, i.e. the bank's CURRENT PAGE, and
      `Bank::OpenModulationView` replaces `visible_` with the drill-down
      page. So the primitive is visible-addressed, not top-level-addressed,
      and 1.2 adds the top-level primitive rather than reusing it.
      Original text: Trace and confirm design A's claims against current source: that
      `ParamSetAbsolute` is slot-addressed, that `bankIx` is consumed only
      by `SelectParamBank`, that `Bank::HandleSetAbsolute` and
      `ParameterManager::BankAt` are public, and that `Deselect()` closes an
      open drill-down. Report anything that has moved.
- [x] 1.2 DONE (Sheaf 4bd711ad). Add the additive framework API in `External/Sheaf`: a new
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
      Sheaf runs its own OpenSpec: every code change in PR #9 carries a
      change directory under `External/Sheaf/openspec/changes/`. Write one
      for this work in Sheaf's own style before the code, not after.
      Sheaf's `AGENTS.md` also keeps `main` clean and expects feature work on
      a branch — the submodule is already on `fix-out-of-tree-app-gaps`, at
      `80c4eab8`, matching the PR head exactly, so commits append there.
- [x] 1.3 DONE. No modifier gate: a modifier reinterprets an operator's own
      encoder gesture, and a write addressed to a bank and position did not
      come from the encoders, so a held modifier must not silently swallow
      it. Epoch IS recorded on the addressed slot, exactly as the slot path
      does — it early-returns on epoch 0, so it costs nothing for callers
      that do not use epochs and keeps the two paths consistent for those
      that do. Both reasons are in the code.
      §8 on the new diff, found versus changed: `FindTopLevelCell` is
      near-identical to the two `FindVisibleCell` overloads — found 3,
      changed 0. Collapsing them would rewrite pre-existing upstream code
      this contribution does not otherwise touch, and the additive-only
      constraint governs an upstream pull request. Recorded, not silently
      left.
      Original text: Decide, and record, what the new path does about the two behaviors
      the slot path performs and this one does not inherit: the
      `GetCurrentModifier() == Modifier::None` gate, and
      `RecordProcessedAbsoluteEpoch`. Neither is optional-by-default; both are
      a choice.
- [x] 1.4 DONE (Sheaf 61b41b1d). Found by forcing a real recompile — 34
      files — so `-Wswitch` had a live instrument; a cached no-op build
      reports zero warnings and proves nothing. Five surfaces, classified:
      `ToJSON`, `FromJSON` and the round-trip equivalence helper serialise
      ANY `MessageIn`, so absence meant the type could not round-trip —
      NEEDED, and added to each fall-through group (every field, `bankIx`
      included, is already written after the switch). `MessageTypeName` and
      its parser are a total name mapping — NEEDED, both directions added.
      `SystemMessageOutputInfo::Evaluate` computes feedback for a mapped
      controller and a programmatic write is not mappable — CORRECTLY
      EXCLUDES, joined to the no-feedback group so the exclusion is explicit
      rather than an omission. The parallel `UISystemMessage` enum needs
      nothing: it enumerates operator-configurable controller actions.
      Two stale range comments naming `PrevParamBank` as last were fixed;
      `TypeOrder` static_casts the enum and already ordered it correctly.
      Original text: Audit every surface in BOTH repositories that enumerates message
      types, not only exhaustive `switch`es: the parallel `UISystemMessage`
      enum and the arg1/arg2 system-message encoding are the two known ones.
      Classify each as needing the new type or deliberately not carrying it,
      and report found versus changed.
- [x] 1.5 DONE — baseline 917 tests, 915 passed, 2 failed, and the two are exactly the known braid-4 96 kHz deadline tests, nothing else. Record Sheaf's test baseline BEFORE the first Sheaf edit (design's
      Gates: two braid-4 96 kHz deadline tests fail deterministically on this
      machine and are not this change's). Counts, not adjectives.
- [x] 1.6 DONE — 5 cases, suite 922/920/2 with the same two known failures. The drilled-in case was observed RED by pointing the new primitive at the visible lookup, then restored (blob hash verified). Tests in Sheaf's own suite, in Sheaf's own style: a bank-addressed
      write lands on the target bank and leaves the shared slot's selection
      and the visible bank's drill-down untouched; and a bank-addressed write
      to a bank that is ITSELF drilled into modulation still lands on the
      top-level parameter, not on a depth cell.

## 2. Automation stops moving the visible page (design A) — the app half

- [x] 2.1 DONE — bridge pushes only ParamSetAbsoluteOnBank now; SelectParamBank push, RequestBankSelect call, needsBankSelect and lastSelectedBankIxForHostWrites_ all removed. Use the new write in the plugin bridge for ALL host automation, not
      only cross-bank writes — a write to the visible bank must not depend on
      that bank's drill-down state either. Drop the `MessageIn::SelectParamBank`
      push and the `RequestBankSelect` call from that path, and remove the
      state that existed only to gate them (`needsBankSelect`,
      `lastSelectedBankIxForHostWrites_`) rather than leaving it dead.
- [x] 2.2 DONE — verified by reading: RandomizePage/ResetPage act on *drillIn_, reconstructed only when pendingBankSelect_ drains, set only by RequestBankSelect, whose only remaining callers are operator surfaces. No core change needed. `RandomizePage`/`ResetPage` must target the operator's bank, not
      the last automated one. Verify and test.
- [x] 2.3 DONE — rewritten, not deleted, as host_automation_in_a_non_visible_bank_lands_there_and_leaves_the_operators_page_untouched. Rewrite the existing test that asserts the OLD contract (visible
      page follows automation, page actions hit the automated bank) to the
      new one — it is
      `host_automation_in_a_non_visible_bank_keeps_active_bank_and_page_actions_in_sync`
      in `app/vst/FroggersVstHostTests.cpp` — name, header comment, and
      assertions. Do not delete it.
- [x] 2.4 DONE — 27/27 in FroggersVstHostTests. The drilled-in case was observed RED by restoring the old SelectParamBank path, then restored byte-identical. Positive control present: operator_selecting_a_bank_does_move_the_visible_page. Tests: a write to a non-visible bank lands correctly and the page
      does not move; two banks automated at once do not oscillate it; **an
      open modulation drill-down survives a cross-bank write** — this is the
      defect the framework route was chosen to prevent, so prove it.
      Positive control: the same assertion must show the page DOES move on
      an operator-driven selection. Also: automation of a parameter in the
      bank the operator is ALREADY viewing and has drilled into lands on the
      top-level parameter, not on a modulation depth — that is the case the
      old `SelectParamBank` push was incidentally covering, and removing it
      is what puts the case at risk.
- [x] 2.5 DONE — 30/30, round trip observed RED by disabling the restore. DECIDED — yes, the visible bank persists in plugin session state.
      Add it as a second key alongside the Freeze latch in the existing
      `sessionExtras` object; the mechanism is already there and this is one
      key plus one restore call. Restore it through the operator-selection
      path, which this change makes the sole authority over the visible page,
      so a restored session enters the same way an operator selection does.
      The saved index comes from a host project file and can name a bank that
      no longer exists, so bounds-check it on restore and fall back to the
      default page rather than trusting the document.

## 3. Plugin audio input bus (design B)

- [x] 3.1 DONE — traced; the finding was that nothing feeds the sources at all, which became 3.4. Trace: JUCE's optional-input-bus layout for an instrument, what
      `processBlock` does with input buffers today, and where `connected`
      should be derived.
- [x] 3.2 DONE — bus declared, selection derived from it and re-validated on layout change, connected published through the existing routed-input signal. Declare the bus; derive connected from bus-enabled-with-nonzero-
      channels through the existing `SetExternalAudioConnected` writer, once
      per layout change, never per block.
- [x] 3.4 DONE — app 294/294. External audio carries real signal; both sources snap back to exactly 0.5/0.0 on disconnect, observed RED by deleting the restore branch. SUPERSEDES a recorded decision, on operator instruction: feed real
      samples to the external-audio sources. Nothing writes
      `externalAudioSource_` in any host today — it holds `0.5f` forever, and
      `FroggersModulation.hpp:411` documents that as deliberate. Write it, and
      drive `externalAudioEf_`, from the block's input each block while
      connected; restore the inert defaults (`0.5f`, follower at `0.0f`) the
      moment connectedness drops, so the defined-and-finite guarantee that
      comment protects still holds. The plugin resolves the operator's channel
      or sum into the core's single input channel; the core reads channel zero
      and never asks which. Rewrite that comment to describe the new behavior.
      Test that a connected source MOVES with its input and returns to inert
      on disconnect — a positive control proving the value actually changed,
      not merely that it equals something.
- [x] 3.3 DONE — VST 41/41; the host-enabled-no-selection case observed RED by deriving connected from bus enablement. Tests: instantiates with the bus absent and with it present;
      routing into it connects the sources; disconnecting returns them to
      inert. Positive control both directions.

## 4. Envelope times map exponentially (design C)

- [x] 4.1 DONE — three separate floors replace the single kMinTimeSeconds (attack 1 ms, decay and release 5 ms); mapGrace left linear. Move `mapAttack`, `mapDecay`, `mapRelease` to `dsp::ExpMapCompute`
      and raise their floors per design C. **Leave `mapGrace` linear** — its
      zero is a real setting.
- [x] 4.2 DONE — measured mean 0.5410 over 100k samples against design C's predicted 0.541. Sustain: exponential over `[0.25, 1.0]`. Report the measured
      random mean and confirm it lands near today's 0.550.
- [x] 4.3 DONE — floor/midpoint/ceiling pinned against literals in FroggersDspParityTests.cpp; attack floor and sustain floor each observed RED by moving the constant, then restored. Tests: each mapping's floor, midpoint and ceiling pinned against
      literal expected values, not re-derived from the same constants the
      production code reads — a symbolic re-derivation cannot detect an
      endpoint change. Prove each new assertion red by moving a constant,
      live, then restore.
- [x] 4.4 DONE — parity tests re-run, 130/130; divergence from the linear src/core reference recorded at the constants themselves. Re-run the parity tests that reference these constants; report
      results rather than assuming. Record the divergence from `src/core` in
      the code, in plain terms.

## 5. Control bounds (design D)

- [x] 5.1 DONE — ceiling 0.25s; top decile 269ms -> 144ms. The literals group 4 pinned were updated deliberately (midpoint 0.0224 -> 0.0158), which is why they were literals. Attack ceiling to 250 ms. Report the resulting top-decile draw.
- [x] 5.2 DONE — all three confirmed from source before changing; peak and scoop bottom decile 40 -> 170 Hz, comb 37 -> 159 Hz against its 10 kHz ceiling. The derived comb low-pass floor moving 80 -> 400 Hz is noted in the code beside both mappings. Raise the 20 Hz floors on Peak freq, Scoop freq and Comb delay to
      about 100 Hz. Confirm from source which parameter each drives before
      changing it. Report the comb low-pass knob's resulting floor too: its
      range starts at `4 * combFreq`, so this change moves it from 80 Hz to
      400 Hz at minimum comb frequency (design D).
- [x] 5.3 DONE — found 31, changed 3, kept 28, verdict and reason on every one. Ring-mod carrier shares the 20 Hz floor and was KEPT on its own traced reasoning rather than matched to the others: a low carrier is audible tremolo, a low resonant peak is nothing. VCO pitch kept as planned. Five shadow copies in the blow-out repro binary are flagged, not touched: updating them would invalidate the repro. Enumerate EVERY bounded control mapping in the live `app/` tree —
      by operand (`ExpMapCompute` and the envelope maps), not by the shape of
      the three already found — and classify each bound as changed or
      deliberately kept. Report found versus changed. VCO pitch shares the
      20 Hz floor and is a known KEEP (design D); the point is to prove the
      list is complete, not to widen it.
- [x] 5.4 DONE — peak gain and fold assert unity, the depths and Grace assert zero. Leave the meaningful no-effect positions alone: Peak gain, Fold,
      Scoop depth, phase-modulation depth, ring-mod depth, Grace. Assert in
      tests that each still reaches a true no-effect setting at its minimum —
      note that is 0.0 for the depths and Grace, but 1.0 (unity, no boost, no
      fold) for Peak gain and Fold, so a "reaches zero" assertion would be
      wrong for those two.
- [ ] 5.5 OPERATOR SMOKE: every number in designs C and D is derived, not
      heard. Play it. The floors, the attack ceiling and the sustain floor
      are the values to argue with.

## 6. Comment sweep, remaining scope (design E)

- [x] 6.1 DONE, after a correction. The first pass swept the processor and
      the test files and PROVED behavior-neutrality on what it touched — but
      I recorded that as a complete sweep of `app/vst/`, and it was not: the
      editor's two files and `CMakeLists.txt` were never opened, labels
      remained in the processor, and group 3's later work added seven more to
      the surface. Postflight caught it. Now grep clean across all seven
      files, including four operator-facing STRINGS that printed planning
      labels into test output and a build error. `app/vst/` under the same method: pre-computed hit list, classify
      before changing, found-versus-changed per file, behavior-neutrality
      proven by comment-stripped diff. Known targets: a `(group 5 brief,
      binding)` reference, a stale comment describing Reset's old
      hardcoded-zero mechanism, four omni-rule citations, and general
      "Group 5"/"Group 8" saturation.
- [x] 6.2 DONE — the Sheaf side carried one leak, a task number from THIS repo's plan sitting in the upstream design notes where it means nothing. Removed and pushed to the pull request. Any `External/Sheaf` file this change touches, to Sheaf's own
      standards.
- [x] 6.3 DONE — 70 shorthand labels cleared from the routing tests, including 5 that were being PRINTED at runtime. My own earlier count of 59 was low; the sweep's pattern was better. T6.1/T6.2 were replaced by inference from the tests below them and are flagged as moderate confidence. Strip `FroggersAudioRoutingTests.cpp`'s bare letter-number labels
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
- [x] 7.2 DONE — bank and global coverage was already complete and correct and was left alone; what was missing was the opening statement of what frogg3rs is and what it runs in. `MANUAL.md`: confirm it covers what frogg3rs is and what it runs
      in; every global control; every bank parameter by parameter.
- [x] 7.3 DONE — traced per host and cited, not described from assumption. Standalone: Audio, Controllers and Sync pages with their real ranges. Plugin: the input opt-in, hidden transport, read-only BPM under host tempo, and MIDI reaching parameters only through the automation surface. Browser: internal clock and gesture-gated input consent. NOT established and deliberately omitted rather than guessed: whether the browser build exposes a Controllers-equivalent CC mapping. Add the audio and MIDI configuration section — absent today.
      TRACE what each host actually exposes: device selection, MIDI CC
      mapping, and what differs in the plugin where the DAW owns transport
      and tempo. Do not describe it from assumption.
- [x] 7.4 DONE — 7 lines corrected against the new constants, plus the false claim that the external sources are permanently unavailable. Scoop freq gained the range it was missing. `QUICK_DICT.md`: correct the claim that the external-audio sources
      are permanently unavailable. Re-check every other parameter line
      against current behavior while there.
- [x] 7.5 DONE — 2 voice violations fixed in passages already being edited, both defining the feature by what it lacked. Apply design F's voice rule to whatever you rewrite: say what the
      control does, give range, units and default, stop.

- [x] 7.6 DONE — both documents land in the standalone .app, the .vst3 and the .component, copied from the repository root at build time and verified present in the real artifacts. Standalone opens them from a Help menu; the plugin from a small button laid over the surface, so the shared surface needed no restructuring. Ship the docs WITH the app: embed `MANUAL.md` and `QUICK_DICT.md`
      as build resources in the standalone and in the VST3/AU plugin, and add
      the surface that opens them. A plugin in a DAW with no internet cannot
      follow a web link, which is what the current app's site does today
      (`app/browser/site/index.html` points at GitHub). Follow the frozen
      desktop's pattern — the ROOT documents listed as resource sources, so
      the copy exists only in the built bundle and there is no second
      checked-in file to keep in sync. `app/Resources/` currently holds only
      icons.
- [x] 7.7 DONE — verified the link resolves after the rename: MANUAL.md and QUICK_DICT.md are on main and the URL returns 200. The browser build keeps linking to the published documents — it is
      already on the network. Confirm that link resolves after the repository
      rename before relying on it.

## 8. Carried from the predecessor

- [x] 8.1 DONE — asserts the rendered ring geometry decodes to a value set to 0.8 against a 0.0 default, so it cannot pass on defaults or on mere node presence. Observed RED by cutting the plugin's own call into the pre-audio publish seam: the encoder rendered blank, with no collateral failures. Assert the plugin editor renders parameters before the host ever
      calls `processBlock` — the site half is covered, this half was never
      asserted.
- [x] 8.2 DONE — `UPSTREAM-SHEAF-ASK.md` retired rather than corrected. It
      tracked what to ask upstream and what had landed; that record is now
      the open pull request, jvictor0/Sheaf#9 ("Fix out-of-tree app gaps",
      branch `fix-out-of-tree-app-gaps`, the branch the submodule is pinned
      to). Correcting rows in a document the PR supersedes is maintaining a
      second source of truth.
- [x] 8.3 DONE — the working folder is `~/Desktop/frogg3rs`; ledger and memory entries citing the old path were updated at the same time. OPERATOR-COORDINATED, machine-local: rename the working folder
      `~/Desktop/FroggersTiga` to `~/Desktop/frogg3rs` at a session
      boundary; it invalidates a running session's absolute paths.
      Afterwards update ledger and memory entries citing the old path.

## 9. Verify and hand off for testing

- [x] 9.1 DONE except browser e2e. App 300/300; plugin host 46/46, smoke 1/1, editor 3/3; VST3 and AU both build; Sheaf 922 total / 920 passed / 2 failed, exactly the known braid-4 pair; browser build ok. Browser e2e is BLOCKED wanting `npx playwright install`, which needs operator approval. Full suite green; browser build and e2e green; plugin builds VST3
      and AU **and its own test targets RUN, not merely built** —
      `FroggersVstHostTests`, `FroggersVstSmokeTest`, `FroggersVstEditorTest`
      from `app/vst/build`, because `cd app && make test` builds none of them
      and this change's plugin coverage lives there; Sheaf's suite green
      (920/2, never a zero exit). Counts reported.
- [x] 9.2 DONE. Found three real divergences, two of them mine: 6.1's completeness claim was false, group 3 introduced new labels after the sweep, and the Sheaf change's own notes called work out of scope that the same branch then did. All three fixed. Duplication pass across eleven new named concepts, searched by operand: ZERO duplicates; FindTopLevelCell remains the one deliberate non-collapse. Change-level postflight: implementation versus proposal
      divergence, plus a fresh duplication pass against the WHOLE diff for
      every new named concept this change introduces — a new name
      retroactively makes previously-fine code redundant, which no
      plan-time pass can catch.
- [ ] 9.3 OPERATOR GATE: DAW smoke, still never performed. Load the plugin
      in a real DAW; confirm host transport and tempo drive it, a parameter
      automates, a DAW-side MIDI mapping moves a parameter, session state
      round-trips, automation does NOT move the visible page, and routing
      into the input bus connects the external sources.
- [x] 9.4 DONE — Sheaf pushed to the fork branch heading the upstream pull request before every pin bump; both trees clean, nothing unpushed, pin c63783df matches the pushed head. Push Sheaf BEFORE the superproject — the superproject records only
      the pin, and CI builds from it. Sheaf commits append to the existing
      open pull request.
- [x] 9.5 DONE — handed off. Hand off for operator testing. Groups 1–9 are testable here; the
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
      `sim-operator-doc-parity` RETIRES rather than being rewritten — traced,
      not assumed. It exists to hold one manual and four generated mirrors in
      sync, and after the merge nothing reads a mirror: the current app links
      `MANUAL.md` on GitHub directly (`app/browser/site/index.html`), the
      browser build copies no markdown at all, and `pages.yml` already
      publishes `app/browser/dist/site` instead of the legacy web tree. Its
      three remaining consumers — the v1 site's help modal, the built v1
      Pages site, and the frozen desktop app's embedded Help — all go with
      this group. One document with no copies has no parity to keep.
- [ ] 10.1 Merge v2 into main.
- [ ] 10.2 DECIDED — the new desktop app is `frogg3rs_v2`, and the old
      naming convention is dropped rather than carried forward. Assets become
      `frogg3rs.dmg` / `frogg3rs-Setup.exe` (confirm the exact Windows form
      when building). The filenames come from the build, not from GitHub, so
      there is no rename step as such: the product name in the packaging
      produces them, the workflow uploads those exact paths, and
      `web/index.html`'s two download links must change in the SAME commit or
      both downloads 404 the moment the release is replaced. Trace all four
      before touching any.
- [ ] 10.3 DECIDED — the release tag is `froggers_v2`, replacing
      `froggerstiga-v1`. Amend `AGENTS.md` in the same commit: it currently
      names `froggerstiga-v1` as the one permitted desktop channel and
      forbids creating other tags, so the rule has to move with the tag or
      the next release violates it.
      CONFIRM BEFORE TAGGING: the operator gave the tag as `froggers_v2` and
      the app as `frogg3rs_v2`, in consecutive messages — the two spellings
      differ by exactly the leetspeak substitution this repository just spent
      a rename adopting. A published tag is effectively permanent, so ask
      once rather than inferring which spelling was meant.
- [ ] 10.4 Move the release-notes source and the release-metadata version
      heading off `SIM_MANUAL.md` and onto `MANUAL.md`
      (`desktop/scripts/render-release-notes.sh`,
      `desktop/scripts/verify-release-metadata.sh`).
- [ ] 10.5 Tear down the mirror apparatus wholesale rather than re-pointing
      it, since it has no consumer left: `scripts/sync-help-docs.sh`,
      `sim/check_operator_docs_sync.sh`, the re-sync step in
      `scripts/hooks/pre-commit`, its invocation in `.github/workflows/
      pages.yml`, the four generated mirrors under `docs/` and
      `web/public/`, and the two CMake resource embeds. `QUICK_DICT.md`
      itself STAYS — it is the terse counterpart to `MANUAL.md`; only its
      copies go. Then delete `SIM_MANUAL.md`, against the deltas from 10.0.
      Verify no remaining reader before deleting each: this list was traced
      2026-08-20 and a consumer added since would make it wrong.
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
