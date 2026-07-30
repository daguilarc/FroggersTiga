> **Reconciled (omni 1.2):** Complete (10/10); aligned with `PairArEnvelope.hpp` + tests.

## 1. DSP authority (OMNI)

- [x] 1.1 Add `kMinTimeSec`, `kMaxTimeSec` to `PairArEnvelope.hpp`; include `PhaseUtils.hpp`
- [x] 1.2 Refactor `KnobToOnePoleCoeff` to use `PhaseUtils::ExpParam::Compute(kMinTimeSec, kMaxTimeSec, clampedKnob)` — remove inline `minSec` / `maxSec` snippet

## 2. Tests

- [x] 2.1 Extend `PairArEnvelope_test.cpp`: assert `kMinTimeSec == 1e-3f`, `kMaxTimeSec == 10.f`; assert `ExpParam::Compute` at knob 0 → 1e-3, 0.5 → 0.1, 1 → 10; existing rise/fall ordering tests still pass
- [x] 2.2 Run sim unit test target locally; all pass

## 3. Documentation

- [x] 3.1 Update pair-AR section in `SIM_MANUAL.md`: add Range column (1 ms – 10 s exponential), follower vs gate note, contrast with Delay ~0–2 s
- [x] 3.2 Sync `docs/sim-manual.md` and `web/public/sim-manual.md`
- [x] 3.3 Fix stale `1 ms – 2 s` and “Times via ExpMap” lines in `openspec/changes/audio-pair-ad-controls/design.md` D3

## 4. Verification

- [x] 4.1 Grep pair-AR paths: no remaining `maxSec = 2` or pair-AR `2.0f` time cap in `PairArEnvelope.hpp`
- [x] 4.2 Rebuild WASM; web Audio page — max Attack 1+2 produces multi-second swell (not ~2 s cap)
- [x] 4.3 Desktop Audio panel — same smoke on horizontal pair-AR band
