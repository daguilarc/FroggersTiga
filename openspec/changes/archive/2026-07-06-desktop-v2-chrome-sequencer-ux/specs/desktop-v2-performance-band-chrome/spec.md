## ADDED Requirements

### Requirement: v2-performance-band-scene-labels-readable

Scene endpoint buttons S1, S2, and S3 in `PerformanceBandV2` SHALL render full button text without ellipsis at default window width (1280 px). Minimum button width SHALL be `DesktopV2ChromeLayout::kSceneButtonMinWidth` (initial 44 px).

#### Scenario: S1 and S2 not truncated

- **WHEN** performance band is visible at 1280 px width
- **THEN** buttons labeled S1, S2, and S3 show complete text
- **THEN** no label reads as "S…" or "S1…"

### Requirement: v2-performance-band-marbles-labels

Random S&H status LEDs in the performance band SHALL have visible labels **S&H 1** and **S&H 2** with label strip height ≥ `DesktopV2ChromeLayout::kPerfMarblesLabelH` (initial 14 px).

#### Scenario: Marbles labels legible

- **WHEN** performance band is visible
- **THEN** text "S&H 1" and "S&H 2" is readable at 100% UI scale
- **THEN** LEDs sit below or beside labels without covering label glyphs

### Requirement: v2-performance-band-blend-labels

Scene blend slider SHALL show **L** and **R** endpoint labels with minimum width `DesktopV2ChromeLayout::kBlendLabelMinWidth` (initial 16 px) each.

#### Scenario: Blend L R visible

- **WHEN** performance band is visible
- **THEN** "L" and "R" labels adjacent to the blend slider are not clipped to a single character

### Requirement: v2-performance-band-excludes-sequencer-clock

`PerformanceBandV2` SHALL NOT host BPM editor, pattern-length (Steps) control, **Start Sequence**, **Stop Sequence**, or **Write Seq.** Those controls belong in `desktop-v2-sequencer-toolbar`.

#### Scenario: Performance band slimmed

- **WHEN** the operator views the performance band
- **THEN** it contains scene endpoints, blend, gestures, marbles LEDs, and carousel randomize controls only
- **THEN** BPM and Steps are absent from the performance band bounds
