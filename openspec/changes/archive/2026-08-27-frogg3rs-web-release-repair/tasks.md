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

- [x] 0.1 Delete `openspec/changes/frogg3rs-cpu-readout-says-it-is-a-peak`.
      Confirm nothing else references it by name first.
      PREFLIGHT FOUND IT ALREADY GONE: the directory does not exist, and the
      only mentions of the name anywhere in either repo are this change's own
      proposal and tasks. Record that rather than perform a deletion; the point
      of the item — that two changes are not both repairing the browser chrome —
      holds.
- [x] 0.2 The superseded change is already archived and its deltas are already
      in the live specs, so nothing is stranded — verify that rather than
      assume it, by checking `openspec/specs/frogg3rs-web-mobile-ux` still
      carries the requirements it landed.

## 1. The activation lease

- [x] 1.1 Read `launchCatalogApplication` and enumerate EVERYTHING it passes
      that `site-boot.mjs` does not. The lease is the one that bit; report
      whether it is the only omission rather than fixing the one already found.
      This is the whole lesson of the defect — a subset copied by hand, with
      nothing saying what the full set was.
      PREFLIGHT ANSWERED THIS; confirm it rather than re-derive it.
      `launchCatalogApplication` (`main.ts:393-421`) passes eight fields to
      `installApp` (`:401-413`): `module`, `runtimeVersions`, `runtimeIdentity`,
      `disposeModule`, `activationLease` (`:410`), `runtimeClientFactory`
      (`:411`), `audioOptions` (`:412`), `frameIntervalMs` (`:413`).
      `site-boot.mjs:76-85` passes the first four. So four omissions, of which
      three are inert: `runtimeClientFactory`, `audioOptions` and
      `frameIntervalMs` are optional pass-throughs that are `undefined` at
      Sheaf's own default call site too (`installSheafPatchLauncher(root)`,
      `main.ts:424`). `activationLease` is the only always-populated one —
      `launchCatalogApplication` builds a real lease at `main.ts:381`. Pass the
      lease; leave the other three omitted, and say in the comment that they are
      omissions that carry nothing.
- [x] 1.2 Build an `ActivationLease` in `site-boot.mjs` and pass it. Its
      constructor is PRIVATE (`browser/src/activation.ts:33-37`) — the factory
      is `ActivationLease.acquire(options)` (`:45`), taking
      `{ audioContextFactory?, requestMIDIAccess? }` (`:8-11`). `consume()`
      (`:65-77`) is single-use and THROWS on a second call, so the site hands
      the lease to the launcher and must not consume it itself; the launcher
      consumes it at `main.ts:336`. It resolves to
      `{ audioContext, midiAccess }`, which `main.ts:343,347` forward.
      REWRITE THE HEADER COMMENT, do not just delete it. It currently argues
      correctly that no lease is needed for output activation and concludes
      that none is needed at all. The replacement has to say what the lease is
      for HERE — supplying the context capture requires — and keep the true
      part, that this page anchors activation to the first in-app action rather
      than to a Launch button. Deleting it would lose the reasoning and invite
      someone to remove the lease again on the same grounds.
- [x] 1.3 MIND THE GESTURE. A browser `AudioContext` starts suspended without a
      user gesture, and `getUserMedia` needs one too. Establish where the
      existing site gets its gesture from — the app already starts audio from a
      UI action (`desktop-layout.spec.mjs`'s "no audio starts on load" pins
      that nothing starts without one). The site has NO gesture anchor of its
      own: Sheaf's `BrowserUiBackend` wiring calls `startUserActivation()` after
      every dispatched UI action (`main.ts:172-175`, guarded once by
      `activationStarted` at `:267-276`), which is the path already in use. Make
      the lease fit that path rather than inventing a second one.
      THE RISK THIS INTRODUCES, AND IT MUST BE CHECKED: the launcher consumes
      the lease during install (`main.ts:336`), i.e. at page load, with no
      gesture — so the `AudioContext` is now CONSTRUCTED on load where before
      none existed. A context constructed on load is suspended, not started, so
      this should be legal, but `desktop-layout.spec.mjs:60-66` asserts
      `data-synth-status` never reaches `audio:online` without a click. Run that
      spec and confirm it still passes for the RIGHT reason — the context exists
      and is `suspended` — rather than assuming a suspended context is invisible
      to it.
