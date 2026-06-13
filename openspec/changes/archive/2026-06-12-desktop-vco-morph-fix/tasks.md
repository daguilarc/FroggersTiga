# Desktop VCO morph fix — tasks

## 1. Fix morph evaluation

- [x] 1.1 `VcoWaveMorph::GetMorph`: linear modulated knob clamped 0–1; remove `ExpParam::Compute(0,1,…)`
- [x] 1.2 Verify `ModulatedMorph` returns non-zero for knob 0.5 (unit or manual)

## 2. Queue cycle morph

- [x] 2.1 Add `HostMutationType::CycleMorph` + `morphIndex` handling in `applyMutation`
- [x] 2.2 `DesktopHostIO::CycleVcoMorph` enqueues only (remove direct `m_engine` call)
- [x] 2.3 `SubModulePanel` wave click → `cycleVcoMorph` (already does; verify path)

## 2b. Correct prior false completions

- [x] 2b.1 `desktop-host-corrections` task 2.2: marked complete — `CycleMorph` queued
- [x] 2b.2 `desktop-host-mutation-safety` task 1.4: marked complete — `CycleMorph` added
- [x] 2b.3 `sim-hosts-multi-ui/specs/froggers-core/spec.md`: linear morph requirement (no `ExpParam(0,1)`)

## 3. Idle mutation drain

- [x] 3.1 Expose `DrainPendingMutations()` on `DesktopHostIO` (wraps `drainMutationQueue`)
- [x] 3.2 `MainComponent::timerCallback`: call when `!isAudioRunning()`

## 4. Verification

- [x] 4.1 Play → click VCO1/2/3 wave icons → hear timbre change + icon updates
- [x] 4.2 Play → **Rand waves** → continuous morph change audible
- [x] 4.3 Stop → **Rand waves** → icons update before Play
- [x] 4.4 No permanent silence after spam-clicking wave + randomize
- [x] 4.5 Desktop debug + Release build
