#pragma once

// synth_froggers::dsp::{RGen, RandomShLane, lanes::MakeSource1..5} -- a
// **copy** of src/core/Marbles.hpp's bag/deja-vu core and
// src/core/RGen.hpp's xorshift32, generalized as described below. Ports
// sources 1-5 of six Random S&H sources total -- source #6 is a Sheaf
// GangedRandomLfo, not a Marbles instance.
//
// ============================================================================
// STRUCTURAL CHOICE: src/core/Marbles.hpp hardcodes exactly two
// channels via `m_marbles[2][8]` (:13), `m_filter[2]` (:15), `m_size[2]`,
// `m_index[2]`, `m_dejaVuKnob[2]`, `m_output[2]` (:16-20), one shared
// `RGen m_rgen` (:14), and one shared `float m_probability` (:19); every
// loop is `for (i = 0; i < 2; i++)` (:69,100,117). This port GENERALIZES
// the hardcoded 2 down to a single-lane struct (every `[2]` array becomes
// one plain scalar member, i.e. the per-channel width becomes 1) and gets
// five sources by constructing five independent RandomShLane instances --
// not by adding a runtime/template channel-count parameter. Reasoning: with
// the width forced to 1, "N instances" and "a struct templated on N" are the
// same amount of code, but N independent objects are far easier to seed,
// test, and reason about independently (each one is a complete, ownable
// unit with its own RGen, matching the "each carries a fixed character"
// framing used throughout this file), whereas an N-wide struct would still
// bury five lanes' state in parallel arrays the way Marbles.hpp does today
// -- exactly the shape this port is trying to get away from.
// ============================================================================

#include "DspMath.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace synth_froggers::dsp {

// ----------------------------------------------------------------------
// src/core/RGen.hpp, ported with ONE necessary structural change.
//
// DISCREPANCY FLAGGED: the firmware RGen's
// xorshift32 state is a **static** class member --
// `static uint32_t s_state;` (RGen.hpp:12), defined out-of-line as
// `inline uint32_t RGen::s_state = 0xa341316cu;` (RGen.hpp:66). It is NOT
// per-instance. Every `RGen` object anywhere in the firmware codebase (both
// of Marbles' channels, 08b5fd3:src/core/FroggersEngine.hpp:311, Parameter.hpp:197/207/223,
// AudioPairArState.hpp:118/128, and every ad-hoc `RGen()` temporary) reads
// and advances that ONE shared stream. Each RGen must be seeded distinctly,
// or the instances emit identical sequences and the sources become clones
// of each other -- but the firmware struct has no
// per-instance seed at all, so "seed distinctly" cannot be satisfied by a
// verbatim copy: five verbatim RGens would not merely correlate, they would
// all be cursors into one interleaved global sequence, each consuming the
// others' draws. This port makes the xorshift32 state an ordinary instance
// member (same recurrence, same constants) and takes a seed in the
// constructor, so distinct per-lane seeds actually produce independent,
// non-interleaved streams -- verified in FroggersDspParityTests.cpp.
struct RGen
{
    explicit RGen(uint32_t seed) : state_(seed != 0u ? seed : 0x6d2b79f5u) {}

    // RGen.hpp:14-27 (NextUInt), same xorshift32 recurrence, per-instance state.
    uint32_t NextUInt()
    {
        uint32_t x = state_;
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        state_ = x;
        return x;
    }

    // RGen.hpp:45-53 (UniGen/UniGenRange), verbatim formulas.
    float UniGen() { return static_cast<float>(NextUInt() >> 8) * (1.0f / 16777216.0f); }
    float UniGenRange(float min, float max) { return min + (max - min) * UniGen(); }

    // RGen.hpp:55-63 (RangeGen), verbatim formula.
    size_t RangeGen(size_t max)
    {
        if (max == 0)
        {
            return 0;
        }
        return static_cast<size_t>(NextUInt() % max);
    }

private:
    uint32_t state_;
};

