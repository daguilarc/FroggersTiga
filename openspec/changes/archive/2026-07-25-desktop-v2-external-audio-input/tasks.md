## 0. Verification contract (binding — see `.sdd/progress.md` and `~/Desktop/omni-rule.md` §16)

- [ ] 0.1 Before any task below, confirm the tree is green: `bash scripts/check_subagent_packet_gates.sh` exit 0, full `ctest --test-dir desktop-v2/build` (no `-R` filter). Never build on red.
- [ ] 0.2 After the implementation is complete, run the SAME full gate set again (parent-verified, not just the implementer's self-report) before considering this change done.
- [ ] 0.3 Commit hygiene: explicit `git add <file list>` only — never `-A`/`-u`/`.`. Do not stage `.claude/`, `.cursor/`, `.sdd/`, or `src/FroggersTiga/build/*`.

## 1. AudioSettingsComponent: Ext. In. toggle (D1, D2)

Reached via the *existing* runtime-page rail Audio button (`m_audioButton`, same rail as MIDI/Controllers) → `AudioRuntimePageComponent` → `AudioSettingsComponent`. No new button, no new rail entry, no `GlobalStripV2` changes.

- [ ] 1.1 Add a toggle to `AudioSettingsComponent.h`/`.cpp` (near the existing `m_inLabel`/`m_inDevice`/`m_inHelp` input row), wired to `m_engine.setExternalInputEnabled(getToggleState())`, matching v1's `MainComponent.cpp:120-125` tooltip text and behavior ("Ext. In.: route line/mic to engine (off = VCO-only)").
- [ ] 1.2 Check whether the existing `InputLevelMeter`/`m_status` (both already reference `isExternalInputEnabled()`) already satisfy the spec's meter/route-hint scenarios, or need extension to match v1's `MainComponent.cpp:451-460` pattern (hint shown only when `extOn && running && getInputRouteStatus() != InputRouteStatus::Ok`).
- [ ] 1.3 No hosted-shell gating needed — confirmed `AudioRuntimePageComponent` is never instantiated by `HostedMainComponentV2`.

## 2. Tests

- [ ] 2.1 New test proving the toggle calls `AudioEngine::setExternalInputEnabled` — no real audio hardware required (JUCE's `AudioDeviceManager` runs with no physical device attached for this purpose).
- [ ] 2.2 New end-to-end test (spec scenarios "external-audio lane contributes to the UI effective value" / "...to the engine sum"): call `FroggersV2ControlCore::setExternalAudioAvailable(true)` directly (no `AudioEngine`/hardware needed for this half), set a non-zero depth on an external-audio lane, assert it appears in `computeEffective`'s sum AND in `V2LaneDepthStore` after `FroggersV2HostBridge::syncModRoutes`'s ToHost push. This is the gap flagged this session — no existing test covers it.

## 3. Docs

- [ ] 3.1 `SIM_MANUAL.md` / `QUICK_DICT.md` (+ web/docs mirrors, matching this session's established sync pattern): document the Ext. In. toggle, what it does, and that it's reached via the Audio settings page (runtime-page rail), standalone-only (the hosted/VST shell never instantiates this page at all).

## 4. Close-out

- [ ] 4.1 Full desktop-v2 suite green (parent-verified per §0.2).
- [ ] 4.2 `openspec validate desktop-v2-external-audio-input --strict`.
- [ ] 4.3 Archive via `openspec archive desktop-v2-external-audio-input` once 4.1/4.2 pass.
