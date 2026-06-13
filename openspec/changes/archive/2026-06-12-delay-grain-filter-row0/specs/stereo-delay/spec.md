## MODIFIED Requirements

### Requirement: Delay parameter layout

Host Delay page SHALL expose **DTIM**, **DSND**, **DFBK**, **DWID**, detune (row 4, display **Detune**), **DMOD**, **DMIX**, and **FUEG** at host page index 5.

#### Scenario: Delay time range

- **WHEN** **DTIM** is maximum at 44100 Hz sample rate
- **THEN** delay time SHALL be within 5% of **2.0** seconds

#### Scenario: Send and mix independence

- **WHEN** **DSND** is zero
- **THEN** delay wet signal SHALL be silent regardless of **DMIX**

### Requirement: Delay buffer capacity

`StereoDelay` SHALL support up to **2.0** seconds at 48 kHz (**96000** samples per channel) in sim builds only.

#### Scenario: Max delay at 48 kHz

- **WHEN** **DTIM** is maximum and sample rate is 48000 Hz
- **THEN** delay time SHALL be within 5% of **2.0** seconds without buffer overrun
