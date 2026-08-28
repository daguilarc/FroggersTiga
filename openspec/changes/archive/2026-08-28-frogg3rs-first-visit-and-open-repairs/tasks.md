# Tasks — `frogg3rs-first-visit-and-open-repairs`

Takes over the open items of `frogg3rs-web-release-repair` and absorbs its
deferred form-control-width requirement, now a delta under `specs/`. That
change delivered its sections 1, 2, 4, 5 and 6; nothing here re-does delivered
work.

The work reports beside this file are named `step-a`/`step-b1`/`step-b2`/
`step-c`/`step-c2`/`step-d`, which was dispatch bookkeeping, not a second
numbering. They map onto the sections below: a = section 0, b1/b2 = section 1
(plus task 0.4), c/c2 = section 3, d = section 2. `preflight-section3.md` is
the preflight for section 3's second attempt. Sections are the only numbering
that counts.

Preflight ran before any of this and its findings are in `preflight.md`. Where
a task below contradicts an earlier draft, preflight is why; the finding is
cited inline.

Gates: `cd app && nice make -j2 test` (310/310 as shipped); `app/vst` ctest
(3/3); browser e2e (47 specs, 46 passing + the `[pages]` failure item 1 is
about). Never above `-j2`, always `nice`, never two builds at once.

