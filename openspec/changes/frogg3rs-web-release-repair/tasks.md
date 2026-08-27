# Tasks — `frogg3rs-web-release-repair`

Supersedes `2026-08-27-frogg3rs-mobile-control-placement`. Absorbs and deletes
the active change `frogg3rs-cpu-readout-says-it-is-a-peak`; section 5 is its
content. Its half-finished Sheaf edits were REVERTED rather than carried: the
submodule is clean, and section 5 starts from the shipped state. An unverified
edit left sitting in a working tree is how an unstable value reaches a build
nobody meant to make.

Gates: `cd app && nice make -j2 test` (308/308 before this change); `app/vst`
ctest (3/3); browser e2e (46/46). Never above `-j2`, always `nice`. The e2e suite
needs a wasm republish first. Sheaf's own `projects/synth` gate for the files
touched there.

**On this machine's braid4 deadline tests:** two fail deterministically, and up
to five fail once the machine has been building for a while — they are load
sensitive. Record which failed and confirm they are only `braid4_*_deadline*`
before reading a Sheaf run as green. Nothing in this change is reachable from
`braid4_deadline_tests.cpp` (it includes `Braid4Core.hpp` and `synth/Engine.hpp`,
not `RuntimePages.hpp`).

## 0. Supersede bookkeeping

- [ ] 0.1 Delete `openspec/changes/frogg3rs-cpu-readout-says-it-is-a-peak`.
      Confirm nothing else references it by name first.
- [ ] 0.2 The superseded change is already archived and its deltas are already
      in the live specs, so nothing is stranded — verify that rather than
      assume it, by checking `openspec/specs/frogg3rs-web-mobile-ux` still
      carries the requirements it landed.

## 1. The activation lease

- [ ] 1.1 Read `launchCatalogApplication` and enumerate EVERYTHING it passes
      that `site-boot.mjs` does not. The lease is the one that bit; report
      whether it is the only omission rather than fixing the one already found.
      This is the whole lesson of the defect — a subset copied by hand, with
      nothing saying what the full set was.
- [ ] 1.2 Construct an `ActivationLease` (`browser/src/activation.ts` exports
      it) in `site-boot.mjs` and pass it. Read its options and its `consume()`
      contract first: a lease is consumed once, and what it yields
      (`audioContext`, `midiAccess`) is what the launcher forwards.
      REWRITE THE HEADER COMMENT, do not just delete it. It currently argues
      correctly that no lease is needed for output activation and concludes
      that none is needed at all. The replacement has to say what the lease is
      for HERE — supplying the context capture requires — and keep the true
      part, that this page anchors activation to the first in-app action rather
      than to a Launch button. Deleting it would lose the reasoning and invite
      someone to remove the lease again on the same grounds.
- [ ] 1.3 MIND THE GESTURE. A browser `AudioContext` starts suspended without a
      user gesture, and `getUserMedia` needs one too. Establish where the
      existing site gets its gesture from — the app already starts audio from a
      UI action (`desktop-layout.spec.mjs`'s "no audio starts on load" pins
      that nothing starts without one) — and make the lease fit that path
      rather than inventing a second one.
- [ ] 1.4 Do NOT change the consent default. External audio staying off until an
      operator selects an input is specified, and a present microphone is not
      consent. This task makes the choice REACHABLE, nothing else. Assert that:
      after boot with no operator action, the external-audio sources are still
      disconnected.
- [ ] 1.5 An e2e assertion that the status line no longer reports
      `microphone requires the launch-owned AudioContext`.
      POSITIVE CONTROL: show it present against the current build first — it is
      in the operator's screenshot, so it is reproducible today.
- [ ] 1.6 Headless browsers do not prompt for a microphone. Decide what the e2e
      suite can honestly assert — that the context exists and the status is no
      longer `audioContextUnavailable` is testable; a granted-permission device
      list may not be. Say which, rather than writing a test that passes for the
      wrong reason. Playwright can grant permissions via a context option;
      check whether that reaches a fake device here before relying on it.

## 2. Disconnected sources render greyed

- [ ] 2.1 The spec delta is the change here; the code follows it. Apply the
      MODIFIED requirement first so the implementation has something to be
      correct against.
