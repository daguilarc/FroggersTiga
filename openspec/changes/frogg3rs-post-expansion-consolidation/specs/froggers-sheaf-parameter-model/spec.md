# Delta — `froggers-sheaf-parameter-model`

**Added 2026-08-12.** `frogg3rs-bank-expansion` established that every bank holds fourteen parameters and
that each new parameter's default reproduces the value it replaced. Both are now built and test-verified.

This delta adds the property those requirements do not reach: a parameter can be registered, bounded,
default-neutral and still be inaudible, backwards, or musically useless across its range. Nothing in the
existing spec requires a control to actually DO anything.

## ADDED Requirements

### Requirement: Every bank parameter is audibly effective across its own range
Each parameter a bank exposes SHALL produce an audible change in the instrument's output as it is swept across its range, in the direction its name implies, with no inaudible dead span other than a deliberate zero position at a control's own floor. A control whose only effect is at one extreme, or whose sense is inverted relative to its name, does not satisfy this requirement even if its value is correctly plumbed and correctly bounded.

#### Scenario: Sweeping a parameter changes the sound
- **WHEN** any bank parameter is swept from its minimum to its maximum with the instrument sounding
- **THEN** the output changes audibly across that sweep
- **THEN** the change proceeds in the direction the parameter's name implies

#### Scenario: A deliberate zero position is the only permitted dead span
- **WHEN** a parameter defines a true zero position at the bottom of its travel
- **THEN** that zero region is inert by design and does not violate this requirement
- **THEN** every other part of that parameter's travel is still audibly effective

#### Scenario: A range chosen without a specification is confirmed by ear
- **WHEN** a parameter's range, maximum or shape was chosen by an implementer with no specified value behind it
- **THEN** that choice is confirmed by ear before the parameter is considered done
- **THEN** a range that is technically safe but musically useless is recorded as a defect, not accepted

### Requirement: A measurement that guards a bound is pinned by a regression test
Where a bound on an audio stage was established by measurement, that measurement SHALL be pinned by a regression test rather than left as a one-time result recorded in prose, so that a later change cannot silently invalidate it. A measurement performed in a standalone harness and reported only in a document does not satisfy this requirement.

#### Scenario: A measured bound survives a later change
- **WHEN** a stage's bound was established by measurement and a later change alters that stage
- **THEN** a regression test fails if the measured bound no longer holds

#### Scenario: A test that cannot observe a violation does not count
- **WHEN** a regression test is written to guard a measured bound
- **THEN** it is confirmed to fail when the bound is deliberately broken
- **THEN** a test that passes without exercising the controlling quantity is treated as absent
