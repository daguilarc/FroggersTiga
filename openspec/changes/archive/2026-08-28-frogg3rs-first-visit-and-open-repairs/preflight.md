# Preflight audit — `frogg3rs-first-visit-and-open-repairs`

**Run 2026-08-27, before any execution.** Every claim below cites a file:line
that was read. Where the proposal's own text is refuted, that is said plainly
rather than softened: this change convicts its predecessor of asserting from
memory, and it does the same thing in three places.

Verdict: **execution may proceed after the fixes recorded here are applied to
`proposal.md` and `tasks.md`.** Nothing found makes the change unworkable. Two
findings change what files the work touches, and one changes what a section
concludes.

---

## P1 — The failure panel has TWO definition sites; task 1.3 names one

§13 requires running §7's enumeration FORWARD on every concept the plan
creates. The concept here is "an isolation attempt is still owed, so do not
paint a failure". Enumerating by operand — the heading string, not the function
name — finds two painters:

- `site-boot.mjs:61-79`, `fail()`, reached by `boot().catch(...)` (`:137`), a
  `window` `error` listener (`:156-159`), and an `unhandledrejection` listener
  (`:160-162`).
- `index.html:120-145`, `renderBootError()`, reached by its own `error`
  (`:146-149`) and `unhandledrejection` (`:150-153`) listeners, registered
  BEFORE the module tag at `:157`.

The two duplicate the heading string `"frogg3rs could not start in this
browser."` (`site-boot.mjs:73` / `index.html:138`), the `boot-error` /
`boot-error-heading` / `boot-error-detail` class names, the
`height`/`minHeight`/`overflow` clearing, and the
`BENIGN_WINDOW_ERROR_MESSAGES` set (`site-boot.mjs:130-133` /
`index.html:116-119`). The duplication is deliberate and documented
(`index.html:100-102`: importing anything there would reintroduce the failure
the inline layer exists to catch) — it is not a defect to collapse.

But it means a suppression written only into `fail()` is written once and
open-coded once. **FOUND 2, plan addresses 1.**

FIX: task 1.3 covers both painters.

## P2 — Task 1.2's mechanism cannot satisfy task 1.6

Task 1.2 says the shim "publishes whether an isolation attempt is still owed"
and the boot path "reads it", set synchronously before `register()`. Task 1.6
requires that a service worker which FAILS to register still produces the
panel rather than a silent blank page.

A flag set synchronously and read once cannot do both. The shim's page branch
(`coi-serviceworker.js:67-119`) has three exits after the synchronous set:

- `register()` rejects → `.catch` at `:119`. No reload will ever come.
- `register()` resolves with a controller already present (`:97-99`) → no
  reload; isolation already holds.
- `ready` resolves without a controller → `reloadOnce()` (`:116-118`), which
  itself returns without reloading if the guard already fired (`:83-90`).

In the first and third cases the page is suppressed forever if the published
state is a write-once boolean. That is exactly the silent blank page 1.6
forbids, converted from a rare condition into a permanent one.

FIX: the published state is settleable — the shim resolves it to "no attempt
is coming" on every exit above, and the boot path waits on it rather than
sampling it. Single-sourcing still holds: `RELOAD_GUARD_KEY`
(`coi-serviceworker.js:81`) stays the shim's, and its string is never
re-derived.

## P3 — Item 3's file scope is wrong, and was wrong in the predecessor too

The proposal says "no edit confined to the grid can separate them", and scopes
task 3.1 to `PortableUIBuilders.hpp`. `frogg3rs-web-release-repair`'s task 3.1
(`tasks.md:220`) scoped the same fix to `ApplyFormGrid` and it did not land.
Both scopings are incomplete, for two independent reasons:

- `Builder::FinishControl` (`PortableUIBuilders.hpp:461-467`) puts the author's
  `style.layout` on the `.row` wrapper and hands the control node a
  default-constructed `LayoutOptions` with `.main = Extent::Weight(1.0f)`
  hardcoded. The author's declaration is discarded here.
- `ApplyFormGrid` (`PortableUILayout.hpp:813-857`) then sets
  `cells[1]->bounds.width = std::max(0.0f, row->bounds.width - controlOffset -
  rowOpts.padding)` at `:845`, **unconditionally**. Even a control node that
  arrived carrying an intrinsic width has that width overwritten.

So fixing the builder alone changes nothing observable, and fixing the grid
alone has nothing to distinguish. **Both files are required.** The proposal's
Impact section does list both; task 3.1's text does not, and task 3.1 is what
an executor reads.

FIX: task 3.1 names both edits and says why neither suffices alone.

## P4 — Task 3.2's open question is already answered by the types

