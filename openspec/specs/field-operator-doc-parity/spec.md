# field-operator-doc-parity Specification

## Purpose

Keep Field hardware operator documentation (`MANUAL.md`) and quick-dict Field glosses aligned with engine external-mix behavior and FUEG semantics.

## Requirements

### Requirement: Field manual external mix documented plainly

`MANUAL.md` SHALL describe external-input mix without product ring mod or FUEG/Crispy mix-topology language:

- Signal-flow ascii: parallel ring mod when external audio is present above the gate; VCO-only at `OLVL` when silent.
- No mix-topology table or FUEG morph between product and parallel.
- Audio page: external gate open → `(ext × VCO1 + ext × VCO2 + ext × VCO3) / 3`; FUEG does not shape external mix.
- Audio page exception: FUEG is fuegoizer plus PM3 depth only (no mix-topology bullet).
- Pair-AR shapes VCO-only mix, not the external ring-mod path.

#### Scenario: Field signal flow

- **WHEN** reader opens `MANUAL.md` signal-flow section
- **THEN** text does not mention FUEG continuum, product ring mod, or mix topology morph

#### Scenario: Field Audio FUEG row

- **WHEN** reader opens Audio page knob table row 8
- **THEN** FUEG is fuegoizer and PM3 depth only

### Requirement: Quick dict mirrors stay aligned

`QUICK_DICT.md`, `docs/quick-dict.md`, and `web/public/quick-dict.md` SHALL use consistent glosses:

- **Crispy** / **FUEG**: scramble knobs 1–7 (mod first); moddable; does not control external ring-mod mix.
- **Ext. In.**: optional line/mic; parallel ring mod when gate open; VCO-only when off/silent.
- Sim and Field sections remain distinct but topology language is absent from all copies.

#### Scenario: Crispy gloss in all quick-dict mirrors

- **WHEN** reader opens any quick-dict mirror Global section
- **THEN** Crispy/FUEG gloss has no external ring-mod topology or blend language

### Requirement: Field manual documents button responsiveness expectations

`MANUAL.md` SHALL state that SW1/SW2 and B1–B4 are polled on a fast path independent of OLED refresh, and that B2/B4 (Rand All / Rand All Mod) may complete over a few milliseconds while lighter buttons are immediate.

#### Scenario: Troubleshooting SW1/SW2

- **WHEN** reader opens SW1/SW2 troubleshooting in `MANUAL.md`
- **THEN** text references control-loop responsiveness (not bootloader) and distinguishes dead switches (no LED) from slow OLED under load