Sheaf's `projects/synth` gate for anything touched there: `make -C
External/Sheaf/projects/synth test`, which builds and runs
`portable_ui_tests` and `portable_ui_layout_tests` among ~25 binaries
(`Makefile:233,254-255`). On this machine two `braid4_*_deadline*` tests fail
deterministically and more fail under load — confirm any failure is only that
family before reading a Sheaf run as green.

**The e2e never rebuilds the site.** `playwright.config.mjs:53-55` says so, and
both `webServer` entries serve `../dist/site`. Any edit under
`app/browser/site/` needs `node package-catalog.mjs` from `app/browser/`
(`app/browser/Makefile:23`) before the suite can see it. Item 1's positive
controls are a red run against today's build and a green one after, which means
a rebuild between them.

**Read a gate's real exit code.** A backgrounded `make ... ; echo` reports the
echo's status, not make's. Capture `NAME_EXIT=$?` and read it, and count
`[FAIL]`/`error:` in the log. A gate was called green twice this way.

**Nothing here installs anything.** Playwright is present in both suites
(`app/browser/e2e/package.json` pins 1.60.0; Sheaf's browser suite 1.62.0) and
`~/Library/Caches/ms-playwright` already holds `chromium-1223`. A task that
appears to need an install is a task that has gone wrong — stop and say so.

## 0. Close the predecessor first

Preflight P11. Two active changes holding the same seven items is the defect
§13 names. This section runs BEFORE section 1 so the spec work later in this
change lands on a synced baseline rather than on top of an unsynced one.

- [x] 0.1 Check off `frogg3rs-web-release-repair`'s shipped ledger against the
      commits its own text cites — `ff1d110`, `0acbe93`, `260b5cf`, `d09e8f7`,
      `faf9db0`, and Sheaf `80d9f4bb`. Verify each section against the TREE,
      not against the commit message: section 1 is `site-boot.mjs:94,105` plus
      `e2e/audio-activation.spec.mjs`. Anything the tree does not confirm stays
      unchecked and is reported here, not ticked to tidy the file.
- [x] 0.2 Its three spec deltas were never synced —
      `openspec/specs/froggers-browser-package/spec.md` carries nothing about
      isolation, the lease, or the audio context. Before syncing, FIX the
      delta's own opening claim: `specs/froggers-browser-package/spec.md:5`
      says "the lease is what carries the audio context and MIDI access", and
      item 4 refutes the MIDI half. Sync the corrected deltas.
- [x] 0.3 Archive it, leaving genuinely-open items open — that is normal here
      (73 archived changes, many with dozens unchecked). What must NOT survive
      is a redirect pointing at a change that no longer lists the item.
      Cross-check every one of its seven redirected items against this file
      before archiving.
- [x] 0.4 §12.0 hygiene on the tree this change touches. The sweep found ONE
      item and it is a false statement in shipping source:
      `app/browser/site/site-boot.mjs:37-38` says "Browser MIDI still has no
      path in -- it arrives only with a lease -- and remains unreachable here."
      Item 4 refutes it. Correct it there; do not leave it for item 4 to
      remember. Everything else in `app/browser/site/`, `app/browser/e2e/` and
      `app/browser/`'s scripts swept clean: every script has an invoker
      (`pages.yml`, `app/browser/Makefile`, `local-smoke.sh`), every spec is
      matched by a project, every helper export is consumed, no stale paths, no
      scratch files.

## 1. The first visit

- [x] 1.1 Reproduce it deliberately before changing anything. It is load
      sensitive: it failed at a 45s timeout inside the full 47-test run and
      passed in 656ms alone. THE 2026-08-27 SNAPSHOT IS GONE —
      `app/browser/e2e/test-results/` holds only `.last-run.json`, reading
      `failedTests: []` — so this reproduces the failure rather than inspecting
      it. Find a repro that fails reliably — CPU throttling via CDP, an
      artificial load, or a delayed service-worker response — and say which.
      A fix verified only against the fast path proves nothing.
      POSITIVE CONTROL: the run must show the boot panel with "SharedArrayBuffer
      transfer requires self.crossOriginIsolated", which is the exact snapshot
      captured on 2026-08-27. That string is the browser's own `DataCloneError`
      text, not ours — it appears in the tree only in comments
      (`serve-site.mjs:119`, `coi-serviceworker.js:12`), so do not expect to
      grep for it in source. If the panel does not show it, the repro is not
      this bug.
      RESULT: reproduced 10/10 by CPU throttling over a CDP session —
      `context.newCDPSession(page)` then
      `Emulation.setCPUThrottlingRate({rate: 20})` before `page.goto("/")`,
      against the `pages` project. Positive control satisfied every run:
      `.boot-error-detail` read "Failed to execute 'postMessage' on 'Worker':
      SharedArrayBuffer transfer requires self.crossOriginIsolated."
      Mechanisms 2-4 (delayed worker response, artificial load, full suite)
      were not needed. Each `test()` gets a fresh context, so every run is a
      genuine first visit with empty `sessionStorage`.
      UNTHROTTLED, the same navigation failed 2/5 in one batch and 0/5 in
      another — with the identical panel and text. That machine was carrying
      this session's own agents at the time, so it shows the race fires under
      ordinary load without artificial throttling; it does NOT establish a
      rate for an idle machine, and nothing here should be read as one.
      Harness: `app/browser/e2e/first-visit-race.spec.mjs`, added to
      `PAGES_SPECS` (`playwright.config.mjs:37`).
- [x] 1.2 The shim publishes whether an isolation attempt is still owed;
      `site-boot.mjs` reads it. SINGLE-SOURCE the guard: `RELOAD_GUARD_KEY`
      (`coi-serviceworker.js:81`, `"frogg3rs-coi-reloaded"`) belongs to the shim
      and its string must not be re-derived in the boot path. Publish
      SYNCHRONOUSLY in the shim's page branch, before its async `register()`,
      or the module will read it before it exists.
      PREFLIGHT P2 — IT MUST BE SETTLEABLE, NOT A WRITE-ONCE FLAG. The page
      branch has three exits after that set where no reload ever comes:
      `register()` rejects (`:119`), a controller is already present
      (`:97-99`), and `ready` resolves but `reloadOnce()` declines because the
      guard already fired (`:83-90`). A boolean sampled once would suppress
      those pages forever. The shim must be able to say "no attempt is coming",
      and every one of those three exits must say it.
      RESULT — THE PROPOSAL'S MECHANISM WAS WRONG, see `step-b2-report.md`.
      There was no reload to lose the race to. The worker calls
      `skipWaiting()` (`coi-serviceworker.js:31`) and `clients.claim()`
      (`:34`), so on a first visit it often claims the document before
      `register()` resolves — leaving the page CONTROLLED and still NOT
      ISOLATED, because its own navigation response was already served
      header-less. The shim keyed its reload on `controller`, read that as
      "isolation holds", and never reloaded. The panel was permanent, not
      transient. Suppression alone therefore made it WORSE (0/5, a blank page
      instead of a false panel); the shim's reload condition had to be
      corrected too — `crossOriginIsolated` is fixed for a document's
      lifetime and this branch only runs when it is already false, so once a
      worker is active the only remedy is a fresh navigation, whoever
      controls the page. The `controller` checks are gone. 5/5 after.
- [x] 1.3 While an attempt is owed: do not boot, and do not paint. Once the
      attempt has settled either way, behave exactly as today — the page has had
      its reload, and any remaining failure is real.
      PREFLIGHT P1 — THERE ARE TWO PAINTERS, NOT ONE. `fail()`
      (`site-boot.mjs:61-79`) is reached from `boot().catch` (`:137`) and this
      module's own listeners (`:156-162`); `renderBootError()`
      (`index.html:120-145`) paints the identical panel from its own listeners
      (`:146-153`). The inline one cannot import — that is the whole point of
      it (`index.html:100-102`) — so do NOT try to collapse them; the
      duplication is load-bearing. Suppression must reach both, or it is
      written once and open-coded once. FOUND 2; report CHANGED.
      Do not weaken either panel for any other condition; they are the only
      thing standing between a broken build and a silent blank page.
- [x] 1.4 Decide, by rendering rather than argument, whether suppressing the
      boot leaves a cold first visit looking blank for long enough to need a
      neutral starting state. Report what the page actually shows during the
      window, measured.
      MEASURED: the page is never blank. The site shell — heading, footer,
      download and manual links — is static HTML that paints immediately and
      stays. Only `#synth-root` is empty (childElementCount 0, height 0) for
      ~0.6s unthrottled and ~2.5s at 20x throttling, then fills in one step.
      Reload lands at 330ms / 592ms. No neutral state added; none is needed.
