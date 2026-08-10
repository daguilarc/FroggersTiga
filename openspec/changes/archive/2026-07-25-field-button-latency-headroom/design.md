## Context

Daisy Field firmware (`DaisyIO` + `FroggersEngine`) already has Phase 2 latency work: OLED ~30 FPS throttle, dirty flag, and `FieldMutationQueue` for B2/B4 with consecutive-type coalescing. User reports still show freezes / slow response when mashing randomize. Code shows:

1. `UpdateParams()` runs every sample inside `ProcessSample` (`src/core/FroggersEngine.hpp:749`, `ProcessSample` def :747; `UpdateParams` def :488) after `ReadParamsBlock()` already runs once per block (:588 inside `ProcessBlock` :586).
2. `DrainOne` still calls full `RandomizeAllPages` / `RandomizeAllPagesMod` (`src/common/FieldMutationQueue.hpp:52` / :56; `DrainOne` def :42).
3. `SwapBuffersAndTransmit` runs every `ProcessControls` poll (`src/common/DaisyIO.hpp:137`, inside `ProcessControls` :32; drain + `MarkScreenDirty` at :139–142).
4. Reverb delay-line DSP runs every sample even when mix is dry (`src/core/FroggersEngine.hpp:741`, `ApplyOutputFx` def :719; `ProcessReverb` def :426). Buffers are fixed: three `float[4096]` lines (`x_rvSize = 4096` :65; `m_rvLineA`/`m_rvLineB`/`m_rvPreLine` :66–68) = 49,152 B ≈ 48 KB — not an OOM path for randomize.

Cursor plan `field_randomize_latency` is the execution overlay; this design is the OpenSpec record.

## Goals / Non-Goals

**Goals:**

- Free main-loop time by cutting audio ISR control-rate work so SW1/SW2, B1–B8, and A-keys stay responsive under load.
- Bound Rand All poll spikes to one page of RNG per drain.
- Keep reverb as an operable page; save CPU when dry.
- Match MANUAL / diagnostics to measured causes (CPU, not RAM / remove-reverb).

**Non-Goals:**

- Removing the reverb page or shrinking delay buffers for “memory.”
- Bootloader / flash layout / `BOOT_SRAM`.
- desktop-v2 Rand All message-thread stacking (separate change).
- SW1 electrical stuck-input (already diagnosed hardware).
- Knob pickup semantics.

## Data flow (required pipelines)

### Audio block pipeline

```
input block[N]
  -> ProcessBlock
       -> ReadParamsBlock()          // SetTarget only (unchanged, once/block)
       -> UpdateParams()             // AFTER change: once/block (see D1)
       -> for i in 0..N-1:
            ProcessSample(in[i])     // no UpdateParams; sample smoothers + DSP
            -> ApplyOutputFx / reverb early-out (see D4)
  -> output block[N]
```

### Mutation drain pipeline

```
B2/B4 rising edge
  -> FieldMutationQueue::Enqueue(type)   // coalesce rules in D2
  -> each ProcessControls:
       DrainOne(pageManager)             // at most one page RNG
       -> if page drained: MarkScreenDirty()
```

### LED poll pipeline

```
each ProcessControls:
  -> compute LED levels (SetLed every poll)
  -> if ledDirty OR (now - lastLedMs) >= kScreenThrottleMs:
       SwapBuffersAndTransmit()
       clear ledDirty; lastLedMs = now
  -> else: skip I2C transmit
```

## Decisions

### D1 — Block-rate `UpdateParams` (sample vs block map)

**Choice:** Call `UpdateParams()` once from `ProcessBlock` immediately after `ReadParamsBlock()`. Remove the call from `ProcessSample`.

**Verified current state:** `UpdateParams()` def `src/core/FroggersEngine.hpp:488`; called per sample at :749 (first line of `ProcessSample` :747). `ReadParamsBlock()` def :368; called once/block at :588 (in `ProcessBlock` :586, which loops `ProcessSample` at :591).

**What already runs once per block (stays in `ReadParamsBlock`):**

- All `SetTarget(...)` calls for audio-gen, reverb, filter, and drive smoothers.
- Immediate (non-smoothed) polynomial drive `SetGain` / `SetCoefs`.

**What moves from every sample to once per block (entire current `UpdateParams` body):**

