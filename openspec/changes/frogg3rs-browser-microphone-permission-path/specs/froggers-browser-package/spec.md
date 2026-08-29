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
