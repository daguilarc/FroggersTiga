# Delta — `froggers-sheaf-runtime-app`

## ADDED Requirements

### Requirement: The MIDI configuration page offers this instrument's controls

The MIDI configuration page SHALL offer exactly the targets this application
registers: its parameter operations, its named actions, and the library
kinds it keeps. Every registered target SHALL be one the application's
on-screen action handler routes, and every front-screen control SHALL be a
registered target. A MIDI encoder push SHALL drill in through the same path
as an on-screen press. The page SHALL offer a layout choice per controller
of this application's registered device defaults and Custom, and choosing a
default SHALL install it. Saving a patch SHALL save the controller mappings
with it, and loading a patch that carries mappings SHALL install them.
Every offered target SHALL dispatch into a routed action of this
application, verified by a check that walks the registered list.

#### Scenario: The offered targets are this app's

- **WHEN** the operator opens the MIDI configuration page
- **THEN** every mappable target names a front-screen control this
  application has, and no held-modifier, gesture, library scene-select,
  clock or continue target appears
- Check: `viewmodel_tests.cpp` with the frogg3rs catalog, offered kinds
  equal the catalog.

#### Scenario: A MIDI encoder push is the app's drill-in

- **WHEN** a `ParamPush` for an encoder arrives on the MIDI bus
- **THEN** the app's drill level rises exactly as it does for the on-screen
  press of that encoder, stops at the cap, and a push on the selected cell
  returns one level
- Check: frogg3rs app suite, the MIDI push drill test.

#### Scenario: The layout choice installs a registered default

- **WHEN** the operator chooses MIDI Fighter Twister, Akai APC40 mkII
  (Generic) or Akai APC40 mkII (Ableton) for a controller
- **THEN** that controller's mappings become the registered default for
  that device, control for control as the proposal's tables give them, and
  editing any row afterwards shows the choice as Custom
- Check: `controller_wizard_tests.cpp` and the controllers page tests with
  the frogg3rs defaults; frogg3rs app suite, the device-default validation
  test.

#### Scenario: No offered target is a dead end

- **WHEN** the check dispatches each registered action through the MIDI
  hook
- **THEN** each one moves the same state the on-screen control moves, and an
  action that moves nothing fails the check by id and argument
- Check: frogg3rs app suite, the catalog walk test.

#### Scenario: A held drill button drills the turned knob and then lets go

- **WHEN** a button mapped to Hold Drill is held and a knob is turned by any
  amount in either direction
- **THEN** that knob drills in once, its value does not change while the
  button is held, and turning it again after release changes its value
- Check: `instrument_tests.cpp`, hold-drill sequence.

#### Scenario: A patch carries the mappings

- **WHEN** the operator saves a patch, changes a mapping, and loads that
  patch
- **THEN** the saved mapping is back, and a patch saved before mappings were
  carried loads its parameters and leaves the mappings alone
- Check: Sheaf patch persistence tests; operator, task 4.4.

#### Scenario: The Ableton layout keeps the knobs on one channel

- **WHEN** the operator chooses Akai APC40 mkII (Ableton), presses any
  Track Select button and turns device knob 1
- **THEN** encoder 9 still moves
- Check: operator, task 4.3. UNCONFIRMED on hardware until then.

#### Scenario: The manual tells the operator how to connect a controller

- **WHEN** the operator reads the manual's MIDI section
- **THEN** it says how to set a MIDI Fighter Twister's encoders to relative
  and its side buttons to CC Hold, which APC40 layout to choose and why the
  Generic one wants Track 1 selected, how to map another device as Custom,
  what Hold Drill does, and that patches carry mappings
- Check: operator, following the section on each controller at factory
  settings with nothing else to go on.