- [ ] 1.4 Do NOT change the consent default. External audio staying off until an
      operator selects an input is specified, and a present microphone is not
      consent. This task makes the choice REACHABLE, nothing else. Assert that:
      after boot with no operator action, the external-audio sources are still
      disconnected.
- [x] 1.5 An e2e assertion that the status line no longer reports
      `microphone requires the launch-owned AudioContext`.
      POSITIVE CONTROL: show it present against the current build first — it is
      in the operator's screenshot, so it is reproducible today.
- [x] 1.6 Headless browsers do not prompt for a microphone. Decide what the e2e
      suite can honestly assert — that the context exists and the status is no
      longer `audioContextUnavailable` is testable; a granted-permission device
      list may not be. Say which, rather than writing a test that passes for the
      wrong reason. Playwright can grant permissions via a context option;
      check whether that reaches a fake device here before relying on it.
      FIRST ATTEMPT: nothing in this repo's e2e suite grants microphone
      permission or uses a fake capture device today, so there is no working
      invocation to diff against. If it does not work, that is expected
      discovery, not a regression — fall back to asserting the context exists
      and the status is no longer `audioContextUnavailable`, which is the
      honest, testable half.

### SECTION 1 OUTCOME — REFUTED, CODE PARKED

1.1 answered as written (four omissions, one substantive). 1.2 implemented, and
implementation disproved the section: a lease resumes its context and requests
MIDI at `acquire()` (`activation.ts:45-51`), and a resolved lease makes
`main.ts:211-219` start audio and MIDI during `start()` with no gesture. 1.3's
risk check is what caught it — POSITIVE CONTROL: `desktop-layout.spec.mjs` 6/6
green on the shipped build, 6/6 failed with the lease wired in, all timing out
in `waitForSurfaceReady` because boot deadlocks in `consume()`.

1.4's assertion cannot be written honestly against this implementation: with a
resolved lease, boot itself reaches `getUserMedia`, so a headless "nothing was
captured" test would pass only because Chromium auto-denies the prompt. It was
correctly NOT written.

The lease attempt's `site-boot.mjs` and `playwright.config.mjs` edits and its
`audio-activation.spec.mjs` were parked in the session scratchpad while the
section was refuted. They did NOT stay parked: the section was then fixed by
the `audioOptions` route below, and all three are now in the tree in their
corrected form. `helpers.mjs` keeps `AUDIO_STATUS_LINE_SELECTOR`
(`[data-synth-node-id="runtime.audio.status_line"]`), which is correct
regardless and is what a future attempt asserts on — the status text is composed
natively in `BrowserAudioDevices.hpp:132` and rendered at `RuntimePages.hpp:57,
925-928`, so the DOM string is the only honest target.

RESOLVED, and with no Sheaf change after all. Tracing the non-lease path showed
the launcher already accepts a context by a route that carries no activation
claim: `AudioBridgeOptions.audioContext` (`audio.ts:19`) reaches the bridge
through `SynthBrowserAppOptions.audioOptions` (`main.ts:40`), which
`launchCatalogApplication` itself passes (`:413`). With no lease, `main.ts:343`
forwards `audioOptions` unchanged and `midiAccess` stays undefined (`:347`), so
the activation branch at `:211-219` never fires. `startFromUserActivation` hands
that context to `startAudioWorklet` (`:130`), which resumes it inside the first
in-app action, and `acquireInput` finds it at `:213`.

