## MODIFIED Requirements

**Audit 2026-06-30:** Prior spec relied on bank paging when height is constrained. User decision: **remove bank paging**; use carousel vertical scroll instead. Default height lowered from 100u to **92u (920px)** for 1080p safety.

### Requirement: all-rows-in-carousel-no-bank-paging

Desktop v2 and VST v2 SHALL **not** use bank prev/next paging for encoder rows. `visibleCount` SHALL always equal `rowsForPage(activePage)`. All rows for the active module SHALL exist in the carousel content; when the window is shorter than the content, the carousel area SHALL scroll vertically (`juce::Viewport` or equivalent).

Bank controls (`m_bankPrev`, `m_bankNext`, `m_bankLabel`), `SelectBank` messages, and `setMaxVisibleRows` height capping SHALL be removed.

#### Scenario: Filter shows ten rows without bank controls

- **WHEN** desktop v2 or VST v2 opens on Filter at default height
- **THEN** no bank prev/next controls are visible
- **THEN** all ten Filter rows exist in the carousel without scroll at default 920px height (after scope consolidation per `desktop-v2-scope-visualization` §0a)

#### Scenario: Pair-AR shows seven rows

- **WHEN** the Pair-AR module is active
- **THEN** seven encoder rows are present (six A/R + Crispy)
- **THEN** no bank controls appear

#### Scenario: Constrained height scrolls

- **WHEN** window height is below the height needed to show every row
- **THEN** the operator scrolls within the carousel to reach clipped rows
- **THEN** bank paging does not appear

### Requirement: default-height-1080p-safe

`kDefaultHeight` SHALL be **92u** (**920px**) at `kGridUnitPx = 10`. This SHALL fit comfortably on 1920×1080 displays with taskbar/dock margin. `kDefaultWidth` remains **128u** (1280px).

`Main.cpp` and `PluginEditorV2` SHALL open at `kDefaultWidth` × `kDefaultHeight` (no hardcoded 820/880/1000).

#### Scenario: Default opens on 1080p laptop

- **WHEN** desktop v2 launches on a 1080p display
- **THEN** the full window is visible without vertical clipping of the window frame itself
- **THEN** standalone carousel viewport shows **ten** encoder rows at 920px after scope consolidation — Filter (10 rows) fits without scroll; Audio (8 rows) fits with margin
- **THEN** carousel scrolls only when window height is below default; VST fits ten rows without scroll at 920px
