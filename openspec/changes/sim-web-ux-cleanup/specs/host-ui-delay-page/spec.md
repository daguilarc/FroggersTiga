## MODIFIED Requirements

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
