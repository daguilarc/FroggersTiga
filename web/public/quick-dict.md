# FroggersTiga Quick Dict

Short glosses for sim knob and mod labels. Full guide → in-app **Manual**. Daisy Field hardware → `MANUAL.md` in the repository.

## Sim mod sources

MIDI CC 1 — Hardware/Web MIDI CC → latched CV (default ch 1, CC 1); disable greys column and clears routes
MIDI CC 2 — Hardware/Web MIDI CC → latched CV (default ch 1, CC 2); disable greys column and clears routes
VCO Envelope — Slow level from VCO mix; scope trace
Random 1 S&H — S&H random mod CV ch.1 — resample with Rand Resample; see Mod indicators in Manual
Random 2 S&H — S&H random mod CV ch.2 — resample with Rand Resample; see Mod indicators in Manual

## Transport

Play — Audio on/off
Stop — Audio off
Ext. In. — Line/mic in for ring mod (desktop)
Randomize — Knobs 1–7 on current page (not Crispy)
Rand mod — Mod sources + depths on current page
Rand All — All pages + Delay knobs
Rand Mods — All mod routes
Rand Resample — Resample both S&H channels (draws from bags)
Rand waveforms — Randomize VCO morph (sine/saw/square blend)

## Global

Crispy — Scramble knobs 1–7 on any page; on sim, also blends external ring-mod when Ext. In. is on (Field: FUEG)

## Audio

VCO1 — Frequency + morph
VCO2 — Frequency + morph
VCO3 — Frequency (sine)
Cross-coupler — CCW 1→2, CW 2→3 from noon
Phase mod 1 — VCO2 → VCO1 when coupled
Phase mod 2 — VCO1+VCO3 → VCO2
Phase mod 3 — VCO2 → VCO3 when cross-coupler is CW (2→3)
Att. 1+2 — Pair-sum attack (VCO1+VCO2); panel abbrev for Attack
Rel. 1+2 — Pair-sum release (VCO1+VCO2); not reverb Decay
Att. 2+3 — Pair-sum attack (VCO2+VCO3)
Rel. 2+3 — Pair-sum release (VCO2+VCO3)

## Random

Step chance — Probability each channel resamples on Rand Resample press
Deja vu 1 — Channel 1 bag walk / re-roll
Bag size 1 — Channel 1 values (2–8)
Slew 1 — Channel 1 glide between held values (not a clock) → Random 1 S&H
Deja vu 2 — Channel 2 bag walk / re-roll
Bag size 2 — Channel 2 values (2–8)
Slew 2 — Channel 2 glide between held values → Random 2 S&H

## Reverb

Wet/dry — Reverb mix
Room size — Delay line lengths
Decay — Feedback / tail length
Pre-delay — Time before reverb tank
Damping — HF loss in feedback
Stereo width — Reverb L/R spread
Diffusion — Cross-feed between reverb lines

## Filter

Comb offset — Short line before comb — smears strike, not pitch
Peak freq — Peaking EQ frequency
Peak gain — Peaking EQ gain
Peak Q — Peaking EQ Q
Comb delay — Comb pitch
Comb feedback — Comb resonance
Comb LP — Darken comb feedback

## Drive

Drive — Polynomial drive amount
Shape — Drive curve
SRR 1 — Sample-rate reducer 1
SRR 2 — Sample-rate reducer 2
XOR — XOR bit mask on samples
Bit depth — Low-bit scramble depth
Fuzz — Sine/tanh blend

## Delay

Delay time — ~0–2 s exponential
Send — Output to delay
Feedback — Delay feedback
Stereo width — L/R separation
Detune — Stereo pitch offset on repeats
Mod depth — LFO on delay time
Wet mix — Delay wet level

## Field-only

Pickup badges, M1–M7 CV assign, SW pages, OLED abbreviations → repository `MANUAL.md`.