- [x] 1.5 An e2e assertion in the `[pages]` project — the one that runs
      `serve-site.mjs --no-isolation-headers` (`playwright.config.mjs:61`),
      which is what reproduces GitHub Pages — that a first visit never paints
      the boot-failure panel. Note the project currently matches only
      `blank-frame.spec.mjs` (`PAGES_SPECS`, `:37`); a new spec file needs
      adding there or it silently never runs.
      POSITIVE CONTROL: red against today's build under 1.1's repro, green
      after — with `node package-catalog.mjs` between the two runs, or the
      second run tests the first run's bytes.
- [x] 1.6 Confirm the real failure still surfaces: with the service worker
      forced to fail registration, the page must still show the panel rather
      than hanging silently. This is the half 1.3 puts at risk, and 1.2's
      settleable state is what makes it reachable — assert it against BOTH
      painters.

## 2. Measure what the readout does, or back it out

### SECTION 2 OUTCOME — VOID AS A PHONE MODEL, ESCALATES TO 6.3

Ran with the RIGHT instrument: raw `deadlineMicrounits` off `audio-worklet-stats`,
real frogg3rs DSP staged into Sheaf's harness, 2579 samples over four rates,
audio confirmed live. Full numbers in `step-d-report.md`.

The run is VOID in §8.1's sense — the controlling quantity moved the WRONG WAY.
`Emulation.setCPUThrottlingRate` throttles the renderer's main thread; the
AudioWorklet has its own real-time audio thread and is not subject to it.
Proof from the run's own data: audio held 375.9 blocks/s against a 375.0
real-time rate IDENTICALLY at 1x and 20x (a genuinely 20x-throttled worklet
would underrun continuously), and the load FELL as throttling rose (27.6% ->
18.7% -> 12.8%), because starving the main thread makes it compete less with
audio.

Zero samples above 100% at any rate, including startup. Steady state 12-28% on
this Mac. So the run never produced the phenomenon either explanation is FOR,
and cannot distinguish them.