Task 3.2 asks how a captioned control says "size me to my content", and offers
"a new field on `ControlStyle`" as a possibility to be justified. Reading the
types answers the two halves differently:

- A content-sized extent already exists and needs no invention:
  `Extent::Mode::Intrinsic` / `Extent::Intrinsic()`
  (`PortableUILayout.hpp:32,39`), which is already `LayoutOptions::main`'s
  default (`:54`). `Fixed`, `Fraction` and `Weighted` exist alongside it.
- What does not exist is any way to address the control node's axis
  separately from the row's. `ControlStyle` (`PortableUIBuilders.hpp:25-39`)
  carries exactly one `LayoutOptions layout{}` (`:38`), and `FinishControl`
  spends it on the row. That is the "one field name, two axes" the proposal
  correctly identifies.

So the answer is a new field on `ControlStyle` — not a new `Extent` kind — and
it must default to `Extent::Weight(1.0f)` so all ten `FinishControl` call sites
(`PortableUIBuilders.hpp:189, 197, 206, 218, 228, 248, 279, 289, 310, 351`)
keep today's behaviour untouched.

FIX: task 3.2 records this as traced rather than leaving it open, and names
`Extent::Intrinsic()` as the value `FormButton` will declare.

## P5 — Proposal §4 asserts something false, in the same way it convicts

Proposal §4 and task 4.1 both state that nothing asserts the `midi:` half of
the status string. That is false:

    External/Sheaf/projects/synth/browser/tests/runtime-core.spec.ts:477
    expect(result.status).toMatch(/audio:online; midi:(online|offline)/);

The assertion exists. What is true — and is the sharper finding — is that it
**accepts `offline`**, so it cannot go red when MIDI is broken, and it runs
against a stubbed audio context (`audioContextFactory` at `:460-466`), so it
demonstrates nothing about a real browser. The gap the plan points at is real;
its description of the gap is not.

This is the same defect the proposal convicts `frogg3rs-web-release-repair` of
in its own §4: a claim about existing behaviour written from memory of the
code rather than from reading it.

FIX: proposal §4 and task 4.1 restated against the assertion that exists.

## P6 — Item 2 is more reachable than the plan says, and the plan omits where

Task 2.1 marks the whole measurement a FIRST ATTEMPT on the grounds that
nothing opens a CDP session. Half of that is right:

- **CDP throttling is genuinely a first attempt.** `newCDPSession` /
  `CDPSession` / `Emulation.set*` / `setCPUThrottlingRate` return zero hits in
  both repos. Playwright is installed and capable
  (`app/browser/e2e/package.json` pins 1.60.0, Sheaf's browser suite has
  1.62.0, `newCDPSession` typed at `playwright-core/types/types.d.ts:10166`),
  and `~/Library/Caches/ms-playwright` already carries `chromium-1223` — so
  nothing needs installing.
- **Logging the published samples is NOT a first attempt.** The raw
  per-window value already leaves the runtime:
  `AudioWorkletDeadlineMeter::SampleMicrounits` (`BrowserRuntime.hpp:318-326`)
  → `Runtime::AudioWorkletDeadlineMicrounits` (`:636-639`) → C ABI
  `synth_browser_audio_worklet_deadline_microunits`
  (`browser/cpp/BrowserRuntimeAbi.cpp:120-123`) → the `audio-worklet-stats`
  worker message (`browser/src/worker.ts:264,549-552`). Three existing specs
  already poll it (`audio-flow.spec.ts:825`, `audio-input.spec.ts:64-98`,
  `first-party-apps-smoke.spec.ts:182-266`).

Two things the plan does not say and an executor needs:

