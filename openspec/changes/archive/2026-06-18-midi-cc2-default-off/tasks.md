## 1. Core default

- [x] 1.1 Set `CvMidiBridge::m_inCcEnabled` to `{true, false}`

## 2. VCV alignment

- [x] 2.1 VCV `plugin.cpp`: CC 2 switch default Off; `m_ccPairEnabled[1] = false`

## 3. Docs

- [x] 3.1 Update SIM_MANUAL + synced copies: desktop/VST default CC 1 on, CC 2 off

## 4. Baseline specs

- [x] 4.1 Apply delta to `openspec/specs/midi-cc-mod-gating/spec.md`
- [x] 4.2 Apply delta to `openspec/specs/vcv-cc-mod-gating/spec.md`
- [x] 4.3 Apply delta to `openspec/specs/juce-vst-cc-mod-gating/spec.md`

## 5. Verify

- [x] 5.1 Rebuild WASM + web bundle
- [x] 5.2 Run sim unit tests if available