The shipped one-second window is therefore NEITHER confirmed NOR refuted. Its
positive control still has not been run, and no headless desktop configuration
can run it. Only a real phone can — 6.3 is now the sole instrument, not a
confirmation of a headless result.

PROCESS COST, recorded so it is not repeated: this was knowable by READING.
Preflight P6 traced whether the SAMPLES were reachable and never asked whether
the THROTTLE could reach the thread under measurement. §1 says name what would
change your answer before you trace; that question would have, and it was one
search away.

- [x] 2.1 THIS IS THE CONTROL THAT WAS SKIPPED, and it decides whether the
      shipped one-second window is the fix or a plausible story. Log the
      PUBLISHED SAMPLES over time — not the held maximum — under Chromium CPU
      throttling at rates approximating a phone.
      PREFLIGHT P6 — ONLY HALF OF THIS IS A FIRST ATTEMPT.
      Reachable today, no new production code: the samples already leave the
      runtime via `AudioWorkletDeadlineMeter::SampleMicrounits`
      (`BrowserRuntime.hpp:318-326`) → `AudioWorkletDeadlineMicrounits`
      (`:636-639`) → `synth_browser_audio_worklet_deadline_microunits`
      (`browser/cpp/BrowserRuntimeAbi.cpp:120-123`) → the `audio-worklet-stats`
      worker message (`browser/src/worker.ts:264,549-552`). Three specs already
      poll it: `audio-flow.spec.ts:825`, `audio-input.spec.ts:64-98`,
      `first-party-apps-smoke.spec.ts:182-266`. Diff against those.
      GENUINELY A FIRST ATTEMPT: the throttling.
      `Emulation.setCPUThrottlingRate` over `newCDPSession` has zero hits in
      either repo, so there is no working invocation to diff against. Failure
      to get a throttled session is expected discovery, not a regression. Say
      so plainly rather than substituting a proxy measurement.
      RUN IT IN SHEAF'S BROWSER SUITE. frogg3rs's site e2e drives a page and
      cannot address the worker, so it cannot read `audio-worklet-stats` at
      all.
- [x] 2.2 Report the distribution: how many windows exceed 100%, whether they
      cluster at startup or recur, and the steady-state figure. The prediction
      to test is a 50-80% steady state with brief excursions crossing 100%.
      State which window each number is: the meter publishes every 100 ms
      (`kPublishWindowMicros`, `BrowserRuntime.hpp:329`), while the display's
      one second is a `RollingMax` over UI frames
      (`RuntimeMainComponent.hpp:284-285`). Reporting one as the other is how
      this section gets a wrong answer that looks right.
- [x] 2.3 If it shows SUSTAINED windows above 100%, the shipped change is not
      the fix: the cause is DSP cost, the readout is honest, and this item
      says so and opens that instead. Do not reinterpret a refutation as a
      partial success.
      NOTE, ADDED AFTER STEP 0 RAN: the requirement "The runtime chrome reports
      its load honestly" is now in the MAIN spec
      (`openspec/specs/froggers-sheaf-runtime-app/spec.md`), synced from the
      predecessor. A refutation here has to reach that file, not only this
      change — the requirement itself may well survive (it asks for an honest
      readout, not for a one-second window), but leaving it unexamined after a
      refutation is how the derivation becomes settled fact.
- [x] 2.4 Whichever way it lands, record the measured numbers in Sheaf's
      `synth-runtime-ui` spec or its change, so the next reader inherits a
      measurement instead of the arithmetic.

## 3. A captioned control declares its width

**Operator decision, 2026-08-27: the button is sized to its label and pinned to
the control column's left edge.** A first pass built this, then reverted it
because `criteria::ColumnAlignment` asserts equal widths within a column. That
revert was wrong: the criterion is an inferred invariant frozen into a test, not
a stated requirement, and it is what moves. See the delta under
`specs/froggers-sheaf-runtime-app/`, which now carries the requirement plus the
MODIFIED alignment requirement. `step-c-report.md` holds the first pass's
findings, which still apply.

