# Tasks — frogg3rs-automation-view-and-musical-ranges

Gate after every group: `cd app && nice make -j2 test` green (baseline
290/290 at this change's open; NEVER above `-j2`, always `nice`). One commit
per group. Subagent dispatch: lightest capable model, one file or one
concern per agent, pre-computed hit lists where a sweep is involved.
Anchors in design.md cite symbols, not line numbers — locate by symbol.

Standing rule: anything found during execution is fixed inside this change.
A finding that contradicts a recorded decision stops for the operator.

## 1. Bank-addressed parameter write (design A) — the framework half

- [ ] 1.1 Trace and confirm design A's claims against current source: that
      `ParamSetAbsolute` is slot-addressed, that `bankIx` is consumed only
      by `SelectParamBank`, that `Bank::HandleSetAbsolute` and
      `ParameterManager::BankAt` are public, and that `Deselect()` closes an
      open drill-down. Report anything that has moved.
- [ ] 1.2 Add the additive framework API in `External/Sheaf`: a new
      `MessageIn` type carrying `bankIx`, its `MessageInBus::Apply` case, and
      a `ParameterManager` method resolving position slot-independently then
      writing through `Bank::HandleSetAbsolute` on `BankAt(bankIx)`. No
      existing signature or behavior changes. Nothing frogg3rs-specific in
      the name, comments, or rationale.
- [ ] 1.3 Audit every exhaustive `switch` over `MessageIn::Type` in BOTH
      repositories for a missing case; report what you found.
- [ ] 1.4 Tests in Sheaf's own suite, in Sheaf's own style: a bank-addressed
      write lands on the target bank and leaves the shared slot's selection
      and the visible bank's drill-down untouched.

## 2. Automation stops moving the visible page (design A) — the app half

- [ ] 2.1 Use the new write in the plugin bridge for cross-bank host
      automation; stop calling `RequestBankSelect` on that path.
- [ ] 2.2 `RandomizePage`/`ResetPage` must target the operator's bank, not
      the last automated one. Verify and test.
- [ ] 2.3 Rewrite the existing test that asserts the OLD contract (visible
      page follows automation, page actions hit the automated bank) to the
      new one — name, header comment, and assertions. Do not delete it.
- [ ] 2.4 Tests: a write to a non-visible bank lands correctly and the page
      does not move; two banks automated at once do not oscillate it; **an
      open modulation drill-down survives a cross-bank write** — this is the
      defect the framework route was chosen to prevent, so prove it.
      Positive control: the same assertion must show the page DOES move on
      an operator-driven selection.
- [ ] 2.5 OPERATOR DECISION: does the visible bank persist in plugin session
      state? The `sessionExtras` mechanism already exists. Decide, implement,
      test.

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
      changing it.
- [ ] 5.3 Leave the meaningful zeros alone: Peak gain, Fold, Scoop depth,
      phase-modulation depth, ring-mod depth, Grace. Assert in tests that
      each still reaches true zero.
- [ ] 5.4 OPERATOR SMOKE: every number in designs C and D is derived, not
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
- [ ] 6.3 OPERATOR DECISION: `FroggersAudioRoutingTests.cpp` keeps bare
      letter-number labels as running cross-reference shorthand, including
      inside runtime diagnostic strings. Rename the scheme to something
      self-explanatory, or record it as intentional.

## 7. Documentation (design F)

- [ ] 7.1 Delete `SIM_MANUAL.md` and repair every inbound link
      (`MANUAL.md`, `DAISY_MANUAL.md`, `README.md` all point at it).
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

## 9. Close

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
- [ ] 9.5 Archive with spec sync. Sync only what is delivered.