So the site constructs its own suspended `AudioContext` and passes
`audioOptions: { audioContext }`. Constructing a context needs no gesture;
only resuming does, and the lease's fatal move was resuming eagerly. The
consent default is untouched, nothing starts on load, and the header comment
now explains why a lease is the wrong instrument here rather than leaving the
next reader to rediscover it.

STILL NOT FIXED, and stated rather than hidden: browser MIDI. It arrives only
with a lease, so it remains unreachable. That is a separate change, and the
proposal's claim that MIDI comes along with this repair does not hold.

1.5 and 1.6 are CLOSED. `app/browser/e2e/audio-activation.spec.mjs` asserts the
status line no longer carries "microphone requires the launch-owned
AudioContext", targeting the DOM text via `helpers.mjs`'s
`AUDIO_STATUS_LINE_SELECTOR` — the native runtime's own string
(`BrowserAudioDevices.hpp:132`, rendered at `RuntimePages.hpp:57,925-928`),
not the JS-side diagnostic, which is never rendered. It is registered in
`playwright.config.mjs`'s `DESKTOP_SPECS`, which is a hand-maintained list an
unregistered spec silently falls out of. It PASSES against the republished
build (487ms).

1.6's honest scope, as decided: no permission grant and no fake device. The
assertion is the ABSENCE of the missing-context condition, not the presence of
a working microphone, which headless Chromium cannot demonstrate. Whether a
real microphone reaches External Audio is 8.2, and only an operator can close
it.

## 2. Disconnected sources render greyed

- [x] 2.1 The spec delta is the change here; the code follows it. Apply the
      MODIFIED requirement first so the implementation has something to be
      correct against.
- [x] 2.2 THE SITE THIS TASK ORIGINALLY NAMED WAS THE WRONG ONE. Preflight
      traced it: the draw lambda returns `{}` on `hidden` at
      `FroggersUiSurface.hpp:1884`, before it calls `BuildEncoderDrawCommands`
      (`:1913`) and before the `!state.connected` branch at `:1915`. Because
      `hidden` is `showingModulationView && !state.connected` (`:1797`), line
      1915 is only reachable OUTSIDE the modulation view. In the modulation
      view — the operator's case — 1884 returns first, so an edit confined to
      1915 would change nothing on screen. BOTH sites blank the cell and both
      have to draw it.
      Build from a dimmed COPY of `EncoderDrawState` with `connected` forced
      true and the colours de-emphasised, at 1884 and at 1915, rather than
      returning `{}`. The struct is copyable — bools, `size_t`, `uint32_t`,
      `synth::Color baseColor`, `std::string shortLabel`, and three vectors
      (`EncoderDraw.hpp:289-304`) — so a copy is the whole mechanism.
      Do not touch Sheaf's `BuildEncoderDrawCommands`: this is one
      application's presentation choice and every other Sheaf app's
      disconnected cells must be unaffected.
- [x] 2.2b SPLIT `hidden`. It currently means both "draw nothing" and "reach
      nothing": it gates the visualizer underlay (`:1798`, `:1826`) and the
      press/drag actions (`:1860`) as well as the drawing (`:1884`). Only the
      drawing changes. Give the two meanings two names so the next reader cannot
      collapse them again, and keep the action gate keyed to `!state.connected`
      — task 2.5 is the assertion that it stayed. The visualizer underlay stays
      suppressed: a disconnected source has no signal to visualize, and drawing
      one would claim it does.

- [x] 2.3 No value readout on a disabled cell. The connected path strips and
      replaces Sheaf's trailing 60-command label block; the disabled path should
      not emit one at all. Verify the command count rather than assuming which
      branch produced what.
- [x] 2.4 Clear the modulation and gesture indicator masks on the copy. An
      unavailable source showing modulation arcs would be claiming something.
- [x] 2.5 Actions and depth parameters are untouched, and asserted so: a
      disconnected cell still dispatches nothing on press or drag, and still
      creates no depth parameter. This is the half of the old requirement that
      survives, and drawing the cell is exactly what puts it at risk.