| Smoother `.Process()` + apply | Applied to |
|-------------------------------|------------|
| `m_pureDelaySeconds` | `m_pureDelay.SetDelaySeconds` |
| `m_bumpFreq` / `m_bumpResonance` / `m_bumpWidth` | `m_resonantBump` + `m_scoopNotch` freq/width |
| `m_filterScoop` | `m_scoopNotch` height |
| `m_comf` / `m_comq` / `m_cmlp` | `m_comFilter` delay/feedback/cutoff |
| `m_srr1` / `m_srr2` / `m_digr` / `m_hash` / `m_fuzz` | frog drive / reorganizer |
| `m_rvDamp` | `m_rvDampFilter.m_alpha` |
| `m_marbles.UpdateParams()` | marbles control-rate |

**Accepted consequence:** Those smoothers advance **one** `.Process()` step per audio block, not per sample. Effective smoothing is slower by block size. That trade-off is intentional for ISR headroom; do not add a per-block multi-step catch-up loop in this change.

**What stays on the sample path (must keep `.Process()` in sample / FX code):**

- Oscillator / mix smoothers consumed in `ProcessSample` (`m_v1vo`, `m_v2vo`, `m_v3vo`, `m_xcpl`, `m_pm*`, `m_oscLvl`, etc.).
- Reverb mix and wet-path smoothers in D4 / `ProcessReverb` (`m_rvMix`, `m_rvPre`, `m_rvSize`, `m_rvDecay`, `m_rvDiffusion`, `m_rvWidth`) when wet path runs.
- Any other per-sample smoother already outside `UpdateParams`.

**Rejected:** Control-rate subframe (every N samples) — extra state once per-block is enough. SysTick button ISR — unnecessary if ISR headroom is restored. Splitting `UpdateParams` into “Process all listed smoothers every sample + apply once per block” — out of scope; revisit only if Task 5/6 bench shows unacceptable filter/drive feel.

### D2 — Page-cursor mutation drain (state machine)

**Choice:** `FieldMutationQueue` owns drain state plus the existing ring buffer.

**Verified current state:** `DrainOne` def `src/common/FieldMutationQueue.hpp:42` calls full `RandomizeAllPages` (:52) / `RandomizeAllPagesMod` (:56) per drain. Call site `src/common/DaisyIO.hpp:139` already marks OLED dirty on a `true` return (:140–141). B2/B4 enqueue at `DaisyIO.hpp:60` / :72; B1/B3 immediate `RandomizeCurrentPage`/`…Mod` at :54 / :66.

**States:**

| State | Meaning |
|-------|---------|
| `Idle` | `m_active == false`; no in-progress Rand All |
| `DrainingRandAll` | `m_active == true`, type `RandAll`, cursor `m_pageIndex` |
| `DrainingRandAllMod` | `m_active == true`, type `RandAllMod`, cursor `m_pageIndex` |

**`Enqueue(type)`:**

1. If state is `DrainingRandAll` or `DrainingRandAllMod` **and** `type` equals the active type → coalesce: return (do not reset `m_pageIndex`, do not enqueue a second full pass).
2. Else if `m_write > m_read` and last queued entry has the same `type` → coalesce: return.
3. Else if queue full → drop: return.
4. Else push `type` and increment `m_write`.

**`DrainOne(pageManager)`:**

1. If `Idle` and queue empty → return `false`.
2. If `Idle` and queue non-empty → copy `m_queue[m_read % kDepth].type` into `m_activeType`, increment `m_read`, set `m_pageIndex = 0`, set `m_active = true` (`DrainingRandAll` or `DrainingRandAllMod`). In-progress work lives only in `m_active` / `m_activeType` / `m_pageIndex`, not in the ring.
3. Randomize **exactly one** page at `m_pageIndex` (knobs for `RandAll`, mods for `RandAllMod`).
4. Increment `m_pageIndex`.
5. If `m_pageIndex >= pageManager.m_numPages` → set `m_active = false` (`Idle`).
6. Return `true` (a page was randomized). Call site marks OLED dirty on every `true` return.

**B1/B3:** remain synchronous in `DaisyIO` (no queue).

**OLED dirty:** Mark screen dirty on **every** successful page drain (mid-drain display tracks). No completion-only option.

**Rejected:** Drain from audio ISR (archived Phase 2 D4) — main-thread page cursor first; reopen only if Task 6 bench fails after Tasks 1+3.

### D3 — LED transmit throttle

**Choice:** Compute LED levels every poll; call `SwapBuffersAndTransmit` only when LED dirty **or** `(now - m_lastLedMs) >= kScreenThrottleMs` (same 33 ms budget as OLED).

