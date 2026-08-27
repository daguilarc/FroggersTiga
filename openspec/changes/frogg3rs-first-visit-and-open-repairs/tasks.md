# Tasks — `frogg3rs-first-visit-and-open-repairs`

Takes over the open items of `frogg3rs-web-release-repair` and absorbs its
`form-control-width-deferred.md`. That change delivered its sections 1, 2, 4, 5
and 6; nothing here re-does delivered work.

Gates: `cd app && nice make -j2 test` (310/310 as shipped); `app/vst` ctest
(3/3); browser e2e (47 specs, 46 passing + the `[pages]` failure item 1 is
about). Never above `-j2`, always `nice`, never two builds at once.

Sheaf's `projects/synth` gate for anything touched there. On this machine two
`braid4_*_deadline*` tests fail deterministically and more fail under load —
confirm any failure is only that family before reading a Sheaf run as green.

**Read a gate's real exit code.** A backgrounded `make ... ; echo` reports the
echo's status, not make's. Capture `NAME_EXIT=$?` and read it, and count
`[FAIL]`/`error:` in the log. A gate was called green twice this way.

## 1. The first visit

- [ ] 1.1 Reproduce it deliberately before changing anything. It is load
      sensitive: it failed at a 45s timeout inside the full 47-test run and
      passed in 656ms alone. Find a repro that fails reliably — CPU throttling
      via CDP, an artificial load, or a delayed service-worker response — and
      say which. A fix verified only against the fast path proves nothing.
      POSITIVE CONTROL: the run must show the boot panel with "SharedArrayBuffer
      transfer requires self.crossOriginIsolated", which is the exact snapshot
      captured on 2026-08-27. If it does not, the repro is not this bug.
- [ ] 1.2 The shim publishes whether an isolation attempt is still owed;
      `site-boot.mjs` reads it. SINGLE-SOURCE the guard: `RELOAD_GUARD_KEY`
      (`coi-serviceworker.js:81`) belongs to the shim and its string must not be
      re-derived in the boot path. Set the flag SYNCHRONOUSLY in the shim's page
      branch, before its async `register()`, or the module will read it before
      it exists.
- [ ] 1.3 While an attempt is owed: do not boot, and do not call `fail()`. Once
      the guard has already fired, boot exactly as today — the page has had its
      reload and any remaining failure is real. Do not weaken the existing error
      panel for any other condition; it is the only thing standing between a
      broken build and a silent blank page.
- [ ] 1.4 Decide, by rendering rather than argument, whether suppressing the
      boot leaves a cold first visit looking blank for long enough to need a
      neutral starting state. Report what the page actually shows during the
      window, measured.
- [ ] 1.5 An e2e assertion in the `[pages]` project — the one that runs
      `serve-site.mjs --no-isolation-headers`, which is what reproduces GitHub
      Pages — that a first visit never paints the boot-failure panel.
      POSITIVE CONTROL: red against today's build under 1.1's repro.
- [ ] 1.6 Confirm the real failure still surfaces: with the service worker
      forced to fail registration, the page must still show the panel rather
      than hanging silently. This is the half 1.3 puts at risk.

## 2. Measure what the readout does, or back it out

- [ ] 2.1 THIS IS THE CONTROL THAT WAS SKIPPED, and it decides whether the
      shipped one-second window is the fix or a plausible story. Log the
      PUBLISHED SAMPLES over time — not the held maximum — under Chromium CPU
      throttling (`Emulation.setCPUThrottlingRate` over a CDP session, which
      Playwright exposes) at rates approximating a phone.
      FIRST ATTEMPT: nothing in either repo opens a CDP session today, so
      failure to get a throttled session is expected discovery, not a
      regression. Say so plainly rather than substituting a proxy measurement.
- [ ] 2.2 Report the distribution: how many windows exceed 100%, whether they
      cluster at startup or recur, and the steady-state figure. The prediction
      to test is a 50-80% steady state with brief excursions crossing 100%.
- [ ] 2.3 If it shows SUSTAINED windows above 100%, the shipped change is not
      the fix: the cause is DSP cost, the readout is honest, and this item
      says so and opens that instead. Do not reinterpret a refutation as a
      partial success.
