# Delta — `froggers-sheaf-runtime-app`

## ADDED Requirements

### Requirement: The MIDI configuration page fits this application's window in every state

The MIDI configuration page SHALL lay every control inside this
application's content width on every host in every reachable state:
controller rows collapsed and expanded, each configuration section open,
and a mapping row in each group that accepts an added row (Turn, Push,
System, Gesture, App action) beside the rows a device layout installs. The controller header SHALL be
two lines: identity (name, device, port status, Layout, and Variant for a
Launchpad) and ports with lifecycle (MIDI in, MIDI out, Rename to,
Rename, Delete, Blacklist). The page SHALL show a controller's device by
its display name, SHALL caption the add row's device selector "Device",
SHALL caption the ports "MIDI in" and "MIDI out" with a legend for the
status dots above the first controller, and SHALL show a controller's
full name. A combo box or text field SHALL never draw past its own box.

#### Scenario: Every state fits

- **WHEN** a Twister, a Generic and a Launchpad controller are configured,
  the Generic row is expanded with Encoders (a Turn and a Push row added),
  System Messages (a row added) and Analogs (a Gesture and an App action
  row added) open, the Launchpad row is expanded with System Messages
  open, and the Twister row is expanded with Encoders open
- **THEN** no control lies outside the page's content width in any of
  those states
- Check: `portable_ui_tests.cpp`,
  `TestControllersRowFitsWithinFroggersNarrowestHost`, the 900-wide
  Controllers fixture over every state (task 2.1); operator, task 5.1.

#### Scenario: The row reads as its parts

- **WHEN** the operator reads a MIDI Fighter Twister row
- **THEN** it shows "MIDI Fighter Twister", "MF Twister", two status dots
  with the legend above, the Layout selector, "MIDI in" and "MIDI out"
  selectors, a field captioned "Rename to" with its Rename button, and
  Delete
- Check: `controllers_page_ui_tests.cpp`, the display-name, caption and
  legend tests; operator, task 5.1.

#### Scenario: A selector's text stays in its box

- **WHEN** a controller's Layout selector shows "MIDI Fighter Twister" in
  the browser build
- **THEN** the selector fills exactly its box and clips its text
- Check: `browser/tests/ui-backend.spec.ts`, the select-fills-wrapper
  assertion.
