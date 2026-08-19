# Design — frogg3rs-host-state-and-visibility

Anchors are carried from the predecessor change's execution (2026-08-19)
and its reviews. Every one marked UNVERIFIED must be READ by the task
that relies on it before it is relied on (§1).

## A — DAW session state

- Current: `getStateInformation`/`setStateInformation` are no-ops
  (reported at `app/vst/FroggersPluginProcessor.hpp:670-671` in the
  group-7 review; UNVERIFIED at write time — re-read, the file has since
  grown).
- The parameter authority is Sheaf's `ParameterManager`, reached through
  the message bus; the plugin holds 92 host parameters bridged to it
  (group 7). Persisted state must round-trip THE AUTHORITY's values, not
  the host-parameter shadows — otherwise a restore fights the bridge's
  feedback guard.
- TRACE FIRST (UNVERIFIED, all): does Sheaf already serialize app state
  for its own hosts (the launcher persists patches —
  `browser/src/persistence.ts`, `synth::SheafPatchDataPathsForApp` used
  by `app/FroggersMain.cpp:16-19`)? If a portable
  serialize/deserialize exists at the app or engine layer, the plugin
  MUST reuse it (§8) rather than inventing a plugin-private format; a
  plugin-private format would also diverge from what the standalone and
  browser hosts save. Report which exists and cite it.
- Compatibility: the format must survive parameter-model growth (a bank
  or slot added later must not corrupt an old session). Prefer the
  authority's own versioned representation; if a version tag must be
  added, that is a finding to report, not a silent invention.
- Interaction with the shared data root: the plugin currently shares the
  standalone's `"frogg3rs"` data root (group 5 report, flagged). Session
  state living in the DAW project must NOT write through to that shared
  root and mutate the standalone's patches as a side effect — trace and
  state the boundary.

## B — Parameters legible before audio (frogg3rs side)

- Blocked on Sheaf `ui-state-before-audio` (preflight-approved; PR
  pending). When the pin moves, the frogg3rs-side work is verification
  plus the spec parity requirement: site loads → encoder cells show
  names and values with no click, no Play.
- The plugin editor gains the same behavior for free (the plugin's
  message pump starts at construction,
  `app/vst/FroggersPluginProcessor.cpp:148-149`, so a DAW-hosted editor
  sits in the identical pre-audio window) — assert it there too.

## C — Site visibility regression tests

- The defect class, stated precisely so the tests target it: an element
  can have correct `getBoundingClientRect` geometry and be entirely
  invisible (ancestor `height: 0` + `overflow: hidden`, as the
  2026-08-19 blackout was; also `display:none`, zero-size canvas, or
  unpainted canvas).
- Assertions to add in `app/browser/e2e/`: mount height non-zero at
  wide AND narrow; the surface root intersects the viewport; a sample of
  encoder canvases contain non-transparent pixels after audio starts
  (and, once B lands, BEFORE it); at least one wide-viewport screenshot
  comparison or pixel-sampling check so a fully blank frame fails.
- §9.1 discipline: each new assertion must be shown able to FAIL —
  demonstrate against a deliberately broken build (e.g. re-apply the
  height clear) and record the failure output in the report. An
  assertion never seen red is not a regression test.

## D — Cross-bank automation and the visible page

- Mechanism (verified, group 7 + 8): a host parameter write for bank N
  pushes `MessageIn::SelectParamBank` and now also
  `FroggersAppCore::RequestBankSelect`, so the core, the authority, and
  the editor agree — and the visible page follows the automation.
- The DECISION this change owes: should automation move the operator's
  view at all? Options to evaluate and cost, not to assume:
  (i) keep today's behavior (view follows automation);
  (ii) decouple view from write — route parameter writes without
  changing the shared selection (needs a way to address a bank's slot
  without selecting it — trace whether the authority supports that at
  all before proposing it);
  (iii) view follows only operator-driven selection, automation writes
  are silent.
  The operator picks; the task presents the trace and the cost of each,
  and does NOT implement a preference before that.

## E — External Audio / External EF re-enable (sar-33 adoption)

The two missing encoders are mod slots 13-14
(`app/FroggersModulation.hpp:186-187`), rendered inert by the archived
phantom-input change. Re-enabling is deliberately TWO coupled edits —
that change made a single-edit re-enable impossible on purpose
(`app/FroggersAppCore.hpp:204-206`), so both must land together:

1. **Request an input channel** — `FroggersAppCore::Config()`'s
   `numAudioInputs` (currently 0, `app/FroggersAppCore.hpp:171-186`).
2. **Drive `connected` from the ROUTED signal, never from channel
   presence** — `SetExternalAudioConnected()` already exists as the
   writer this needs (`app/FroggersModulation.hpp:464-467`, kept
   explicitly for this day). Its input is Sheaf's `sar-33`
   `InputRoutingSignal`: `Routed()` plus
   `SetInputRoutedChangedCallback()`
   (`External/Sheaf/.../AppContext.hpp:203-294`), published by both
   backends' `RefreshInputRoutedState`. UNVERIFIED and to be traced
   before wiring: the exact accessor the app reaches it through
   (`InputRouted()` on the context per `:290-294`), which thread the
   callback fires on, and whether `Step()` may read it or must take it
   from a published snapshot (the phantom-input change removed the
   per-sample recompute for good reason — do NOT reintroduce it;
   subscribe to the change callback and write `connected` once per
   transition).

The privacy property the phantom-input change bought must survive:
requesting a channel must not by itself present the sources as
connected, and — UNVERIFIED, trace before deciding — confirm whether
`numAudioInputs > 0` still opens the default device unasked at the
current Sheaf pin. If it does, that is a REPORTABLE conflict between
edit 1 and the privacy property, not something to trade away silently:
stop and surface it, because the whole point of sar-33 is that routing
is now knowable without guessing.

**Plugin host note:** in a DAW the routing question answers itself — a
sidechain/input bus is explicit operator routing. The plugin currently
declares NO input bus (hosts change, group 5). Whether the plugin gets
an input bus here, or in its own change, is an operator call to make
once the standalone/browser path is traced.

## Gates

`cd app && nice make -j2 test` after every group; plugin builds VST3+AU
at -j2 nice'd; browser build + e2e green; NEVER above -j2 on this
machine.
