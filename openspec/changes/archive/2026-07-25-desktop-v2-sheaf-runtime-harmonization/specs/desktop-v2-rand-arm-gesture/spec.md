## ADDED Requirements

### Requirement: Rand toggle is global
Desktop v2 SHALL provide a rand **toggle** that engages global randomization (Rand All / equivalent global command path). While the toggle is on, randomization applies with global scope via the Sheaf `Bank`/`ParameterManager` authority (the "all-banks" glue over `ApplyModifierToTopLevel(Random)`), per D16 — not the retired bespoke control-core mutator.

#### Scenario: Toggle on runs global rand
- **WHEN** the operator turns the rand toggle on and triggers rand
- **THEN** randomization uses the global authority path
- **THEN** the action does not require a subsequent parameter click to select a target

### Requirement: Rand held arms next-click local
Desktop v2 SHALL treat rand **held** as a one-shot local arm: the next parameter press or click randomizes only that parameter (Froggers local semantics), then clears the arm. This SHALL diverge from Sheaf’s hold-while-press continuous modifier model.

#### Scenario: Hold then click randomizes one parameter
- **WHEN** the operator holds rand and then presses one parameter encoder
- **THEN** only that parameter is randomized
- **THEN** the local arm is cleared afterward

#### Scenario: Hold without click does not mutate
- **WHEN** the operator holds then releases rand without pressing a parameter
- **THEN** no parameter values change from the arm alone