- The meter's own publish window is `kPublishWindowMicros = 100'000`
  (`BrowserRuntime.hpp:329`) — 100 ms. The display's one-second window is a
  `RollingMax` over UI frames (`RuntimeMainComponent.hpp:284-285`, capacity
  from `DeadlineWindowCapacity`, `MidiConfigViewModel.hpp:71-74`). These are
  different windows; "the published samples" means the 100 ms ones.
- The measurement belongs in **Sheaf's** browser suite, which owns the worker
  handle. frogg3rs's site e2e drives a page, not the worker, so it cannot read
  `audio-worklet-stats`. Task 2.4 already puts the write-up in Sheaf; the
  measurement has to run there too.

FIX: task 2.1 splits the first-attempt marking and names the suite.

## P7 — Item 5.1's premise is loose: three greys, three different values

Enumerating by operand (`Rgb(` literals) across both repos: **FOUND 197 call
sites, 92 distinct triples, 27 appearing 2+ times.** None of the repeats is a
disabled grey. The three the plan names each appear exactly once:

| literal | site |
|---|---|
| `Rgb(90, 96, 100)` | `app/FroggersUiSurface.hpp:1994` |
| `Rgb(125, 132, 138)` | `RuntimePageStyle.hpp:14` (`kDisabledText`) |
| `Rgb(45, 49, 53)` | `RuntimePageStyle.hpp:20` (`kDisabledButton`) |

So this is a concept collision — three colours expressing "disabled" — not a
duplicated value, and §7 does not bite. The decision the task asks for already
exists in prose at `app/FroggersUiSurface.hpp:1991-1993`, which says exactly
that unifying them is a palette decision made once for both, not an include
added at the cell.

FIX: 5.1 restated as "move an existing decision somewhere a reader finds it",
with the FOUND counts recorded, and not as a de-duplication.

## P8 — Item 5.2's premise is wrong in a way that changes the conclusion

`RollingBuffer<N>` (`DspBuffers.hpp:98-117`) has **zero production
instantiations**. Its only instantiation anywhere is
`tests/dsp_tests.cpp:1294`. `RollingMax` (`MidiConfigViewModel.hpp:36-63`) has
one: `RuntimeMainComponent.hpp:672`.

That is not dead code, because `DspBuffers.hpp` is a shipped library header —
listed in `DSP_HEADERS` (`projects/synth/Makefile:39`) and included by
`apps/braid-4/Braid4Core.hpp:9` — on a branch named
`fix-out-of-tree-app-gaps`, where an out-of-tree app author is a real
consumer. §12.0's fork applies and lands on "keep": the consumer outlives the
primitive.

The comparison the task asks for produces a finding the plan did not
anticipate. `RollingMax::Max()` iterates `filled_` slots (`:50`), so a
partially-filled window is correct. `RollingBuffer::Min()`/`Max()`
(`:110-116`) run `std::min_element`/`max_element` over the whole
`std::array<float, Size>` including never-written zero-initialised slots. For
`Min()` on any non-negative signal that is wrong until the buffer wraps.

FIX: 5.2 ends in the comparison the plan asked for, and its output is that
finding — not a consolidation.

## P9 — Item 5.3's claim is imprecise

`app/browser/e2e/helpers.mjs:78` does hardcode the whole selector:

    export const AUDIO_STATUS_LINE_SELECTOR = '[data-synth-node-id="runtime.audio.status_line"]';

Sheaf does not. `audio-input.spec.ts:84` passes the **id** through a helper,
`synthNode()` (`browser/tests/helpers/fake-app.ts:472-474`), which builds the
attribute selector. The resulting strings are identical, but what is written
twice is the id, not the selector. The id is single-sourced in C++
(`RuntimePages.hpp:57`), which no JS test can reach.

FIX: 5.3 restated to name the id as the duplicated operand.

## P10 — The change has no `specs/` directory

`frogg3rs-web-release-repair` and `frogg3rs-drive-tone-floor` each carry
`specs/`. This change carries none, while its proposal names three affected
specs and `form-control-width-requirement.md` sits loose at the change root as
a requirement with no delta around it.

FIX: `specs/` added, with `form-control-width-requirement.md`'s requirement
moved into `specs/froggers-sheaf-runtime-app/spec.md` as a proper delta and the
loose file removed, plus a `froggers-browser-package` delta for what a first
visit owes.

## P11 — Two active changes, one stale ledger, and unsynced deltas

§13 requires enumerating the other active changes. There are two:

- `frogg3rs-drive-tone-floor` — DSP only (`app/dsp/Drive.hpp:456`,
  `Delay.hpp:730`, `DspMath.hpp`), 25/26 done, one operator item open. **No
  overlap.**
- `frogg3rs-web-release-repair` — overlaps this change at seven points by
  design, and its own `tasks.md:423-430` redirects them here. That is a
  documented handoff, not accidental duplication.

Two real defects in that handoff:

- Its ledger reads **0 checked of 49**, though its own text names the commits
  that shipped sections 1, 2, 4, 5 and 6 (`ff1d110`, `0acbe93`, `260b5cf`,
  `d09e8f7`, `faf9db0`, Sheaf `80d9f4bb`) and the tree confirms it
  (`site-boot.mjs:94,105` carries the `audioOptions.audioContext` fix;
  `e2e/audio-activation.spec.mjs` exists). Archiving with unchecked items is
  normal here — 73 archived changes, many with dozens open — but archiving
  with nothing checked against known-shipped work is not.
- Its spec deltas were never synced. `openspec/specs/froggers-browser-package/
  spec.md` says nothing about isolation, the lease, or the audio context.
  Worse, the unsynced delta
  (`changes/archive/2026-08-27-frogg3rs-web-release-repair/specs/froggers-browser-package/spec.md:5`)
  states "the lease is what carries the audio context and MIDI access" — the
  claim item 4 refutes. Syncing it as-is would write the refuted claim into the
  main spec.

FIX: a new section 0 that checks off the shipped ledger, corrects the delta's
MIDI sentence, syncs, and archives — sequenced so it happens before this
change's own spec work.

## P12 — §12.0 hygiene: the refuted MIDI claim is in shipping source

`app/browser/site/site-boot.mjs:37-38`:

    // Browser MIDI still has no path in -- it arrives only with a lease -- and
    // remains unreachable here.

This is the claim proposal §4 refutes, sitting in the file this change edits.
§12.0 counts it (a stale statement that no longer resolves to what it claims);
task §4 does not mention it.

FIX: listed in the new section 0 hygiene sweep and corrected in item 4.

## P13 — Proposal §1's evidence is no longer on disk

Proposal §1 cites a 2026-08-27 run "with that exact panel in the snapshot".
`app/browser/e2e/test-results/` now holds only `.last-run.json`, and that file
reads `{"status":"failed","failedTests":[]}`. The artifacts are gone.

This does not block anything — task 1.1 requires reproducing the failure
before changing code, which is the correct remedy — but the proposal should not
read as though the snapshot is available to inspect.

FIX: proposal §1 marks the snapshot as no longer retained.

## P14 — Execution order the tasks do not state

`app/browser/e2e/playwright.config.mjs:53-55` says the config "never rebuilds"
`dist/site`; both `webServer` entries serve `../dist/site`. So an edit to
`app/browser/site/*` is invisible to the suite until `node package-catalog.mjs`
runs from `app/browser/` (`app/browser/Makefile:23`; also
`local-smoke.sh:26`, `.github/workflows/pages.yml:65`).

Item 1 edits exactly those files, and task 1.5's positive control depends on
running against today's build FIRST and the fixed build SECOND — which is a
rebuild between two runs.

FIX: stated in the gates preamble and in task 1.5.

## P15 — Reported, not actioned: 32 MB of session dumps in Sheaf

`External/Sheaf/analysis/sdd-model-analysis/data/` holds 32 MB of AI session
transcripts (`claude_sessions.json`, `codex_sessions.json`,
`sessions_raw.json`, …). Under §12.0 these are correspondence artifacts, and
they measurably degrade `grep` across the submodule — every enumeration run for
this audit had to exclude them by hand.

They are outside this change's touched tree, and removing them is a Sheaf
decision with its own consumers to trace (`analysis/` may have scripts that
read them). Saying something, per the omni rule's first line; not expanding
scope to fix it.

---

## Traces confirmed unchanged

These proposal claims were checked and hold as written:

- `index.html:156-157` loads the shim as a classic script and the boot as a
  module. CONFIRMED.
- `RELOAD_GUARD_KEY` at `coi-serviceworker.js:81`, value
  `"frogg3rs-coi-reloaded"`, sole definition site; no other reader or writer in
  `app/browser/` outside the built copy under `dist/`. CONFIRMED.
- The `[pages]` project runs only `blank-frame.spec.mjs`
  (`playwright.config.mjs:37,92-93`) against
  `serve-site.mjs --no-isolation-headers` (`:61`). CONFIRMED.
- `main.ts:178` constructs `BrowserMidiManager` with no options, so
  `startFromUserActivation` (`midi.ts:108-120`) always reaches
  `navigator.requestMIDIAccess({ sysex: true })` at `:114`. CONFIRMED — the
  MIDI path exists.
- The lease branch (`main.ts:211-219`) sets `activationStarted = true` at
  `:212`, making it an alternate eager path rather than the only one.
  CONFIRMED.
- `BackButton` declares its width with `.cross`
  (`RuntimePages.hpp:434-439`). CONFIRMED.
- `RuntimePages.hpp:57` single-sources `runtime.audio.status_line`.
  CONFIRMED.
- `portable_ui_layout_tests.cpp` `int main()` at `:1368-1415` is a
  hand-maintained list of 45 calls with no discovery mechanism. CONFIRMED —
  task 3.4's warning is right.
- `portable_ui_tests.cpp:1618-1650` compares the live
  `BuildAudioPageTree` against a frozen
  `ReferenceAudioPageTreeBeforeAppSection`, node-for-node on id, kind and
  bounds. CONFIRMED — task 3.5's stop condition is real and likely to fire.

## Line-number corrections

- `main.ts` dispatch wiring is at `:174-177`, not `:172-175`.
- `ApplyFormGrid` begins at `PortableUILayout.hpp:813`; `:845` is the
  operative width line inside it, not the function head.
- `RollingMax` spans `MidiConfigViewModel.hpp:36-63`; `fail()` spans
  `site-boot.mjs:61-79`.