- [x] 2.6 Assert it is VISIBLY different from a connected cell, not merely
      non-empty — compare the emitted commands' colours against the connected
      case rather than counting commands.

## 3. Form-grid buttons

- [ ] 3.1 Sheaf, on the PR #9 branch. `ApplyFormGrid` (starts
      `PortableUILayout.hpp:813`) stretches every control cell at `:845` —
      unconditionally, for every row with two or more in-flow cells. Decide where
      the fix belongs: the grid respecting a control's declared extent, or the
      button declaring one the grid honours. Read how a cell's width is used
      downstream before choosing — the combos must keep the column.
- [ ] 3.2 Cross-host by construction, so assert it in the portable layout tests
      (`External/Sheaf/projects/synth/tests/portable_ui_layout_tests.cpp`, whose
      form-grid cases are at `:446` and `:472`, run from the block at
      `:1383-1384`), not in a browser test. `Toggle()` IS `FormButton()` and the Sync page
      builds four, so the Sync page is the desktop-visible case.
- [ ] 3.3 POSITIVE CONTROL: a test that fails against today's full-width
      behaviour, run red before the fix.
- [ ] 3.4 Verify no configuration page regressed: the device selectors still
      span the control column.

### SECTION 3 OUTCOME — DEFERRED, WITH THE TRACE THAT DEFERS IT

Not done, and not because it is hard to reach: there is no way to say it. A
captioned control's `style.layout` is applied to the `.row` WRAPPER
(`PortableUIBuilders.hpp` `FinishControl`, :438-480), where `.main` is the row's
HEIGHT in the vertical form column. The control node itself gets a freshly
constructed `LayoutOptions` with `.main = Extent::Weight(1.0f)` hardcoded
(:465-467), where `.main` is its WIDTH inside the horizontal row. One field
name, two axes, and the author's declaration never reaches the second one.

So `FormButton` and `Field` are indistinguishable by the time `ApplyFormGrid`
runs (`PortableUILayout.hpp:845`), and no edit confined to that function can
separate them. `BackButton` (:435-439) declares a width with `.cross`, but on a
captioned control that sizes the whole row, not the control.

Teaching a captioned control to declare its own width is a new layout-model
capability, not the repair this change scoped. Deferred to its own change so
the decision gets made deliberately rather than inside a button fix. The
requirement stays written in this change's `froggers-sheaf-runtime-app` delta
and is NOT satisfied — that is a known gap, stated rather than quietly dropped.

## 4. The Random S&H 6 background

- [x] 4.1 `app/FroggersModulation.hpp:230`, construct `source6Visualizer_` with
      `drawBackground = false`. Sheaf already takes the argument
      (`GangedRandomLfoVisualizer.hpp:260`); no Sheaf change.
- [x] 4.2 Assert lane 6 emits no full-node background fill, so the six lanes are
      consistent. The fill is `Color::Rgb(12, 14, 16)` over the whole node
      (`GangedRandomLfoVisualizer.hpp:63-64`) — assert on that, not on a command
      count.
      POSITIVE CONTROL: red against the default-constructed visualizer.
- [x] 4.3 Check the other five lanes were never affected, and that lane 6 still
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
      FIRST ATTEMPT: no test in either repo opens a CDP session or calls
      `Emulation.setCPUThrottlingRate` today, so there is no working invocation
      to diff this against. Failure to get a throttled session is expected
      discovery. If it cannot be made to work, say so and do NOT proceed on the
      derivation alone — section 5 rests on this being a positive control, and
      an unrun control is not a passed one.
      The proposal now DERIVES the answer as (a): at a phone's 50-80% steady
      state, a 1.25x-1.9x transient crosses 100%, and each crossing then holds
      the readout for 8.5s. So this task is a POSITIVE CONTROL on that
      reasoning, not an open question. It passes if a throttled run shows the
      predicted steady state with brief excursions; it REFUTES the whole
      section if it shows sustained windows above 100%, in which case the cause
      is DSP cost and the readout is not the fix. Say which it found.

