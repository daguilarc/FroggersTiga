# Proposal — `frogg3rs-host-state-and-visibility`

**Created 2026-08-19 at the operator's instruction.** Successor to
`frogg3rs-browser-and-vst-hosts`, superseding the VST host's REMAINING
scope only: that change's implemented work (browser build, packaging,
site, plugin host, transport, tempo, parameters, editor) stays where it
is and archives on its own gates. This change carries the four items
that surfaced during its execution and operator smoke and were
deliberately NOT expanded into it.

## Why

Four deferrals, each with a traced cause rather than a hunch:

- **DAW session state is not persisted.** `getStateInformation`/
  `setStateInformation` remain no-ops (`app/vst/FroggersPluginProcessor.hpp`
  — verify the exact line at execution; they were reported at `:670-671`
  during group 7). A DAW project save/reload therefore restores nothing
  a user changed by hand; only host-automated parameters come back,
  because the DAW re-writes those itself. The hosts change's spec scope
  was the automation SURFACE ("parameters are external via a stable
  automation surface"), not session serialization, so implementing it
  there would have been scope expansion — but shipping an instrument
  that forgets its patch is not acceptable, so it lands here.
- **Parameters are invisible before Play in the browser.** Encoder cells
  render nothing until audio starts or an action is dispatched, because
  `Engine::PopulateUIState` is reachable only from the audio pump
  (Sheaf `include/synth/Engine.hpp:413-421`). The predecessor site
  guaranteed the opposite (`openspec/specs/web-mobile-knob-labels`:
  labels visible on load, before Play). The platform half of the fix is
  Sheaf's `ui-state-before-audio` change (preflight-approved 2026-08-19,
  landing upstream by PR); THIS change owns the frogg3rs-side
  verification and the parity requirement.
- **The site's regression tests cannot see a blank page.** The e2e suite
  and the controller's own checks asserted geometry via
  `getBoundingClientRect`, which reports full boxes for elements clipped
  to invisibility — so a total wide-viewport blackout (found by operator
  smoke 2026-08-19, fixed in the hosts change) would have passed every
  gate. Visibility must be asserted, not inferred.
- **Cross-bank host automation moves the visible bank.** Writing a
  parameter in a non-visible bank selects that bank on the shared
  `BankSlot` (group 7 design; correctness verified, UX unresolved). With
  a DAW running automation lanes across banks, the editor's visible page
  can change under the user's hands. The hosts change made the editor
  track the real selection; it did not decide what SHOULD happen.

- **External Audio and External EF are missing from the modulation
  pages, and the blocker they were waiting on has been fixed.** The
  archived `frogg3rs-external-audio-phantom-input` change (2026-08-10)
  set `Config().numAudioInputs = 0` and left mod slots 13-14
  (`kModSlotExternalAudio`, `kModSlotExternalAudioEf`,
  `app/FroggersModulation.hpp:186-187`) present-but-`connected=false`,
  because a nonzero input count made Sheaf open the platform's DEFAULT
  input device unasked (`app/FroggersAppCore.hpp:171-186`) — an app
  could not distinguish "the device presented a channel" from "the
  operator routed something in". That is Sheaf issue #4, and it is
  FIXED: `sar-33`'s `InputRoutingSignal` (`Routed()`, change callback,
  published by both backends' `RefreshInputRoutedState` —
  `External/Sheaf/projects/synth/include/synth/AppContext.hpp:203-294`)
  is on the branch this repo already pins, in open upstream PR #9.
  frogg3rs simply never adopted it. Re-enabling is the two coupled
  edits that change deliberately required, now unblocked.
  (`UPSTREAM-SHEAF-ASK.md`'s ledger still says ask 8 / this capability
  is not landed — that row is STALE, written 2026-08-09, and this
  change corrects it.)

## What Changes

- **froggers-vst-host** (delta): ADDED — session state is persisted and
  restored through the host's own state calls; ADDED — cross-bank
  automation does not disturb the operator's visible page.
- **froggers-web-host** (delta): ADDED — parameter controls are legible
  before audio starts; ADDED — automated checks assert rendered
  visibility, not geometry alone.
- **froggers-modulation-slate / the modulation capability** (delta):
  ADDED — External Audio and External EF are live modulation sources
  when, and only when, the operator has actually routed an input.
- Code: `app/vst/` (state serialization; bank-visibility policy),
  `app/browser/e2e/` (visibility assertions), `app/FroggersAppCore.hpp`
  + `app/FroggersModulation.hpp` (the two coupled external-audio
  edits), plus the frogg3rs-side verification of Sheaf's upstream
  UI-state fix.

## Impact

- Affected specs: `froggers-vst-host`, `froggers-web-host` (both ADDED
  deltas; no removals, no rewrites of the hosts change's requirements).
- Upstream dependency: the pre-Play requirement cannot be satisfied
  until Sheaf's `ui-state-before-audio` lands and the submodule pin
  moves. That ordering is a task gate here, not an assumption.
- Explicitly NOT in scope: re-litigating the hosts change's delivered
  behavior; anything in the frozen `desktop/`, `desktop-v2/`, `web/`,
  `wasm/`, `src/` trees.
- Delivery: separate branch per repo convention; operator gates
  testing before any commit, as with the predecessor.
