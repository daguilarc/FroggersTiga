# Design — frogg3rs-host-state-and-visibility

All anchors below were RE-READ 2026-08-19 during the operator's
omni-rule audit unless explicitly marked UNVERIFIED; the handful still
marked UNVERIFIED must be read by the task that relies on them (§1).
Standing operator rule for this change: every problem found during
execution is fixed inside this change — no flag-and-defer, no
"future change" notes. If a finding genuinely conflicts with a decision
recorded here, stop and ask the operator; do not silently trade either
way.

## A — DAW session state

- Current: `getStateInformation`/`setStateInformation` are no-ops at
  `app/vst/FroggersPluginProcessor.hpp:204-205` (verified 2026-08-19,
  with the group-7 comment above them explicitly flagging session
  recall as unimplemented).
- The parameter authority is Sheaf's `ParameterManager`, reached through
  the message bus; the plugin holds 92 host parameters bridged to it
  (group 7). Persisted state must round-trip THE AUTHORITY's values, not
  the host-parameter shadows — otherwise a restore fights the bridge's
  feedback guard.
- TRACE FIRST (UNVERIFIED, all): does Sheaf already serialize app state
  for its own hosts? Known entry points to read: the browser launcher's
  patch persistence
  (`External/Sheaf/projects/synth/browser/src/persistence.ts` — path
  corrected at audit; the draft's bare `browser/src/persistence.ts`
  does not exist) and the standalone's data-path wiring
  (`synth::SheafPatchDataPathsForApp(dataRoot_, "frogg3rs")`,
  `app/FroggersMain.cpp:53`). If a portable serialize/deserialize
  exists at the app or engine layer, the plugin MUST reuse it (§8)
  rather than inventing a plugin-private format; a plugin-private
  format would also diverge from what the standalone and browser hosts
  save. Report which exists and cite it.
- Compatibility: the format must survive parameter-model growth (a bank
  or slot added later must not corrupt an old session). Prefer the
  authority's own versioned representation; if a version tag must be
  added, add it in this change and record it — not a silent invention,
  and not a deferral.
- Interaction with the shared data root: the plugin currently shares the
  standalone's `"frogg3rs"` data root (group 5 report, flagged). Session
  state living in the DAW project must NOT write through to that shared
  root and mutate the standalone's patches as a side effect — trace the
  boundary and enforce it with a test (task 2.3), not a comment.

## B — Parameters legible before audio (frogg3rs side) — UNBLOCKED

- CORRECTED AT AUDIT: Sheaf `ui-state-before-audio` is landed AT THE
  CURRENT PIN. Submodule commit `80c4eab8` is that feature; the
  mechanism is the `uiStatePublisher_` claim machine with the one-way
  `audioOwnsUiState_` latch
  (`External/Sheaf/projects/synth/include/synth/Engine.hpp:413-460`,
  read 2026-08-19). No pin move is required. The frogg3rs-side work is
  verification plus the spec parity requirement: site loads → encoder
  cells show names and values with no click, no Play.
- The plugin editor gains the same behavior in the identical pre-audio
  window: the plugin's message pump starts in the constructor —
  `startTimerHz(30)` at `app/vst/FroggersPluginProcessor.cpp:158`
  (citation corrected at audit; the draft's `:148-149` pointed into the
  comment above it), guarded by
  `MessageManager::getInstanceWithoutCreating()` at `:157`. A real host
  always has a MessageManager before constructing a processor, so the
  guard is production-inert; headless tests drive
  `PumpMessageThreadForTest()` instead. Assert pre-audio legibility in
  the plugin editor too.

## C — Site visibility regression tests

- The defect class, stated precisely so the tests target it: an element
  can have correct `getBoundingClientRect` geometry and be entirely
  invisible (ancestor `height: 0` + `overflow: hidden`, as the
  2026-08-19 blackout was; also `display:none`, zero-size canvas, or
  unpainted canvas). The current suite is geometry-only
  (`app/browser/e2e/helpers.mjs:41` and `mobile-stacking.spec.mjs`
  assert via `boundingBox()`).
