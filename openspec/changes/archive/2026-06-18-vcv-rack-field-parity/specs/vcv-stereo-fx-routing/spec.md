## ADDED Requirements

### Requirement: Stereo FX routing matches desktop when expander linked

When the FX expander module is linked, VCV SHALL apply the same stereo bus math as desktop `AudioEngine`: `makeStereoFxSpread` + stereo delta restoration from `StereoDelay` wet L/R and reverb stereo deltas.

#### Scenario: FX L/R differ when delay width or reverb width is active

- **WHEN** `FroggersTigaFx` is expander-linked and delay width or reverb width is above zero
- **THEN** FX `audio L` and `audio R` outputs are not identical
- **THEN** stereo width matches the existing `StereoDelay` / reverb DSP (same as desktop)

#### Scenario: Main out carries mono core mix

- **WHEN** FX expander is linked
- **THEN** main module `audio` output carries `coreMono` (full processed mix, mono-folded — includes delay/reverb energy in mono form)
- **THEN** FX L/R carry `coreMono + stereo deltas` per `applyStereoBus` formula

#### Scenario: Option C intended Rack patch

- **WHEN** a user wants stereo output with expander linked
- **THEN** patching FX L/R to mixer L/R is the primary stereo path
- **THEN** main `audio` out is optional mono fold; patching main + FX L/R to the same bus is discouraged (mono core would sum multiple times)

#### Scenario: No expander — mono full mix on main

- **WHEN** only the main module is used without FX expander
- **THEN** main `audio` out carries the full mix on a single jack (current standalone behavior)

#### Scenario: No triple identical duplicate (regression guard)

- **WHEN** FX expander is linked
- **THEN** FX L and FX R are not both set to the same voltage as main out without width active
- **THEN** `setStereoOutputs(sameVoltage)` pattern is removed
