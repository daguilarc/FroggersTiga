# Tasks — frogg3rs-host-state-and-visibility

Gate after every group: `cd app && nice make -j2 test` green (baseline
279/279 + whatever the predecessor added; NEVER above -j2). One commit
per group. Subagent dispatch per omni-rule: lightest capable model,
sequential code changes, per-task review with §14 postflight. Design
anchors marked UNVERIFIED must be read by the executing task first.

Ordering note: group 3 is BLOCKED on Sheaf's `ui-state-before-audio`
landing and the submodule pin moving. Groups 1, 2, 4 are independent.

## 1. Site visibility regression tests (do first — it protects everything else)

- [ ] 1.1 Add the visibility assertions in `app/browser/e2e/` per design
      C: non-zero mount height at wide and narrow, surface root
      intersects the viewport, encoder canvases carry painted pixels, a
      blank-frame check that fails on an all-background render.
- [ ] 1.2 §9.1 proof: re-apply the known blackout (the wide-branch
      height clear) on a scratch copy, show EACH new assertion red,
      restore, show green. Record both outputs. An assertion that stays
      green under the reintroduced defect is not done.

## 2. DAW session state (design A)

- [ ] 2.1 Trace: existing portable serialize/deserialize at the app or
      engine layer (Sheaf persistence path, `SheafPatchDataPathsForApp`
      usage in `app/FroggersMain.cpp`); the parameter authority's own
      representation; the shared-data-root boundary. Cite each. If a
      portable path exists, REUSE it; report if it does not.
- [ ] 2.2 Implement `getStateInformation`/`setStateInformation` over the
      traced representation; restores must go through the parameter
      authority (not the host-parameter shadows) and must not fight the
      group-7 feedback guard.
- [ ] 2.3 Tests: round-trip (set values → save → construct fresh
      processor → restore → authority values match, host parameters
      reflect them); forward-compatibility behavior when the stored
      model is smaller/larger than the current one (assert the defined
      behavior, whatever the trace establishes it to be); no write-
      through to the standalone's shared data root during a project
      restore.

## 3. Parameters legible before audio (design B) — BLOCKED on Sheaf

- [ ] 3.1 Move the Sheaf pin once `ui-state-before-audio` has landed
      upstream; re-run the frogg3rs gates against the new pin.
- [ ] 3.2 Verify and assert: site loads → encoder names/values visible
      with no click and no Play; plugin editor shows the same before the
      host ever calls `processBlock`.

## 4. Cross-bank automation policy (design D)

- [ ] 4.1 Trace whether the parameter authority can address a bank's
      slot WITHOUT selecting that bank; cost each of the three options.
- [ ] 4.2 OPERATOR DECISION: present the trace and costs; implement only
      the chosen option. Do not implement a default first.
- [ ] 4.3 Tests for the chosen behavior, including the multi-lane case
      (two banks automated simultaneously).

## 5. External Audio + External EF re-enable (design E)

- [ ] 5.1 Trace first, cite each: the app-side accessor for sar-33's
      routed signal (`InputRouted()` / `SetInputRoutedChangedCallback`,
      `External/Sheaf/.../AppContext.hpp:203-294`), the thread its
      callback fires on, and whether `numAudioInputs > 0` still opens
      the default input device unasked at the current pin. If it does,
      STOP and report — that conflicts with the privacy property the
      phantom-input change bought, and the operator decides.
- [ ] 5.2 Both coupled edits together: `Config().numAudioInputs`, and
      `SetExternalAudioConnected()` driven by the routed-signal change
      callback (once per transition — NOT per sample; the phantom-input
      change removed that recompute deliberately).
- [ ] 5.3 Tests: sources inert with nothing routed (positive control:
      prove the test COULD see them connected — flip the signal and
      watch the same assertion go the other way); both encoders appear
      on the modulation pages once routed; modulation actually reaches
      a destination from a routed input; no default-device open when
      nothing is routed.
- [ ] 5.4 Operator smoke: route an input, confirm External Audio and
      External EF appear and modulate; unroute, confirm they go inert.

## 5b. Carried over from the predecessor (not new work)

- [ ] 5b.1 OPERATOR-COORDINATED, machine-local: rename the working
      folder `~/Desktop/FroggersTiga` -> `~/Desktop/frogg3rs` at a
      session boundary (it invalidates a running session's absolute
      paths). Inherited unfinished from
      `frogg3rs-browser-and-vst-hosts` so it is not lost at archive.
      Afterwards the controller updates its own memory/ledger entries
      that cite the old absolute path. Note: the plugin's compiled wasm
      currently embeds the old folder name via local build paths, which
      the renamed-origin gate reports as a WARN; this rename clears it.

## 6. Whole-change gate and operator acceptance

- [ ] 6.1 Full suite green; browser build + e2e green; plugin builds
      VST3+AU; counts reported.
- [ ] 6.2 OPERATOR GATE: DAW smoke. Two parts.
      (a) INHERITED from `frogg3rs-browser-and-vst-hosts` (moved here by
      operator decision 2026-08-19 — that change shipped the plugin on
      automated evidence alone and archived without a real-DAW run, so
      this is the FIRST time the plugin is exercised in a DAW and any
      finding lands against the predecessor's delivered behavior, not
      this change's): load the VST3 or AU in a real DAW and confirm host
      transport drives it, host tempo drives the clock with the BPM
      control display-only, a parameter automates, a DAW-side MIDI
      mapping moves a parameter, and the editor renders the surface with
      Play/Stop/Record absent and Freeze labeled "FREEZE".
      (b) THIS change: save a project with hand-edited parameters,
      reload, confirm restoration; confirm the chosen cross-bank
      behavior; confirm pre-Play legibility in both hosts; route an
      input and confirm External Audio and External EF appear and
      modulate, then unroute and confirm they go inert.
- [ ] 6.3 Archive with spec sync (both ADDED deltas).
