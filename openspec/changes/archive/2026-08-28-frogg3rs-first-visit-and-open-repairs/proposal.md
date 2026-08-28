# Proposal — `frogg3rs-first-visit-and-open-repairs`

**Created 2026-08-27.** Takes over every open item of
`frogg3rs-web-release-repair`, which shipped its sections 1, 2, 4, 5 and 6 in
commits `ff1d110`, `0acbe93`, `260b5cf`, `d09e8f7`, `faf9db0` (and Sheaf
`80d9f4bb`). That change is finished except for what is listed here, and
absorbs its deferred form-control-width requirement, which now lives as a
proper delta in `specs/froggers-sheaf-runtime-app/spec.md` beside this file.

Two of these are defects that reached the published site. The rest are things
the previous change found, recorded, and did not act on.

## 0. Close the predecessor before building on it

`frogg3rs-web-release-repair` is still an ACTIVE change whose ledger reads 0
checked of 49, though its own text names the commits that shipped its sections
1, 2, 4, 5 and 6 and the tree agrees (`site-boot.mjs:94,105` carries the
`audioOptions.audioContext` fix; `e2e/audio-activation.spec.mjs` exists).
Archiving with open items is normal here — 73 archived changes, many with
dozens still unchecked — but archiving with NOTHING checked against
known-shipped work leaves no record of what shipped.

Its spec deltas were also never synced. `openspec/specs/froggers-browser-package/
spec.md` says nothing about isolation, the lease, or the audio context. And the
unsynced delta itself opens with "the lease is what carries the audio context
and MIDI access" — the claim item 4 refutes. Syncing it unedited would write a
refuted claim into the main spec, where the next reader would inherit it as
settled.

Two active changes holding the same seven items is the same defect as two
functions computing the same value. This closes one of them first.

## 1. The published site can fail on a first visit, and that is the visit that matters

GitHub Pages cannot send COOP/COEP headers, so isolation comes from
`app/browser/site/coi-serviceworker.js`. `index.html:156-157` loads that shim as
a classic script and `site-boot.mjs` as a module. A module script is deferred,
but the shim's work is asynchronous — `register().then(...)` and
`serviceWorker.ready.then(...)` (`coi-serviceworker.js:94-119`) — so on a first
visit `site-boot.mjs` runs and boots BEFORE the worker controls the page.

`crossOriginIsolated` is then false, the module worker's `SharedArrayBuffer`
transfer throws, and `fail()` (`site-boot.mjs:61`) paints "frogg3rs could not
start in this browser." The shim reloads once afterwards and the second load
works, so the panel is thrown away — but it is what the visitor sees until the
reload lands, and the busier the machine the longer that is.

Measured on 2026-08-27: `[pages] blank-frame.spec.mjs` failed at the 45s
timeout during the full 47-test run, with that exact panel in the snapshot, and
passed in 656ms when run alone. THAT SNAPSHOT IS NO LONGER ON DISK —
`app/browser/e2e/test-results/` now holds only `.last-run.json`, reading
`{"status":"failed","failedTests":[]}`. So the measurement is a report, not an
artifact anyone can re-open, which is why task 1.1 reproduces it before
anything is changed rather than building on it.

**The panel has two painters, not one.** `fail()` (`site-boot.mjs:61-79`) is
reached from `boot().catch` (`:137`) and from this module's own `error` and
`unhandledrejection` listeners (`:156-162`). `renderBootError()`
(`index.html:120-145`) paints the same panel — same heading string, same class
names, same clearing, same benign-message set — from its own listeners
(`:146-153`), registered before the module tag. That duplication is deliberate
and documented (`index.html:100-102`): the inline layer exists to catch a
failure of this module's own imports, so it cannot import anything. It is not
a defect to collapse, but it does mean suppression written into `fail()` alone
is written once and open-coded once.

**The fix is to stop racing.** While the shim still owes this page an isolation
attempt, `site-boot.mjs` should not boot and should not paint a failure — the
reload is going to discard both. Once the shim's one-shot guard has already
fired, booting is right: the page has had its reload, and any remaining failure
is real and should surface exactly as it does today.

That needs one bit of shared state, and it must be SINGLE-SOURCED. The shim
already owns `RELOAD_GUARD_KEY` (`coi-serviceworker.js:81`, value
`"frogg3rs-coi-reloaded"`, and the only reader or writer of it in the tree);
`site-boot.mjs` must not re-derive that string. The shim publishes whether an
attempt is still owed; the boot path reads it.

