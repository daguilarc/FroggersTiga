## 1. Audit and verify current state

- [x] 1.1 Confirm `DesktopPanelBackend::getRowName` returns Phase mod 3 for Audio row 7 via `ParamDisplayNames`
- [x] 1.2 Confirm `DesktopHostIO::Init` and `PagedHostIO::Init` call `SetSimDedicatedPm3Knob(true)`
- [x] 1.3 Confirm `FroggersEngine::ReadParamsBlock` maps param 6 → `m_pm3` when sim flag set
- [x] 1.4 Document audit result in design.md appendix (pass/fail per host)

## 2. Fix stale operator docs

- [x] 2.1 Update `docs/sim-manual.md`: row 7 → Phase mod 3; Crispy naming; PM3/Crispy/VCO Envelope note (match `SIM_MANUAL.md`)
- [x] 2.2 Update `docs/quick-dict.md`: Phase mod 3 entry; remove row-7-as-VCO-level; Crispy not Crunch; fix Crispy gloss (no PM3 on sim)
- [x] 2.3 Verify root `SIM_MANUAL.md` and `QUICK_DICT.md` already correct; patch any remaining drift
- [x] 2.4 Sync `web/public/sim-manual.md` and `web/public/quick-dict.md` if diffs found vs root

## 3. Label table deduplication

- [x] 3.1 Compare `web/src/main.ts` `HOST_PAGE_LABELS` to `ParamDisplayNames.hpp` byte-for-byte on Audio row
- [x] 3.2 Add build or CI check that fails if TS labels diverge from header (or document manual sync rule in README)
- [x] 3.3 Fix `ModRackPanel` to use `ParamDisplayNames::forModSource(4)` for VCO Envelope label (match Random 1/2 pattern)

## 4. Rebuild and behavioral verification

- [x] 4.1 Rebuild desktop (`FroggersTigaAssets` embeds updated manuals)
- [x] 4.2 Rebuild web dist (`web/dist/`)
- [x] 4.3 Manual test desktop: Audio row 7 label; PM3 audible with XCPL CW + row 7 up; Crispy at min (user confirmed)
- [x] 4.4 Manual test web: same label and PM3 behavior (labels from wasm `rows[].name`)
- [x] 4.5 Confirm VCO Envelope scope moves with VCO activity but is independent of row 7 knob position (see appendix)

## 5. Cross-change handoff

- [x] 5.1 Captured in `vcv-vst-field-parity-panel` design (D4, appendix), specs, and tasks 0.1 / 3.4 / 5.5

## Appendix: Manual verification checklist

### PM3 audible (desktop or web)

1. Play audio.
2. Audio page: turn **Cross-coupler** toward CW (2→3 coupling).
3. Raise **Phase mod 3** (row 7 knob, not VCO Envelope mod rack).
4. Expect timbre change from increased VCO2→VCO3 PM depth.
5. **Crispy** at minimum — PM3 is independent of Crispy on sim hosts.

### VCO Envelope scope independence

1. Play audio with audible VCO mix.
2. Watch **VCO Envelope** mod rack scope — trace moves with VCO activity.
3. Move **Phase mod 3** knob only — scope trace unchanged.
