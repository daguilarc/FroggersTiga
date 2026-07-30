## Context

```
Label flow today (OMNI violation):

  ParamDisplayNames.hpp ──► desktop PanelBackend ✓
                         ──► wasm froggers_row_name ✓
                         ──► PagedHostIO::GetRowName ✓ (fixed)
                         ──► web HOST_PAGE_LABELS ✗ (duplicate, used for UI)
                         ──► web modBayIndicators strings ✗ (hardcoded)

Web screen update path:

  worklet rows[].name = froggers_row_name()   ← correct
  main.ts syncKnobUi()                       ← ignores row.name
  applyStaticKnobLabels(HOST_PAGE_LABELS)    ← wins visually
```

**PM3 / VCO Envelope (audit conclusion):**

| Control | Index | Label | DSP |
|---------|-------|-------|-----|
| Audio row 7 knob | param 6 | Phase mod 3 | `m_pm3` when `SetSimDedicatedPm3Knob(true)` |
| Mod rack source | mod 4 | VCO Envelope | `UpdateM5FromVco` → `m_mods[4]` |

Desktop and website labels for row 7 were **already correct**; stale `docs/` and conflation with mod source 4 caused the “unfixed spec” report. VCO Envelope scope traces **mod CV** (`GetCvOut(4)` / `m_mods[4]`), not audio waveform — updated every audio block from smoothed |mean(v1,v2,v3)| sampled every 64 samples.

**Phase A status (implemented, artifacts stale):**

- `PagedHostIO` page-indexed APIs ✓
- VCV `DelayState` + block ProcessBlock ✓ (single-module stub, not expander stack)
- VST `FroggersTigaPlugin` boots with full `MainComponent` ✓
- VCV widget still 12 HP stub with invalid multi-row `box.size` — Phase B

## Goals / Non-Goals

**Goals:**

- One runtime label path per host; TS tables generated or eliminated.
- CI fails on label drift before Pages deploy.
- VST saves/restores sim state; respects bypass.
- VCV process() avoids per-block heap alloc.
- OpenSpec context diagrams match repo.

**Non-Goals:**

- VCV 72 HP chonker widget (stays in `vcv-vst-field-parity-panel` Phase B/C).
- Firmware OLVL behavior changes.
- Replacing four manual copies with a CMS — only scripted sync in build.

## Decisions

### D1 — Web labels from wasm screen payload

**Choice:** In `syncKnobUi` / `onScreenUpdate`, set `knobMainLabels[i].textContent = rows[i].name`. Remove `HOST_PAGE_LABELS` and `applyStaticKnobLabels` except as fallback when engine not ready.

**Why:** wasm already calls `ParamDisplayNames`; eliminates duplicate table (OMNI repetition rule).

**Alternative rejected:** Keep TS table + lint only — drift already happened once.

### D2 — Mod bay labels from wasm mod-level indices

**Choice:** Export `ParamDisplayNames::forModSource` via wasm (`froggers_mod_source_name`) or reuse existing string table; web mod bay constructs indicators from that at init.

**Alternative rejected:** Shared JSON generated at build — heavier toolchain; wasm export is one function.

### D3 — CI gate in pages.yml

**Choice:** After checkout, run `sim/check_param_display_names.sh` before WASM build; add `sim/check_mod_source_labels.sh` for mod bay strings vs header.

### D4 — VST preset = serialized PageManager snapshot

**Choice:** Binary blob: current page index, all page knob positions, mod assignments/depths, delay sidecar knobs. Use JUCE `MemoryBlock` + simple version header in `getStateInformation`.

**Bypass:** When `isBypassed()`, clear output or pass silence per JUCE convention; still tick UI if editor open.

### D5 — VCV member buffers

**Choice:** `std::vector<float> m_extIn, m_mono` on `FroggersTigaModule`, resize once in `onSampleRateChange` / first process to max block size.

### D6 — Artifact sync as separate task batch

**Choice:** Update `vcv-vst-field-parity-panel/design.md` Context ASCII block; add cross-link from `sim-pm3-knob-parity` design Open Questions → this change for web label dedupe.

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| Web labels blank before worklet ready | Show page name only until first `screen` message |
| Preset blob incompatible across versions | Version byte + ignore unknown fields |
| CI false positive on intentional label change | Update header + TS in same commit |
| VST bypass silences mod MIDI out | Document; match standalone stop semantics |

## Migration Plan

1. Land web label path (D1–D2); verify Audio row 7 shows Phase mod 3 from wasm.
2. Add CI scripts (D3).
3. VST preset + bypass (D4); manual DAW smoke.
4. VCV buffers (D5).
5. Refresh OpenSpec context (D6).
6. Close `sim-pm3-knob-parity` manual tasks with checklist doc.

## Open Questions

1. Generate `PAGE_NAMES` from wasm at build time vs runtime-only?
2. VST: strip record/export cluster in plugin editor (already hidden play/stop)?