- [x] 5.1 Shorten the hold. `RollingMax256` is defined at
      `MidiConfigViewModel.hpp:37-61` (`kCapacity = 256`; `Write`, `Max`) and
      referenced in four files, re-counted at preflight: its definition,
      `RuntimeMainComponent.hpp:283-284,671`, `runtime/Runtime.hpp:475,477`
      (comments only — they describe the hold and go stale the moment it
      changes, so they are part of this edit), and
      `tests/viewmodel_tests.cpp:66,1327,1338,1349`. FOUND 4, CLAIMED 4. Read it before choosing
      between a smaller capacity, a parameterised capacity, and a time-based
      window; `uiFrameHz` is per-application, so a frame count is a different
      hold on different hosts and time is the honest unit.
- [x] 5.2 About one second (30 frames at the default 30Hz). Justify the number
      against what it has to catch: a spike must still be readable by eye.
      Record the figure and the reasoning, since this is a judgement not a
      derivation.
- [x] 5.3 Whole percent, not tenths. The format string is `"CPU %.1f%%"`
      (`RuntimePages.hpp:353`, in `FormatDeadlineText` at `:350-354`); the
      sidebar column is `kSidebarWidth = 96.0f` (`:320`) and `"Controllers"`
      (`:709`) is the label that already overflows the intrinsic figure and
      ships looking correct. A held maximum has no tenth-of-a-percent
      accuracy, and dropping the decimal is also what lets `"CPU 100%"` fit the
      96px column — verify the fit by RENDERING, not by `TextWidth` arithmetic:
      `"Controllers"` computes to 111px against a 96px column and ships looking
      correct, so the intrinsic figure is not what clips.
- [x] 5.4 The label stays `"CPU"`. The relabelling to `"CPU max"` is dropped —
      it explained a misleading number instead of fixing it. Say so where the
      decision lives, so the next reader does not re-derive the discarded
      option.
- [x] 5.5 Enumerate the consumers of the readout's TEXT before changing the
      format, by the format string and by the node id, not by the function
      name.
      PREFLIGHT RE-RAN THIS ENUMERATION and confirms FOUND 4 + 1, matching the
      earlier pass: `RuntimePagesJuceTests.cpp:87` (`"CPU 9.0%"`),
      `portable_ui_tests.cpp:4280` (`"CPU 12.5%"`) and `:4496` (`"CPU 3.0%"`),
      `runtime_main_component_tests.cpp:851` (`"CPU 12.5%"`), plus
      `External/Sheaf/projects/synth/browser/tests/audio-flow.spec.ts:823`
      — NOT `app/browser/`, which has no `.spec.ts` at all — (sentinel `deadlineText:
      "CPU 0.0%"`, read by node id `runtime.sidebar.deadline` at `:829-830`,
      asserted `not.toBe("CPU 0.0%")` at `:850`). Every one of them changes.
      Note two of them assert `"CPU 12.5%"` — exactly the `.5` boundary task
      5.6 warns about.
- [x] 5.6 `%.0f` rounds half to even: 12.5 renders 12, not 13. Do not pick test
      values on a `.5` boundary, or the test asserts the rounding mode as much
      as the format.
- [x] 5.7 sru-2 still holds; preflight re-read it and the reading is right. It
      does NOT live in this repo — it is
      `External/Sheaf/openspec/specs/synth-runtime-ui/spec.md:30`, and it
      requires "the maximum audio callback load percentage over a rolling window
      of recent UI frames, updated on the UI timer". No number is fixed, so a
      shorter window satisfies it.

