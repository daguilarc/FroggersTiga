# Proposal — `frogg3rs-host-state-and-visibility`

**Created 2026-08-19 at the operator's instruction; amended the same day
by the operator's omni-rule audit.** Successor to
`frogg3rs-browser-and-vst-hosts`, superseding the VST host's REMAINING
scope only: that change's implemented work (browser build, packaging,
site, plugin host, transport, tempo, parameters, editor) stays where it
is and archived on its own gates. This change carries the deferrals that
surfaced during its execution and operator smoke, PLUS the operator's
2026-08-19 additions, under one standing rule the operator set during
the audit: **nothing found here is deferred to a future change — every
item this change surfaces, this change fixes.**

## Why

Each item with a traced cause, cited per §1 (all citations re-read
2026-08-19 during the audit; the stale ones the original draft carried
are corrected below, not preserved):

- **DAW session state is not persisted.** `getStateInformation`/
  `setStateInformation` are no-ops at
  `app/vst/FroggersPluginProcessor.hpp:204-205` (verified 2026-08-19;
  the draft's ":670-671 per group 7" was wrong — the file is 540 lines).
  A DAW project save/reload therefore restores nothing a user changed by
  hand; only host-automated parameters come back, because the DAW
  re-writes those itself. The hosts change's spec scope was the
  automation SURFACE, not session serialization — but shipping an
  instrument that forgets its patch is not acceptable, so it lands here.
- **Parameters are invisible before Play in the browser — and the fix
  has already landed.** Encoder cells rendered nothing until audio
  started because `Engine::PopulateUIState` was reachable only from the
  audio pump. CORRECTED at audit: Sheaf's `ui-state-before-audio` is
  MERGED AT THE CURRENT PIN — submodule commit `80c4eab8` IS that
  feature ("populate UI state before the audio pump has ever run"), and
  the claim-machine mechanism is readable at
  `External/Sheaf/projects/synth/include/synth/Engine.hpp:413-460`
  (`audioOwnsUiState_` latch, message-thread pre-audio populate). The
  draft's "blocked on Sheaf landing / pin moving" was stale. What
  remains is frogg3rs-side: verify at this pin, and assert the parity
  requirement the predecessor site guaranteed
  (`openspec/specs/web-mobile-knob-labels`: labels visible on load,
  before Play).
- **The site's regression tests cannot see a blank page.** The e2e
  suite asserts geometry via `boundingBox()`
  (`app/browser/e2e/helpers.mjs:41`, `mobile-stacking.spec.mjs`), which
  reports full boxes for elements clipped to invisibility — so a total
  wide-viewport blackout (found by operator smoke 2026-08-19, fixed in
  the hosts change) would have passed every gate. Visibility must be
  asserted, not inferred.
- **Cross-bank host automation moves the visible bank.** Writing a
  parameter in a non-visible bank selects that bank on the shared
  `BankSlot` (group 7 design; correctness verified, UX unresolved). With
  a DAW running automation lanes across banks, the editor's visible page
  can change under the user's hands. The hosts change made the editor
  track the real selection; it did not decide what SHOULD happen. The
  decision is made inside this change (design D).
- **External Audio and External EF are missing from the modulation
  pages, and their blocker is fixed at the current pin.** The archived
  `frogg3rs-external-audio-phantom-input` change (2026-08-10) set
  `Config().numAudioInputs = 0` and left mod slots 13-14
  (`kModSlotExternalAudio`, `kModSlotExternalAudioEf`,
  `app/FroggersModulation.hpp:186-187`) present-but-`connected=false`,
  because a channel merely existing could not be told apart from "the
  operator routed something in". Sheaf PR #9 (`sar-33`,
  github.com/jvictor0/Sheaf/pull/9) fixes exactly that, and it is ON THE
  PINNED COMMIT: `InputRoutingSignal`
  (`External/Sheaf/projects/synth/include/synth/AppContext.hpp:224-241`,
  `InputRouted()` `:300-302`, `SetInputRoutedChangedCallback()`
  `:309-311`) derives routed iff the operator's persisted input-device
  SELECTION matches the currently open device
  (`External/Sheaf/projects/synth/runtime/Runtime.hpp:688-694`); a
  host-opened platform-default device derives FALSE by construction
  (`Runtime.hpp:671-677`). **Operator ruling 2026-08-19: this routed
  signal IS the fix for the phantom-mic problem — external audio is off
  by default because the sources connect only on affirmative routing.**
  (Recorded for honesty, decided, not re-litigated: with
  `numAudioInputs=1` the runtime still opens a default input device at
  launch, reported as not-routed — `Runtime.hpp:261-262`; the operator
  accepted this, citing PR #9.) `UPSTREAM-SHEAF-ASK.md`'s ask-8 rows
  (`:27`, `:52`, `:124`) still say not-landed; they are stale and this
  change corrects them.
- **The PM rate control's minimum is a redundant off switch (operator
  addition, 2026-08-19).** The rate knob (Audio slot 12,
  `app/FroggersAppCore.hpp:1482`) maps its minimum to
  `kPmLfoMinHz = 0.05f` (`app/dsp/Vco.hpp:103`, mapped at `:192`) — a
  20-second cycle, effectively static. The per-VCO PM depth knobs
  already own the true zero (`froggers-vco-topology` "Phase modulation
  has a true zero position"), so a near-frozen rate floor duplicates
  them. Operator ruling: the floor becomes **slow but audible, above
  zero**; the exact Hz is the executor's pick, confirmed at smoke — the
  operator explicitly does not gate on the specific value.
- **Reset All / Reset Page reset to zero, not to the default patch
  (operator addition, 2026-08-19).** `ResetParameterValueAndDepths`
  writes 0.0 to both scene poles (`app/FroggersModulation.hpp:1330`),
  but a fresh launch applies `ApplyFroggersDefaultPatch`
  (`app/FroggersAppCore.hpp:272`, `app/FroggersModulation.hpp:1596`):
  scene-mirrored VCO shapes 0/0.5/1, cross-VCO pitch-detent depths,
  Drive 0.2. So reset lands on a state no fresh launch ever shows.
  Operator ruling: reset reverts to DEFAULT values — "0 for most but
  not all parameters". **And Reset All is global: the existing
  carve-out that skips per-bank Crispy (`includeCrispy=false`,
  `FroggersModulation.hpp:1546-1556`) and never touches global Crunchy
  is overruled — Reset All resets every bank's Crispy AND global
  Crunchy (slot 15, `app/FroggersParameters.hpp:37,79`) to their
  defaults too.** Found alongside: no deployed spec covers the Reset
  buttons at all — `froggers-transport-and-reset-controls`' Purpose
  promises "reset semantics for parameters and depths" and its only
  requirement is Stop silencing. That broken promise is fixed by this
  change's delta.
- **The live code is saturated with openspec-iteration comments
  (operator addition, 2026-08-19).** "task 6.5", "packet P6a", "group
  7 review fix, Minor 3", "design D16" — comments that explain how the
  code got here through openspec iterations, not what it does. A reader
  of the code should not need the change archive. A full sweep of the
  live tree rewrites them (design H), as the change's final group.

## What Changes

- **froggers-vst-host** (delta): ADDED — session state is persisted and
  restored through the host's own state calls; ADDED — cross-bank
  automation does not disturb the operator's visible page; MODIFIED —
  the bus posture gains the audio input bus its own text anticipated
  ("arrives only if the core itself gains inputs, a separate change" —
  this is that change), feeding the external-audio sources.
- **froggers-web-host** (delta): ADDED — parameter controls are legible
  before audio starts; ADDED — automated checks assert rendered
  visibility, not geometry alone.
- **froggers-modulation-slate** (delta): MODIFIED — external-audio
  availability is DEFINED by the host's affirmative routed signal
  (sar-33), replacing the zero-channels mandate; sources connect when,
  and only when, the operator has actually routed an input. (The
  original draft promised this delta and did not carry it — audit fix.)
- **froggers-vco-topology** (delta): ADDED — the PM rate control's
  minimum is a slow-but-audible moving rate, never a second off switch.
- **froggers-transport-and-reset-controls** (delta): ADDED — the Reset
  controls restore the default patch; Reset All is global, Crispy and
  Crunchy included.
- Code: `app/vst/` (state serialization; bank-visibility policy; input
  bus), `app/browser/e2e/` (visibility assertions),
  `app/FroggersAppCore.hpp` + `app/FroggersModulation.hpp` (the two
  coupled external-audio edits; reset-to-defaults over a single-sourced
  default patch), `app/dsp/Vco.hpp` (PM rate floor),
  `UPSTREAM-SHEAF-ASK.md` (stale ask-8 rows corrected), plus the
  comment sweep across the live `app/` tree.

## Impact

- Affected specs: `froggers-vst-host`, `froggers-web-host`,
  `froggers-modulation-slate`, `froggers-vco-topology`,
  `froggers-transport-and-reset-controls`.
- Upstream dependency: NONE PENDING — both required Sheaf capabilities
  (`ui-state-before-audio`, `sar-33` routed signal) are verified present
  at the pinned submodule commit `80c4eab8` (checked out and recorded at
  HEAD). Upstream PR #9 merging into Sheaf main does not gate this
  repo; the pin is on the PR branch already.
- Explicitly NOT in scope: re-litigating the hosts change's delivered
  behavior; edits inside the frozen `desktop/`, `desktop-v2/`, `web/`,
  `wasm/`, `src/`, `sim/` trees (the web/wasm trees' byte-identity is
  itself a deployed requirement, and `src/` is the DSP parity
  reference).
- **`External/Sheaf` is editable, under the omni rule's own test**
  (operator 2026-08-19). Ranked: an app-side route is PREFERRED when a
  clean one exists; a framework edit is warranted only when it makes
  Sheaf more flexible for apps IN GENERAL — never to mold Sheaf to
  frogg3rs. This is §6 at framework scale: an abstraction earns its
  place by being a real domain concept with genuine reuse, not a
  single-caller convenience wearing a shared-API costume. The operative
  test: would this API make sense to an app author who has never heard
  of frogg3rs? If it only makes sense because of our bridge's shape, it
  is disqualified. And §1's corollary still binds the other way — when
  avoiding the framework forces a worse structure (a second definition
  site, a lookup table, an added branch), the constraint is the suspect
  part, not the implementer. Qualifying edits are ADDITIVE (new API
  alongside the existing one, never changed semantics — other Sheaf
  apps consume the same surfaces), carry their own tests and postflight,
  and append to the existing open upstream PR #9 (branch
  `fix-out-of-tree-app-gaps`, already the pinned branch).
- Delivery: separate branch per repo convention; operator gates
  execution and testing before any commit, as with the predecessor.