// ----------------------------------------------------------------------
// One Random S&H lane: the generalized-to-width-1 Marbles bag/deja-vu core
// (src/core/Marbles.hpp:67-96 Increment, :115-121 Process), plus
// construction-time-only "character" (no m_page->GetParam() coupling of
// any kind -- no source-level controls).
//
// Character constants determine each lane's fixed character as follows:
//   - bagSize      -> the loop length (locked-loop sources use 8; the
//                      free-running sources' size is irrelevant to their
//                      output since they regenerate every step -- see
//                      dejaVuKnob below -- but must still be in [1,8]).
//   - dejaVuKnob    -> selects the firmware Increment()'s two regimes:
//                      0.5 exactly takes the "else" (generative) branch
//                      with a computed regen chance of 2*(0.5-0.5) = 0, so
//                      the index only ever steps through the bag -- i.e. a
//                      genuinely LOCKED loop of the construction-time
//                      random values (sources #1/#2/#3). 0.0 takes the
//                      same branch with regen chance 2*(0.5-0) = 1, so
//                      every step regenerates -- i.e. fully FREE-RUNNING
//                      (sources #4/#5).
//   - stepChance    -> was Marbles.hpp's single shared m_probability
//                      (:19), now a per-lane construction constant.
//                      No source currently uses a "sometimes skip the
//                      step" character (its differentiation is all in
//                      Loop/Range/slew), so all five are constructed at
//                      1.0 (always step) below -- the structural
//                      capability is per-lane even though no source
//                      currently uses a non-1.0 value.
//   - filterCutoff  -> the OPLowPassFilter slew (Marbles.hpp:115-121),
//                      per-lane instead of Marbles.hpp's shared-by-neither-
//                      channel-but-still-two m_filter[2].
//   - spread, bias  -> NEW DSP, NOT ported from Marbles.hpp (Marbles.hpp
//                      reads neither and implements no range narrowing or
//                      centring). Narrows/re-centers the
//                      output around 0.5; used only by source #3.
//   - quantizeLevels-> ALSO new DSP, not in Marbles.hpp, but required to
//                      realize source #4's explicit "~5 quantised levels"
//                      character (this is the weakest choice here, most
//                      likely to be overruled on hearing; status
//                      provisional until validated by ear). 0/1 disables
//                      quantization.
struct RandomShLane
{
    static constexpr size_t kNumSlots = 8;  // src/core/Marbles.hpp:12 (x_numMarbles)

    // The five X-style sources' visualizer draws "the
    // remembered loop as a waveform with a playhead at the current index" --
    // Sheaf has nothing for this (a bag/deja-vu loop has no equivalent in
    // DspRandomLfo), so this is new state, not a
    // ported shape. Atomics only (no synth:: dependency at all here, unlike
    // the UIState additions in Vco.hpp/FilterFx.hpp) -- the
    // actual synth::ui::Visualizer subclass that reads this lives in the
    // app tier (app/FroggersRandomShVisualizer.hpp), keeping this DSP file's
    // Sheaf-dependency surface at zero.
    struct UiState
    {
        std::atomic<std::size_t> currentIndex{0};
        std::atomic<std::size_t> size{kNumSlots};
        std::array<std::atomic<float>, kNumSlots> slots{};
    };

    RandomShLane(uint32_t seed,
                 size_t bagSize,
                 float dejaVuKnob,
                 float stepChance,
                 float filterCutoffCyclesPerSample,
                 float spread = 1.0f,
                 float bias = 0.0f,
                 int quantizeLevels = 0)
        : rgen_(seed)
        , size_(std::min(bagSize, kNumSlots) < 1 ? 1 : std::min(bagSize, kNumSlots))
        , dejaVuKnob_(dejaVuKnob)
        , probability_(stepChance)
        , spread_(spread)
        , bias_(bias)
        , quantizeLevels_(quantizeLevels)
    {
        // Marbles.hpp:98-113 (constructor loop): every slot starts as an
        // independent draw from this lane's own RGen.
        for (size_t i = 0; i < kNumSlots; ++i)
        {
            slots_[i] = rgen_.UniGenRange(0.0f, 1.0f);
        }
        filter_.SetAlphaFromNatFreq(filterCutoffCyclesPerSample);
    }

    // Marbles.hpp:67-96 (Increment), generalized from the two-channel
    // `for (i < 2)` loop to this lane's own scalars. Deja-vu branch at
    // Marbles.hpp:76 (`if (0.5 < m_dejaVuKnob[i])`).
    void Increment()
    {
        if (probability_ < rgen_.UniGen())
        {
            return;
        }

        if (0.5f < dejaVuKnob_)
        {
            if (rgen_.UniGen() < 2.0f * (dejaVuKnob_ - 0.5f))
            {
                index_ = rgen_.RangeGen(size_);
            }
            else
            {
                index_ = (index_ + 1) % size_;
            }
        }
        else
        {
            index_ = (index_ + 1) % size_;
            if (rgen_.UniGen() < 2.0f * (0.5f - dejaVuKnob_))
            {
                slots_[index_] = rgen_.UniGenRange(0.0f, 1.0f);
            }
        }
    }