Two findings from that pass are load-bearing here and were paid for with a
build:
- `ApplyFormGrid`'s unconditional fill at `:845` does TWO jobs — it sizes
  weighted cells AND rescues a row whose cells overrun after label-column
  realignment. `RequireContainerHoldsItsChildren`'s own comment documents that
  contract. Merely skipping the fill for non-weighted cells broke
  `TestFormGridAlignsLabelAndControlColumns` with a container overflow. The
  rescue must survive. PREFLIGHT TRACED THE REMEDY: the 459.72 is a ComboBox's
  fixed 160px leaf intrinsic width (`PortableUIMetrics.hpp:50-51`) plus the
  shared label-column offset. Clamping — `min(resolvedWidth, remaining)` for
  non-weighted cells, `remaining` for weighted — cannot overflow, because
  `controlOffset + min(...) <= contentEdge` holds always. It must be a literal
  `min()`, not a skip.
- The Sheaf gate's `test` target is ONE linear recipe and the pre-existing
  `braid4_*_deadline*` failures abort it BEFORE the portable-UI binaries run,
  so a log reading "clean except braid4" can hide later failures. Run those two
  binaries directly.

- [x] 3.1 Both files, because each alone is a no-op. `PortableUIBuilders.hpp`:
      add one `Extent controlWidth` to `ControlStyle` (`:25-39`) defaulting to
      `Extent::Weight(1.0f)`, and have `FinishControl` use it at `:466` instead
      of the hardcoded weight. `PortableUILayout.hpp`: `ApplyFormGrid` (`:845`)
      stops overwriting a declared width — while STILL holding every cell to
      the row, per the finding above and the delta's third scenario.
- [x] 3.2 No new `Extent` kind: `Extent::Intrinsic()` / `Mode::Intrinsic`
      already exist (`PortableUILayout.hpp:32,39`) and are `LayoutOptions::main`'s
      default (`:54`). `FormButton` (`RuntimePages.hpp:444-450`) sets
      `style.controlWidth = Extent::Intrinsic()` — the NEW field, NOT
      `style.layout.main`, which is spent on the row. Preflight flagged the
      earlier wording as easy to misread into the wrong field.
- [x] 3.3 `Toggle` IS `FormButton` (`RuntimePages.hpp:456`), so editing
      `FormButton` silently changes the four Sync toggles
      (`RuntimePages.hpp:779-794`). THE OPERATOR ASKED ABOUT THE BUTTON, NOT
      THE TOGGLES. Carve `Toggle` out so it keeps the column, and say so — do
      not silently restyle a second page. `Field` (`:459-467`) and the device
      selectors change nothing.
- [x] 3.4 `criteria::ColumnAlignment` (`tests/support/VisualCriteria.hpp:430`)
      checks TWO things per column: shared `x` and equal `width`. KEEP the
      shared-`x` check — that is the alignment, and it is what "left-hand side"
      means. RETIRE the equal-width check, and update the criterion's own
      comment (`:417-422`) which currently states equal width as the rule.
      PREFLIGHT: callers are `portable_ui_layout_tests.cpp:1297` and `:1304`
      (both inside one test) and `portable_ui_tests.cpp:2604` — the
      `:1333,1340` in an earlier draft were stale.
      PREFLIGHT CLEARED THE RISK: `TestColumnAlignmentCheckCatchesAControlLeavingItsColumn`
      (`:1281-1309`) misaligns its tree with `bounds.x += 4.0f` ONLY and never
      touches width, so retiring the width half does not weaken it. That test
      must still pass UNTOUCHED — if it goes red, STOP, because the premise
      this section rests on was wrong.
- [x] 3.5 Tests in `tests/portable_ui_layout_tests.cpp`, whose `int main()`
      (`:1368-1415`) is a HAND-MAINTAINED list of 45 calls — a test without a
      call line silently never runs. Assert: the button is near label width;
      its left edge equals the selectors'; a `Field` still spans the column; a
      content-sized control cannot overflow its row.
      POSITIVE CONTROL: red against today's full-width behaviour.
