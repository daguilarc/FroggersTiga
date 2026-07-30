## ADDED Requirements

### Requirement: Module expansion excludes Random page rows
Desktop v2 module expansion SHALL NOT add or retain Random S&H module-page expansion rows (Spread/Bias/Crispy tails for the former Random page). Envelope and Audio expansions follow ASR and three cross-coupler requirements in `froggers-v2-app-manifest`.

#### Scenario: Inventory has no Random expansion tails
- **WHEN** host-parameter inventory is validated
- **THEN** no expansion-tail rows remain bound to a Random S&H module page index

## REMOVED Requirements

### Requirement: Random S&H as an expandable FX module page
**Reason:** Sheaf-parity — random sources are mod lanes with ganged visualizers only; bag/deja-vu page params deleted.  
**Migration:** Delete page params from inventory/engine/UI; update manuals; drop obsolete preset axes on load.
