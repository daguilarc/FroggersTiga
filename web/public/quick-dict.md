# FroggersTiga Quick Dict

Sim column labels → short gloss. Full sim guide → in-app **Manual**. Daisy Field hardware → `MANUAL.md` in the repository.

## Sim mod sources

MIDI : QWERTY piano or hardware notes → pitch CV × velocity
VCO Envelope : Slow level from VCO mix
Marbles 1 S&H : S&H random mod CV ch.1 — step with Marbles; LED green when held CV > 55%
Marbles 2 S&H : S&H random mod CV ch.2 — step with Marbles; LED green when held CV > 55%

## Transport (sim)

Play : Audio on/off
Stop : Audio off
Ext. In. : Line/mic in for ring mod (desktop)
Randomize : Knobs 1–7 on current page (not Crunch)
Randmod : Mod sources + depths on current page
Rand All : All pages + Delay knobs
Rand Mods : All mod routes
Marbles : Step both marble bags
Rand waveforms : Randomize VCO morph (sine/saw/square blend)

## Audio

VCO1 : Frequency + morph
VCO2 : Frequency + morph
VCO3 : Frequency (sine)
Cross-coupler : CCW 1→2, CW 2→3 from noon
Phase mod 1 : VCO2 → VCO1 when coupled
Phase mod 2 : VCO1+VCO3 → VCO2
VCO Envelope : VCO-only level when no external in
Crunch : Scramble knobs 1–7 (Field: FUEG; also PM3 + mix on Audio page)

## Marbles

Step chance : Probability each channel steps on Marbles press
Deja vu 1 : Channel 1 bag walk / re-roll
Bag size 1 : Channel 1 marbles (2–8)
Slew 1 : Channel 1 glide between held values (not a clock) → Marbles 1 S&H
Deja vu 2 : Channel 2 bag walk / re-roll
Bag size 2 : Channel 2 marbles (2–8)
Slew 2 : Channel 2 glide between held values → Marbles 2 S&H
Crunch : Scramble knobs 1–7 (Field: FUEG)

## Reverb

Wet/dry : Reverb mix
Room size : Delay line lengths
Decay : Feedback / tail length
Pre-delay : Time before reverb tank
Damping : HF loss in feedback
Stereo width : Reverb L/R spread
Diffusion : Cross-feed between reverb lines
Crunch : Scramble knobs 1–7 (Field: FUEG)

## Filter

Comb offset : Short line before comb — smears strike, not pitch
Peak freq : Peaking EQ frequency
Peak gain : Peaking EQ gain
Peak Q : Peaking EQ Q
Comb delay : Comb pitch
Comb feedback : Comb resonance
Comb LP : Darken comb feedback
Crunch : Scramble knobs 1–7 (Field: FUEG)

## Drive

Drive : Polynomial drive amount
Shape : Drive curve
SRR 1 : Sample-rate reducer 1
SRR 2 : Sample-rate reducer 2
XOR : XOR bit mask on samples
Bit depth : Low-bit scramble depth
Fuzz : Sine/tanh blend
Crunch : Scramble knobs 1–7 (Field: FUEG)

## Delay

Delay time : ~0–2 s exponential
Send : Output to delay
Feedback : Delay feedback
Stereo width : L/R separation
Detune : Stereo pitch offset on repeats
Mod depth : LFO on delay time
Wet mix : Delay wet level
Crunch : Scramble knobs 1–7 (Field: FUEG)

## Field-only

Pickup badges, M1–M7 CV assign, SW pages, OLED abbreviations → repository `MANUAL.md`.
