# Delta — `pair-ar-vcv-time-range`

The requirement named a manual that no longer exists. The behaviour it
documents is unchanged; only the document carrying it moves.

## MODIFIED Requirements

### Requirement: Manual documents time range and follower behavior

The operator manual SHALL state that pair-AR Attack/Release knobs span **1 ms – 10 s** (exponential), that the envelope follows pair-sum level (not a gate), and that the knob mapping matches VCV ADSR Attack/Release **time range** — not gate-triggered ADSR behavior.

#### Scenario: Manual pair-AR table

- **WHEN** a reader opens the operator manual's Audio pair-AR section
- **THEN** the documented time range is 1 ms – 10 s, follower semantics are described, and Delay time (~0–2 s) is not conflated with pair-AR
