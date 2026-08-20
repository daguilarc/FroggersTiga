# Delta — `froggers-sheaf-runtime-app`

**Added 2026-08-20 by operator instruction.** The instrument's manual and
quick dictionary must travel with the instrument. A plugin loaded in a DAW
on a machine with no internet, or a standalone app opened offline, cannot
reach documentation that lives only behind a web link.

## ADDED Requirements

### Requirement: Operator documentation ships with the app
THE app SHALL carry its manual and quick dictionary locally in every host
it ships in — standalone, VST3 and AU — and SHALL let the operator open
both from inside the app without a network connection. The documents SHALL
be embedded from the repository's single copy at build time, so that no
second checked-in copy exists to drift from the first. The browser build
MAY instead link to the published documents, because it is already running
in a browser with the network available.

#### Scenario: Reading the manual offline in a DAW
- **WHEN** the plugin is loaded in a DAW on a machine with no network
- **THEN** the operator can open the manual and the quick dictionary from
  the plugin itself
- **AND** the content matches the repository's copy for that build

#### Scenario: The standalone app carries its own documentation
- **WHEN** the standalone app is opened with no network
- **THEN** both documents are reachable from inside the app

#### Scenario: One copy, not two
- **WHEN** the manual or the quick dictionary is edited in the repository
- **THEN** the next build carries the edit
- **AND** no checked-in duplicate of either document has to be re-synced
