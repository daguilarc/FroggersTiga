# Tasks — frogg3rs-host-state-and-visibility

**Closed 2026-08-20.** Delivered: groups 1, 2, 5, 6, 7 and the comment sweep
(group 8) over the live `app/` tree excluding `app/vst/`. `[x]` = delivered
and reviewed. `[~]` = not delivered here; carried to
`frogg3rs-automation-view-and-musical-ranges`.

The comment sweep ran under a narrowed token set by operator decision; the
`app/vst/` pass and the letter-code labels kept as internal cross-reference
shorthand in `FroggersAudioRoutingTests.cpp` carry forward with it.

Gate after every group: `cd app && nice make -j2 test` green (baseline
279/279 + whatever the predecessor added — re-establish the real count
at group 1; NEVER above -j2). One commit per group. Subagent dispatch
per omni-rule: lightest capable model, sequential code changes,
per-task review with §14 postflight; verification runs through a cheap
subagent (§16.1). Design anchors marked UNVERIFIED must be read by the
executing task first. Standing operator rule: anything found during
execution is fixed inside this change — no deferrals; genuine
conflicts with a recorded decision stop for the operator.

Ordering note (corrected at audit): NOTHING is blocked — both Sheaf
capabilities are verified at the pinned commit `80c4eab8`. Groups
1, 2, 4, 5, 6, 7 are independent; 3 is verification against the
current pin; 8 (comment sweep) runs after all other code groups;
9 closes the change.

## 1. Site visibility regression tests (do first — it protects everything else)

- [x] 1.1 Add the visibility assertions in `app/browser/e2e/` per design
      C: non-zero mount height at wide and narrow, surface root
      intersects the viewport, encoder canvases carry painted pixels, a
      blank-frame check that fails on an all-background render.
- [x] 1.2 §9.1 proof: re-apply the known blackout (the wide-branch
      height clear) on a scratch copy, show EACH new assertion red,
      restore, show green. Record both outputs. An assertion that stays
      green under the reintroduced defect is not done.

## 2. DAW session state (design A)

- [x] 2.1 Trace: existing portable serialize/deserialize at the app or
      engine layer (Sheaf browser persistence,
      `External/Sheaf/projects/synth/browser/src/persistence.ts`;
      `SheafPatchDataPathsForApp` usage at `app/FroggersMain.cpp:53`);
      the parameter authority's own representation; the shared-data-root
      boundary. Cite each. If a portable path exists, REUSE it (§8);
      if none exists, build it in this change and report which layer
      it landed in.
- [x] 2.2 Implement `getStateInformation`/`setStateInformation`
      (`app/vst/FroggersPluginProcessor.hpp:204-205`) over the traced
      representation; restores go through the parameter authority (not
      the host-parameter shadows) and must not fight the group-7
      feedback guard.
- [x] 2.3 Tests: round-trip (set values → save → construct fresh
      processor → restore → authority values match, host parameters
      reflect them); forward-compatibility when the stored model is
      smaller/larger than the current one (assert the defined
      behavior); no write-through to the standalone's shared data root
      during a project restore.

## 3. Parameters legible before audio (design B) — verification at the current pin

- [x] 3.1 Confirm the pinned Sheaf (`80c4eab8`, the
      `ui-state-before-audio` commit) is what the browser build and
      plugin link against; re-run the frogg3rs gates once against it.
      No pin move is expected — if one turns out to be needed, that is
      a finding to report with the trace, then execute it here.
- [~] 3.2 **PARTIAL — site pre-Play visibility IS asserted (app/browser/e2e/visibility.spec.mjs); the plugin-editor half was never asserted. MOVED to the successor.** Original text: Verify and assert: site loads → encoder names/values visible
      with no click and no Play (extend group 1's e2e checks to the
      pre-Play window); plugin editor shows the same before the host
      ever calls `processBlock` (message pump starts at construction,
      `app/vst/FroggersPluginProcessor.cpp:157-158`).

## 4. Cross-bank automation policy (design D)

- [~] 4.1 **MOVED to `frogg3rs-automation-view-and-musical-ranges`.** Original text: Trace whether the parameter authority can address a bank's
      slot WITHOUT selecting that bank; cost each of the three options.
- [~] 4.2 **MOVED — route decided (framework-side bank-addressed write); implementation moves.** Original text: OPERATOR DECISION (in-change gate): present the trace and
      costs; implement the chosen option in this change. Do not
      implement a default first.
- [~] 4.3 **MOVED.** Original text: Tests for the chosen behavior, including the multi-lane case
      (two banks automated simultaneously).

## 5. External Audio + External EF re-enable (design E)

- [x] 5.1 Read the one remaining UNVERIFIED consumer contract: whether
      `Step()`/the audio thread reads `connected`, and therefore
      whether the message-thread write needs ordering or a snapshot
      (design E; the signal's thread contract is already verified —
      message thread, `AppContext.hpp:230-231`,
      `Runtime.hpp:683-687`).
- [x] 5.2 Both coupled edits together: `Config().numAudioInputs` 0 → 1
      (`app/FroggersAppCore.hpp:207`) with its history-essay comment
      rewritten to the new contract, and `SetExternalAudioConnected()`
      (`app/FroggersModulation.hpp:464-467`) driven by
      `SetInputRoutedChangedCallback` once per transition plus an
      `InputRouted()` read at startup — never per sample.
- [x] 5.3 Tests: sources inert with nothing routed (§9.1 positive
      control: flip the signal, watch the same assertion go the other
      way); both encoders appear on the modulation pages once routed;
      modulation actually reaches a destination from a routed input;
      a default-opened device (persisted selection empty) derives
      NOT-routed.
