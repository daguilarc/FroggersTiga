# Delta — `froggers-sheaf-runtime-app`

Supersedes the default-patch requirement's sufficiency test. The 2026-07-28 GUI test showed the
existing "makes sound with no user input" requirement (D16) is satisfiable by a signal the
operator cannot hear: the default patch left all three VCO pitches at their 20 Hz mapping floor,
which consumer laptop speakers cannot reproduce. Nonzero output is not the requirement; audible
output is.

## ADDED Requirements

### Requirement: The default patch is audible on consumer laptop speakers
With a fresh data root and the transport started, the default patch SHALL produce output whose
energy is predominantly above the reproduction floor of consumer laptop speakers, not merely
nonzero. A default whose fundamentals sit at the parameter mapping's low extreme SHALL be treated
as silent for acceptance purposes, regardless of measured RMS.

#### Scenario: Fresh start produces audible sound
- **WHEN** the application starts with a fresh data root and the transport is started
- **THEN** the default patch's output contains significant energy above roughly 150 Hz
- **THEN** total RMS alone is not accepted as evidence of audibility

#### Scenario: Every default is justified for audibility
- **WHEN** the default parameter table is reviewed
- **THEN** each default's mapped physical value is recorded alongside it
- **THEN** any default whose physical value falls outside the audible/usable range carries either
  a corrected value or an explicit justification

### Requirement: Transport stop silences the instrument
Stopping the transport SHALL bring the instrument to silence within a short, bounded time,
including feedback-sustained tails in the delay and reverb. A stop that closes the envelope gate
while self-sustaining feedback paths continue ringing indefinitely SHALL be treated as a failed
stop.

#### Scenario: Stop ends self-sustained tails
- **WHEN** the delay or reverb has been driven into self-sustaining feedback and the transport is
  stopped
- **THEN** output decays below audibility within a bounded interval
- **THEN** it does not return until the transport restarts