- [ ] 2.2 In the DrawFactory's `!state.connected` branch
      (`FroggersUiSurface.hpp:1915`), build from a dimmed COPY of the state with
      `connected` forced true, rather than returning `{}`. Do not touch Sheaf's
      `BuildEncoderDrawCommands`: this is one application's presentation choice
      and every other Sheaf app's disconnected cells must be unaffected.
- [ ] 2.3 No value readout on a disabled cell. The connected path strips and
      replaces Sheaf's trailing 60-command label block; the disabled path should
      not emit one at all. Verify the command count rather than assuming which
      branch produced what.
- [ ] 2.4 Clear the modulation and gesture indicator masks on the copy. An
      unavailable source showing modulation arcs would be claiming something.
- [ ] 2.5 Actions and depth parameters are untouched, and asserted so: a
      disconnected cell still dispatches nothing on press or drag, and still
      creates no depth parameter. This is the half of the old requirement that
      survives, and drawing the cell is exactly what puts it at risk.
- [ ] 2.6 Assert it is VISIBLY different from a connected cell, not merely
      non-empty — compare the emitted commands' colours against the connected
      case rather than counting commands.

## 3. Form-grid buttons

- [ ] 3.1 Sheaf, on the PR #9 branch. `ApplyFormGrid`
      (`PortableUILayout.hpp:844`) stretches every control cell. Decide where
      the fix belongs: the grid respecting a control's declared extent, or the
      button declaring one the grid honours. Read how a cell's width is used
      downstream before choosing — the combos must keep the column.
- [ ] 3.2 Cross-host by construction, so assert it in the portable layout tests,
      not in a browser test. `Toggle()` IS `FormButton()` and the Sync page
      builds four, so the Sync page is the desktop-visible case.
- [ ] 3.3 POSITIVE CONTROL: a test that fails against today's full-width
      behaviour, run red before the fix.
- [ ] 3.4 Verify no configuration page regressed: the device selectors still
      span the control column.

## 4. The Random S&H 6 background

- [ ] 4.1 `app/FroggersModulation.hpp:230`, construct `source6Visualizer_` with
      `drawBackground = false`. Sheaf already takes the argument
      (`GangedRandomLfoVisualizer.hpp:260`); no Sheaf change.
- [ ] 4.2 Assert lane 6 emits no full-node background fill, so the six lanes are
      consistent. The fill is `Color::Rgb(12, 14, 16)` over the whole node
      (`GangedRandomLfoVisualizer.hpp:63-64`) — assert on that, not on a command
      count.
      POSITIVE CONTROL: red against the default-constructed visualizer.
- [ ] 4.3 Check the other five lanes were never affected, and that lane 6 still
      draws its trace and dot.

## 5. The CPU readout (absorbed, redesigned)

- [ ] 5.0 FIRST, AND THIS DECIDES WHETHER THE REST OF THIS SECTION IS THE FIX
      AT ALL. "Above 100% on a phone" has two possible causes and the remedy is
      different for each:
      (a) a STARTUP TRANSIENT held 8.5 seconds, in which case shortening the
          window fixes what the operator is seeing outright; or
      (b) GENUINE RECURRING OVERRUNS, in which case shortening the window
          changes nothing about how often 100% is crossed -- it would be
          crossed because it is really being crossed, and an honest readout
          must keep saying so. The fix would then be DSP cost, not the readout.
      Do not assume (a) because it is the convenient one. The operator reports
      no audible dropouts, which is evidence FOR (a) -- an AudioWorklet that
      misses its deadline glitches audibly -- but "frequently" is their word for
      how often they see it, and a once-per-load transient is not frequent.
      MEASURE IT: log the PUBLISHED SAMPLES over time, not the held maximum,
      under Chromium CPU throttling (`Emulation.setCPUThrottlingRate` over a
      CDP session, which Playwright exposes) at rates that approximate a phone.
      Report the distribution: how many windows exceed 100%, whether they
      cluster at startup or recur, and what the steady-state figure is.
      The proposal now DERIVES the answer as (a): at a phone's 50-80% steady
      state, a 1.25x-1.9x transient crosses 100%, and each crossing then holds
      the readout for 8.5s. So this task is a POSITIVE CONTROL on that
      reasoning, not an open question. It passes if a throttled run shows the
      predicted steady state with brief excursions; it REFUTES the whole
      section if it shows sustained windows above 100%, in which case the cause
      is DSP cost and the readout is not the fix. Say which it found.

