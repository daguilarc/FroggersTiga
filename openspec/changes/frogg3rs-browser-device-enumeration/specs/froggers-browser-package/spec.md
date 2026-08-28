# Delta — `froggers-browser-package`

The scenario removed from the main spec returns here, as the thing to deliver
rather than as an assertion about behaviour that does not exist.

It was synced into `froggers-browser-package` by
`2026-08-28-frogg3rs-first-visit-and-open-repairs` without being checked against
the implementation, which offers exactly one input option — "No Input" — and
throws if any other is selected (`BrowserAudioDevices.hpp:196,214`). A spec is
not the place to record an intention as though it were behaviour, so it comes
out of the main spec and lives here until it is true.

## ADDED Requirements

### Requirement: The browser's audio device controls offer the devices that exist

A device control in the browser host SHALL offer the devices the browser makes
available, and selecting one SHALL take effect. Neither control's contents may
be a constant.

The browser reveals devices under conditions that do not apply to a desktop
host, and the control SHALL be honest about them rather than hiding them:

- Device identifiers are origin-scoped and are reset when the viewer clears
  storage, so a stored selection SHALL be re-resolved against the current
  enumeration and SHALL fall back rather than resolve to a device this origin
  can no longer name.
- Labels are empty until a capture permission has been granted. Unlabelled
  entries SHALL NOT be presented as though they were named devices.
- Devices appear and disappear while a page is open. A selection whose device
  has gone SHALL NOT keep claiming it.

Offering a device SHALL NOT start capture. Listing what exists and using it are
separate acts, as supplying an audio context and starting audio are separate.

Where a browser cannot honour an output selection, the control SHALL report
that rather than presenting a choice that does nothing.

#### Scenario: An input device can be chosen
- **WHEN** the operator opens the Audio I/O page after granting microphone
  permission
- **THEN** the Input device control offers at least one device besides No Input

#### Scenario: Listing a device does not start it
- **WHEN** input devices are listed and the operator selects nothing
- **THEN** no capture has been requested and No Input remains selected

#### Scenario: An unpermitted page does not invent names
- **WHEN** the page has been granted no capture permission
- **THEN** no unlabelled entry is presented as a named device

#### Scenario: A stored selection for a device that is gone
- **WHEN** a previously selected device is absent from the current enumeration
- **THEN** the control falls back rather than reporting a device that is not there