- [x] 3.6 COVERAGE GAP, preflight Q4: three sites set `showInputRetry` true,
      but NONE asserts the retry button's geometry on the real Audio page — the
      only proof would be the synthetic layout test. Add an assertion on the
      ACTUAL Audio page that the button is narrower than the selectors and
      shares their left edge. Without it this ships verified only in the
      abstract.
- [x] 3.7 `tests/portable_ui_tests.cpp:1618-1650`'s byte-identical-tree
      assertion did NOT fire in the first pass — the reference copies the
      builder call sequence, not frozen numbers. If it fires now, STOP and
      report rather than editing it; that is a different assertion from the one
      the operator decided about.
- [x] 3.8 Gate: build and run `portable_ui_tests` and `portable_ui_layout_tests`
      DIRECTLY, not via `make test`, which aborts at braid4 before reaching
      them. `nice make -j2` only, foreground, never two builds at once.

## 4. MIDI in the browser

### SECTION 4 OUTCOME — THE PATH WORKS, AND HEADLESS CAN PROVE IT

Measured, not assumed. After a first in-app action the status reads
`audio:online; midi:offline`, and a direct `requestMIDIAccess({ sysex: true })`
rejects with `NotAllowedError: Permission to use Web MIDI API was not granted.`
So the API exists in headless Chromium, the path RUNS, and `offline` is a
permission verdict — not a missing path, and not a broken one.

Playwright CAN grant it, which the plan treated as unlikely.
`grantPermissions(["midi"])` alone is NOT enough — the manager asks for
`{ sysex: true }` (`midi.ts:114`), so Chromium needs `midi-sysex` too. With
both granted the status reaches `midi:online`. That makes the two assertions
each other's control: the grant is what moves the value, so the test
discriminates rather than passing for free.

`inputs=0` under headless — `online` means access granted and the manager
started, not that a controller is attached. Only 4.5 can show that.

- [x] 4.1 `midi-activation.spec.mjs`, registered in `DESKTOP_SPECS`. Two
      assertions: the status always carries a `midi:` half (a regression that
      dropped it, or never reached `startFromUserActivation`, fails here), and
      it reaches `midi:online` once Web MIDI is permitted. REPORTED what it
      actually says rather than assuming: `audio:online; midi:offline`
      ungranted, `audio:online; midi:online` granted.
- [x] 4.2 Established by running: headless CAN demonstrate this, with
      `["midi", "midi-sysex"]`. `midi` alone leaves it offline. Both facts are
      in the spec's own header comment so the next reader does not repeat the
      experiment.
- [x] 4.3 The status is `online` when permitted, so MIDI works and the
      predecessor's "unreachable without a lease" claim was simply wrong. That
      claim was also corrected in shipping source at `site-boot.mjs:37-38`
      (task 0.4) and in the predecessor's spec delta before it was synced
      (task 0.2). Nothing to repair.
- [x] 4.4 No lease was added. The spec's header comment says why, so the next
      person to look at this does not add one.

- [ ] 4.5 OPERATOR: a real controller against the published site. Headless CAN
      now grant Web MIDI (above), so what remains unproven is narrower than the
      plan assumed: granted access reports `inputs=0` because headless exposes
      no devices. This check is therefore about a real DEVICE being enumerated
      and its messages reaching the runtime — not about whether the path works,
      which is now asserted in CI.

## 5. The duplications

- [x] 5.1 PREFLIGHT P7 — THE PREMISE WAS LOOSE AND §7 DOES NOT BITE. The three
      greys are three DIFFERENT values, each appearing exactly once:
      `Rgb(90, 96, 100)` (`app/FroggersUiSurface.hpp:1994`), `Rgb(125, 132, 138)`
      (`kDisabledText`) and `Rgb(45, 49, 53)` (`kDisabledButton`)
      (`RuntimePageStyle.hpp:14,20`). Enumerated by operand across both repos:
      FOUND 197 `Rgb(` sites, 92 distinct triples, 27 repeated — and none of
      the repeats is a disabled grey. So this is three colours expressing one
      concept, not one value written three times.
      The decision this task asks for already exists in prose at
      `app/FroggersUiSurface.hpp:1991-1993`. The work is to give it a home a
      reader finds without already knowing where to look — not to unify the
      values, which that prose explicitly says is a palette decision made once
      for both.
      DONE: the decision now lives as a requirement, "The encoder grid owns its
      own colour language", in `specs/froggers-modulation-slate/spec.md`. FOUND
      197 `Rgb(` sites / 92 distinct triples / 27 repeated, CHANGED 0 — there
      was no duplicated value to eliminate, and the requirement says so where a
      reader meets it rather than in a comment at one of the three sites.