- [ ] 5.1 Shorten the hold. `RollingMax256` is used by this readout and its own
      tests and nothing else — four files, checked. Read it before choosing
      between a smaller capacity, a parameterised capacity, and a time-based
      window; `uiFrameHz` is per-application, so a frame count is a different
      hold on different hosts and time is the honest unit.
- [ ] 5.2 About one second (30 frames at the default 30Hz). Justify the number
      against what it has to catch: a spike must still be readable by eye.
      Record the figure and the reasoning, since this is a judgement not a
      derivation.
- [ ] 5.3 Whole percent, not tenths. A held maximum has no tenth-of-a-percent
      accuracy, and dropping the decimal is also what lets `"CPU 100%"` fit the
      96px column — verify the fit by RENDERING, not by `TextWidth` arithmetic:
      `"Controllers"` computes to 111px against a 96px column and ships looking
      correct, so the intrinsic figure is not what clips.
- [ ] 5.4 The label stays `"CPU"`. The relabelling to `"CPU max"` is dropped —
      it explained a misleading number instead of fixing it. Say so where the
      decision lives, so the next reader does not re-derive the discarded
      option.
- [ ] 5.5 Enumerate the consumers of the readout's TEXT before changing the
      format, by the format string and by the node id, not by the function
      name. The earlier pass found four assertions plus a sentinel in
      `audio-flow.spec.ts` comparing against the zero-value string — the kind a
      name-based grep misses.
- [ ] 5.6 `%.0f` rounds half to even: 12.5 renders 12, not 13. Do not pick test
      values on a `.5` boundary, or the test asserts the rounding mode as much
      as the format.
- [ ] 5.7 sru-2 still holds and SHOULD be re-read to confirm it: it requires a
      max over "a rolling window of recent UI frames" and fixes no number, so a
      shorter window satisfies it. If that reading is wrong, this item stops
      and the label option comes back.
- [ ] 5.8 A test that the sidebar's deadline node carries the new text, through
      `BuildSidebarTree` rather than `FormatDeadlineText` alone, and a test that
      the shortened window drops a stale peak within its own span.
      POSITIVE CONTROL for the window: a spike must still be visible one frame
      later, or the change has traded one wrong reading for another.

## 6. Docs for a reader, not a builder

- [ ] 6.1 `README.md`: the 229-line Daisy firmware build section leaves the
      README. Establish what of it is NOT already in `DAISY_MANUAL.md` before
      deleting anything, and move that rather than lose it.
- [ ] 6.2 The "Local Planning And Hygiene" section goes: OpenSpec artifact
      locations, agent git conventions and where shared source belongs are
      working process, not operator documentation.
- [ ] 6.3 What survives leads with the instrument and reaches the parameter
      reference without passing through build instructions. Build commands stay
      — someone has to be able to build it — but as a short section, not the
      body.
- [ ] 6.4 `MANUAL.md` re-read end to end against the same test: it was rewritten
      once already this session for tone, but not audited for whether a learner
      can follow it. Report what is still addressed to a developer.
- [ ] 6.5 Every link and path in both files resolves. The last pass found three
      dangling references and a build command for a deleted directory; that is
      the class of defect to sweep for, not just the ones already named.

## 7. Nothing else moved

- [ ] 7.1 App suite green with counts.
- [ ] 7.2 `app/vst` ctest 3/3.
- [ ] 7.3 Sheaf gate, with the braid4 caveat above stated explicitly.
- [ ] 7.4 ONE republish, then the full e2e suite, idle and under load.
- [ ] 7.5 Pin bump for the Sheaf work, as its own step, after PR #9's head moves.
- [ ] 7.6 Both trees clean and pushed.

## 8. Operator

- [ ] 8.1 OPERATOR: the greyed rendering, by eye — dim enough to read as
      unavailable, visible enough to read as present.
- [ ] 8.2 OPERATOR: microphone input actually reaching External Audio on the
      published site, which no headless test can confirm.
