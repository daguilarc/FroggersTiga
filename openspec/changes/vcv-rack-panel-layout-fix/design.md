## Context

```
Current (broken) topology:

  Row 1:  FroggersTiga  [========72 HP========|~22 used~|------ empty ------|]
  Row 2:  Expander      [6 cols × (label+knob+jack) crammed into 72 HP — overlap]

Rack constraints:
  - box.size.y MUST == RACK_GRID_HEIGHT (380 px) per module
  - Browser preview instantiates full widget tree before placement
  - Expander API: leftExpander.module links horizontally; chain can be A ← B ← Primary
```

**Root cause chain (why this occurred):**

| Layer | Failure | Evidence |
|-------|---------|----------|
| **Design intent vs implementation** | `vcv-vst-field-parity-panel` D3 preferred 2-row layout but D3 **fallback** (3+3 split) was documented, never implemented when density failed | `design.md` D3 fallback table; code has `kNumColumns = 6` on one expander |
| **Layout math error** | Six 12 HP columns with widgets spanning ~12 HP each (label at `colX−3.5`, jack at `colX+2.5`) → adjacent columns overlap | Column centers at 6,18,30…66 HP; label width 10 HP |
| **Primary panel oversize** | 72 HP panel for ~22 HP of I/O → library preview scales entire module down; user sees “most isn’t visible” | Jack positions stop at 21.5 GRID (~322 px); panel is 1080 px |
| **Widget count / browser stress** | Single expander: 48 params + 48 inputs + 48 labels + screws = 150+ child widgets in preview | Rack log: browser preview + Framebuffer 0×0 on lights |
| **Incomplete refactor** | Header rewritten (`primaryPanelSize`, 3-arg `columnCenterX`) without updating `plugin.cpp` | `make` fails: missing `panelSize`, `addRowLabel`, `HostPanelLayout` in header |
| **Expander chain bug (latent)** | `primaryModule()` only checks immediate `leftExpander` — breaks when B sits left of A | One-hop `dynamic_cast`; design requires chain walk |

**Installed vs repo:** User runs last successful x64 build (pre-refactor). Repo HEAD does not compile.

## Goals / Non-Goals

**Goals:**

- Readable, non-overlapping widgets at 100% Rack zoom on a 1440 px-wide display
- Module browser preview does not crash when selecting any FroggersTiga model
- Clean compile on Rack SDK 2.4.1 x64 (Rosetta on Apple Silicon)
- Layout constants in one header; bounds verified by CI script
- Expander chain resolves primary regardless of A/B order

**Non-Goals:**

- DSP, MIDI CC gating, or host API changes
- Multi-row height on one module (Rack forbids)
- Re-adding 48 on-panel row text labels (tooltips suffice)
- VST layout changes (desktop UI already correct)

## Decisions

### D1 — Adopt D3 fallback: 24 HP primary + 36 HP × 2 expanders

**Choice:** Primary 24 HP; Expander A pages 0–2; Expander B pages 3–5; each expander 36 HP (3 × 12 HP columns).

**Why:** Measured footprint per column: knob ~2 HP + mod jack ~1.5 HP + margin fits in 12 HP without overlap. Six columns on 72 HP failed density test.

**Alternative rejected:** Keep 72 HP expander, shrink labels — still 48 widgets in one browser preview tree.

### D2 — Column layout within 12 HP

Per column (center `columnCenterX`):

| Element | X offset from center | HP |
|---------|---------------------|-----|
| Page title | centered, y = 0.6 × rowStep | — |
| Knob | 0 | 2 |
| Mod jack | +3 GRID | 1.5 |

Row Y: `rowStep = panelHeight / (kRows + 2)`; rows at `(1.5 + row) × rowStep`.

No `addRowLabel` — param name from `ParamDisplayNames::forHostPageRow` in `configParam`.

### D3 — Template expander module (DRY)

**Choice:** `FroggersTigaExpanderModuleT<kFirstPage, kNumColumns=3>` shared by A and B.

**Why:** Same `syncColumn` / `applyModJack` logic; only page offset differs. Satisfies repetition rule.

### D4 — `primaryModule()` chain walk (O(depth), depth ≤ 3)

```cpp
for (Module* m = leftExpander.module; m && depth < 8; m = m->leftExpander.module)
    if (auto* p = dynamic_cast<FroggersTigaModule*>(m)) return p;
```

Depth is bounded by Rack expander chain length (2 expanders + primary = 3). Not hot-path expensive vs audio.

### D5 — O(1) optimization hypotheses (runtime)

| Hypothesis | Current cost | Proposed | Expected gain |
|------------|--------------|----------|---------------|
| **H1: Precompute column X / row Y at widget build** | Division in loop during ctor | `constexpr float kColumnCenters[3]`, `kRowY[8]` in header | Build-time only; zero runtime benefit — **accept** (clarity) |
| **H2: Cache primary pointer on expander link** | `dynamic_cast` every `process()` sample | Set `FroggersTigaModule*` in `onExpanderChange()` / first process | O(1) per block vs O(chain) per sample — **implement** |
| **H3: Skip disconnected mod jack reads** | Already guarded by `isConnected()` | Keep | Already O(1) per row |
| **H4: Block ProcessBlock on primary** | Currently `ProcessBlock(..., 1)` per sample | Use `processArgs.sampleRate / 60` block size with member buffers | Reduces host overhead; separate change — **defer** (not layout) |
| **H5: Browser crash from zero-size lights** | `SmallLight` above mod outputs | Ensure light offset ≥ 0.5 GRID; omit lights in preview if needed | Stability — **test** in browser first; remove lights only if crash persists |

**Verdict:** Implement H2 (cached primary). H1 for layout authority. H4 out of scope. H5 validate manually.

### D6 — Layout bounds CI script

`sim/check_vcv_panel_bounds.sh` parses `FieldParityWidget.hpp` constants and asserts:

- All widget X ∈ [0, panelHp × 15]
- Column widget spans do not cross column boundaries
- Primary jacks fit within 24 HP

Pure bash + awk — no Rack runtime required.

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| **BREAKING:** Users with old single expander patches | Document re-add A+B; patch JSON won't migrate automatically |
| Three modules to place vs two | Primary+expander is standard Rack pattern (Marbles, etc.) |
| Browser crash persists after split | Isolate: test expander-only model; drop lights last |
| Rosetta build forgotten | `vcv/build.sh` one-liner in DEVELOPMENT.md |

## Migration Plan

1. Implement layout fix; bump plugin version 2.3.0
2. `arch -x86_64 make dist && make install` → `~/Documents/Rack2/plugins-mac-x64/`
3. User removes old FroggersTiga modules from patch; adds Primary + Expander A + Expander B
4. Verify browser preview + full patch

## Open Questions

- None blocking — D3 fallback is already approved in `vcv-vst-field-parity-panel`.