    // Marbles.hpp:115-121 (Process): read the current slot through the
    // slew filter. quantize/spread/bias (new DSP, see struct comment) are
    // applied to the raw slot value before filtering, in that order, so a
    // locked loop's construction-time-only values are also quantized /
    // narrowed -- not just values that happen to regenerate later.
    float Process()
    {
        float raw = slots_[index_];
        if (quantizeLevels_ > 1)
        {
            const float steps = static_cast<float>(quantizeLevels_ - 1);
            raw = std::round(raw * steps) / steps;
        }
        const float narrowed = std::clamp(0.5f + (raw - 0.5f) * spread_ + bias_, 0.0f, 1.0f);
        return filter_.Process(narrowed);
    }

    // Publishes the
    // lane's current index and its full bag of held values (raw slot
    // contents, i.e. pre quantize/spread/bias/filter -- the same convention
    // Sheaf's own PopulateUIState methods use, publishing the state a
    // visualizer needs rather than a processed sample).
    void PopulateUiState(UiState& state) const
    {
        state.currentIndex.store(index_, std::memory_order_relaxed);
        state.size.store(size_, std::memory_order_relaxed);
        for (size_t i = 0; i < kNumSlots; ++i)
        {
            state.slots[i].store(slots_[i], std::memory_order_relaxed);
        }
    }

private:
    RGen rgen_;
    size_t size_;
    size_t index_ = 0;
    float dejaVuKnob_;
    float probability_;
    float spread_;
    float bias_;
    int quantizeLevels_;
    float slots_[kNumSlots];
    OnePoleLowPass filter_;
};

// ----------------------------------------------------------------------
// Five fixed characters, one per Random S&H source (rate/loop wiring
// happens elsewhere -- NOT here; these factories fix only the
// bag/deja-vu/slew/spread/quantize constants).
namespace lanes {

// Locked-loop sources (#1/#2/#3) and the stepped-jump source (#4) want no
// audible glide between held values; the free-running "slow deliberate
// moves" source (#5) wants an explicit slew. Both cutoffs are implementer
// defaults, still to be validated by ear ("validate the
// table by ear"), the same convention FroggersEngine.hpp uses for
// x_pmLfoDepth ("implementer default... flagged for operator tuning").
inline constexpr float kFastCutoff = 0.45f;   // cycles/sample -- near-instant
inline constexpr float kSlowCutoff = 0.002f;  // cycles/sample -- audible glide

// #1: quarter-note rate, loop 8 => 2-bar period, full range,
// locked loop ("long phrase, quarter pulse").
inline RandomShLane MakeSource1(uint32_t seed)
{
    return RandomShLane(seed, /*bagSize=*/8, /*dejaVuKnob=*/0.5f, /*stepChance=*/1.0f, kFastCutoff);
}

// #2: eighth-note rate, loop 8 => 1-bar period, full range,
// locked loop ("eighth pulse, one-bar phrase").
inline RandomShLane MakeSource2(uint32_t seed)
{
    return RandomShLane(seed, 8, 0.5f, 1.0f, kFastCutoff);
}

// #3: eighth-triplet rate, loop 8 => 2/3-bar period, NARROW
// CENTRED range, locked loop ("polyrhythmic triplet shimmer"). spread=0.3
// is an implementer default (loop/rate are confident; the exact spread
// value is not) -- still to be validated by ear alongside the cutoffs
// above.
inline RandomShLane MakeSource3(uint32_t seed)
{
    return RandomShLane(seed, 8, 0.5f, 1.0f, kFastCutoff, /*spread=*/0.3f, /*bias=*/0.0f);
}

// #4: quarter-note rate, free-running, full range, ~5
// quantised levels ("unpredictable stepped jumps") -- the least-confident
// choice among these five, most likely to be overruled on hearing.
inline RandomShLane MakeSource4(uint32_t seed)
{
    return RandomShLane(seed, 8, /*dejaVuKnob=*/0.0f, 1.0f, kFastCutoff, 1.0f, 0.0f, /*quantizeLevels=*/5);
}

// #5: 1-bar rate, free-running, full range, slewed ("slow
// deliberate moves").
inline RandomShLane MakeSource5(uint32_t seed)
{
    return RandomShLane(seed, 8, 0.0f, 1.0f, kSlowCutoff);
}

}  // namespace lanes
}  // namespace synth_froggers::dsp
