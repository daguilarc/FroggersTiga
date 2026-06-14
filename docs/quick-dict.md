# FroggersTiga Quick Dict

Sim column labels → short gloss. Full sim guide → in-app **Manual**. Daisy Field hardware → `MANUAL.md` in the repository.

## Sim mod sources

MIDI CC 1 : Hardware/Web MIDI CC → latched CV on mod channel 1 (default ch 1, CC 1)
MIDI CC 2 : Hardware/Web MIDI CC → latched CV on mod channel 2 (default ch 1, CC 2)
VCO Envelope : Slow level from VCO mix
Random 1 S&H : S&H random mod CV ch.1 — step with Random; LED green when held CV > 55%
Random 2 S&H : S&H random mod CV ch.2 — step with Random; LED green when held CV > 55%

## Transport (sim)

Play : Audio on/off
Stop : Audio off
Ext. In. : Line/mic in for ring mod (desktop)
Randomize : Knobs 1–7 on current page (not Crispy)
Randmod : Mod sources + depths on current page
Rand All : All pages + Delay knobs
Rand Mods : All mod routes
Random : Step both random bags
Rand waveforms : Randomize VCO morph (sine/saw/square blend)

## Audio

VCO1 : Frequency + morph
VCO2 : Frequency + morph
VCO3 : Frequency (sine)
Cross-coupler : CCW 1→2, CW 2→3 from noon
Phase mod 1 : VCO2 → VCO1 when coupled
Phase mod 2 : VCO1+VCO3 → VCO2
Phase mod 3 : VCO2 → VCO3 when cross-coupler is CW (2→3)
Crispy : Scramble knobs 1–7 (Field: FUEG; mix topology when external in)

## Random

Step chance : Probability each channel steps on Random press
Deja vu 1 : Channel 1 bag walk / re-roll
Bag size 1 : Channel 1 values (2–8)
Slew 1 : Channel 1 glide between held values (not a clock) → Random 1 S&H
Deja vu 2 : Channel 2 bag walk / re-roll
Bag size 2 : Channel 2 values (2–8)
Slew 2 : Channel 2 glide between held values → Random 2 S&H
Crispy : Scramble knobs 1–7 (Field: FUEG)

## Reverb

Wet/dry : Reverb mix
Room size : Delay line lengths
Decay : Feedback / tail length
Pre-delay : Time before reverb tank
Damping : HF loss in feedback
Stereo width : Reverb L/R spread
Diffusion : Cross-feed between reverb lines
Crispy : Scramble knobs 1–7 (Field: FUEG)

## Filter

Comb offset : Short line before comb — smears strike, not pitch
Peak freq : Peaking EQ frequency
Peak gain : Peaking EQ gain
Peak Q : Peaking EQ Q
Comb delay : Comb pitch
Comb feedback : Comb resonance
Comb LP : Darken comb feedback
Crispy : Scramble knobs 1–7 (Field: FUEG)

## Drive

Drive : Polynomial drive amount
Shape : Drive curve
SRR 1 : Sample-rate reducer 1
SRR 2 : Sample-rate reducer 2
XOR : XOR bit mask on samples
Bit depth : Low-bit scramble depth
Fuzz : Sine/tanh blend
Crispy : Scramble knobs 1–7 (Field: FUEG)

## Delay

Delay time : ~0–2 s exponential
Send : Output to delay
Feedback : Delay feedback
Stereo width : L/R separation
Detune : Stereo pitch offset on repeats
Mod depth : LFO on delay time
Wet mix : Delay wet level
Crispy : Scramble knobs 1–7 (Field: FUEG)

## Field-only

Pickup badges, M1–M7 CV assign, SW pages, OLED abbreviations → repository `MANUAL.md`.