- Assertions to add in `app/browser/e2e/`: mount height non-zero at
  wide AND narrow; the surface root intersects the viewport; a sample of
  encoder canvases contain non-transparent pixels after audio starts
  (and, with B verified, BEFORE it); at least one wide-viewport
  screenshot comparison or pixel-sampling check so a fully blank frame
  fails.
- §9.1 discipline: each new assertion must be shown able to FAIL —
  demonstrate against a deliberately broken build (e.g. re-apply the
  height clear) and record the failure output in the report. An
  assertion never seen red is not a regression test.

## D — Cross-bank automation and the visible page

- Mechanism (verified, group 7 + 8): a host parameter write for bank N
  pushes `MessageIn::SelectParamBank` and now also
  `FroggersAppCore::RequestBankSelect`, so the core, the authority, and
  the editor agree — and the visible page follows the automation.
- **DECIDED (operator, 2026-08-19): option (iii) — the visible page
  follows operator-driven selection ONLY; automation writes are
  silent.** A DAW lane must never move the page under the operator's
  hands, and simultaneous cross-bank lanes must not oscillate it. The
  value still lands on its own bank's parameter.
- Why this is not free (traced): `MessageIn::ParamSetAbsolute` is
  SLOT-addressed, not (bank, slot)-addressed — the bridge pushes
  `slotIx=0` (`app/vst/FroggersPluginProcessor.cpp:921`) and the value
  lands on whatever bank that shared slot points at, which is why a
  `SelectParamBank` push must precede it (`:908`). The standalone never
  hits this because its encoders only ever edit the visible bank; the
  plugin is the first host that writes to banks nobody is looking at.
  The ordering constraints are documented at
  `app/vst/FroggersPluginProcessor.cpp:768-835` and must not be
  violated (a `RequestBankSelect` used as a REPLACEMENT applies one
  block late).
