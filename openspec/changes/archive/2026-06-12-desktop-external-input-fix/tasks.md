# Desktop external input + headroom — tasks

## 1. Routing + peak tap

- [x] 1.1 `AudioEngine`: split `getInputPeakLevel()` from envelope; track max `fabs(m_inBlock[i])` per callback when Ext. In. on
- [x] 1.2 `AudioEngine` ctor: `m_externalInputEnabled` defaults **off**
- [x] 1.3 `MainComponent`: rename toggle **Ext. In.**; toggle unchecked at launch
- [x] 1.4 Update tooltips (Ext. In. + meter Schmidt hint)

## 2. Input level meter (fix black box)

- [x] 2.1 `InputEnvelopeIndicator`: idle paint (grey track + centre tick) when level ≤ 0 or disabled
- [x] 2.2 Active paint: blue fill width = peak; dim alpha below 0.02
- [x] 2.3 `MainComponent::timerCallback`: drive meter from `getInputPeakLevel()` when Play + Ext. In.; idle state otherwise
- [x] 2.4 `resized()`: adjust **Ext. In.** width (~72px) + meter (~80px)

## 3. Output headroom

- [x] 3.1 `AudioEngine::audioDeviceIOCallbackWithContext`: soft-limit `outL`/`outR` after `applyStereoBus`
- [x] 3.2 `FroggersEngine::SoftResetFxState`: zero `m_comFilter` delay line + state

## 4. Docs

- [x] 4.1 `QUICK_DICT.md`: **Ext. In.** transport line
- [x] 4.2 `MANUAL_VERIFY.md`: Ext. In. + meter + Schmidt threshold check

## 5. Verification

- [ ] 5.1 Cold launch → **Ext. In.** off; enable + Play + speak → meter moves
- [ ] 5.2 Uncheck **Ext. In.** → meter idle chrome, VCO-only timbre
- [ ] 5.3 Rand All + Delay hot patch → no permanent hard clip; Stop/Play recovers
- [x] 5.4 Desktop Release build