**That state has to be settleable, not a write-once flag.** The shim's page
branch has three exits after any synchronous set: `register()` rejects
(`:119`), a controller is already present (`:97-99`), or `ready` resolves and
`reloadOnce()` declines because the guard already fired (`:83-90`). In the
first and third the reload never comes. A boolean sampled once by the module
would suppress those pages permanently — turning a rare failure into a silent
blank page, which is the one thing the error panel exists to prevent. So the
shim must be able to say "no attempt is coming after all", and the boot path
must wait on that rather than read a snapshot of it.

UNVERIFIED and settled by running, not by argument: whether the shim's
`ready`-keyed reload is itself fast enough on a cold first visit that suppressing
the boot is sufficient, or whether the page also needs to render a neutral
starting state so a slow first visit is not a blank page.

## 2. The load-readout fix shipped on a derivation, with its control never run

`frogg3rs-web-release-repair` task 5.0 was a POSITIVE CONTROL on the whole
section: measure the PUBLISHED SAMPLES over time under Chromium CPU throttling
(`Emulation.setCPUThrottlingRate` over a CDP session), and report whether
windows over 100% cluster at startup or recur. Its own text says the section is
refuted if a throttled run shows sustained windows above 100%, in which case the
cause is DSP cost and the readout is not the fix — and it says explicitly not to
proceed on the derivation alone.

