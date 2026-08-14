# Empty-encoder-slot research — Drive bank (slots 10-13) and Delay bank (slots 9,10,12,13)

Sources read in-tree before research: `app/FroggersParameters.hpp`, `app/dsp/Drive.hpp`,
`app/dsp/Delay.hpp`, `app/dsp/DspMath.hpp`, `app/dsp/EnvelopeFollowers.hpp`, and the active
`openspec/changes/frogg3rs-bank-expansion/` (proposal.md + tasks.md), which independently confirms
this task's own "already rejected" list (Peak Slope, VCO Spread, Sub-Oscillator, Cross XOR, Hard Sync,
Cycle) and confirms Drive/Delay slot-fills are still fully open, with "Feedback Drive"/"Feedback Tone"
recorded there too as the lead's own (undecided) recommendation.

Screening rule applied to every candidate below: continuous, whole-sweep-musical (constraint 1); does
not add a "route A to B" mechanism that duplicates the 15-source modulation matrix (constraint 2); not
on the rejected list (constraint 3).

---

## DRIVE bank — ranked candidates for slots 10-13

### 1. Bias — `Bias` — rank 1
- **0.0** full negative DC bias into the wavefolder: fold skews down, strong even-harmonic, hollow/
  "starved" character. **0.5** symmetric — today's unbiased fold (odd-harmonic-dominant). **1.0** full
  positive bias — the mirror-image skew, same asymmetric character flipped in polarity.
