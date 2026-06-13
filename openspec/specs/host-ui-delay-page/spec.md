# host-ui-delay-page Specification

## Purpose
TBD - created by archiving change stereo-delay-page. Update Purpose after archive.
## Requirements
### Requirement: Sim page list of six

Web and desktop sim hosts SHALL present six pages: **Audio**, **Marbles**, **Reverb**, **Filter**, **Drive**, **Delay**. Pages 0–4 SHALL map to core engine pages. Page 5 (**Delay**) SHALL use host `DelayState` only. Web Delay page chrome SHALL NOT use a distinct accent color or border treatment from other host pages.

#### Scenario: Web page indicator

- **WHEN** the user navigates to Delay on web
- **THEN** the page label SHALL read `Delay (6/6)` or equivalent
- **AND** row labels SHALL show sim display names from delay exports (**Delay time** through **Crunch**)

#### Scenario: Desktop six visible panels

- **WHEN** the desktop application opens at default width
- **THEN** six sub-module panels SHALL be visible including **Delay**

#### Scenario: Delay chrome matches other pages

- **WHEN** the user views the Delay page on web
- **THEN** page chrome border and title color match Audio–Drive pages
- **AND** no orange accent border is applied

### Requirement: Web mobile page arrows

The web simulator SHALL provide large previous/next controls flanking the knob area on viewports at most 720px wide. Touch targets SHALL be at least 44×44 CSS pixels. On viewports wider than 720px, visible flanking arrows are optional; page pills and keyboard `[` / `]` navigation SHALL remain available.

#### Scenario: Mobile layout

- **WHEN** viewport width is at most 720 px
- **THEN** prev/next arrows SHALL remain beside the knob column without horizontal scroll of the knobs

#### Scenario: Desktop layout without flanking arrows

- **WHEN** viewport width is greater than 720 px
- **THEN** the knob grid MAY span the full `#app` content width without visible flanking arrows
- **AND** page navigation via pills and keyboard SHALL still work

### Requirement: Randomize parity

Per-panel and global randomize actions SHALL include Delay parameters with the same UX as core pages.

#### Scenario: Per-panel Randomize on Delay

- **WHEN** the user clicks **Randomize** on the Delay panel
- **THEN** only Delay knob positions 0–6 SHALL randomize
- **AND** core page parameters SHALL remain unchanged

#### Scenario: Per-panel Randomize mod on Delay

- **WHEN** the user clicks **Randomize mod** on the Delay panel
- **THEN** only Delay mod depths SHALL randomize

#### Scenario: Global Randomize all

- **WHEN** the user clicks **Randomize all** on the global strip
- **THEN** all five core pages SHALL randomize via core APIs
- **AND** Delay knob positions 0–6 SHALL randomize via `DelayState`

#### Scenario: Global Randomize mod all

- **WHEN** the user clicks **Randomize mod (all)** on the global strip
- **THEN** core mod randomize SHALL run for pages 0–4
- **AND** Delay mod depths SHALL randomize via `DelayState`

#### Scenario: Web global randomize

- **WHEN** the user clicks **Randomize all** or **Randomize mod (all)** on web while on any host page including Delay
- **THEN** core and Delay randomize SHALL both apply

### Requirement: Patch cables on Delay panel

Desktop patch cables SHALL assign mod sources to Delay rows through `DelayState`, not through `PageManager` or `SetPageModSource` with page index 5.

#### Scenario: New cable to Delay row

- **WHEN** the user drags from a mod rack output to a Delay panel input jack
- **THEN** `DelayState` SHALL store the mod source for that row
- **AND** `PageManager` page 5 SHALL not be accessed

#### Scenario: Repatch Delay row

- **WHEN** the user moves an existing cable to a different Delay input
- **THEN** the prior Delay row assignment SHALL clear
- **AND** the new row SHALL receive the mod source

#### Scenario: Disconnect Delay cable

- **WHEN** the user drags a connected Delay plug to void
- **THEN** that row mod source SHALL become none (255)

#### Scenario: Delay mod affects knob

- **WHEN** a Delay row has mod source Marbles 1 assigned with non-zero depth
- **THEN** the corresponding Delay parameter SHALL respond to Marbles 1 level during audio processing
- **AND** mod levels SHALL be read from the same `m_modMgr.m_mods` bus core pages use (no duplicate mod state)

#### Scenario: Synthetic OLED on web Delay page

- **WHEN** host page index is 5 on web
- **THEN** row labels SHALL show **DTIM** through **DMIX** and **FUEG** from delay exports
- **AND** WASM `froggers_row_name` SHALL NOT be used for the visible OLED

#### Scenario: Overlay draws Delay cables

- **WHEN** a Delay row has mod source VCO feat assigned via patch cable
- **THEN** the overlay SHALL draw a persistent cable from the VCO feat output to that Delay input
- **AND** `GetPageModSource(5, row)` SHALL NOT be called

### Requirement: Web mod dropdown on Delay page

When host page is Delay, per-row mod dropdowns SHALL offer `None | VCO feat | Marbles 1 | Marbles 2` and store assignments in `DelayState`.

#### Scenario: Mod dropdown on DTIM row

- **WHEN** the user selects **Marbles 2** for row 0 on the Delay page
- **THEN** **DTIM** SHALL be modulated by Marbles 2 according to mod depth

### Requirement: FUEG row on Delay page

**FUEG** SHALL appear as row 7 on the Delay page with the same slider/mod behavior as core pages. FUEG SHALL affect params 0–6 through `Fuegoize.hpp`.

#### Scenario: FUEG excluded from panel Randomize

- **WHEN** the user clicks per-panel **Randomize** on the Delay panel
- **THEN** positions 0–6 SHALL randomize
- **AND** **FUEG** (position 7) SHALL NOT randomize

### Requirement: Firmware excluded

Firmware SHALL NOT expose Delay UI or parameters.

#### Scenario: Hardware page count

- **WHEN** the user cycles SW1/SW2 on Daisy Field
- **THEN** only five core pages SHALL be reachable