It was never run. The window was shortened to one second and the decimal
dropped (Sheaf `80d9f4bb`, "Report the load over a one-second window, in whole
percent"), and it shipped. So the claim that this fixes what the operator saw
on a phone rests on the proposal's own arithmetic and nothing else.

Half of what 5.0 called a first attempt is not one. The per-window samples
already leave the runtime without new production code:
`AudioWorkletDeadlineMeter::SampleMicrounits` (`BrowserRuntime.hpp:318-326`) →
`Runtime::AudioWorkletDeadlineMicrounits` (`:636-639`) → the C ABI
`synth_browser_audio_worklet_deadline_microunits`
(`browser/cpp/BrowserRuntimeAbi.cpp:120-123`) → the `audio-worklet-stats`
worker message (`browser/src/worker.ts:264,549-552`), which three existing
specs already poll (`audio-flow.spec.ts:825`, `audio-input.spec.ts:64-98`,
`first-party-apps-smoke.spec.ts:182-266`). Only the CPU throttling is new
ground: `newCDPSession` and `Emulation.setCPUThrottlingRate` have zero hits in
either repo.

Note which window "the published samples" means. The meter publishes every
100 ms (`kPublishWindowMicros = 100'000`, `BrowserRuntime.hpp:329`); the
display's one second is a separate `RollingMax` over UI frames
(`RuntimeMainComponent.hpp:284-285`, capacity from `DeadlineWindowCapacity`,
`MidiConfigViewModel.hpp:71-74`). The measurement wants the 100 ms values.
Because they are reachable only through the worker handle, the measurement runs
in SHEAF's browser suite — frogg3rs's site e2e drives a page and cannot ask the
worker anything.

Nothing here argues the change was wrong. It argues that an unrun control is not
a passed one, and that the number on the operator's phone is still unexplained
by measurement.

## 3. A captioned control cannot declare its own width

Carried from the predecessor's deferred requirement, whose text moves into
`specs/froggers-sheaf-runtime-app/spec.md` unchanged.

`Builder::FinishControl` (`PortableUIBuilders.hpp:438-480`) applies the author's
`style.layout` to the `.row` wrapper, where `.main` is the row's HEIGHT in the
vertical form column, then gives the control node a fresh `LayoutOptions` with
`.main = Extent::Weight(1.0f)` hardcoded (`:465-467`), where `.main` is its
WIDTH inside the horizontal row. One field name, two axes, and the author's
declaration never reaches the second. `FormButton` and `Field` therefore arrive
at `ApplyFormGrid` (`PortableUILayout.hpp:845`) indistinguishable, so no edit
confined to the grid can separate them. `BackButton` (`:435-439`) declares a
width with `.cross`, but on a captioned control that sizes the whole row.

This is a layout-model capability, and it is the reason the previous change
stopped rather than the reason it was lazy: two agents were sent at it, and the
second correctly refused a file list that made the fix unreachable.

**Both files are required, and each earlier attempt named only one.**
`frogg3rs-web-release-repair`'s task 3.1 scoped this to `ApplyFormGrid`; the
first draft of this change scoped it to `FinishControl`. Neither works alone:
`FinishControl` discards the author's declaration, and `ApplyFormGrid`
(`PortableUILayout.hpp:813-857`) then sets `cells[1]->bounds.width` from the
row's leftover width at `:845` UNCONDITIONALLY, overwriting whatever the
control node resolved to. Fix the builder alone and nothing moves; fix the grid
alone and it has nothing to tell the two controls apart.

What the fix does NOT need is a new kind of extent. `Extent::Intrinsic()` and
`Mode::Intrinsic` already exist (`PortableUILayout.hpp:32,39`) and are already
`LayoutOptions::main`'s default (`:54`). What is missing is a way to address
the control node's axis at all: `ControlStyle` (`PortableUIBuilders.hpp:25-39`)
carries exactly one `LayoutOptions layout{}` (`:38`) and `FinishControl` spends
it on the row. So the answer is one new field on `ControlStyle`, defaulted to
`Extent::Weight(1.0f)` so all ten `FinishControl` call sites
(`PortableUIBuilders.hpp:189,197,206,218,228,248,279,289,310,351`) keep today's
behaviour.

**OUTCOME: REFUTED AND REVERTED.** Built, verified working, then reverted.
`FormButton` exists precisely so a button lines up with the selectors around it
(`RuntimePages.hpp:440-442`), the Audio page captions it deliberately for that
reason (`:900-902`), `criteria::ColumnAlignment`
(`tests/support/VisualCriteria.hpp:430`) asserts that column across every page
and app, and `Toggle` IS `FormButton` (`:456`) so the change silently shrank
Sync's toggles. Landing it required amending a passing cross-page assertion,
which task 3.5's own rule forbids. Neither this change nor its predecessor read
any of that before writing the requirement. The delta now records the existing
behaviour as the requirement instead, so this is not proposed a third time.

The operator's observation is not dismissed: on the Audio page with capture
offline, "Retry Input" does span the control column. If that should change, the
question is whether a COMMAND belongs in a settings grid at all — a deliberate
redesign with `ColumnAlignment` as its first obstacle, not a width fix.

## 4. Browser MIDI — the claim that it is unreachable is FALSE

`frogg3rs-web-release-repair` asserted that MIDI arrives only through a
consumed `ActivationLease`, so a page with no launch gesture can never have it.
That was repeated into this proposal without being traced. Tracing it:

- `main.ts:178` constructs the MIDI manager unconditionally and with no
  options: `this.midi = new BrowserMidiManager(new BrowserMidiWorkerRuntime(...))`.
- `BrowserMidiManager.startFromUserActivation()` (`midi.ts:112-115`) therefore
  falls through to `navigator.requestMIDIAccess({ sysex: true })` itself.
- `startUserActivation()` (`main.ts:267-276`) calls exactly that, and the
  dispatch wiring at `main.ts:172-175` runs it after the first in-app action.

So the site already has a MIDI path, on the same first-gesture route the audio
context now uses. The lease branch at `main.ts:211-219` is the EAGER path, not
the only one — which is the same mistake section 1 of the previous change made
about the AudioContext, made a second time about MIDI.

One more claim in that list was written from memory and is also wrong — this
proposal's own. It said no e2e asserts the `midi:` half of the status string
that `startUserActivation` renders (`main.ts:275`). One does:

    External/Sheaf/projects/synth/browser/tests/runtime-core.spec.ts:477
    expect(result.status).toMatch(/audio:online; midi:(online|offline)/);

The gap is real but it is a different gap. That assertion **accepts
`offline`**, so it cannot go red when MIDI is broken, and it runs against a
stubbed audio context (`audioContextFactory`, `:460-466`), so it says nothing
about a real browser. frogg3rs's own site e2e asserts only the audio half
(`e2e/helpers.mjs:158`, `e2e/desktop-layout.spec.mjs:65`).

Two mistakes of the same kind, one paragraph apart, is the point rather than an
embarrassment: the escape hatch was one grep wide in both cases.

What is actually unknown is whether that path WORKS on the published site. No
assertion pins the value, and no operator has tried a controller against the
browser build. `requestMIDIAccess` with `sysex: true` needs both Web MIDI
support and a permission grant, and Chromium headless is not a fair test of
either.

So this item is verification first, and repair only of whatever verification
actually finds. It is not an interface design, and it was never a question of
whether MIDI is wanted.

## 5. Duplications the previous change created or left standing

Enumerating these by operand rather than by name changed two of the three.

- The disabled greys are three DIFFERENT values, each appearing exactly once:
  `Rgb(90, 96, 100)` (`app/FroggersUiSurface.hpp:1994`), `kDisabledText`
  `Rgb(125, 132, 138)` and `kDisabledButton` `Rgb(45, 49, 53)`
  (`RuntimePageStyle.hpp:14,20`). Across both repos there are 197 `Rgb(` sites
  and 92 distinct triples, 27 of them repeated — and none of the repeats is a
  disabled grey. So §7 does not bite: this is three colours expressing one
  concept, not one value written three times. The reasoning already exists in
  prose at `app/FroggersUiSurface.hpp:1991-1993`, and says exactly that
  unifying them is a palette decision made once for both. What it deserves is
  a home a reader finds, not a rediscovery.
- `RollingMax` (`MidiConfigViewModel.hpp:36-63`) and `RollingBuffer<N>`
  (`DspBuffers.hpp:98-117`) are two ring buffers of the same shape, and the
  previous change's §7 enumeration counted references to the old NAME and never
  asked whether an equivalent structure existed. Doing the comparison now finds
  something neither change predicted: `RollingBuffer` has ZERO production
  instantiations (only `tests/dsp_tests.cpp:1294`), and its `Min()`/`Max()`
  (`:110-116`) run over the whole `std::array` including never-written
  zero-initialised slots, where `RollingMax::Max()` iterates `filled_` (`:50`)
  and is correct on a partial window. It is not dead code — `DspBuffers.hpp` is
  a shipped library header (`projects/synth/Makefile:39` `DSP_HEADERS`,
  included by `apps/braid-4/Braid4Core.hpp:9`) on a branch named
  `fix-out-of-tree-app-gaps`, where an out-of-tree app author is a real
  consumer. So the answer is neither consolidation nor deletion.
- `AUDIO_STATUS_LINE_SELECTOR` (`app/browser/e2e/helpers.mjs:78`) writes the
  whole selector literally. Sheaf does not: `audio-input.spec.ts:84` passes the
  id through `synthNode()` (`browser/tests/helpers/fake-app.ts:472-474`). The
  strings that result are identical, so what is written twice is the ID, not
  the selector — and the id is single-sourced in C++ (`RuntimePages.hpp:57`),
  where no JS test can reach it.

## Non-goals

- The DSP's cost on a phone. Item 2 is about measuring the readout's behaviour,
  not about making the instrument cheaper.
- Redesigning the site's landing experience. Item 1 suppresses a failure panel
  during a reload the shim already performs; it does not add a launch screen.
- External audio's consent default, which is specified and unchanged.

## Impact

- `app/browser/site/coi-serviceworker.js`, `app/browser/site/site-boot.mjs`,
  `app/browser/site/index.html` (the second painter), and `app/browser/e2e/`.
- Sheaf, on the `fix-out-of-tree-app-gaps` branch (PR #9):
  `PortableUIBuilders.hpp` AND `PortableUILayout.hpp` (item 3 needs both),
  `RuntimePages.hpp`, the portable layout tests, and the browser test suite for
  item 2's measurement.
- `openspec/changes/archive/2026-08-27-frogg3rs-web-release-repair/` for section 0, which closes
  and archives it.
- Affected specs: `froggers-browser-package` (what a first visit owes),
  `froggers-sheaf-runtime-app` (the form-control width requirement returns
  here), and Sheaf's own `synth-runtime-ui` for item 2's measurement.

Note for anyone running the e2e: `app/browser/e2e/playwright.config.mjs:53-55`
never rebuilds. Both servers serve `../dist/site`, so an edit under
`app/browser/site/` is invisible until `node package-catalog.mjs` runs from
`app/browser/` (`app/browser/Makefile:23`).

Preflight findings and their evidence are in `preflight.md` beside this file.
