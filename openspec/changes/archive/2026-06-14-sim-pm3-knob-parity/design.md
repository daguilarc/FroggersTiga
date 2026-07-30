## Context

```
Firmware (Field)                         Sim hosts (desktop / web / VCV)
─────────────────                        ─────────────────────────────────
Audio knob 7 = OLVL                      Audio row 7 = Phase mod 3 (label)
  (osc level, VCO-only mix)                (param index 6 → m_pm3)
Audio knob 8 = FUEG                        Audio row 8 = Crispy
  (fuegoizer + PM3 + mix topology)         (FUEG; PM3 no longer from Crispy on sim)

Mod source 4 = VCO envelope CV           Mod source 4 = "VCO Envelope"
  (slow level from VCO mix)                  (scope on desktop/web; LED on VCV future)
```

**Current code state (verified):**

| Layer | Phase mod 3 row 7 | PM3 from param 6 | Crispy owns PM3 |
|-------|-------------------|------------------|-----------------|
| `ParamDisplayNames.hpp` | ✓ "Phase mod 3" | — | — |
| `DesktopPanelBackend` | ✓ reads header | — | — |
| `DesktopHostIO` / `PagedHostIO` | — | ✓ `SetSimDedicatedPm3Knob(true)` | ✓ `m_pm3` not `fuegKnob` |
| `web/src/main.ts` labels | ✓ duplicated table | via Wasm `PagedHostIO` | ✓ |
| `SIM_MANUAL.md` / `web/public` | ✓ | documented | ✓ |
| `docs/sim-manual.md` | ✗ "VCO level" | — | ✗ "Crunch" |
| `docs/quick-dict.md` | ✗ "VCO level" | — | ✗ "Crunch" + wrong Crispy note |

**Confusion vector:** “VCO Envelope” names mod source 4 (`UpdateM5FromVco` → `m_mods[4]`), not Audio row 7. The scope traces that mod CV (0–1), not audio waveform.

## Goals / Non-Goals

**Goals:**

- Every sim surface shows **Phase mod 3** on Audio row 7 and routes knob 7 to PM3 depth (VCO2→VCO3 when cross-coupler CW).
- Operator docs on desktop (embedded), web, and `docs/` match `ParamDisplayNames`.
- Single label authority documented; duplicate tables either removed or build-synced.

**Non-Goals:**

- Field firmware OLVL / FUEG behavior changes.
- Renaming mod source 4 (stays **VCO Envelope** on desktop/web; **VCO level** rename in `docs/` only if consolidating to header spelling).
- VCV field-parity widget implementation (separate change; must consume same header).
- VCO Envelope scope UX changes (continuous CV trace remains on desktop/web).

## Decisions

### D1 — `ParamDisplayNames.hpp` is the only label authority

**Choice:** All sim UI and docs derive row/mod names from `sim/ParamDisplayNames.hpp`. TypeScript duplicates must match verbatim or be generated at build time.

**Why:** OMNI single authority; `vcv-vst-field-parity-panel` already plans `HostPanelLayout.hpp` extraction from the same tables.

**Alternative rejected:** Per-host label strings — caused the current drift.

### D2 — Sim PM3 DSP flag stays as-is

**Choice:** Keep `SetSimDedicatedPm3Knob(true)` in `DesktopHostIO::Init` and `PagedHostIO::Init`. Param 6 drives `m_pm3`; `m_oscLvl` fixed at 0.4 on sim.

**Why:** Already correct in engine; this change closes label/doc gaps, not DSP rewrite.

### D3 — Doc consolidation: one manual track

**Choice:** Update `docs/sim-manual.md` and `docs/quick-dict.md` to match root `SIM_MANUAL.md` / `QUICK_DICT.md` / `web/public/*`. Desktop embeds root files via `desktop/CMakeLists.txt` — no CMake change if root files stay canonical.

**Why:** Two manual tracks (`docs/` vs root) caused desktop/website to look fixed while `docs/` still said VCO level.

### D4 — VCO Envelope scope semantics (document, don’t change)

**Choice:** Manual explains mod source 4 scope shows **smoothed tanh-compressed mean |VCO mix|** updated every 64 samples — mod CV out, not audio.

**Data flow:**

```
StepOscillators → v1,v2,v3
       ↓
UpdateM5FromVco (every 64 samples: |mean(v1,v2,v3)| → hold → tanh → smooth)
       ↓
m_modMgr->m_mods[4]  →  GetCvOut(4)  →  CvScopeDisplay / CvScopeCanvas
```

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| Users expect knob 7 to control osc level on sim | Manual: sim differs from Field; OLVL fixed internally on sim |
| `web/src/main.ts` label table drifts again | Task: add CI grep or build-time check against `ParamDisplayNames` |
| Confusion between VCO Envelope mod vs PM3 knob | Quick-dict + sim-manual explicit two-row glossary |
| Stale desktop binary assets | Rebuild `FroggersTigaAssets` after doc edits |

## Migration Plan

1. Fix `docs/` manuals and quick-dict.
2. Verify desktop panel labels at runtime (Audio row 7 = Phase mod 3).
3. Rebuild web dist and desktop embedded assets.
4. Manual spot-check: PM3 audible with cross-coupler CW and row 7 up; Crispy no longer acts as PM3 on sim.

## Open Questions

1. Generate TS labels from C++ header in wasm build, or manual sync + lint only? **Resolved:** web uses wasm `rows[].name` + `modSourceNames`; see `omni-host-compliance-fixes`.
2. Should `docs/` be deleted in favor of root-only manuals long-term?

## Appendix: Host audit (2026-06-14)

| Host | Row 7 label | PM3 from param 6 | Crispy ≠ PM3 on sim |
|------|-------------|------------------|---------------------|
| Desktop UI (`DesktopPanelBackend`) | Pass — Phase mod 3 | Pass — `SetSimDedicatedPm3Knob(true)` | Pass |
| Web UI (`HOST_PAGE_LABELS`) | Pass — Phase mod 3 | Pass — via `PagedHostIO` | Pass |
| Web wasm (`froggers_row_name`) | Pass — `ParamDisplayNames` | Pass | Pass |
| `PagedHostIO::GetRowName` (before fix) | Fail — firmware `OLVL` | Pass (DSP) | Pass |
| `docs/sim-manual.md` (before fix) | Fail — VCO level | — | Fail — Crunch |
| Root `SIM_MANUAL.md` / embedded desktop | Pass | Pass | Pass |

After this change: `PagedHostIO::GetRowName` and `docs/` aligned with `ParamDisplayNames`.
