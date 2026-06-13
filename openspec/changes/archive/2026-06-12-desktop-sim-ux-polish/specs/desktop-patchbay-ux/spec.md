## ADDED Requirements

### Requirement: Patch cables connect reliably from mod output to panel input

The desktop patch cable overlay SHALL allow a user to drag from a mod rack **output jack** and drop on any submodule **input jack** (pages 0–5, rows 0–7) to assign modulation. Connection SHALL persist and render a colored cable until cleared.

#### Scenario: New connection

- **WHEN** the user drags from the VCO feat output jack and drops on Audio panel row 0 input jack
- **THEN** `SetPageModSource(0, 0, 4)` is invoked
- **AND** a cable is drawn from VCO feat jack to that input jack

#### Scenario: Disconnect by drag away

- **WHEN** the user drags an existing cable from an input jack and releases on empty space
- **THEN** that row's mod source is cleared (mod index 255)

#### Scenario: Delay panel patching

- **WHEN** the user patches to a Delay panel row input jack
- **THEN** `DelayState::setModSource` is used instead of `SetPageModSource(5, …)`

### Requirement: Jack hit targets are usable at six-panel width

Patch port hit testing SHALL use a circular hit radius of at least **20 px** centered on each jack, independent of the visual jack diameter. Port screen bounds SHALL be refreshed at UI timer rate (not only on window resize).

#### Scenario: Hit near jack edge

- **WHEN** the user clicks within 20 px of a jack center
- **THEN** the overlay intercepts the click for cable drag
- **AND** the click does not reach the knob below

#### Scenario: Port bounds after layout

- **WHEN** panels are laid out at six-column width
- **THEN** input jack screen bounds match drawn jack positions within 2 px

### Requirement: Each parameter row exposes a mod-in jack

Every knob row (parameters 1–7 and FUEG) on every submodule panel SHALL expose a dedicated **mod-in input jack** rendered by the patch overlay. Mod rack boxes SHALL expose **mod-out output jacks**. The user SHALL patch by dragging from mod-out to any parameter mod-in; each parameter is independently patchable.

#### Scenario: Per-knob patch destination

- **WHEN** the user views any submodule panel row
- **THEN** a visible mod-in socket appears beside that row's knob
- **AND** dropping a cable on that socket assigns mod only to that parameter

#### Scenario: Six panels × eight rows

- **WHEN** the desktop app shows six panels
- **THEN** forty-eight mod-in input ports are registered with the patch overlay

### Requirement: Patch cables are thick and randomly colored

Connected patch cables SHALL render with a stroke width of at least **6 px**, a dark outline for contrast, and a **random hue from the color wheel** assigned per connection (stable until disconnected). Drag-preview cables SHALL use the same visual style.

#### Scenario: New cable appearance

- **WHEN** the user connects VCO feat to Audio panel row 2
- **THEN** a thick brightly colored cable is drawn from the mod-out jack to that row's mod-in jack
- **AND** the mod-in jack ring matches the cable color

### Requirement: Hover affordance on jacks

While the cursor is within hit radius of a patch port during an active drag, the target input jack SHALL show a visible highlight.

#### Scenario: Drag highlight

- **WHEN** the user drags a cable over a valid input jack
- **THEN** that jack displays a highlight before mouse release