- **ROUTE DECIDED (operator, 2026-08-19, after 4.1's trace): route (b),
  the framework-side bank-addressed write.** Reason it beat the
  preferred app-side route: route (a) cannot avoid collateral damage.
  Writing to a non-visible bank requires selecting it, and
  `BankSlot::SelectBank` calls `Deselect()` on the outgoing bank
  (`External/Sheaf/projects/synth/src/ParameterModulation.cpp:2924-2932`),
  which resets `visible_ = topLevel_; selected_ = nullptr`
  (`:2693-2708`) — closing the operator's open modulation drill-down —
  while `FroggersModulationDrillIn::level_` is a plain cached counter
  never re-derived on read (`app/FroggersModulation.hpp:745,863`), so it
  goes stale. Under this change's "the page never moves" behavior that
  damage becomes SILENT. Route (b) never touches `BankSlot`, so the
  defect stops existing instead of being managed; leaving it in place
  would violate the no-deferrals rule.
  It also passes §6's generality test: `Bank`/`BankSlot` already model
  "one physical surface, many logical banks", `Bank::HandleSetAbsolute`
  is ALREADY a public bank-direct primitive
  (`External/Sheaf/projects/synth/include/synth/ParameterModulation.hpp:618`)
  with no `BankSlot` dependency — `ParameterManager` simply never
  exposed a slot-agnostic wrapper over it. Any Sheaf app layering
  external automation (host parameters, MIDI-learn, a sequencer) on a
  shared `BankSlot` hits the identical gap.
- Routes as traced (both were live before the decision above):
  (a) app-side: select → write → restore the previous selection inside
  the same `DrainMessageBus` call, keeping `activeBankIx_`/`drillIn_`
  consistent through the matching `RequestBankSelect`;
  (b) framework-side: add a BANK-ADDRESSED parameter write to Sheaf's
  message bus / `ParameterManager` so the write never touches the
  shared selection at all.
  **Route (a) is preferred when it is clean.** Route (b) is warranted
  only if "address a parameter by (bank, slot) without disturbing the
  shared selection" is a GENERAL gap any multi-bank app facing external
  automation would hit — §6 at framework scale, the same test a helper
  faces: a real domain concept with genuine reuse, not a single-caller
  convenience. Would the API make sense to an app author who has never
  heard of frogg3rs? If yes, it is additive framework work (new API
  alongside the slot-addressed one, never changed semantics) appended to
  open PR #9. If no, it is disqualified. Conversely, if avoiding the
  framework forces a worse structure — a second definition site, a
  lookup table, an added branch — the constraint is the suspect part
  (§1 corollary), so say so rather than building the workaround.
- Whichever route lands, `RandomizePage`/`ResetPage` targets
  (`app/FroggersAppCore.hpp:687-706`, acting on `*drillIn_`) must
  follow the OPERATOR's bank, not the last automated one, and the
  existing test
  `host_automation_in_a_non_visible_bank_keeps_active_bank_and_page_actions_in_sync`
  is rewritten to the new behavior, not deleted.

## E — External Audio / External EF re-enable (sar-33 adoption)

The two missing encoders are mod slots 13-14
(`app/FroggersModulation.hpp:186-187`, verified). Re-enabling is
deliberately TWO coupled edits — the phantom-input change made a
single-edit re-enable impossible on purpose
(`app/FroggersAppCore.hpp:197-206`, the "What would justify raising
this again" block) — so both land together:

1. **Request an input channel** — `FroggersAppCore::Config()`'s
   `numAudioInputs` 0 → 1 (`Config()` at `app/FroggersAppCore.hpp:169`,
   the assignment at `:207`; citations corrected at audit). The
   surrounding history-essay comment is rewritten in the same edit to
   state the NEW contract plainly (routed signal gates connection),
   ahead of the group-8 sweep.
2. **Drive `connected` from the ROUTED signal, never from channel
   presence** — `SetExternalAudioConnected()` is the writer, kept for
   this day (`app/FroggersModulation.hpp:464-467`, verified). Its
   input is sar-33's signal, verified at the pin:
   - `InputRoutingSignal`
     (`External/Sheaf/projects/synth/include/synth/AppContext.hpp:224-241`),
     app-facing accessors `InputRouted()` (`:300-302`) and
     `SetInputRoutedChangedCallback()` (`:309-311`).
   - Thread contract VERIFIED: the callback fires synchronously from
     `Publish()` on the message thread — every publisher is a
     message-thread path (`AppContext.hpp:230-231`,
     `Runtime.hpp:683-687`).
   - Routed semantics VERIFIED
     (`External/Sheaf/projects/synth/runtime/Runtime.hpp:662-694`):
     routed iff the operator's persisted input-device selection is
     non-empty AND matches the currently open device. A default-opened
     device leaves the persisted selection EMPTY, so it derives FALSE
     by construction (`:671-677`).
   Wire it once per transition: subscribe the change callback at app
   construction, write `SetExternalAudioConnected(routed)` on each
   change plus once at startup from `InputRouted()`. Do NOT reintroduce
   any per-sample recompute in `Step()` (the phantom-input change
   removed that deliberately). Remaining executor trace (UNVERIFIED):
   whether `Step()`/the audio thread reads `connected` and therefore
   whether the message-thread write needs a release/acquire pair or a
   published snapshot — read the consumer before wiring the writer.

**Privacy property — DECIDED, recorded:** with `numAudioInputs = 1`,
the runtime still opens the platform-default input device at launch
(`Runtime.hpp:261-262`, unconditional; `:671-677` documents it), and
sar-33 reports that device as NOT routed, so the sources stay inert.
Operator ruling 2026-08-19, citing Sheaf PR #9: sources-off-by-default
via the routed signal IS the accepted fix; the launch-time default
device open is accepted and is not a blocker. The modulation-slate
delta rewrites the deployed requirement to match (availability =
affirmative routing, replacing the zero-channels mandate). Do not
re-open this decision at execution.

**Plugin host — IN SCOPE HERE (nothing deferred):** the deployed
`froggers-vst-host` bus requirement reserved external audio for "a
separate change" once the core gained inputs; this is that change. The
plugin gains an audio input bus (sidechain-style: optional, not
required for instantiation), and in a DAW the routing question answers
itself — an input bus the DAW user connected IS explicit operator
routing. Executor trace (UNVERIFIED): the JUCE bus-layout API for an
optional instrument input bus, what the archived hosts change's
`processBlock` does with input buffers today, and the cleanest
host-side derivation of "connected" (bus enabled with nonzero
channels) feeding the same `SetExternalAudioConnected()` writer once
per layout change — never per block.

## F — PM rate floor (operator addition 2026-08-19)

- Trace (verified): the shared PM rate knob is Audio slot 12
  (`app/FroggersAppCore.hpp:1482`), fed to every VCO's `StepPmLfo`,
  which maps knob01 exponentially over
  `[kPmLfoMinHz = 0.05f, kPmLfoMaxHz = 20.0f]`
  (`app/dsp/Vco.hpp:103-104`, mapping at `:192`). Knob minimum today =
  0.05 Hz, a 20 s cycle — effectively static, a de-facto second off
  switch. True zero already belongs to the per-VCO depth knobs
  (`PmDepthScale`, `froggers-vco-topology`'s true-zero requirement).
- Change: raise the floor so the knob's minimum is a slow but plainly
  audible rate. Operator ruling: the exact value is the executor's
  pick (order of a few seconds per cycle, e.g. ~0.2-0.5 Hz), confirmed
  at operator smoke; the requirement is "slow but audible, above zero",
  nothing more specific.
- Consequences to handle in the same group, not after:
  - `app/FroggersDspParityTests.cpp:4926` asserts against the constant
    symbolically (`kPmLfoMinHz / sr`) so it tracks the new floor;
    re-run, don't assume.
  - This is a deliberate divergence from `src/core`'s
    `x_pmLfoMinHz = 0.05f` (`src/core/FroggersEngine.hpp:135`). The
    rate-knob decoupling already diverged deliberately; state the new
    divergence in `Vco.hpp`'s comment in plain terms (what the floor
    is and why), not as packet history.
  - `app/FroggersParameters.hpp:174-179`'s claim that the unset 0.0
    default "reproduces today's PM LFO rate exactly" becomes stale the
    moment the floor moves — rewrite it in the same edit.

## G — Reset restores the default patch (operator addition 2026-08-19)

- Trace (verified): `ResetPage` (`app/FroggersModulation.hpp:1537`) and
  `ResetAll` (`:1566`) reset page-parameter values to 0.0 via
  `detail::ResetBankValues` (`:1349`) /
  `ResetParameterValueAndDepths` (`:1330`: `HandleSetAbsolute(pole,
  0.0f)` on both scene poles + depths to neutral). A fresh launch
  instead applies `ApplyFroggersDefaultPatch`
  (`app/FroggersAppCore.hpp:272`; `app/FroggersModulation.hpp:1596`):
  scene-1 VCO shapes 0/0.5/1 mirrored in scene 2, cross-VCO
  pitch-detent depths on Audio slots 0-2, Drive (Drive bank slot 0)
  = 0.2. So reset produces a state no fresh launch ever shows.
- Required behavior (operator rulings, both 2026-08-19):
  - Reset = revert to the DEFAULT PATCH ("0 for most but not all
    parameters"), at every level. One rule, no special cases.
  - **Reset All is GLOBAL**: every bank's page parameters AND every
    bank's local Crispy (slot 14, `app/FroggersParameters.hpp:79`) AND
    the single shared global Crunchy (slot 15,
    `app/FroggersParameters.hpp:37`) revert to their defaults (0 for
    all of these). The existing `includeCrispy=false` carve-out and
    the never-touch-Crunchy rule (`FroggersModulation.hpp:1546-1556`,
    inherited from the Randomize siblings' reasoning) are OVERRULED
    for Reset — randomize keeps its own rules, reset does not borrow
    them. Executor trace (UNVERIFIED): the accessor path for writing
    Crunchy (it lives outside `ResetBankValues`'s per-bank
    enumeration; find its parameter object and writer).
  - Reset Page = the CURRENT bank's slice of the same default patch:
    that bank's page parameters, its Crispy, and the default-patch
    depths that target that bank's parameters (Audio gets its shape
    values and pitch detents back; Drive gets 0.2; other banks get
    zeros/neutral).
  - Drilled-in grids (Level ≥ 1): same rule — the selected parameter's
    depths revert to their DEFAULT-PATCH values (neutral for
    everything except the three Audio pitch parameters' detents),
    replacing today's flat to-neutral clear where the two differ.
- Structure (§8, single source of truth): the default patch must have
  ONE definition shared by boot and reset. Restructure
  `ApplyFroggersDefaultPatch` into bank-addressable form (e.g. a
  per-bank apply the whole-patch apply iterates, plus the globals) so
  `ResetPage`/`ResetAll` reuse the same values and a future default-
  patch edit cannot desynchronize boot from reset. No second copy of
  the constants anywhere.
- Test oracle (the strongest available): reset-after-randomize must
  land field-for-field on the state of a freshly constructed model
  with the default patch applied — compare against a fresh instance,
  not against hand-written expected literals. §9.1 positive control:
  prove the pre-reset state was non-default before asserting the
  post-reset equality.

## H — Openspec-comment sweep (operator addition 2026-08-19; final group)

- The rule: comments explain what the code does and why it is shaped
  that way — never how it got there through openspec iterations. "task
  6.5", "packet P6a", "group 7", "design D16", "review fix, Minor 3",
  "T5.1", "carry-forward 3", "audit-added", citations into
  `tasks.md`/`proposal.md`/change archives — all of it goes.
- Method (§8, operand-based enumeration): grep the live tree for the
  tokens the concept cannot avoid — `task `, `Task [A-Z0-9]`,
  `packet`, `group [0-9]`, `tasks.md`, `proposal.md`, `design.md`,
  `design [A-Z]?[0-9]`, `[DT][0-9]+`, `P[0-9]+[ab]?`, `§`, `review
  fix`, `Minor [0-9]`, `Major [0-9]`, `openspec`, `carry-forward`,
  `C[0-9]+[abc]?`, `sar-[0-9]+`, `audit`, `operator ruling`, plus
  whatever new tokens groups 1-7 of THIS change introduced. Classify
  EVERY hit before editing any (§8):
  (a) pure openspec provenance → delete or rewrite as a plain
  statement of behavior;
  (b) load-bearing rationale wearing openspec clothes (the
  depth-neutral trap, the MessageManager timer guard, the stop-edge
  teardown reasoning, the phantom-input contract) → KEEP the
  mechanism explanation, strip the provenance tokens;
  (c) citations to real code (`src/core/...:line` parity anchors,
  Sheaf headers) → keep; they cite code, not openspec artifacts.
- Report count FOUND vs count CHANGED per file (§8) — a silent partial
  sweep is worse than none.
- Scope: the live `app/` tree (all sources, tests, e2e, build
  scripts). Excluded, with reasons the operator can override: `web/` and
  `wasm/` (their byte-identity is a deployed `froggers-web-host`
  requirement), `desktop/`, `desktop-v2/`, `sim/`, `src/` (frozen; and
  `src/` is the parity reference the tests cite), `External/Sheaf`
  (upstream's repo).
- Ordering: this group runs LAST among the code groups, so the sweep
  also catches comments groups 1-7 introduce. The change-level
  postflight then re-runs the grep and reports zero remaining hits
  outside the classified keepers.

## Gates

`cd app && nice make -j2 test` after every group; plugin builds VST3+AU
at -j2 nice'd; browser build + e2e green; NEVER above -j2 on this
machine. Baseline at the predecessor's close: app suite 279/279,
app/vst ctest 3/3 — re-establish the real number at group 1 and hold
it plus additions thereafter. Verification runs go through a cheap
subagent per §16.1; the parent reads counts and failure tails only.
