## Packet 0 — Pin accessor

- [x] 0.1 Confirm `GetCvOut(lane)` as pinned accessor; document in design.md (CvScopeDisplay = generic; global strip = ≤4 projection of 15 lanes)

## Packet 1 — Store + single push + scope consumes store

- [x] 1.1 Add `CvLaneHistoryStore.hpp` (15 lanes, `kBufferSize = CvScopeDisplay::kBufferSize`)
- [x] 1.2 Own one store with oscilloscope; push `GetCvOut(0..14)` once per tick in MainComponent + HostedMainComponentV2
- [x] 1.3 `GlobalOscilloscopeDisplay::refreshTraces` reads store; no `GetCvOut` in that function

## Packet 2 — Encoder underlay

- [x] 2.1 `EncoderRingComponent` `setUnderlay` / `clearUnderlay`; paint underlay first; availability alpha; degenerate midline

## Packet 3 — Detail bind

- [x] 3.1 Shared `bindDetailUnderlays` used by SubmodulePagePanel + AdsrPagePanel
- [x] 3.2 Wire store into both panels from shells

## Packet 4 — Verify

- [x] 4.1 ModSourceGrid_test: underlay bind, Target clear, unavailable safe
- [x] 4.2 SIM_MANUAL + QUICK_DICT + mirrors
- [x] 4.3 Full ctest + packet gates; mark tasks complete
