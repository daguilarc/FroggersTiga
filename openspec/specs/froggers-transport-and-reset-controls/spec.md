# froggers-transport-and-reset-controls Specification

## Purpose
Transport (Play/Stop/Freeze) and Reset controls for the Froggers app: silencing guarantees on Stop, the Freeze latch's sustained-drone behavior, and reset semantics for parameters and depths.
## Requirements
### Requirement: Stop silences the instrument in bounded time, in every patch
Pressing Stop SHALL always silence the instrument: its output SHALL decay below audibility and every voice SHALL reach its idle state within a fixed bound measured in seconds, regardless of the parameter values, modulation depths, or modulation sources in effect — including any state reachable through Randomize All, and including while the Freeze latch is engaged, which Stop SHALL disarm as part of stopping. (⚠ AMENDED TWICE on 2026-08-17, both times by the operator exercising the built app. First version made the Freeze effect unreachable by any route. Second version reached it by making Stop-while-latched sustain instead of stop — which made a button labelled Stop conditionally mean "sustain", a trap. This version: the drone is reached by the FREEZE button alone, and Stop is unconditional. Operator ruling: Freeze stops the transport itself.) On the running-to-stopped edge, every non-idle voice SHALL enter Release immediately, bypassing the Grace minimum-hold and any in-progress Attack or Decay: Grace is a play-time musical guarantee and SHALL NOT delay transport stop (audit-added 2026-08-17, restoring the pre-Grace synchronous-force semantic that the Grace mechanism removed from Stop as a side effect). While the transport is stopped, the delay's feedback-drive, the reverb's tank-drive and the filter's comb-drive pre-gains SHALL resolve to unity, and the Freeze and reverb Grit parameters SHALL resolve to zero, without altering their commanded values, so that resuming play restores the patch bit-exactly. No stage inside a feedback loop SHALL be able to hold that loop's state up while the loop's input is silent: any in-loop stage whose local gain is unbounded resolves to its bypass value while stopped (measured 2026-08-17: reverb Grit re-amplifies a sub-audible tail into a bounded but non-decaying limit cycle). The sustained-drive character SHALL be reachable through the transport Freeze BUTTON ALONE, requiring no other control: engaging the latch SHALL itself stop the transport — reproducing the stopped-transport-plus-sustaining-audio state the effect only ever existed in — and SHALL suppress the entire stop-edge teardown (the forced release, the stopped-state effective-value overrides, and the stateful-unit clear), so the instrument holds its sounding state instead of silencing. Releasing the latch SHALL restore the teardown and silence the instrument within the same bound Stop guarantees; resuming audio then requires Play, since the transport is genuinely stopped. Parameter edits SHALL remain live while the latch is engaged: the drone responds to encoder changes exactly as the original accidental state did (operator ruling 2026-08-17, choosing faithful reproduction over a full state lock).

#### Scenario: Randomized patches stop like any other
- **WHEN** Randomize All has been applied any number of times and Stop is then pressed, with the Freeze latch in either state
- **THEN** the output peak falls below audibility within the bound
- **THEN** every voice reaches idle within the bound, so the stateful-unit clear actually fires

#### Scenario: A stage that cannot advance cannot hold a voice open
- **WHEN** the transport stops while any voice is in any non-idle stage, at any Curve and Grace setting
- **THEN** that voice enters Release on the stop edge, without waiting for its current stage or the Grace minimum-hold
- **THEN** that voice reaches idle within the bound
- **THEN** no envelope configuration exists whose ramp fails to complete within a small multiple of its knob time

#### Scenario: Stopping does not edit the patch
- **WHEN** the transport is stopped and later restarted
- **THEN** every commanded parameter value is bit-identical to its pre-stop value
- **THEN** the stop-time overrides were applied to the resolved values only

#### Scenario: The Freeze button alone reaches the sustained drone
- **WHEN** the transport is running and the Freeze button is pressed
- **THEN** the transport stops as part of engaging the latch
- **THEN** the instrument SUSTAINS its sounding state — the drone the accidental stop-state used to produce — instead of silencing
- **THEN** encoder edits continue to reach the audio, reshaping the sustained drone
- **WHEN** the Freeze button is pressed a second time
- **THEN** the instrument silences within the same bound Stop guarantees, and resuming audio requires Play

#### Scenario: Stop always means stop
- **WHEN** the Freeze latch is engaged and sustaining the drone, and Stop is pressed
- **THEN** the latch is disarmed
- **THEN** the instrument silences within the bound, exactly as any other Stop
- **THEN** no sequence of Freeze and Stop presses leaves the instrument sounding after Stop

### Requirement: Reset restores the default patch
THE Reset controls SHALL revert to the instrument's fresh-launch
default patch — the values a first launch presents, which are 0 for
most parameters but not all — never to a flat all-zeros state no
launch ever shows. Reset All SHALL be global: every bank's page
parameters, every parameter's modulation depths, every bank's local
Crispy, and the shared global Crunchy all revert to their defaults.
Reset Page SHALL revert the currently shown bank's slice of that same
default patch, including that bank's Crispy, and SHALL NOT touch other
banks. From a drilled-in modulation grid, reset SHALL revert the
selected parameter's modulation depths to their default-patch values.
The default patch SHALL have a single definition shared by launch and
reset, so the two can never drift apart.

#### Scenario: Reset All lands exactly on a fresh launch
- **WHEN** the operator has changed parameters, depths, Crispy, and
  Crunchy — including via Randomize All — and presses Reset All
- **THEN** the instrument's entire state equals a fresh launch's
  default patch, field for field, in both scene poles
- **THEN** Crispy on every bank and global Crunchy are at their
  default values

#### Scenario: Reset Page restores that page's defaults, not zeros
- **WHEN** the Audio page's parameters have been edited and Reset Page
  is pressed while the Audio page is shown
- **THEN** the Audio bank's parameters return to their default-patch
  values — including the non-zero VCO shape defaults and the default
  cross-VCO pitch modulation depths — and the bank's Crispy returns to
  its default
- **THEN** every other bank's state is untouched

#### Scenario: One definition of the default patch
- **WHEN** the default patch is changed in a future edit
- **THEN** launch and reset both present the changed defaults, because
  both read the same single definition

