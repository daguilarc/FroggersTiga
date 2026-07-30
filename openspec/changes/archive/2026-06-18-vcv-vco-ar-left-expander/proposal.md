> **Superseded by `omni-repository-harmonization`.** The VCO-AR left expander was never started and is out of scope for harmonization. Archive with `--skip-specs`.

## Why

Desktop and web sim use **pair-sum** A/R (Att./Rel. 1+2 and 2+3) as level followers on `(VCO1+VCO2)` and `(VCO2+VCO3)` — correct for the horizontal Audio band and WASM UI, but wrong for VCV Rack. Operators patching the main + right voicing expander need **per-VCO Attack and Release** on a dedicated **left extension** that drives the three oscillators in the main engine, with its own **Crispy** and **Randomize / Randmod** — not a copy of the sim pair-AR band and not folded into the Audio column Crispy row.

## What Changes

- **New VCV module:** `Froggers Tiga VCO AR` — left expander linked to the main module (`leftExpander` chain)
- **Six A/R knobs:** Attack + Release for VCO1, VCO2, VCO3 (1 ms – 10 s exponential; same time law as `pair-ar-vcv-time-range`)
- **Per-VCO envelopes** in engine: three `PairArEnvelope` instances on `|v1|`, `|v2|`, `|v3|` before osc mix (VCV path only when left expander is linked)
- **Dedicated Crispy** on the left expander — fuegoizes/scrambles VCO-domain params on the main/right modules (VCO1–3 rows), independent of Audio column row-7 Crispy on the right expander
- **Dedicated Randomize + Randmod** on the left expander — randomize only the six A/R knobs (and mod depths if mod inputs added)
- **Label authority:** new `ParamDisplayNames::forVcvVcoAr` table; path silkscreen from `VcvPanelLayout` constants
- **Explicit non-change:** desktop/web pair-AR band, snapshot layout, and `AudioPairArState` stay as-is

## Capabilities

### New Capabilities

- `vcv-vco-ar-left-expander`: Left expander module, HP layout, expander linking, panel silkscreen, mod jacks per A/R param
- `vcv-vco-ar-dsp`: Per-VCO A/R engine path (3 envelopes, VCV-gated), time mapping, mix integration
- `vcv-vco-ar-crispy-randomize`: Left-expander Crispy + Randomize/Randmod wiring and fuego scope

### Modified Capabilities

- (none — baseline specs not archived on main; sim pair-AR unchanged)

## Impact

- `vcv/src/plugin.cpp` — new module class, expander discovery on main `process()`
- `vcv/src/widgets/` — left-expander column layout helper
- `sim/VcvPanelLayout.hpp` — `kVcoArHp`, grid anchors
- `sim/ParamDisplayNames.hpp` — `forVcvVcoAr(index)`
- `src/core/` — `VcoArState.hpp` (or extend host IO); `FroggersEngine::MixOscVoices` VCV branch
- `vcv/res/*.svg`, `vcv/scripts/generate_panels.py`
- `vcv/README.md`, `vcv/DEVELOPMENT.md` — patch layout diagram (left AR → main → right voicing → FX)
- **Not impacted:** web, desktop, WASM bindings, `AudioPairArState`, GitHub Releases workflow
