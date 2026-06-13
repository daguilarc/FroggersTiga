## Context

**Current layout chain:**

```text
Main.cpp centreWithSize(2016, 720)
MainComponent::resized → panelW = area.getWidth() / 6   // ~336 px at default
SubModulePanel::layoutPanel → btnRow 50/50 split
  m_randomizeMod text = "Randomize mod"   // never updated to Randmod
```

**Why columns feel bloated:** panel width is driven by **window width**, not button intrinsic size — but the **2016 default** was chosen when five panels became six (`stereo-delay-page`) and reaffirmed in `desktop-sim-ux-polish` §6. Long button copy + equal split makes the chrome row look wasteful and encouraged keeping a wide window so labels/knobs still fit.

**Quick Dict:** `app-header-help-menu` created table-based `QUICK_DICT.md`. Panel row labels come from `getRowName()` — desktop Audio already shows **VCO1** / **VCO2** / **VCO3** (display aliases in `PanelBackend`), not firmware `V1VO` / `V2VO` / `V3VO`. Quick Dict left-hand keys SHALL match **what the sim UI shows**, not internal OLED names.

## Goals / Non-Goals

**Goals:**

- Default window fits a single **1680px-wide** display at 100% scale (six panels ≈ **280 px** each).
- Per-panel **Randmod** + compact **Randomize** with intrinsic-width bounds.
- Global strip uses short labels consistent with web chrome (`web-sim-page-ux` design).
- Quick Dict: **`PRMT : Parameter Name`** per line where **PRMT** is the sim panel label (`getRowName()`), Manual for depth.

**Non-Goals:**

- Resizable minimum width enforcement or horizontal scroll.
- Renaming firmware `Parameter` OLED strings (V1VO etc. stay in core/Manual).
- Rewriting `MANUAL.md` body (only cross-link from Quick Dict).
- Web page-chrome button rename (web can follow in a later pass).

## Decisions

### 1. Default size 1680×720

```text
kDefaultWidth  = 1680
kDefaultHeight = 720
panelW ≈ (1680 - margins) / 6 ≈ 277–280 px
```

**Rationale:** 2016 exceeds 1920×1080 usable width with window chrome; 1680 fits MacBook 14" (1728 logical) and 1080p displays. Rotary knob layout from `desktop-sim-ux-polish` (fixed 38 px knob + 48 px min label) still fits at 280 px.

**Rejected:** 1440 default — Audio page wave rows tighter; 1680 is safer without layout refactor.

**Superseded by `desktop-chrome-cohesion`:** default **1440×720** (resizable); global strip labels **Rand All**, **Rand Mods** (was Rand all / Randmod all).

### 2. Intrinsic-width panel buttons

```text
constexpr int kBtnPad = 4;  // ≤6 px total horizontal inset beyond text
btnRow = top 26 px of panel
font   = m_randomize.getLookAndFeel().getTextButtonFont(m_randomize, btnRow.getHeight())
modW   = font.getStringWidth("Randmod") + kBtnPad
randW  = font.getStringWidth("Randomize") + kBtnPad
layout: [Randomize randW][gap 4][Randmod modW] left-aligned in btnRow
```

**Rationale:** Removes 50/50 dead space. `Font::getStringWidth` on the button’s LookAndFeel font scales with HiDPI; no fixed pixel widths.

**Alternative rejected:** Equal `btnRow.getWidth() / 2` split — wastes half the row on short labels.

### 3. Global strip labels

| Current | New |
|---------|-----|
| Randomize all | Rand all |
| Randomize mod (all) | Randmod all |
| Randomize VCO Waveform | Rand waves |
| Marbles | Marbles |

Strip `resized()` uses intrinsic widths where possible; remaining width distributed to longest control (Rand waves).

### 4. Quick Dict format

**Rule:** left token = **sim UI label** (same string as the knob row in desktop/web sim). Right token = short human name from `MANUAL.md`. Never map firmware-only names the user does not see (e.g. do not use `V1VO` when the panel says `VCO1`).

```markdown
# FroggersTiga Quick Dict

Panel labels → short names. Detail → **Manual**.

## Audio
VCO1 : Frequency + morph
VCO2 : Frequency + morph
VCO3 : Frequency
XCPL : Cross-coupler
PM1A : Phase mod 1
...
## Reverb
RVMX : Wet/dry
RSIZ : Room size
...
```

- No markdown tables in parameter sections.
- **Transport** and **Sim mod sources** sections use the same `LABEL : Name` pattern (not tables), e.g. `Play : Audio on/off`, `Randmod all : All mod routes`, `MIDI : QWERTY or hardware pitch CV`.
- Audio rows 0–2: **VCO1**, **VCO2**, **VCO3** (matches `DesktopPanelBackend::getRowName`, not `InitParam` names).
- Field-only: one line “Pickup, M1–M7, SW pages, V1VO… → Manual”.

### 5. Supersede 2016 width in prior specs

On apply, footnote these artifacts that default is now **1680×720**: `stereo-delay-page`, `desktop-sim-ux-polish` (design §6 + `desktop-panel-knobs` spec), `desktop-host-corrections` `desktop-wave-controls` spec, `app-header-help-menu` `quick-dict-doc` (format superseded by `quick-dict-format`).

### 6. Wave rows at 1680

At ~280 px/panel, Audio rows 0–2 still fit: 64 px label + 28 px wave button + 38 px knob + 20 px mod jack (from `desktop-sim-ux-polish` / `desktop-host-corrections`). Verification task 4.6 confirms VCO1/2/3 + wave icon not clipped.

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| 1680 still overflows 1440 laptops | User can resize; v2 can lower default if needed |
| Shorter strip labels less discoverable | Tooltips retain full phrases |
| Quick Dict loses transport prose | Manual + tooltips unchanged |

## Migration Plan

1. Change default size constants in `Main.cpp` + `MainComponent`.
2. Update `SubModulePanel` + `GlobalStrip` labels and `resized()` layout.
3. Rewrite `QUICK_DICT.md`; copy to `web/public/quick-dict.md`.
4. Rebuild desktop BinaryData (CMake picks up `QUICK_DICT.md` automatically).
5. Visual verify at 1680: six columns, Randmod visible, no horizontal clip on 1920 display.

## Open Questions

- None blocking.