- **Precedent:** Surge XT's Waveshaper `Bias` control — "adds DC offset before the waveshape and
  subtracts it after, making the waveshaping asymmetric" ([Surge XT manual](https://surge-synthesizer.github.io/manual-xt/)).
  Joranalogue **Fold 6** `Symmetry` knob — "adds a manually variable DC offset... to achieve asymmetric
  wavefolding, with either the positive or negative part of the waveform being folded more than the
  other" ([Fold 6 manual](https://joranalogue.com/products/fold-6)). Old Blood Noise Endeavors **Alpha
  Haunt** fuzz ships a dedicated `Bias` knob for the same asymmetric-clipping idea on a fuzz circuit
  ([Perfect Circuit](https://www.perfectcircuit.com/old-blood-noise-alpha-haunt.html)).
- **Reuses:** `dsp::PolynomialDrive` — add the offset immediately before `Process()`, subtract it after
  (mirrors this same file's own DC-block idiom already used in `DigitalReorganizer::Process`, Drive.hpp).
- **Cost:** composes-existing.
- **Headroom:** **FLAG.** An offset run through a 5th-order polynomial does not cancel linearly the way
  it does through a linear stage — subtract-after keeps the *DC* neutral but the peak swing through
  `coefs[1..4]` is not guaranteed unchanged across the whole Bias sweep. Needs the same kind of sweep
  measurement this file's other authored stages (DriveBlendPhase) already required before shipping.
- **Why it earns a slot:** doubles Shape's expressive range (odd- vs. even-harmonic fold) for the price
  of two adds around a struct that's already in the chain.

### 2. Tone — `Tone` — rank 2
- **0.0** dark/muffled, all crush/fold harmonics rolled off. **0.5** moderate brightness. **1.0** fully
  open, unfiltered grit (today's implicit behavior).
- **Precedent:** ProCo **RAT**'s `Filter` control — a passive one-pole lowpass sweeping roughly 475 Hz-
  32 kHz, "the passive Tone Control is a simple low pass filter" ([ElectroSmash RAT
  analysis](https://www.electrosmash.com/proco-rat-analysis/pedals/distortion/pro-co-rat-distortion.html)).
  Electro-Harmonix **Big Muff Pi**'s `Tone` knob ([ehx.com](https://www.ehx.com/products/big-muff-pi-2-with-tone-wicker/)).
  Klevgrand **Degrader**'s post-resample lowpass, run after its own bit-crush stage
  ([klevgrand.com](https://klevgrand.com/products/degrader)).
- **Reuses:** `dsp::OnePoleLowPass` (DspMath.hpp) — a fresh instance placed after the whole `FrogBlock`
  chain, distinct in position from Anti-Alias Brightness (which filters mid-chain, inside the
  oversampler, before the downsample).
- **Cost:** composes-existing.
- **Headroom:** none — a lowpass only removes energy, cannot raise peak.
- **Why it earns a slot:** this is the single most universal post-drive control in the reference
  hardware/plugin space, and this bank is currently the only DSP-heavy bank without one.

### 3. SRR Spread — `SRRb` — rank 3
- **0.0** SRR2 tracks SRR1 exactly (today's two-independent-knob behavior is unaffected at this default).
  **0.5** slight relative detune between the two crushers — faint metallic beating/interference between
  their two hold-rates. **1.0** wide detune — harsh, aliasing-heavy interference pattern.
- **Precedent:** softer than the others — this is a documented DIY/modular technique rather than a named
  shipped control: "using two sample rate reducers with slightly detuned oscillators would create the
  beating/interference pattern" ([MOD WIGGLER discussion on dual
  bitcrushers](https://www.modwiggler.com/forum/viewtopic.php?t=67408&start=30)); it is the same
  "unison detune" idiom applied to a pair of sample-and-hold stages instead of a pair of oscillators.
- **Reuses:** `dsp::SampleRateReducer` ×2 — both structs already exist side by side in `FrogBlock`; this
  only changes what value feeds the second instance's `SetFreq`, no new struct.
- **Cost:** reuses-existing (cheapest of this bank's list — no new state).
- **Headroom:** none — SRR is sample-and-hold; it can never raise peak amplitude above its input's.
- **Why it earns a slot:** free to build (one new mapping into an existing `SetFreq` call), and the two
  SRR units are otherwise a slightly redundant independent pair today.

### 4. Cascade — `Casc` — rank 4
- **0.0** single pass (today's behavior). **0.5** partial re-drive: signal is fed back through the
  polynomial+fuzz stage a second time at reduced level — thicker, more compressed-sounding harmonics.
  **1.0** full double-pass gain-staking — heavily saturated, fizzy/compressed extreme.
- **Precedent:** "gain stacking" — cascading two overdrive/fuzz/saturation stages in series is a
  standard, named pedalboard and mix technique: "their effect is more cumulative when using multiple
  devices of the same species... causes that second overdrive to become more saturated, generating a
  whole new flavor of distortion" ([Wampler Pedals, "Gain Stacking
  101"](https://www.wamplerpedals.com/blog/music/2020/05/gain-stacking-101/); [MusicRadar, "Pedal gain
  stacking"](https://www.musicradar.com/how-to/pedal-gain-stacking-order-and-more-explained)).
- **Reuses:** `dsp::PolynomialDrive` + the existing fuzz-blend math already in `FrogBlock::Process`
  (the `Sine01(...)  * (1-fuzz) + fuzz * PadeSaturator::Saturate(...)` line), called a second time and
  crossfaded against the single-pass result.
- **Cost:** composes-existing.
- **Headroom:** **FLAG, explicitly.** A second full drive pass compounds gain by construction — this is
  exactly the kind of level-raising change constraint 5 calls out. Needs its own limiter-budget
  re-derivation before shipping (same standing rule the codebase's `DriveBlendPhase`/`StereoDelay` wet
  limiters were built to satisfy).
- **Why it earns a slot:** it is the most idiomatically "in-vocabulary" of this whole list — this bank's
  own Drive/Fuzz controls already are stompbox-style gain stages, and stacking them is exactly the move
  guitarists already make with this exact class of pedal.

### 5. Edge — `Edge` — rank 5
- **0.0** no pre-emphasis, low end reaches the folder unchanged. **0.5** moderate high-frequency boost
  ahead of the fold — brighter, more transient-reactive folding. **1.0** strong pre-emphasis — the
  folder reacts mostly to high content, producing a thin, glassy, metallic fold.
- **Precedent:** weaker/softer than 1-4 — general synthesis literature confirms the technique ("boosting
  the middle range of a wavefolder sounds really awesome... frequency-selective amplification before
  folding can shape the tonal character," [Perfect Circuit, "Learning Synthesis:
  Waveshapers"](https://www.perfectcircuit.com/signal/learning-synthesis-waveshapers)) but no single
  shipped product names an "Edge"/pre-emphasis knob the way RAT names `Filter` or Surge names `Bias`.
- **Reuses:** `dsp::OnePoleLowPass` used as the subtracted term of a cheap 1-pole highpass
  (`input - LP(input)`), feeding `dsp::PolynomialDrive`.
- **Cost:** composes-existing.
- **Headroom:** mild flag — boosting highs ahead of a 5th-order polynomial can push the higher-order
  terms harder at a given Drive setting; worth a quick sweep even though it is pre-gain shaping, not a
  literal level raise.
- **Why it earns a slot:** cheap (one subtract feeding an already-present struct), and gives Shape a
  frequency-reactive character it doesn't have today. Ranked below 1-4 purely on precedent strength.

### 6. Grind — `Grnd` — rank 6
- **0.0** the digital back end (`DigitalReorganizer` + both `SampleRateReducer`s) is fully bypassed —
  pure analog-style fold+fuzz. **0.5** crush stages blended in parallel against the pre-crush signal.
  **1.0** full digital chain, today's always-serial default.
- **Precedent:** the general "parallel distortion" mix-knob idiom (this file's own header comment on
  `Blend`/`Phase` already cites the identical rationale — "keeps the raw input available underneath the
  processed tone"); shipped analogues include parallel-saturation mix knobs on multiband/parallel
  saturation plugins generally (e.g., Soundtoys Decapitator-style mix controls). No single product blends
  specifically "crush stages only" in parallel the way this would.
- **Reuses:** `dsp::DigitalReorganizer`, `dsp::SampleRateReducer` ×2 — same structs, but requires tapping
  the signal before them and crossfading, not just feeding them serially as today.
- **Cost:** composes-existing, but restructures `FrogBlock`'s signal flow (a new parallel tap) —
  structurally closer to the composes/genuinely-new boundary than any other Drive candidate here.
- **Headroom:** none inherent (crossfade of two already-bounded taps), but the two taps' relative levels
  should be checked before shipping so the crossfade doesn't have an audible level bump at either end.
- **Why it earns a slot:** the only way to reach "no crush at all" today is Bit depth/XOR both at zero,
  which still leaves the signal serially routed through those (now-neutral) stages; this exposes that
  state as its own musical parameter. Ranked last: real cost tier is closer to genuinely-new.

---

## DELAY bank — ranked candidates for slots 9, 10, 12, 13

### 1. Feedback Drive — `FbDr` — rank 1 (re-proposes the brief's own sketch, now precedent-backed)
- **0.0** unity pre-gain — today's behavior (the loop's `PadeSaturator::Saturate` only engages once a
  signal is already large). **0.5** moderate pre-gain — repeats warm into soft saturation, building with
  every generation. **1.0** heavy pre-gain — repeats saturate hard and fast, an aggressively fuzzed-out
  degradation within one or two round trips.
- **Precedent:** Strymon **El Capistan**'s `Tape Bias` knob: "high-headroom, extra clean, crisp echoes at
  lower settings, or crunchy, saturated echoes with higher bias levels. Higher bias settings also limit
  the echo volume, which can be useful... when you want to create high feedback and self-oscillation
  effects while still keeping output level under control" ([strymon.net secondary
  functions](https://www.strymon.net/secondary-functions-el-capistan-dtape-echo/)). Soundtoys
  **EchoBoy**'s `Drive` control adds saturation specifically in the feedback/repeat path ([EchoBoy
  manual, soundtoys.com](https://www.soundtoys.com/wp-content/uploads/EchoBoy-Manual.pdf)).
- **Reuses:** `dsp::PadeSaturator` — already the in-loop saturator at `StereoDelay::Process`'s
  `fbk * PadeSaturator::Saturate(fbL)` / `fbR` lines; this candidate is one multiply (pre-gain) inserted
  before that existing `Saturate` call, no new struct.
- **Cost:** composes-existing (cheapest of the Delay list — literally one multiply into an existing call).
- **Headroom:** **FLAG, per the brief's own instruction.** This is exactly the loop the codebase has
  already been burned by (the unbounded-feedback B2 finding this same file documents). Pre-gain raises
  what reaches `Saturate` on every round trip; the wet-limiter tuning (`kDelayWetLimiterThreshold` /
  attack / release, all empirically measured against `dfbk=1.0` alone) needs re-sweeping with Feedback
  Drive pushed hard simultaneously with Feedback — the El Capistan precedent itself notes this exact
  combination is where "self-oscillation" lives, i.e. the worst case is reachable, not hypothetical.
- **Why it earns a slot:** the operator's own sketch, and it turns out to be almost exactly what two
  shipped tape-delay products (one hardware, one plugin) already ship as their signature control.

### 2. Feedback Tone — `FbTn` — rank 2 (re-proposes the brief's own sketch, now precedent-backed)
- **0.0** full-bandwidth repeats — today's behavior. **0.5** repeats audibly darken after a few
  generations. **1.0** heavily damped — repeats collapse to low thuds within 2-3 generations, classic
  "aging tape" character.
- **Precedent:** Strymon El Capistan's `Tape Age` knob — "adjusts bandwidth as tapes wear out over
  time... darkens the tape, reducing its full bandwidth... morph between the full bandwidth character of
  brand new stock, and the warm top end of older and well-used tape"
  ([strymon.net](https://www.strymon.net/secondary-functions-el-capistan-dtape-echo/)). More broadly,
  "a lot of delays offer some level of control over the equalization... in the feedback loop... creating
  echoes that lose energy in problematic frequency ranges with each repeat" ([Perfect Circuit, "Delays +
  External Feedback Loops"](https://www.perfectcircuit.com/signal/delay-external-feedback-loop)).
- **Reuses:** `dsp::OnePoleLowPass` (DspMath.hpp) — one instance inserted into the feedback tap ahead of
  `WriteSample`, alongside (not instead of) the existing `PadeSaturator::Saturate`.
- **Cost:** composes-existing.
- **Headroom:** none — a lowpass in the loop can only remove energy from what recirculates; it improves
  the loop's stability margin rather than threatening it.
- **Why it earns a slot:** the single most common character control on every real tape/analog delay
  reference (El Capistan, RE-201-style boxes, generic "damping" in delay/reverb algorithms alike), and
  currently entirely absent from this bank.

### 3. Crush — `Crsh` — rank 3
- **0.0** full-resolution repeats — today's behavior. **0.5** repeats visibly grainy/aliased after a
  couple of generations — classic lo-fi delay. **1.0** heavily crushed — repeats collapse into harsh
  digital noise within one or two generations.
- **Precedent:** Strymon **Timeline**'s dedicated Lo-Fi delay machine ships exactly this as two knobs
  (`Filter`/`Grit`) shaping bit-rate/sample-rate degradation specifically of the repeats — "set the
  sample rate down to 2kHz and the bit-depth to 10-bits to shape the lo-fi character of the delays...
  the Filter knob works in conjunction with the Grit knob to shape the fidelity of the repeats"
  ([strymon.net](https://www.strymon.net/weeks-preset-timeline-crushd-guitar/)). Also Malekko **616
  Lofi**, Caroline **Kilobyte** ("adds increasing simulated distortion to each echo"), and Pigtronix
  **Echolution 2**'s lofi modes all do the same thing on hardware ([Delicious Audio, "Best Bitcrusher
  Pedals"](https://delicious-audio.com/best-bitcrusher-pedals-sample-rate-reducers/)).
- **Reuses:** `dsp::SampleRateReducer` and/or `dsp::DigitalReorganizer` (Drive.hpp) — the exact same
  structs already driving the Drive bank, instantiated fresh and placed on the delay's feedback tap.
- **Cost:** composes-existing (reuses Drive-bank units wired into a new signal path).
- **Headroom:** mild flag — must reuse `DigitalReorganizer`'s existing DC-blocked `Process()` (its
  `Mangle(input,...) - Mangle(0,...)` form), not a hand-rolled copy of the raw bit-scramble formula; a
  naive copy would reintroduce the exact `f(0) != 0` DC-bump-into-an-unbounded-loop bug this file's own
  header comment documents having already fixed once. Otherwise no new peak-raising risk — SRR/hash
  outputs never exceed their input's magnitude.
- **Why it earns a slot:** directly on-brand — this instrument's whole drive vocabulary is SRR/XOR/hash
  — and it is the single most-requested "lo-fi delay" flavor across the reference products surveyed.

### 4. Diffusion — `Diff` — rank 4
- **0.0** sharp, discrete repeats — today's behavior. **0.5** repeat edges soften/blur, starting to feel
  reverb-adjacent. **1.0** repeats smear into a wash, edging toward comb-free "reverb built from a
  delay."
- **Precedent:** Valhalla **Delay**'s dedicated Diffusion section (`Amount`/`Size`) — "smears the repeats
  toward reverb... introduced subtly, it just softens and blurs the edges of the repeats... turn the
  Amount control up full and you can get a pretty convincing reverb"
  ([valhalladsp.com](https://valhalladsp.com/2019/06/13/valhalladelay-the-diffusion-section/)). Chase
  Bliss **Mood**'s `Modify` knob does the same on repeats, "ranging from a jagged, particle delay to a
  washed out, ethereal reverb, simply smearing the repeats"
  ([Vintage King](https://vintageking.com/blog/chase-bliss-audio-mood/)). The underlying DSP is the
  classic Schroeder/Moorer allpass diffuser
  ([dsprelated.com](https://www.dsprelated.com/freebooks/pasp/Schroeder_Allpass_Sections.html)).
- **Reuses:** the one-pole allpass recurrence already implemented in-tree —
  `dsp::DriveBlendPhase`'s own allpass math (`Drive.hpp`, `phased = -a*wet + x1 + a*y1`) is the identical
  building block a diffuser needs; a fresh instance (or short chain) reused here rather than re-derived.
- **Cost:** composes-existing (allpass math reused), but is structurally new plumbing — a short allpass
  chain on the wet tap, not a single drop-in unit — so it sits closer to the composes/genuinely-new
  boundary than candidates 1-3.
- **Headroom:** none — allpass sections are unity-gain by construction, the same property this
  codebase's own `DriveBlendPhase` header comment already proves, *provided* the coefficient is kept
  strictly inside the unit circle (the same `0.98` margin `DriveBlendPhase` and `dsp::Comb` both already
  use) — this stage must copy that same margin, not invent a new one.
- **Why it earns a slot:** the most-requested "dub/tape" delay flavor beyond plain repeats across every
  modern delay plugin surveyed, and the exact allpass building block it needs already exists in this
  tree.

### 5. Ducking — `Duck` — rank 5 (flagged: weakest fit to the brief's "characterful over corrective" bias)
- **0.0** repeats always play at full level regardless of new input — today's behavior. **0.5** repeats
  duck moderately under a new incoming note, then recover. **1.0** repeats duck hard, nearly muting
  under sustained input — a very "studio-clean," utility-feeling extreme.
- **Precedent:** TC Electronic's delay ducking feature — "looks to see if there are other notes being
  played between the time a note is played and when the delayed note repeats, and if there are, it will
  reduce the volume of the repeats... by a preset amount (1%-100%), which allows for higher amounts of
  delay without becoming a trainwreck of notes" (via TDPRI forum discussion of TC Electronic pedals,
  corroborated by Boss/TC product literature). Real and common, but explicitly a corrective/clarity
  feature, not a gritty one.
- **Reuses:** the one-pole attack/release follower *idiom* already in `dsp::VcoEnvelopeFollowers`
  (`EnvelopeFollowers.hpp`) — not the struct itself (that one is hardwired to the three audio VCOs for
  modulation-source use), so a ducking envelope follower on the delay's dry input would be a new instance
  built on the same math, not a literal reuse of an existing object.
- **Cost:** composes-existing at the idiom level only; closer to genuinely-new as an object.
- **Headroom:** none — ducking only ever attenuates the wet signal.
- **Why it (barely) earns a slot:** real, continuous, and not a duplicate of the 15-source modulation
  matrix (it's a dedicated dynamics stage wired into the DSP chain, not a mod-matrix route) — but it
  is the most "corrective" item on either bank's list, working against the brief's explicit bias toward
  characterful controls, and its DSP reuse is the weakest of the five. Recommend treating this as the
  first cut if the operator wants a stronger fifth candidate elsewhere.

### 6. Reverse Blend — `Rev` — rank 6 (flagged: costliest, and precedent is a mode, not a knob)
- **0.0** normal forward repeats — today's behavior. **0.5** forward and reverse taps mixed — a swirly,
  smeared hybrid texture unavailable from either extreme alone. **1.0** fully reversed repeats, classic
  "backwards delay."
- **Precedent, with an honest caveat:** reverse delay is a well-established, real feature — Valhalla
  **Delay** ships dedicated `Pitch`/`RevPitch` **modes**
  ([valhalladsp.com](https://valhalladsp.com/shop/delay/valhalladelay/)) and reverse is a long-standing
  mode on hardware delays generally — but in essentially every shipped reference it is a discrete
  mode/toggle, not a continuous forward↔reverse crossfade knob. Proposing it as continuous here is a
  genuine design step beyond what any cited product does, not a documented precedent for the *control*
  itself (only for the *sound* at each end of the proposed sweep).
- **Reuses:** `dsp::StereoDelay`'s own delay-line buffers (`lineL`/`lineR`) and `ReadAt` — a second,
  backward-incrementing read pointer into the same already-allocated lines.
- **Cost:** genuinely-new — needs a second read index with its own wrap/boundary handling and a
  click-free crossfade between the two taps; not a parameter reuse of any existing struct's knob input.
- **Headroom:** none from the read itself (still bounded by the same buffer contents), but reversed-read
  crossfades are a known source of edge-of-buffer discontinuities/clicks, worth flagging as an
  implementation risk distinct from the headroom rule.
- **Why it earns a slot at all:** strongest "wow factor" of any Delay candidate and clearly in the
  dub/tape idiom, but ranked last on cost (genuinely-new, not reuse/composes) and on the honesty that the
  continuous-blend framing is this document's own extrapolation, not something any cited product ships.

---

## Cross-bank notes
- Both banks' rank-1/2 picks are the cheapest possible (one multiply or one filter instance dropped into
  an existing call site) and the best-precedented (each has 2+ independent shipped-product citations).
- Every Drive/Delay candidate that reaches into a feedback or gain-compounding path (Drive's Cascade;
  Delay's Feedback Drive) is explicitly flagged for the headroom rule (constraint 5) — no candidate here
  is claimed "safe" without a re-measurement in that recommendation.
- No candidate on either list proposes a new modulation route, per-parameter LFO, or cross-parameter
  coupling (constraint 2); Ducking (Delay #5) was checked most carefully against this because sidechain-
  style processing is adjacent to "route A to B," and is a dedicated DSP stage, not a mod-matrix entry.
