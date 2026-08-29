# Delta — `froggers-browser-package`

## ADDED Requirements

### Requirement: The browser's audio device controls offer the devices that exist

A device control in the browser host SHALL offer the devices the browser makes
available, and selecting one SHALL take effect. Neither control's contents may
be a constant, and neither may present a choice that changes nothing.

The browser reveals devices under conditions that do not apply to a desktop
host, and the controls SHALL be honest about them:

- Device identifiers are origin-scoped and reset when the viewer clears storage,
  so a stored selection SHALL be re-resolved against the current enumeration and
  SHALL fall back rather than resolve to a device this origin cannot name.
- Labels are empty until a capture permission has been granted. Unlabelled
  entries SHALL NOT be presented as though they were named devices.
- Devices appear and disappear while a page is open. A selection whose device
  has gone SHALL NOT keep claiming it.

A device SHALL be identified by its label, which is what a stored selection
records and what every host's persisted device name already means. Two devices
presenting the same label are not distinguishable, and the first match is the
one a stored selection resolves to.

Offering a device SHALL NOT start capture. Listing what exists and using it are
separate acts, as supplying an audio context and starting audio are separate.

Capture SHALL follow the operator's selection and nothing else. The host SHALL
NOT request capture on the strength of the application's declared input channel
count. A page that has been opened and not touched SHALL have requested no
microphone.

Output selection SHALL route audio to the selected device. Where the browser
provides no means to honour it, the control SHALL report that rather than
offering choices that do nothing.

#### Scenario: An input device can be chosen
- **WHEN** the operator opens the Audio I/O page after granting microphone permission
- **THEN** the Input device control offers at least one device besides No Input

#### Scenario: Listing a device does not start it
- **WHEN** input devices are listed and the operator selects nothing
- **THEN** no capture has been requested and No Input remains selected

#### Scenario: Opening the page asks for no microphone
- **WHEN** the site is opened by an application declaring an input channel
- **AND** the operator selects no input device
- **THEN** no capture has been requested and no permission prompt was raised

#### Scenario: The selection is what the capture request carries
- **WHEN** the operator selects an enumerated input device
- **THEN** capture is requested for that device rather than for the system default
- **AND** it takes effect on the running graph rather than at the next boot

#### Scenario: An unpermitted page does not invent names
- **WHEN** the page has been granted no capture permission
- **THEN** no unlabelled entry is presented as a named device

#### Scenario: A stored selection for a device that is gone
- **WHEN** a previously selected device is absent from the current enumeration
- **THEN** the control falls back rather than reporting a device that is not there

#### Scenario: An output device receives the audio
- **WHEN** the operator selects an enumerated output device
- **THEN** audio is routed to that device rather than to the system default

#### Scenario: A browser that cannot route output says so
- **WHEN** the runtime finds no means of honouring an output selection
- **THEN** the control reports that rather than offering devices it cannot route to
