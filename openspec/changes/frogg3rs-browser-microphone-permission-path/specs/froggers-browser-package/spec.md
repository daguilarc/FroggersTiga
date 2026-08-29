# Delta — `froggers-browser-package`

## ADDED Requirements

### Requirement: A visitor can reach a microphone from a page that has never been granted one

The browser host SHALL provide a route, taken by the operator, from a page
holding no capture permission to a page holding one. The route SHALL be
reachable using only what the interface offers.

Enumeration alone cannot satisfy this: a page without permission enumerates
input devices whose label and identifier are both empty, and those SHALL NOT be
presented as named devices. So the route SHALL be an explicit request for
access, presented as such rather than as a device to choose.

Requesting access SHALL NOT leave a capture device open. A permission prompt
necessarily opens a device for the interval the browser requires; that interval
SHALL end when the request settles, and the input selection SHALL remain at No
Input, because earning a label is not choosing a device.

Where the route would achieve nothing — permission already held, or no input
device present — it SHALL NOT be offered.

A denied request SHALL be reported to the operator.

#### Scenario: An unpermitted page can ask
- **WHEN** a page holding no capture permission is opened
- **THEN** the interface offers an action that requests capture access
- **AND** taking that action results in a permission request

#### Scenario: Granting fills the list without choosing anything
- **WHEN** the operator grants access through that action
- **THEN** the Input control lists the devices by name
- **AND** the selection is still No Input
- **AND** no capture stream is left running

#### Scenario: Denial is not silent
- **WHEN** the operator denies the request
- **THEN** the Audio page reports it

#### Scenario: The page still asks for nothing on its own
- **WHEN** the page is opened and the operator does nothing
- **THEN** no permission request has been raised

### Requirement: A failure the bridge swallows is still reported

The browser audio bridge SHALL report the reason a device enumeration or capture
attempt failed, even where it degrades rather than propagating the failure.

Degrading is correct: a browser that refuses to enumerate should leave the
operator with a working instrument. But a swallowed failure reads exactly like a
browser with no devices, and the two call for different responses. The
degradation SHALL be kept and the reason SHALL be made observable alongside it.

#### Scenario: An enumeration failure is distinguishable from an empty machine
- **WHEN** device enumeration throws
- **THEN** the instrument keeps running
- **AND** the reason is reported rather than only the empty list

#### Scenario: A machine with no input devices reports that, not a failure
- **WHEN** enumeration succeeds and reports no input devices
- **THEN** no failure is reported
