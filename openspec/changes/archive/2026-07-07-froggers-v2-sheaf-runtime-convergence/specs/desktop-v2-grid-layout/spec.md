## ADDED Requirements

### Requirement: Manifest-era v2 layout constants
`DesktopV2ChromeLayout.hpp` SHALL define module row, top-chrome-stack, and performance constants consumed by manifest-backed layout projections, including `kModuleRowModGap`, `kModCellW`, `kPerfMarblesLabelH`, transport/signal band bounds, global-command band bounds, and `moduleRowColumns`. Legacy center-cluster constants SHALL NOT be used as the authority for global-control placement after the top chrome stack projection is implemented.

#### Scenario: Constants consumed by layout code
- **WHEN** submodule row layout computes mod cell bounds
- **THEN** it uses `moduleRowColumns(rowWidth).modX` and mod column width from the same struct
- **THEN** mod cells are placed at x=0 inside dedicated mod column content

#### Scenario: No independent magic mod offset
- **WHEN** the helper-authority grep runs
- **THEN** panel layout code contains no independent `gridPx(31)` mod X placement

### Requirement: Top chrome stack has explicit bands
Desktop v2 SHALL reserve one top chrome stack above the carousel header and body. The stack SHALL contain exactly two always-visible bands at the default standalone size: a transport/signal band and a global-command band. The persistent right-side File, Audio, and MIDI runtime rail SHALL remain separate from the top chrome stack and SHALL NOT be treated as a second top strip.

#### Scenario: Transport signal band contents are fixed
- **WHEN** desktop v2 lays out the top chrome stack
- **THEN** the transport/signal band contains Play, Stop, and the global oscilloscope
- **THEN** the transport/signal band does not contain global randomization controls

#### Scenario: Global command band contents are fixed
- **WHEN** desktop v2 lays out normal module pages or parameter-detail pages
- **THEN** the global-command band contains Randomize All, Randomize Mod, waveform-randomize, Marbles/Rand Resample, Crunchy, and Shift
- **THEN** Randomize All and Randomize Mod show the shared `All Scenes` / `Current Scene` and `All Steps` / `Current Step` scope pairs directly below the command buttons

#### Scenario: Default app chrome fits
- **WHEN** desktop v2 lays out the app screen at 1280x920
- **THEN** the transport/signal band, global-command band, carousel header, center body, persistent right-side runtime rail, and fixed sequencer region are visible without overlapping
- **THEN** the layout does not create horizontal scrolling, hidden top controls, or clipped top-band labels

#### Scenario: Top chrome authority is singular
- **WHEN** layout bounds tests inspect the desktop v2 shell
- **THEN** there is one top chrome stack containing the transport/signal and global-command bands
- **THEN** no module-local top strip, hidden center cluster, or runtime rail projection owns duplicate global randomization controls

### Requirement: Carousel module pages use compact parameter grids
Desktop v2 carousel module pages SHALL lay out actual module parameters as compact, logical encoder grids in the center body at the default standalone size. Module pages SHALL avoid vertical scrolling at 1280x920 by fitting all manifest-declared visible controls into the grid or by splitting oversized modules into named manifest groups/subpages.

#### Scenario: Module parameter grid fits default standalone size
- **WHEN** desktop v2 lays out any carousel module page at 1280x920
- **THEN** every visible module parameter cell fits in the center body without overlapping global controls, runtime buttons, carousel header, the global oscilloscope, the sequencer direction/speed icon strip, or the 16-step sequencer grid
- **THEN** no module-page vertical scrollbar is visible
- **THEN** the page uses named grouping or subpages rather than hidden offscreen parameter cells when more than sixteen cells would otherwise be needed

#### Scenario: Parameter-detail grid fits default standalone size
- **WHEN** desktop v2 lays out a parameter-detail modulation view at 1280x920
- **THEN** the 4x4 parameter-detail encoder grid fits in the visible body without overlapping global controls, runtime buttons, carousel header, the global oscilloscope, the sequencer direction/speed icon strip, or the 16-step sequencer grid
- **THEN** all fifteen permanent source lanes and the dedicated Crispy/target encoder are visible
- **THEN** any scrollbar appears only when the viewport is smaller than the supported default size

### Requirement: Sequencer controls fit the default app screen
Desktop v2 SHALL reserve a stable sequencer region at the default standalone size. The sequencer region SHALL contain a two-row icon strip above exactly 16 step cells. The direction row SHALL expose `<`, `>`, and `RND`; the speed row SHALL expose `/2`, `/1.5`, `1`, `x1.5`, and `x2`.

#### Scenario: Sequencer region fits without truncation or scroll
- **WHEN** desktop v2 lays out the app screen at 1280x920
- **THEN** all 16 sequencer step cells are visible without horizontal or vertical scrolling
- **THEN** the direction icons `<`, `>`, and `RND` are visible without truncation
- **THEN** the speed icons `/2`, `/1.5`, `1`, `x1.5`, and `x2` are visible without truncation
- **THEN** the sequencer region does not overlap the carousel module grid, parameter-detail grid, global oscilloscope, global controls, runtime buttons, or carousel header