**Verified current state:** `SwapBuffersAndTransmit()` called unconditionally every poll at `src/common/DaisyIO.hpp:137` (inside `ProcessControls` :32), after the per-poll `SetLed` computations at :128–135.

**Dirty rule:** Any `SetLed` that changes a stored last-sent level sets `m_ledDirty`. Transmit clears dirty and updates `m_lastLedMs`.

**Transmit frequency:** Zero or one transmit per `ProcessControls` call. Across one OLED throttle window, multiple transmits are allowed when LED state becomes dirty again after a prior transmit in that window (dirty path is intentional; unchanged LEDs stay on the 33 ms floor).

**Rejected:** Drop LED updates until page change — loses mod-assign / tracking feedback.

### D4 — Dry reverb early-out (ordered pipeline)

**Choice:** Keep page, params, and static buffers. Skip delay-line work when mix is dry, using hysteresis.

**Verified current state:** `ApplyOutputFx` def `src/core/FroggersEngine.hpp:719` runs `rvb = ProcessReverb(output)` (:741), then `rvMix = m_rvMix.Process()` (:742), `m_lastRvMix = rvMix` (:743), and returns `(1 - rvMix) * output + rvMix * rvb` (:744). Members `m_reverbWetL`/`m_reverbWetR` :59–60, `m_lastRvMix` :61; `ProcessReverb` def :426 advances the delay lines every sample regardless of mix. `m_reverbDryBypass` does not yet exist.

**Constants:**

- `kRvMixDryEnter = 1e-4f` — enter bypass when mix ≤ this.
- `kRvMixDryExit = 5e-4f` — leave bypass when mix ≥ this.
- Member `bool m_reverbDryBypass` (initial `false`).

**Ordered sample FX pipeline (replaces current `ProcessReverb` then `m_rvMix.Process()` order):**

1. `rvMix = m_rvMix.Process()`
2. `m_lastRvMix = rvMix`
3. If `m_reverbDryBypass`: set `m_reverbDryBypass = (rvMix < kRvMixDryExit)`; else set `m_reverbDryBypass = (rvMix <= kRvMixDryEnter)`
4. If `m_reverbDryBypass`:
   - Do **not** call delay-line body of `ProcessReverb` (no pre/line index advance, no wet smoother `.Process()` inside that body).
   - Set `m_reverbWetL = 0`, `m_reverbWetR = 0`.
   - Return dry `output` (equivalent to mix with wet = 0).
5. Else:
   - `rvb = ProcessReverb(output)` (full delay-line path; wet smoothers advance).
   - Return `(1 - rvMix) * output + rvMix * rvb`.

**Rejected:** Remove reverb page; shrink `x_rvSize` in this change; single-threshold early-out without hysteresis.

### D5 — Cursor plan 1:1 with `tasks.md`

**Choice:** OpenSpec `tasks.md` and Cursor plan todos share the same ordered work; if they diverge, OpenSpec wins and the plan updates.

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| Block-rate `UpdateParams` slows filter/drive smoother response by ~block size | Documented in D1; Task 5/6 listen for obvious zipper; no catch-up loop in this change |
| Page-cursor Rand All completes over several polls (audible steps) | Coalesce spam; OLED dirty every drained page |
| LED dirty path transmits more than once per OLED window | Accepted; unchanged LEDs stay on 33 ms floor |
| Crossing dry/wet threshold | Hysteresis enter `1e-4` / exit `5e-4` in D4 |
| Hosts share `FroggersEngine` | `UpdateParams` is a private call moved between two internal sites (no API change), so the five consumers listed in Task 5.2 (`DesktopHostIO.hpp`, `PagedHostIO.hpp`, `FroggersTiga.hpp`, `sim/HookIdentity_test.cpp`, `sim/PageBootNav_test.cpp`) get the block-rate behavior transparently; Task 5 compiles all of them |

## Migration Plan

1. Implement Tasks 1–4 in firmware headers.
2. Build `src/FroggersTiga` (fit 128 KB) and the five `FroggersEngine.hpp` consumers in Task 5.2, all at `-j2` + `nice` via a reporting subagent.
3. Flash DFU; run acceptance bench (Rand All spam + SW2/B keys).
4. Update MANUAL / diagnostics.
5. Rollback = revert the three headers + docs; binary only.

## Open Questions

- None blocking. If page-cursor drain still fails SW/B latency bench after Task 1+3, reopen archived Phase 2 audio-tick drain (prior D4) as a follow-on change.