- [x] 5.2 PREFLIGHT P8 — THE COMPARISON PRODUCES A FINDING NEITHER CHANGE
      PREDICTED. `RollingBuffer<N>` (`DspBuffers.hpp:98-117`) has ZERO
      production instantiations; its only one anywhere is
      `tests/dsp_tests.cpp:1294`. `RollingMax` (`MidiConfigViewModel.hpp:36-63`)
      has one, `RuntimeMainComponent.hpp:672`.
      It is NOT dead code: `DspBuffers.hpp` is a shipped library header
      (`projects/synth/Makefile:39` `DSP_HEADERS`, included by
      `apps/braid-4/Braid4Core.hpp:9`) on a branch named
      `fix-out-of-tree-app-gaps`, where an out-of-tree app author is a real
      consumer — §12.0's fork lands on keep.
      The real finding is a defect: `RollingMax::Max()` iterates `filled_`
      (`:50`) and is correct on a partially-filled window, while
      `RollingBuffer::Min()`/`Max()` (`:110-116`) run over the whole
      `std::array` including never-written zero slots. `Min()` on any
      non-negative signal returns 0 until the buffer wraps. Decide whether to
      fix that, and record the decision either way. Consolidation is not the
      question any more.
      DONE — AND THE PREMISE WAS STILL TOO NARROW. It is dead code: §12.0's own
      test is invocation by bare name AND by path, and there is none. The
      "out-of-tree consumer" that kept it alive was inferred from a branch name
      and never cited, which §12.0 calls a claim to verify, not to assume.
      Enumerating the batch it arrived with rather than the one symbol the plan
      named found THREE dead primitives, not one: `RollingBuffer`,
      `BoundedAudioBuffer` and `BufferResampler`, each appearing exactly once
      under `include/` — its own definition. `FirDecimator` and
      `OversampledOutputStage` from the same port are live (4 production uses
      each) and were left alone.
      Deleted all three plus the 5 test cases that existed only to exercise
      them, and the includes that became unused. `DspBuffers.hpp` 412 -> 195
      lines; `dsp_tests.cpp` 2999 -> 2908. Verified: `dsp_tests` 106/106,
      `braid4_system_tests` 29/29 (proving `Braid4Core.hpp:9`'s include still
      compiles), `braid4_deadline_tests` only the two pre-existing 96kHz
      timing failures. Detail in `step-e-report.md`.
      There is no `Min()` defect to fix, because there is no `Min()`.
- [x] 5.3 PREFLIGHT P9 — THE CLAIM WAS IMPRECISE. `e2e/helpers.mjs:78` writes
      the whole selector literally; Sheaf does not — `audio-input.spec.ts:84`
      passes the id through `synthNode()` (`browser/tests/helpers/fake-app.ts:472-474`).
      What is written twice is the ID `runtime.audio.status_line`, not the
      selector. The id is single-sourced in C++ (`RuntimePages.hpp:57`), which
      no JS test can reach. Decide whether test support can cross the submodule
      boundary, and if it cannot, say that once rather than leaving it to be
      rediscovered.
      DECIDED: IT CANNOT, and this is the once. The id is a C++ constant
      (`RuntimePages.hpp:57`) compiled into the runtime; neither JS suite can
      read it at any build step, because nothing emits it as data. Sharing the
      SELECTOR instead would mean Sheaf exporting test support across a
      submodule boundary, which makes frogg3rs's e2e depend on Sheaf's internal
      test helpers — a coupling that costs more than the duplicated string.
      Closing it properly needs the id emitted as a generated artifact both
      suites read, which is a build-pipeline change and does not belong here.
      Two suites hardcoding one id is the accepted cost. Do not re-raise it
      without that generated artifact on the table.

