# Delta — `froggers-sheaf-runtime-app`

## ADDED Requirements

### Requirement: Each controller row control does one job
The MIDI configuration page SHALL offer exactly one control that lists devices — the add row's selector — and SHALL NOT offer a device or preset list on a configured row. A configured row SHALL NOT name a preset at all; it SHALL offer a Restore action, and only while it was created from a preset and its stored configuration differs from that preset, so that the action's presence is itself the signal that the row has been edited. Every distinct device or operating mode SHALL be its own preset, chosen once when the row is created; the page SHALL NOT offer a second control asking which model or mode a row is. A row SHALL offer to release a bound controller whenever a device is bound to it, and SHALL NOT offer that control otherwise.

#### Scenario: A row never offers another device's preset
- **WHEN** a MIDI Fighter Twister row is presented
- **THEN** no control on that row offers an Akai APC40 preset, or any preset for a kind other than the row's own
- **AND** the only control listing devices anywhere on the page is the add row's selector
- Check: `controllers_page_ui_tests.cpp`, the row-control tests (task 2.6); operator, tasks 6.1 and 6.3.

#### Scenario: Restore appears only when there is something to restore
- **WHEN** a row created from a preset has had a mapping edited
- **THEN** the row offers Restore, and names no preset anywhere on it
- **AND** pressing Restore reinstalls that row's own preset
- **AND** a row whose configuration still matches its preset offers no Restore
- **AND** a row that was never created from a preset offers none either
- **AND** editing a mapping and setting it back by hand withdraws Restore again
- Check: `controllers_page_ui_tests.cpp`, the Restore tests (task 2.6); operator, task 6.2.

#### Scenario: A device model is chosen once, as a preset
- **WHEN** the add row is opened
- **THEN** each Launchpad model is listed as its own preset, alongside the Twister and each APC40 mode
- **AND** no control anywhere on a created row asks which model or mode that row is
- **AND** a row created from a preset carrying a connect-time message sends exactly that message when its output connects, and one created from a preset without such a message sends none
- Check: `controllers_page_ui_tests.cpp`, the preset tests, and `instrument_tests.cpp`, the connect-message tests (task 2.6); operator, tasks 6.1a and 6.1b.

#### Scenario: Releasing a controller frees it and keeps its mappings
- **WHEN** a row with both endpoints bound is released
- **THEN** its open endpoints are closed, its stored references are retained, and another application can take the device
- **AND** reclaiming it restores its mappings
- **AND** a row with no bound device offers no release control at all, rather than a disabled one
- Check: `viewmodel_tests.cpp` and `browser_runtime_contract_tests.cpp`, the release round trip (task 2.5); operator, task 6.5.

### Requirement: A row remembers which preset created it
A controller row SHALL retain the identity of the preset that created it for as long as the row exists, and editing the row's mappings SHALL NOT discard that identity. Whether the row still matches that preset SHALL be determined by comparing the row's stored configuration against the preset's generated configuration, rather than by treating the recorded identity as a marker of an unedited row. Controls that depend on a row resolving to a known preset SHALL remain available after the row's mappings have been edited.

#### Scenario: An edited row keeps its provenance
- **WHEN** a mapping on a row created from a preset is edited, deleted, or added to
- **THEN** the row still resolves to the preset that created it
- **AND** the row is reported as differing from that preset
- Check: `viewmodel_tests.cpp`, the provenance tests (task 2.6).

#### Scenario: Editing a row does not withdraw its other controls
- **WHEN** a row with both endpoints bound has one of its mappings edited
- **THEN** the row still offers to release the bound controller
- **AND** a released row that has been edited still offers Configure
- Check: `controllers_page_ui_tests.cpp`, the row-control tests (task 2.6); operator, task 6.5.
