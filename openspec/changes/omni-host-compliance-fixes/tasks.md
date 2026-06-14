## 1. Web label single authority

- [x] 1.1 Update `syncKnobUi` / `onScreenUpdate` to set knob labels from `rows[i].name`
- [x] 1.2 Remove `HOST_PAGE_LABELS` and `applyStaticKnobLabels` (or replace with generated fallback only)
- [x] 1.3 Add wasm export `froggers_mod_source_name(modIndex)` using `ParamDisplayNames::forModSource`
- [x] 1.4 Wire web mod bay titles from wasm export; remove hardcoded strings in `main.ts`
- [ ] 1.5 Manual: Audio page row 7 shows **Phase mod 3** from wasm after Play

## 2. CI and doc sync

- [x] 2.1 Add `sim/check_mod_source_labels.sh` for mod bay vs header
- [x] 2.2 Run both label scripts in `.github/workflows/pages.yml` before WASM build
- [x] 2.3 Add `web` build step to copy root manuals into `web/public/` (if not already in `npm run build`)
- [x] 2.4 Verify CI fails when header/TS intentionally diverged (local dry-run)

## 3. VST host compliance

- [x] 3.1 Implement preset blob (version byte + page/knob/mod/delay snapshot) in `PluginProcessor`
- [x] 3.2 Honor `isBypassed()` in `processBlock`
- [x] 3.3 Verify `prepareToPlay` sets host sample rate on engine and delay
- [ ] 3.4 Manual: save/reload DAW session preserves knob state

## 4. VCV runtime + artifact sync

- [x] 4.1 Replace per-block vectors in `vcv/src/plugin.cpp` with member buffers
- [x] 4.2 Update `vcv-vst-field-parity-panel/design.md` Context ASCII to Phase A truth
- [x] 4.3 Cross-link `sim-pm3-knob-parity/design.md` Open Questions → this change for web dedupe

## 5. Close sim-pm3 verification gap

- [x] 5.1 Document manual PM3 test procedure in `sim-pm3-knob-parity/tasks.md` appendix (XCPL CW + row 7 up)
- [x] 5.2 Document VCO Envelope scope independence test (row 7 static, scope moves with VCO level)
- [x] 5.3 Mark sim-pm3 tasks 4.3–4.5 complete after user sign-off or recorded smoke results