## 6. Operator

- [ ] 6.1 OPERATOR: the disabled cell's grey by eye — `Rgb(90, 96, 100)` on the
      body stroke, no value arc. Both are first guesses.
- [ ] 6.2 OPERATOR: a real microphone reaching External Audio on the published
      site, which no headless test can confirm. Same visit settles 4.5 if a
      controller is to hand.
- [ ] 6.3 OPERATOR: the load readout on a phone, after item 2 measures it.

## 7. Nothing else moved

- [x] 7.1 App suite 310 PASS / 0 FAIL across 10 binaries, `APP_EXIT=0` read
      from the log rather than the wrapper. `app/vst` ctest 3/3. Sheaf:
      `portable_ui_layout_tests` and `portable_ui_tests` both EXIT=0, `dsp_tests`
      EXIT=0 (106/106 after the deletion), `braid4_system_tests` 29/29. The only
      failures anywhere are the two pre-existing `braid4_*_deadline*` 96kHz
      timing tests, unchanged by this work. The Sheaf `make test` target was NOT
      used as the gate: it aborts at those braid4 failures before reaching the
      portable-UI binaries, so its log reads clean while hiding them.
- [x] 7.2 Rebuilt the wasm (`build-browser.sh`) and repackaged, &&-chained so a
      failed build could not stage a half-built site. New buildId
      `98cf9b9d...`, replacing `71c923e0...` — the layout fix is C++ compiled
      into the wasm, so repackaging alone would NOT have carried it.
      Full suite 51/51 idle, and 51/51 under artificial load (load average 6.5
      -> 8.1 against a ~3.3 baseline). The under-load run matters because the
      original failure was load-sensitive and first appeared as a 45s timeout
      inside a full-suite run.
- [x] 7.3 Sheaf pushed to `fork/fix-out-of-tree-app-gaps`,
      `80d9f4bb..5d95fcbe`, then the pin bumped as its own commit.
- [x] 7.4 Both trees clean and pushed. frogg3rs `6fe7085..e695ebb` on
      `origin/main`; Sheaf `80d9f4bb..5d95fcbe` on
      `fork/fix-out-of-tree-app-gaps`. Both report 0 dirty. Desktop app rebuilt
      from the bumped pin (`app/build-launcher/Frogg3rs.app`, 37.7MB signed
      binary — not the 16KB stub `make` alone produces) and the browser wasm
      rebuilt, so the layout fix is testable on both hosts.
- [x] 7.5 Done — `postflight.md`. Divergences reported strictly, and §7 re-run
      against the diff on every new symbol.
      ORIGINAL TEXT: POSTFLIGHT §13: re-run §7 against the DIFF, not the proposal. Every
      new field, constant, predicate and helper this change introduces —
      `ControlStyle`'s width field (3.2), the shim's published state (1.2) —
      gets a fresh enumeration of THAT concept across both trees. The question
      is not "did I implement the plan" but "what did writing this make
      redundant that was not redundant before."

## 8. Reported, not actioned

- [x] 8.1 REPORTING WAS THE DELIVERABLE, so this is done, not outstanding.
      `External/Sheaf/analysis/sdd-model-analysis/data/` holds 32 MB of AI
      session transcripts (`claude_sessions.json`, `codex_sessions.json`,
      `sessions_raw.json`, …). Under §12.0 these are correspondence artifacts,
      and they measurably degrade `grep` across the submodule — every
      enumeration run for this change's preflight had to exclude them by hand.
      Outside this change's touched tree, and removing them is a Sheaf decision
      with its own consumers to trace (`analysis/` may hold scripts that read
      them). 1,983 files are git-tracked there. Recorded so it is not
      rediscovered; NOT actioned here, and no one should read this as pending
      work — acting on it needs its own change against Sheaf.
