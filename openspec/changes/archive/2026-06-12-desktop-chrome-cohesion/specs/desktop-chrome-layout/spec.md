## ADDED Requirements

### Requirement: Two-row header eliminates dead band

The desktop header SHALL use two rows: **transport row** (32 px) with Play, Stop, External, input level, MIDI, Audio, and **RECORD**; **mod rack row** (72 px) with the mod rack on the left and format toggles in a right column (120 px). The format column SHALL share row 2 with the mod rack — not a full-height right column beside an empty band under transport.

#### Scenario: No empty band under transport

- **WHEN** the user views the desktop header at default size
- **THEN** no empty horizontal band appears between the transport controls and the mod rack beneath the format column

#### Scenario: RECORD in transport row

- **WHEN** the user views the transport row
- **THEN** the red record circle and **RECORD** label sit immediately right of **Audio**

### Requirement: Mod rack boxes use fixed width centered in row

The mod rack SHALL lay out four `ModModuleBox` components at a **fixed preferred width** (~96 px) with uniform gaps (**16 px**). The group SHALL be **horizontally centered** in the mod rack row. Box width SHALL NOT grow proportionally to window width at default or wider sizes.

#### Scenario: Wide window mod rack

- **WHEN** the window is 1680 px or wider
- **THEN** each mod scope box remains approximately 96 px wide
- **AND** empty margin appears on both sides of the rack group

#### Scenario: Scope width at default

- **WHEN** the window is at default 1440×720
- **THEN** each scope trace area is no wider than 96 px minus padding

### Requirement: Global strip buttons show full labels

Global strip buttons (**Rand All**, **Rand Mods**, **Rand waves**, **Marbles**) SHALL be sized with `TextButton::getBestWidthForHeight` (or equivalent LookAndFeel fit). The button group SHALL be **horizontally centered** in the strip. No strip button SHALL truncate its label with ellipsis at default window size.

#### Scenario: Marbles label visible

- **WHEN** the user views the global strip at default window size
- **THEN** the Marbles button reads **Marbles** in full

#### Scenario: Rand Mods label visible

- **WHEN** the user views the global strip at default window size
- **THEN** the mod-randomize button reads **Rand Mods** in full

### Requirement: Chrome layout uses shared constants

`DesktopChromeLayout.hpp` (included by layout components) SHALL define mod rack box width, gap, min width, record cluster width, format row height, transport row height, and mod rack row height in one place. Components SHALL NOT duplicate conflicting magic numbers.

#### Scenario: Record cluster height matches format rows

- **WHEN** the record export cluster is laid out in row 2
- **THEN** its format area accommodates four equal rows without clipping the last row
