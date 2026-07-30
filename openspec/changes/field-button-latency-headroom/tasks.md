## 1. Block-rate UpdateParams

- [ ] 1.1 In `src/core/FroggersEngine.hpp`, call `UpdateParams()` once from `ProcessBlock` immediately after `ReadParamsBlock()`
- [ ] 1.2 Remove `UpdateParams()` from `ProcessSample`; leave sample-path smoother `.Process()` calls that are outside `UpdateParams` (osc/mix/reverb mix per D1/D4)
- [ ] 1.3 Do not add a per-block multi-step smoother catch-up loop; accept one `.Process()` step per block for smoothers listed in D1
- [ ] 1.4 Verify nesting ≤3 in modified functions; firmware still builds under existing `-Os` / `BOOT_NONE` flags

## 2. Page-cursor Rand All drain

- [ ] 2.1 Extend `src/common/FieldMutationQueue.hpp` with `m_active`, `m_activeType`, `m_pageIndex` per design D2
- [ ] 2.2 Implement `Enqueue` coalesce: same type while draining active OR same type as last queued entry → no-op; do not reset `m_pageIndex`
- [ ] 2.3 Make `DrainOne` start a mutation from the ring into active fields, randomize exactly one page per call, clear `m_active` when `m_pageIndex >= m_numPages`
- [ ] 2.4 Keep B1/B3 immediate in `DaisyIO.hpp`
- [ ] 2.5 Confirm the `DaisyIO` call site (`DaisyIO.hpp:139`) still calls `MarkScreenDirty()` on every successful `DrainOne` — already present today; the behavior change is that `DrainOne` now returns `true` per page (2.3), so dirty now fires per drained page rather than per full-pass drain. No new edit required unless call-site wording changes.

## 3. LED transmit throttle

- [ ] 3.1 In `src/common/DaisyIO.hpp`, compute LED levels every `ProcessControls` poll; track dirty when a SetLed level changes
- [ ] 3.2 Call `SwapBuffersAndTransmit` at most once per poll, and only when LED dirty or `(now - m_lastLedMs) >= kScreenThrottleMs`
- [ ] 3.3 Confirm mod-assign / tracking / SW LEDs update within one throttle frame when idle, and on the next poll when dirty

## 4. Dry reverb early-out

- [ ] 4.1 In `FroggersEngine` output FX path, follow D4 order: `m_rvMix.Process()` → update `m_lastRvMix` → hysteresis update `m_reverbDryBypass` (`enter 1e-4`, `exit 5e-4`) → bypass or full `ProcessReverb` → dry or dry/wet mix
- [ ] 4.2 On bypass: skip delay-line body; set wet L/R to 0; return dry output
- [ ] 4.3 Keep reverb page, parameters, and static buffers unchanged (no page removal, no buffer shrink)
- [ ] 4.4 Raise RVMX through exit threshold and confirm wet path resumes without audible clicks beyond the hysteresis band

## 5. Shared-host regression

> Build discipline: run every build/compile in this section through a subagent that reports only pass/fail + the failure tail (not the raw log). Cap all C++/JUCE builds on this machine at `-j2` with `nice` (8-core/16 GB Mac freezes/crashes above that).

- [ ] 5.1 Build firmware in `src/FroggersTiga` (`-j2` + `nice`); confirm `.bin` fits 128 KB internal flash
- [ ] 5.2 Compile every in-tree host that includes `FroggersEngine.hpp` / `ProcessBlock`, each at `-j2` + `nice`; fail the task if any does not compile after the block-rate `UpdateParams` move. The complete consumer set in the tree today:
  - `src/core/DesktopHostIO.hpp`
  - `src/core/PagedHostIO.hpp`
  - `src/FroggersTiga/FroggersTiga.hpp`
  - `sim/HookIdentity_test.cpp`
  - `sim/PageBootNav_test.cpp`

## 6. Docs and on-device acceptance

- [ ] 6.1 Update `MANUAL.md` troubleshooting: incremental one-page B2/B4 drain; CPU/poll headroom; reverb page retained
- [ ] 6.2 Update `docs/daisy-field-diagnostics.md`: randomize freeze ≠ reverb RAM / page removal
- [ ] 6.3 Flash via DFU (`make program-dfu`); do not change bootloader
- [ ] 6.4 Bench: audio on; ≥5 rapid SW2 taps in 2 s; B2/B4 every few seconds; no >200 ms dead window on SW2/B1–B4; RVMX dry vs wet OK