- [x] 5.9 BECAUSE sru-2 IS SHEAF'S, THE SPEC CHANGE IS SHEAF'S TOO. This change
      alters behaviour that Sheaf's own `synth-runtime-ui` capability specifies
      — the window's unit and length, and the displayed precision — and this
      change's `froggers-sheaf-runtime-app` delta cannot specify another repo's
      capability. Write the corresponding delta in Sheaf's own OpenSpec on the
      PR #9 branch, or record why the frogg3rs-side requirement is sufficient.
      Do not leave Sheaf's spec describing a readout Sheaf no longer has.
- [x] 5.8 A test that the sidebar's deadline node carries the new text, through
      `BuildSidebarTree` rather than `FormatDeadlineText` alone, and a test that
      the shortened window drops a stale peak within its own span.
      POSITIVE CONTROL for the window: a spike must still be visible one frame
      later, or the change has traded one wrong reading for another.

## 6. Docs for a reader, not a builder

- [x] 6.1 `README.md`: the 229-line Daisy firmware build section (lines 78-306,
      measured) leaves the README. Preflight established the gap, so this is a
      MOVE and not a delete: `DAISY_MANUAL.md` (331 lines) already covers the
      DFU procedure and address (`:304-317`, `0x08000000`) and mentions the
      bootloader (`:327`), but has NOTHING on the ARM toolchain paths, the
      linker scripts, the `APP_TYPE` policy, or the vendored bootloader binary
      filenames (`README.md:32-36,62,148-195,233`). Those four topics are what
      has to land in `DAISY_MANUAL.md` before the README section goes.
- [x] 6.2 The "Local Planning And Hygiene" section goes: OpenSpec artifact
      locations, agent git conventions and where shared source belongs are
      working process, not operator documentation.
- [x] 6.3 What survives leads with the instrument and reaches the parameter
      reference without passing through build instructions. Build commands stay
      — someone has to be able to build it — but as a short section, not the
      body.
- [x] 6.4 `MANUAL.md` re-read end to end against the same test: it was rewritten
      once already this session for tone, but not audited for whether a learner
      can follow it. Report what is still addressed to a developer.
- [x] 6.5 Every link and path in both files resolves. Preflight swept both:
      `MANUAL.md` is clean, and `README.md` has two dangling paths, both unbuilt
      artifacts — `External/DaisySP/build/libdaisysp.a` (`:146`) and
      `External/DaisySP/DaisySP-LGPL/build/libdaisysp-lgpl.a` (`:148`). Both sit
      inside the 229-line block 6.1 removes, so confirm they leave with it
      rather than fixing them in place. `src/Froggers` (`:172`,
      `DAISY_MANUAL.md:319`) is NOT dangling — the text says it lives in the
      external `dazed-and-con-fielded` repo. Re-sweep after the move, since 6.1
      carries paths INTO `DAISY_MANUAL.md`.

## 7. Nothing else moved

- [ ] 7.1 App suite green with counts.
- [ ] 7.2 `app/vst` ctest 3/3.
- [ ] 7.3 Sheaf gate, with the braid4 caveat above stated explicitly.
- [ ] 7.4 ONE republish, then the full e2e suite, idle and under load.
      RESULT: one republish, then 46 passed / 1 failed. The failure is
      `[pages] blank-frame.spec.mjs` timing out at 45s, and it is NOT logged as
      a flake. The page showed the boot panel with "Failed to execute
      'postMessage' on 'Worker': SharedArrayBuffer transfer requires
      self.crossOriginIsolated" — the first-visit cross-origin-isolation race.
      The `[pages]` project runs `serve-site.mjs --no-isolation-headers`
      deliberately, to reproduce GitHub Pages, which cannot send COOP/COEP
      headers; isolation there comes from `site/coi-serviceworker.js`, which
      registers, waits for `ready`, and reloads once (`:67-110`).
      Re-run ALONE it passes in 656ms. So the shim's first-visit path is
      correct and the 45s timeout is that path widened by machine load while
      the other 46 tests ran. NOT introduced by this change: the failure is a
      SharedArrayBuffer transfer in the module worker, and nothing here touches
      isolation headers, the shim, or the service worker.
      STILL WORTH SOMEONE'S ATTENTION, because it is the published
      configuration and a first visit is the failing case: a real visitor on a
      slow device is the same shape as a loaded CI machine. Not fixed here.