- [~] 5.4 **NOT DONE — never dispatched. The plugin still declares output-only. MOVED, together with its unsatisfied froggers-vst-host input-bus delta.** Original text: Plugin input bus (design E, plugin note — in scope here):
      trace the JUCE optional-input-bus layout for an instrument and
      what `processBlock` does with input buffers today; declare the
      bus; derive `connected` from the bus being enabled with nonzero
      channels, through the same writer, once per layout change; tests
      for bus-present/bus-absent instantiation and connected
      derivation. Update the vst-host delta if the trace forces a
      different shape (report it, don't silently diverge).
- [~] 5.5 **NOT DONE — `UPSTREAM-SHEAF-ASK.md` untouched. MOVED.** Original text: Correct `UPSTREAM-SHEAF-ASK.md`'s stale ask-8 rows (`:27`,
      `:52`, `:124`): landed at pin `80c4eab8` via PR #9 / sar-33, with
      the routed-signal semantics, superseding the not-landed
      corrections.
- [~] 5.6 **MOVED — operator smoke for external audio.** Original text: Operator smoke: route an input, confirm External Audio and
      External EF appear and modulate; unroute, confirm they go inert.

## 5b. Carried over from the predecessor (not new work)

- [~] 5b.1 **MOVED — machine-local folder rename, operator-coordinated.** Original text: OPERATOR-COORDINATED, machine-local: rename the working
      folder `~/Desktop/FroggersTiga` -> `~/Desktop/frogg3rs` at a
      session boundary (it invalidates a running session's absolute
      paths). Inherited unfinished from
      `frogg3rs-browser-and-vst-hosts` so it is not lost at archive.
      Afterwards the controller updates its own memory/ledger entries
      that cite the old absolute path. Note: the plugin's compiled wasm
      currently embeds the old folder name via local build paths, which
      the renamed-origin gate reports as a WARN; this rename clears it.

## 6. PM rate floor (design F)

- [x] 6.1 Raise `kPmLfoMinHz` (`app/dsp/Vco.hpp:103`) to a slow but
      plainly audible floor (executor's pick per the operator ruling —
      order of a few seconds per cycle); rewrite the now-stale
      "reproduces today's rate exactly" comment at
      `app/FroggersParameters.hpp:174-179` and state the deliberate
      divergence from `src/core/FroggersEngine.hpp:135` in plain terms.
- [x] 6.2 Tests: assert the floor is above a named audibility bound
      (not a bare magic number in the test); §9.1 positive control:
      knob 0 vs knob 1 produce measurably different LFO rates and the
      floor rate is nonzero and moving; re-run the parity tests that
      cite the constant symbolically
      (`app/FroggersDspParityTests.cpp:4926`) and report, don't assume.

## 7. Reset restores the default patch (design G)

- [x] 7.1 Restructure the default patch into a single bank-addressable
      source of truth shared by boot and reset (§8; no second copy of
      the constants). Trace and cite the Crunchy writer path (slot 15,
      `app/FroggersParameters.hpp:37` — UNVERIFIED accessor).
- [x] 7.2 Rewire `ResetPage`/`ResetAll`
      (`app/FroggersModulation.hpp:1537,1566`): Reset Page = the
      current bank's default-patch slice including its Crispy; Reset
      All = the whole default patch, every bank's Crispy AND global
      Crunchy included (operator ruling — the Randomize-inherited
      carve-outs are overruled for Reset); drilled-in levels revert
      the selected parameter's depths to their default-patch values.
- [x] 7.3 Tests against the fresh-instance oracle: reset-after-
      randomize equals a freshly constructed model with the default
      patch applied, field-for-field, at every bank and both scene
      poles; Reset Page on Audio restores shapes and pitch detents
      while other banks stay untouched; Reset All resets Crispy and
      Crunchy (§9.1 positive control: prove the pre-reset state was
      non-default first).

## 8. Openspec-comment sweep (design H — LAST code group)

- [x] 8.1 Enumerate per design H's operand list across the live `app/`
      tree (including tokens groups 1-7 introduced); classify every
      hit (provenance / load-bearing-keep-mechanism / real-code
      citation) BEFORE editing any; report count FOUND vs count
      CHANGED per file.
- [x] 8.2 Rewrite: delete pure provenance; keep mechanism rationale
      with provenance tokens stripped; keep real-code citations. Gate
      stays green after the sweep (comments only — zero behavior
      diffs; verify the build hash of compiled objects where cheap, or
      by the suite).
- [x] 8.3 Postflight re-grep (§14): the operand list returns zero hits
      outside the classified keepers; record the grep output in the
      report.

## 9. Whole-change gate and operator acceptance

- [~] 9.1 **PARTIAL — full suite green (290/290) and the browser suite passes; the §14 postflight (§8 re-run against the whole diff) was NOT done. MOVED.** Original text: Full suite green; browser build + e2e green; plugin builds
      VST3+AU; counts reported. Change-level §14 postflight: §8 re-run
      against the whole diff (every new named concept enumerated
      across the tree), plus the group-8 grep proof.
- [~] 9.2 **MOVED — operator smoke, both inherited and new.** Original text: OPERATOR GATE: DAW + app smoke. Two parts.
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
      input (standalone device selection, and DAW bus) and confirm
      External Audio and External EF appear and modulate, then unroute
      and confirm they go inert; confirm the PM rate floor feels "slow
      but audible" at knob minimum; Reset Page on the Audio page and
      Reset All both land on the fresh-launch default patch, Crunchy
      included.
- [~] 9.3 **Superseded by this archive.** Original text: Archive with spec sync (all five deltas: froggers-vst-host,
      froggers-web-host, froggers-modulation-slate,
      froggers-vco-topology, froggers-transport-and-reset-controls).
