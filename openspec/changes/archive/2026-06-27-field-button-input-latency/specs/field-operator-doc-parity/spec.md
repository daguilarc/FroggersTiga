## ADDED Requirements

### Requirement: Field manual documents button responsiveness expectations

`MANUAL.md` SHALL state that SW1/SW2 and B1–B4 are polled on a fast path independent of OLED refresh, and that B2/B4 (Rand All / Rand All Mod) may complete over a few milliseconds while lighter buttons are immediate.

#### Scenario: Troubleshooting SW1/SW2

- **WHEN** reader opens SW1/SW2 troubleshooting in `MANUAL.md`
- **THEN** text references control-loop responsiveness (not bootloader) and distinguishes dead switches (no LED) from slow OLED under load
