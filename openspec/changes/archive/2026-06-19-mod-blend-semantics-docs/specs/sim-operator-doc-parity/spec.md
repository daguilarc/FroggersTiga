## ADDED Requirements

### Requirement: Mod blend semantics documented in sim manuals

All sim operator documentation (`SIM_MANUAL.md`, `QUICK_DICT.md`, `docs/sim-manual.md`, `docs/quick-dict.md`, `web/public/sim-manual.md`, `web/public/quick-dict.md`) SHALL include, in or under **Mod bay**, a **Mod depth & blend** subsection stating:

- Effective value crossfades between stored base and mod source via mod depth (not `knob × CV`).
- At depth 0 → base only; depth 1 → mod only; between → mix.
- UI shows live effective value while idle; drag edits mod depth.
- M1–M4 ignored when CV input inactive.

#### Scenario: sim-manual Mod bay section

- **WHEN** reader opens `web/public/sim-manual.md` Mod bay
- **THEN** Mod depth & blend subsection is present

#### Scenario: quick-dict mod depth entry

- **WHEN** reader opens `web/public/quick-dict.md`
- **THEN** mod depth defined as crossfade amount

#### Scenario: All manual copies in sync

- **WHEN** change is applied
- **THEN** `SIM_MANUAL.md`, `docs/sim-manual.md`, `web/public/sim-manual.md` contain equivalent text

### Requirement: Mod-then-fuego pipeline documented

Sim manuals SHALL state that on fuego-enabled pages (including Delay), modulation is applied first, then Crispy scrambles low bits of the result. Crispy itself can be modulated (scramble intensity follows effective Crispy). Pair-AR knobs are not fuegoized.

#### Scenario: Crispy mod gloss in manual

- **WHEN** reader opens Delay or Global controls section
- **THEN** text explains Crispy mod affects scramble intensity on rows 1–7

#### Scenario: Pair-AR exclusion noted

- **WHEN** reader opens Audio pair-AR section
- **THEN** pair-AR knobs described as moddable but not fuegoized
