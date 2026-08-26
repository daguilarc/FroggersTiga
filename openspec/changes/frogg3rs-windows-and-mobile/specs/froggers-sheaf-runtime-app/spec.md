# Delta — `froggers-sheaf-runtime-app`

The requirement already says "every host it ships in". Adding a Windows
standalone adds a host, and the requirement is correct as written — what was
missing is that both mechanisms behind it were macOS-shaped: an in-app menu
that only exists on macOS, and a document path that only resolves inside a
macOS bundle. The requirement gains the platform-independence it already
implied, so a new host cannot satisfy it on paper while opening nothing.

## MODIFIED Requirements

### Requirement: Operator documentation ships with the app
THE app SHALL carry its manual and quick dictionary locally in every host
it ships in — standalone, VST3 and AU — and SHALL let the operator open
both from inside the app without a network connection. The documents SHALL
be embedded from the repository's single copy at build time, so that no
second checked-in copy exists to drift from the first. The browser build
MAY instead link to the published documents, because it is already running
in a browser with the network available.

Neither the way the operator reaches the documents nor the way the app
locates them SHALL assume a single platform's conventions. Where a host's
platform has no equivalent of the macOS main menu, the app SHALL present the
same entries by that platform's own means; where it has no application
bundle, the app SHALL find the documents where that platform's build places
them.

#### Scenario: Reading the manual offline in a DAW
- **WHEN** the plugin is loaded in a DAW on a machine with no network
- **THEN** the operator can open the manual and the quick dictionary from
  the plugin itself
- **AND** the content matches the repository's copy for that build

#### Scenario: The standalone app carries its own documentation
- **WHEN** the standalone app is opened with no network
- **THEN** both documents are reachable from inside the app

#### Scenario: Every shipped standalone platform reaches its documents
- **WHEN** the standalone app is opened on any platform it is released for
- **THEN** the manual and quick dictionary open from inside the app
- **AND** the files opened are the ones that build placed, not a path that
  resolves only on the platform the feature was written on

#### Scenario: One copy, not two
- **WHEN** the manual or the quick dictionary is edited in the repository
- **THEN** the next build carries the edit
- **AND** no checked-in duplicate of either document has to be re-synced