- [ ] 2.4 Whichever way it lands, record the measured numbers in Sheaf's
      `synth-runtime-ui` spec or its change, so the next reader inherits a
      measurement instead of the arithmetic.

## 3. A captioned control declares its width

- [ ] 3.1 Sheaf, on the PR #9 branch. The fix REQUIRES `PortableUIBuilders.hpp`
      (`FinishControl`, :438-480, the hardcoded `.main = Weight(1.0f)` at
      :465-467). A previous attempt was scoped to `PortableUILayout.hpp` +
      `RuntimePages.hpp` and correctly refused, because the distinction is
      erased upstream of both.
- [ ] 3.2 Decide how a captioned control says "size me to my content" when
      `.main` on its style already means the row's height. Read how `.cross`
      is used for `BackButton` (`RuntimePages.hpp:435-439`) before inventing a
      field. If the answer is a new field on `ControlStyle`, say so and name
      what else will want it.
- [ ] 3.3 Device selectors and text fields keep the column. Assert it on the
      same page, so a regression there fails loudly rather than quietly.
- [ ] 3.4 Tests in `tests/portable_ui_layout_tests.cpp`, whose runner list in
      `int main()` (:1368) is HAND-MAINTAINED — a test without a call line
      silently never runs.
      POSITIVE CONTROL: red against today's full-width behaviour.
- [ ] 3.5 `tests/portable_ui_tests.cpp` has a byte-identical-tree assertion
      (`:1618-1648`) that builds both sides through the current pipeline. If
      the fix breaks it, STOP and report rather than editing the assertion.

## 4. MIDI, and the interface underneath it

- [ ] 4.1 Establish first whether this is wanted now or is a placeholder. It is
      the only item here that adds capability rather than repairing something
      shipped, and it should not be built because it was listed.
- [ ] 4.2 If wanted: separate "supply a context" from "activation happened" in
      Sheaf's launcher, so `midiAccess` can arrive without a lease asserting a
      gesture. The current coupling is `main.ts:211-219` gating auto-start on
      `options.midiAccess` being present.
- [ ] 4.3 Whatever lands must keep the site's consent default: nothing starts on
      load, and `desktop-layout.spec.mjs`'s "no audio starts on load" must pass
      for the right reason, not because a permission was denied.

## 5. The duplications

- [ ] 5.1 The disabled greys: one decision for both palettes, or a written
      reason they differ that lives somewhere better than a comment at one of
      the three sites. Enumerate by OPERAND — grep the `Rgb(` literals, not the
      names — before deciding, and report FOUND vs CHANGED.
- [ ] 5.2 `RollingMax` vs `RollingBuffer<N>`: compare them properly and either
      consolidate or record why they stay separate. The differences are real
      (template vs runtime capacity, DSP vs message thread, Min-and-Max vs Max),
      so this may correctly end in "no change" — but it ends in a comparison,
      not in the absence of one.
- [ ] 5.3 The status-line selector written in both repos. The node id is
      single-sourced (`RuntimePages.hpp:57`); decide whether test support can
      cross the submodule boundary, and if it cannot, say that once rather than
      leaving it to be rediscovered.

## 6. Operator

- [ ] 6.1 OPERATOR: the disabled cell's grey by eye — `Rgb(90, 96, 100)` on the
      body stroke, no value arc. Both are first guesses.
- [ ] 6.2 OPERATOR: a real microphone reaching External Audio on the published
      site, which no headless test can confirm.
- [ ] 6.3 OPERATOR: the load readout on a phone, after item 2 measures it.

## 7. Nothing else moved

- [ ] 7.1 App suite with counts; `app/vst` ctest; Sheaf gate with the braid4
      caveat stated explicitly.
- [ ] 7.2 ONE republish, then the full e2e suite, idle AND under load — the
      under-load run is not optional here, it is what item 1 is about.
- [ ] 7.3 Pin bump as its own step after PR #9's head moves.
- [ ] 7.4 Both trees clean and pushed.
