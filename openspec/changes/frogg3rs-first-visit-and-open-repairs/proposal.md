# Proposal — `frogg3rs-first-visit-and-open-repairs`

**Created 2026-08-27.** Takes over every open item of
`frogg3rs-web-release-repair`, which shipped its sections 1, 2, 4, 5 and 6 in
commits `ff1d110`, `0acbe93`, `260b5cf`, `d09e8f7`, `faf9db0` (and Sheaf
`80d9f4bb`). That change is finished except for what is listed here, and
absorbs its `form-control-width-deferred.md`.

Two of these are defects that reached the published site. The rest are things
the previous change found, recorded, and did not act on.

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
passed in 656ms when run alone.

**The fix is to stop racing.** While the shim still owes this page an isolation
attempt, `site-boot.mjs` should not boot and should not paint a failure — the
reload is going to discard both. Once the shim's one-shot guard has already
fired, booting is right: the page has had its reload, and any remaining failure
is real and should surface exactly as it does today.

That needs one bit of shared state, and it must be SINGLE-SOURCED. The shim
already owns `RELOAD_GUARD_KEY` (`coi-serviceworker.js:81`); `site-boot.mjs`
must not re-derive that string. The shim publishes whether an attempt is still
owed; the boot path reads it.

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
dropped, and it shipped. So the claim that this fixes what the operator saw on a
phone rests on the proposal's own arithmetic and nothing else.

Nothing here argues the change was wrong. It argues that an unrun control is not
a passed one, and that the number on the operator's phone is still unexplained
by measurement.

## 3. A captioned control cannot declare its own width

Carried from `form-control-width-deferred.md`, whose requirement text moves into
this change unchanged.

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

## 4. Browser MIDI has no path in

`installSynthBrowserApp` receives MIDI access only from a consumed
`ActivationLease` (`main.ts:336,347`), and a lease cannot be acquired on a page
with no launch gesture — that is what section 1 of the previous change
established. So the site's MIDI is unreachable, and the previous proposal's
claim that MIDI came along with the microphone fix was withdrawn rather than
delivered.

Separating "here is a context" from "activation happened" in Sheaf's own
interface is what would fix both this and the lease problem properly.

## 5. Duplications the previous change created or left standing

- The disabled-cell grey (`FroggersUiSurface.hpp`, `Rgb(90, 96, 100)`) is a
  third "disabled" colour beside `pagestyle::kDisabledText` and
  `kDisabledButton` (`RuntimePageStyle.hpp:14,20`). It was left deliberate and
  documented, on the grounds that those belong to the config-page palette and
  the encoder grid is a separate visual system. That reasoning deserves a
  decision rather than a comment.
- `RollingMax` (`MidiConfigViewModel.hpp:36`) and `RollingBuffer<N>`
  (`DspBuffers.hpp:99`) are two ring buffers of the same shape. They differ in
  template versus runtime capacity, DSP versus message thread, and Min-and-Max
  versus Max, so collapsing them is a real change. The previous change's §7
  enumeration counted references to the old NAME and never asked whether an
  equivalent structure existed.
- `AUDIO_STATUS_LINE_SELECTOR` (`app/browser/e2e/helpers.mjs`) and Sheaf's
  `audio-input.spec.ts:84` both hardcode the node id `runtime.audio.status_line`.
  The id is single-sourced in `RuntimePages.hpp:57`; the selector is written
  twice, across a submodule boundary.

## Non-goals

- The DSP's cost on a phone. Item 2 is about measuring the readout's behaviour,
  not about making the instrument cheaper.
- Redesigning the site's landing experience. Item 1 suppresses a failure panel
  during a reload the shim already performs; it does not add a launch screen.
- External audio's consent default, which is specified and unchanged.

## Impact

- `app/browser/site/coi-serviceworker.js`, `app/browser/site/site-boot.mjs`,
  and `app/browser/e2e/`.
- Sheaf, on the `fix-out-of-tree-app-gaps` branch (PR #9): `PortableUIBuilders.hpp`,
  `PortableUILayout.hpp`, `RuntimePages.hpp` and the portable layout tests for
  item 3; the browser launcher interface for item 4.
- Affected specs: `froggers-browser-package` (what a first visit owes),
  `froggers-sheaf-runtime-app` (the form-control width requirement returns
  here), and Sheaf's own `synth-runtime-ui` for item 2's measurement.