- [x] 7.5 Pin bump for the Sheaf work, as its own step, after PR #9's head moves.
- [ ] 7.6 Both trees clean and pushed.

## SUPERSEDED FOR WHAT REMAINS

Sections 1, 2, 4, 5 and 6 shipped (`ff1d110`, `0acbe93`, `260b5cf`, `d09e8f7`,
`faf9db0`; Sheaf `80d9f4bb`). Everything still open below — section 3, the
postflight findings, the operator checks, and task 5.0, which was a positive
control that WAS NEVER RUN before section 5 shipped — is taken over by
`openspec/changes/frogg3rs-first-visit-and-open-repairs`. Do not work these
items here.

## 7b. Postflight findings not fixed here

Recorded rather than actioned, so they are not rediscovered as surprises.

- [ ] 7b.1 `RollingMax` (`MidiConfigViewModel.hpp:36`) and `RollingBuffer<N>`
      (`DspBuffers.hpp:99`) are two ring buffers with the same shape. Task 5.1's
      "FOUND 4, CLAIMED 4" counted references to the OLD NAME and never asked
      whether an equivalent structure already existed — a §7 enumeration by
      name where it should have been by concept. They differ in ways that make
      collapsing them a real change, not a tidy-up: template capacity versus
      runtime capacity, DSP thread versus message thread, Min-and-Max versus
      Max. Not consolidated here; naming it so the next reader starts from the
      comparison rather than the discovery.

- [ ] 7b.2 `AUDIO_STATUS_LINE_SELECTOR` (`e2e/helpers.mjs`) and Sheaf's own
      `audio-input.spec.ts:84` both hardcode the node id
      `runtime.audio.status_line`. Two repos, two suites, one id. A shared
      helper cannot cross the submodule boundary without exporting test
      support from Sheaf, which is a larger decision than this change should
      make. The id itself is single-sourced in `RuntimePages.hpp:57`; it is the
      SELECTOR that is written twice.

- [ ] 7b.3 The disabled-cell grey is a third "disabled" colour in the combined
      codebase, alongside `pagestyle::kDisabledText` and `kDisabledButton`
      (`RuntimePageStyle.hpp:14,20`). Deliberate, and the reason is at the
      constant: those belong to the runtime chrome's config-page palette and
      the encoder grid is a separate visual system. If the two should agree,
      that is one palette decision for both.

- [ ] 7b.4 The `[pages]` first-visit cross-origin-isolation race, recorded at
      7.4. It is the published configuration and a first visit is the failing
      case.

## 8. Operator

- [ ] 8.1 OPERATOR: the greyed rendering, by eye — dim enough to read as
      unavailable, visible enough to read as present.
      WHAT YOU WILL ACTUALLY SEE, so the judgement is against the real thing:
      the disabled cell draws the knob body and frame in a flat muted grey
      (`Rgb(90, 96, 100)` on the body stroke), with NO value arc on it. The
      grey is explicit rather than derived: the slate publishes `Color::Off`
      for a disconnected source, and darkening black is black, so a cell dimmed
      from its own colour would have been invisible against a connected one.
      The number is a first guess and is exactly the kind of thing to change by
      eye. The arc is absent by construction rather than by
      choice — every arc `BuildEncoderDrawCommands` emits comes from a per-voice
      layer (`EncoderDraw.hpp:736-760`) and a disconnected source publishes no
      voices. If a bare dimmed body does not read as an encoder to you, the fix
      is a deliberate placeholder ring drawn by this app, not a synthesised
      voice — an arc on an unavailable source would be reporting a value it does
      not have.
- [ ] 8.2 OPERATOR: microphone input actually reaching External Audio on the
      published site, which no headless test can confirm.
