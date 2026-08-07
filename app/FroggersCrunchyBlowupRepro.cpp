// FroggersCrunchyBlowupRepro.cpp -- headless repro for the
// operator-confirmed "audio still blows out and gets permanently killed" bug
// that survives the scoopNotch fix (FroggersAppCore.hpp:613-638). NOT part of
// the regular suite (not wired into app/Makefile's `test` target) -- a
// one-off investigation binary, built with the same flags app/Makefile's
// AUDIO_ROUTING_BIN target uses:
//
//   clang++ -std=c++20 -Wall -Wextra -Wpedantic -O2 \
//     -IExternal/Sheaf/projects/synth/include -IExternal/Sheaf/projects/synth/tests \
//     app/FroggersCrunchyBlowupRepro.cpp External/Sheaf/projects/synth/build/libsynth.a \
//     -o app/build/froggers_crunchy_blowup_repro
//
// Repro steps, per the investigation brief:
//   1. Start the transport the real way (SynthRig::StartAt pushes the same
//      synth::MessageIn::Start(...) through the uiBus the real Play button
//      does -- identical to FroggersRandomizeAllRepro.cpp).
//   2. Sweep the global Crunchy control (bank slot 15, one shared
//      synth::Parameter across all six banks -- FroggersParameters.hpp:80)
//      across its full 0..1 range via synth::MessageIn::ParamSetAbsolute
//      pushed onto the same uiBus a real absolute-position hardware encoder
//      would use (ParameterManager::HandleSetAbsolute,
//      ParameterModulation.cpp:3478-3486) -- NOT by reaching into
//      FroggersParameterModel directly, matching FroggersSurfaceTests.cpp's
//      own "both end-to-end through the real message path" discipline.
//   3. Hold at maximum (1.0) for several seconds.
//   4. Repeat independently for Crispy (per-bank; this run exercises the
//      Filter bank's Crispy at slot 14, selected via the same SelectBank
//      message a real bank-select button uses).
//   5. In lock-step with the real engine, run a SHADOW copy of
//      FroggersAppCore::RouteAudioSample's exact DSP chain (VCOs -> envelope
//      mix -> Drive -> Filter chain -> Delay -> Reverb, formulas copied
//      verbatim from FroggersAppCore.hpp:545-681, own dsp:: unit instances,
//      zero-initialized identically to the real engine's members) fed the
//      SAME per-sample CachedKnobValue() reads the real RouteAudioSample
//      consumes. Both paths are deterministic scalar float recursions given
//      identical inputs from identical (all-zero) initial state, so the
//      shadow diverges to non-finite at the exact sample the real internal
//      chain does -- this is how the instrumentation below identifies WHICH
//      stage goes non-finite and WHEN without modifying any production file
//      (this is a read-only investigation; no fix is implemented here).
//   6. blockSize=1 throughout, so every RunBlocks(1) call corresponds to
//      exactly one real audio sample (FroggersAppCore::ProcessBlock's
//      per-frame loop, FroggersAppCore.hpp:381-441) -- the shadow tick reads
//      CachedKnobValue() immediately after that call, so it sees exactly the
//      post-fuego/post-modulation values RouteAudioSample used for that same
//      sample.
//
// Note (same caveat FroggersRandomizeAllRepro.cpp documents):
// SanitizeOutputSample (FroggersAppCore.hpp:698-707) replaces any non-finite
// OUTPUT sample with 0.0f before it reaches the stereo bus, so rig.SawNaN()
// (which checks captured OUTPUT samples) can never observe an internal NaN
// directly -- it stays false even when the bug fires. The shadow chain's own
// isfinite() checks are therefore the only direct evidence of an internal
// blowup; the per-second RMS/peak series is the externally-observable
// corroboration (a sustained drop to permanent 0.0 after a loud stretch).

#include "Froggers.hpp"
#include "FroggersModulation.hpp"
#include "FroggersParameters.hpp"
#include "support/SynthRig.hpp"

#ifdef JUCE_MAJOR_VERSION
#error "Froggers crunchy blowup repro test must not see JUCE headers"
#endif

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

using Rig = synth_rig::SynthRig<synth_froggers::FroggersApp>;

namespace {

synth::RuntimeDataPaths ScratchPaths(const char* label) {
    const std::filesystem::path dataRoot =
        std::filesystem::temp_directory_path() / (std::string("froggers-crunchy-blowup-repro-") + label);
    std::filesystem::remove_all(dataRoot);
    synth::RuntimeDataPaths paths = synth::RuntimeDataPaths::FromDataRoot(dataRoot);
    std::filesystem::create_directories(paths.patchesRoot);
    return paths;
}

// Pushes the same synth::MessageIn::ParamSetAbsolute a real absolute-position
// hardware encoder would send, at timestamp 0 -- guaranteed to be eligible
// for the very next RunBlocks(1) call regardless of how far the rig's own
// internal timestamp counter has already advanced (MessageInBus::Pop only
// requires message.timestamp <= the block's processing timestamp, and 0 is
// <= everything -- the same trick rig.StartAt(0) itself relies on).
void PushParamAbsolute(Rig& rig, std::size_t position, float value) {
    rig.Engine().UiBus().Push(synth::MessageIn::ParamSetAbsolute(/*timestamp=*/0, /*slotIx=*/0, position, value));
}

// -- Shadow DSP chain: a byte-for-byte copy of FroggersAppCore::
// RouteAudioSample's stage order and formulas (FroggersAppCore.hpp:545-681),
// with isfinite() checks inserted between stages. Own dsp:: unit instances
// (never touches the real engine's filterChain_/delay_/reverb_/drive_
// members), so this is read-only instrumentation, not a modification of any
// production file.
struct ShadowChain {
    float sampleRate = 48000.0f;
    synth_froggers::dsp::Vco vco1, vco2, vco3;
    synth_froggers::dsp::VcoAdsrState adsr;
    synth_froggers::dsp::FrogBlock drive;
    synth_froggers::dsp::DriveBlendPhase driveBlendPhase;
    synth_froggers::dsp::FilterFxChain filterChain;
    synth_froggers::dsp::StereoDelay delay;
    synth_froggers::dsp::Reverb reverb;

    void Prepare(float sr) {
        sampleRate = sr;
        delay.SetSampleRate(sr);
        // B5 (openspec/changes/archive/2026-08-06-frogg3rs-modulation-truth-and-voicing/
        // tasks.md): mirrors FroggersAppCore::PrepareToPlay's new
        // `filterChain_.Configure(sampleRate_)` call -- this struct's own
        // header comment promises a byte-for-byte copy of
        // RouteAudioSample's stage order/formulas, so the peak branch's own
        // limiter (FilterFxChain::peakLimiter) needs the same real-sample-
        // rate configuration production gives it, not the constructor's
        // assumed-48kHz default (FilterFx.hpp).
        filterChain.Configure(sr);
        // B6a/B6b (tasks.md CONSOLIDATED PUSH table): same reasoning,
        // mirroring FroggersAppCore::PrepareToPlay's new
        // `delay_.SetSampleRate(sampleRate_)` (already called two lines
        // above)/`reverb_.Configure(sampleRate_)` calls -- Delay's own
        // wetLimiterL/R are already configured by the `delay.SetSampleRate`
        // call above (dsp/Delay.hpp folds that in); Reverb's wetLimiter
        // needs this explicit call the same way filterChain's peakLimiter
        // does, since Reverb has no other sample-rate entry point of its
        // own (dsp/Reverb.hpp).
        reverb.Configure(sr);
    }

    // Same threshold SanitizeOutputSample clamps the real output to
    // (FroggersAppCore.hpp:705, kMaxOutputMagnitude) -- used here to flag a
    // "large but still finite" excursion, distinct from an outright
    // non-finite one, so a stage that merely gets loud (and would be
    // audibly clipped/blown-out at the real output) is distinguishable from
    // one that has actually gone non-finite (unrecoverable per PART 2).
    static constexpr float kLargeMagnitude = 8.0f;

    struct StageEvent {
        const char* stage = nullptr;
        bool nonFinite = false;
        float value = 0.0f;
    };

    // Returns the first stage this sample that either went non-finite or
    // exceeded kLargeMagnitude, or a default-constructed (stage == nullptr)
    // StageEvent if every stage stayed finite and small.
    StageEvent Tick(synth_froggers::FroggersParameterModel& params) {
        namespace dsp = synth_froggers::dsp;
        using synth_froggers::FroggersBankId;
        auto knob = [&](FroggersBankId bank, std::size_t ix) -> float {
            return params.PageParameter(bank, ix).CachedKnobValue(0);
        };
        auto check = [](const char* name, float value) -> StageEvent {
            if (!std::isfinite(value)) {
                return StageEvent{name, true, value};
            }
            if (std::fabs(value) > kLargeMagnitude) {
                return StageEvent{name, false, value};
            }
            return StageEvent{};
        };

        // -- Audio bank -> 3x dsp::Vco --------------------------------------
        const float v1 = vco1.Process(knob(FroggersBankId::Audio, 0), knob(FroggersBankId::Audio, 3),
                                       knob(FroggersBankId::Audio, 6), sampleRate);
        const float v2 = vco2.Process(knob(FroggersBankId::Audio, 1), knob(FroggersBankId::Audio, 4),
                                       knob(FroggersBankId::Audio, 7), sampleRate);
        const float v3 = vco3.Process(knob(FroggersBankId::Audio, 2), knob(FroggersBankId::Audio, 5),
                                       knob(FroggersBankId::Audio, 8), sampleRate);
        if (StageEvent e = check("VCO", v1); e.stage) return e;
        if (StageEvent e = check("VCO", v2); e.stage) return e;
        if (StageEvent e = check("VCO", v3); e.stage) return e;

        // -- Envelope bank -> ASR + voice mix --------------------------------
        const float chainIn = dsp::MixOscVoices(
            adsr, v1, v2, v3, knob(FroggersBankId::Envelope, 0), knob(FroggersBankId::Envelope, 1),
            knob(FroggersBankId::Envelope, 2), knob(FroggersBankId::Envelope, 3), knob(FroggersBankId::Envelope, 4),
            knob(FroggersBankId::Envelope, 5), knob(FroggersBankId::Envelope, 6), knob(FroggersBankId::Envelope, 7),
            knob(FroggersBankId::Envelope, 8));
        if (StageEvent e = check("Envelope/Mix", chainIn); e.stage) return e;

        // -- Drive bank -> dsp::FrogBlock + DriveBlendPhase ------------------
        drive.polynomialDrive.SetGain(knob(FroggersBankId::Drive, 0));
        drive.polynomialDrive.SetCoefs(knob(FroggersBankId::Drive, 1));
        drive.sampleRateReducer1.SetFreq(1e-2f + dsp::ZeroedExpCompute(10.0f, 1.0f - knob(FroggersBankId::Drive, 2)));
        drive.sampleRateReducer2.SetFreq(1e-2f + dsp::ZeroedExpCompute(10.0f, 1.0f - knob(FroggersBankId::Drive, 3)));
        drive.digitalReorganizer.SetFlip(knob(FroggersBankId::Drive, 4));
        drive.digitalReorganizer.SetHash(knob(FroggersBankId::Drive, 5));
        drive.fuzz = knob(FroggersBankId::Drive, 6);
        const float driveWet = drive.Process(chainIn);
        const float driveOut =
            driveBlendPhase.Process(chainIn, driveWet, knob(FroggersBankId::Drive, 7), knob(FroggersBankId::Drive, 8));
        if (StageEvent e = check("Drive", driveOut); e.stage) return e;

        // -- Filter bank -> dsp::FilterFxChain -------------------------------
        const float combOffsetSeconds =
            std::min(dsp::ExpMapCompute(0.001f, 0.1f, knob(FroggersBankId::Filter, 0)),
                      static_cast<float>(dsp::PureDelay::kSize - 1) / sampleRate);
        filterChain.pureDelay.SetDelaySeconds(combOffsetSeconds, sampleRate);
        const float bumpFreq =
            dsp::ExpMapCompute(20.0f / sampleRate, 20000.0f / sampleRate, knob(FroggersBankId::Filter, 1));
        const float bumpWidth = dsp::ExpMapCompute(0.1f, 10.0f, knob(FroggersBankId::Filter, 3));
        filterChain.peak.SetFreq(bumpFreq);
        filterChain.peak.SetHeight(
            dsp::ExpMapCompute(1.0f, dsp::kMaxResonantBumpHeight, knob(FroggersBankId::Filter, 2)));
        filterChain.peak.SetWidth(bumpWidth);
        filterChain.scoopNotch.SetFreq(bumpFreq);
        filterChain.scoopNotch.SetWidth(bumpWidth);
        filterChain.scoopNotch.SetHeight(std::max(0.05f, 1.0f - 0.95f * knob(FroggersBankId::Filter, 8)));
        const float combFreq =
            dsp::ExpMapCompute(20.0f / sampleRate, 10000.0f / sampleRate, knob(FroggersBankId::Filter, 4));
        filterChain.comb.delaySamples = std::min<std::size_t>(
            dsp::Comb::kSize - 1, std::max<std::size_t>(1, static_cast<std::size_t>(dsp::Comb::GetDelaySamples(combFreq))));
        filterChain.comb.SetFeedback(dsp::Comb::GetFeedback(knob(FroggersBankId::Filter, 5)));
        const float cmlp =
            dsp::ExpMapCompute(4.0f * combFreq, 20000.0f / sampleRate, knob(FroggersBankId::Filter, 6));
        filterChain.comb.SetCutoffAlpha(1.0f - std::exp(-2.0f * static_cast<float>(M_PI) * cmlp));
        const float filterOut = filterChain.Process(driveOut, /*useParallel=*/true, knob(FroggersBankId::Filter, 7),
                                                      knob(FroggersBankId::Filter, 8));
        if (StageEvent e = check("Filter", filterOut); e.stage) return e;

        // -- Delay bank -> dsp::StereoDelay -----------------------------------
        const dsp::DelayParams delayParams = dsp::MapRowsToDelayParams(
            knob(FroggersBankId::Delay, 0), knob(FroggersBankId::Delay, 1), knob(FroggersBankId::Delay, 2),
            knob(FroggersBankId::Delay, 3), knob(FroggersBankId::Delay, 4), knob(FroggersBankId::Delay, 5),
            knob(FroggersBankId::Delay, 6), knob(FroggersBankId::Delay, 7), knob(FroggersBankId::Delay, 8));
        const dsp::DelayWetPair delayWet = delay.Process(filterOut, delayParams);
        const float delayOut = delay.ToReverbMono(filterOut, delayWet, delayParams.dmix);
        if (StageEvent e = check("Delay", delayOut); e.stage) return e;

        // -- Reverb bank -> dsp::Reverb ---------------------------------------
        const float reverbOut = reverb.Process(
            delayOut, knob(FroggersBankId::Reverb, 0), knob(FroggersBankId::Reverb, 1),
            knob(FroggersBankId::Reverb, 2), knob(FroggersBankId::Reverb, 3), knob(FroggersBankId::Reverb, 4),
            knob(FroggersBankId::Reverb, 5), knob(FroggersBankId::Reverb, 6), sampleRate,
            knob(FroggersBankId::Reverb, 7), knob(FroggersBankId::Reverb, 8));
        if (StageEvent e = check("Reverb", reverbOut); e.stage) return e;

        return StageEvent{};
    }
};

struct SweepResult {
    bool foundBadStage = false;
    bool badStageWasNonFinite = false;
    std::string badStage;
    float badValue = 0.0f;
    double badTimeSeconds = 0.0;
    bool sawOutputNaN = false;
};

// Runs StartAt + a hold-at-each-step sweep of `position` (Crunchy or a bank's
// Crispy) from 0.0 up through 1.0, then several extra seconds held at 1.0,
// printing per-second RMS/peak and the first shadow-detected non-finite
// stage (if any). `label` names the run in stderr output.
SweepResult RunSweepAndReport(const char* label, std::size_t position, const char* scratchTag,
                              std::optional<synth_froggers::FroggersBankId> selectBank = std::nullopt,
                              double extraHoldSeconds = 4.0, bool randomizeAllFirst = false) {
    constexpr double kSampleRate = 48000.0;
    Rig rig(/*patchPumpBudgetBlocks=*/64, ScratchPaths(scratchTag),
            Rig::AudioSettings{kSampleRate, /*blockSize=*/1});
    rig.StartAt(0);
    if (selectBank.has_value()) {
        // Crispy is a distinct synth::Parameter PER BANK (unlike Crunchy,
        // the one shared object at slot 15 in every bank) -- position 14
        // only resolves to the intended bank's Crispy once that bank is
        // actually selected for slot 0, via the same generic
        // MessageIn::SelectParamBank a real bank-select button uses
        // (ParameterManager::SelectBankForSlot, independent of
        // FroggersAppCore's own display-only activeBankIx_ bookkeeping).
        rig.SelectBank(0, static_cast<std::size_t>(*selectBank));
        rig.RunBlocks(1);
    }

    ShadowChain shadow;
    shadow.Prepare(static_cast<float>(kSampleRate));
    synth_froggers::FroggersParameterModel& params = rig.Application().Parameters();

    SweepResult result;
    double sumSquares = 0.0;
    float peak = 0.0f;
    std::size_t samplesThisSecond = 0;
    std::size_t secondIx = 0;
    std::uint64_t totalSamples = 0;

    std::size_t channelSamplesThisSecond = 0;
    const auto stepOneSample = [&]() {
        rig.RunBlocks(1);
        const Rig::OutputFrame frame = rig.LastOutput();
        rig.ClearOutput();
        for (float s : frame.channels) {
            sumSquares += static_cast<double>(s) * static_cast<double>(s);
            peak = std::max(peak, std::fabs(s));
            ++channelSamplesThisSecond;
        }
        ++samplesThisSecond;
        ++totalSamples;

        if (!result.foundBadStage) {
            if (const ShadowChain::StageEvent event = shadow.Tick(params); event.stage != nullptr) {
                result.foundBadStage = true;
                result.badStageWasNonFinite = event.nonFinite;
                result.badStage = event.stage;
                result.badValue = event.value;
                result.badTimeSeconds = static_cast<double>(totalSamples) / kSampleRate;
            }
        } else {
            // Keep advancing the shadow's own recursive state so later
            // checks upstream of the first bad stage stay meaningful, but
            // we've already recorded the first failure.
            shadow.Tick(params);
        }

        if (samplesThisSecond >= static_cast<std::size_t>(kSampleRate)) {
            const double rms = std::sqrt(sumSquares / static_cast<double>(channelSamplesThisSecond));
            std::fprintf(stderr, "[%s] second %zu: rms=%.6f peak=%.6f sawNaN=%d shadowBad=%s(%s)\n", label,
                         ++secondIx, rms, peak, rig.SawNaN(),
                         result.foundBadStage ? result.badStage.c_str() : "none",
                         result.foundBadStage ? (result.badStageWasNonFinite ? "non-finite" : "large") : "-");
            sumSquares = 0.0;
            peak = 0.0f;
            samplesThisSecond = 0;
            channelSamplesThisSecond = 0;
        }
    };

    // 1s baseline at position == 0.0 (default patch, control unaffected).
    PushParamAbsolute(rig, position, 0.0f);
    for (std::size_t i = 0; i < static_cast<std::size_t>(kSampleRate); ++i) {
        stepOneSample();
    }

    // Optional compound scenario: fire Randomize All (the already-fixed
    // scoopNotch bug's own trigger, FroggersRandomizeAllRepro.cpp) BEFORE
    // sweeping this control, matching the operator's actual usage sequence
    // (Randomize All in an earlier session, Crunchy cranked in this one) --
    // not just a fresh default patch.
    if (randomizeAllFirst) {
        rig.Application().RequestRandomizeAll();
        for (std::size_t i = 0; i < static_cast<std::size_t>(kSampleRate); ++i) {
            stepOneSample();
        }
    }

    // Sweep 0.0 -> 1.0 in six steps, holding each for 1 second.
    const float steps[] = {0.0f, 0.2f, 0.4f, 0.6f, 0.8f, 1.0f};
    for (float target : steps) {
        PushParamAbsolute(rig, position, target);
        for (std::size_t i = 0; i < static_cast<std::size_t>(kSampleRate); ++i) {
            stepOneSample();
        }
    }

    // Hold at maximum for `extraHoldSeconds` additional seconds (so the
    // sweep's own last step plus this hold covers a genuinely long dwell at
    // the extreme, in case the failure mode is cumulative -- e.g. a
    // switched/time-varying-coefficient instability from Fuegoize's
    // every-sample chaotic scramble driving the resonant biquads' freq/
    // width/height, rather than an instantaneous blow-up).
    PushParamAbsolute(rig, position, 1.0f);
    for (std::size_t i = 0; i < static_cast<std::size_t>(extraHoldSeconds * kSampleRate); ++i) {
        stepOneSample();
    }

    result.sawOutputNaN = rig.SawNaN();
    return result;
}

// PART 2 item 4 support: a short run demonstrating a LEGITIMATE all-zero
// output (every VCO's ASR sustain target forced to 0, so dsp::VcoAdsrState
// holds every voice's level at 0 by design -- VoiceEnvelope.hpp's
// stepVoice()'s Attack/Hold branches ramp toward/hold at `sustainLevel`,
// which is exactly 0 here) -- contrasted against the blowup case, which
// shows RMS > 0 for a stretch BEFORE collapsing to permanent 0.
void RunLegitimateSilenceCheck() {
    constexpr double kSampleRate = 48000.0;
    Rig rig(/*patchPumpBudgetBlocks=*/64, ScratchPaths("legitimate-silence"),
            Rig::AudioSettings{kSampleRate, /*blockSize=*/1});
    rig.StartAt(0);

    // Envelope bank slots: (Attack, Sustain, Release) x (VCO1, VCO2, VCO3) --
    // Sustain is slots 1, 4, 7 (FroggersParameters.hpp's documented layout).
    // Position addressing is bank-relative to whatever bank is currently
    // selected for slot 0 (default: Audio, index 0) -- must select the
    // Envelope bank first via the same generic MessageIn::SelectParamBank a
    // real bank-select button uses, or these absolute-sets land on Audio's
    // own slots 1/4/7 instead (a bug caught by this run's own nonzero RMS:
    // an earlier draft of this check omitted the SelectBank call and
    // observed rms=0.78/peak=1.0 instead of the expected silence).
    rig.SelectBank(0, static_cast<std::size_t>(synth_froggers::FroggersBankId::Envelope));
    rig.RunBlocks(1);
    PushParamAbsolute(rig, 1, 0.0f);
    PushParamAbsolute(rig, 4, 0.0f);
    PushParamAbsolute(rig, 7, 0.0f);

    double sumSquares = 0.0;
    float peak = 0.0f;
    std::size_t sampleCount = 0;
    const std::size_t totalSamples = static_cast<std::size_t>(2.0 * kSampleRate);
    for (std::size_t i = 0; i < totalSamples; ++i) {
        rig.RunBlocks(1);
        const Rig::OutputFrame frame = rig.LastOutput();
        rig.ClearOutput();
        for (float s : frame.channels) {
            sumSquares += static_cast<double>(s) * static_cast<double>(s);
            peak = std::max(peak, std::fabs(s));
            ++sampleCount;
        }
    }
    const double rms = std::sqrt(sumSquares / static_cast<double>(sampleCount));
    std::fprintf(stderr,
                 "[legitimate-silence] 2s with all-VCO Sustain=0: rms=%.6f peak=%.6f sawNaN=%d "
                 "(expected: rms/peak == 0.0 from sample 0, no preceding loud stretch -- a "
                 "parameter-legitimate silence, not a blowup)\n",
                 rms, peak, rig.SawNaN());
}

void ReportSweep(const char* name, const SweepResult& result) {
    if (result.foundBadStage && result.badStageWasNonFinite) {
        std::printf("%s: FIRST non-finite stage = %s at t=%.4fs, value=%g (sawNaN in real output=%d)\n", name,
                     result.badStage.c_str(), result.badTimeSeconds, static_cast<double>(result.badValue),
                     result.sawOutputNaN);
    } else if (result.foundBadStage) {
        std::printf(
            "%s: no stage went non-finite, but FIRST stage to exceed the %.1f output-clamp magnitude = %s at "
            "t=%.4fs, value=%g (sawNaN in real output=%d) -- a large-but-finite excursion, not a NaN latch\n",
            name, static_cast<double>(ShadowChain::kLargeMagnitude), result.badStage.c_str(), result.badTimeSeconds,
            static_cast<double>(result.badValue), result.sawOutputNaN);
    } else {
        std::printf("%s: did not reproduce (every shadow stage stayed finite and within +/-%.1f)\n", name,
                     static_cast<double>(ShadowChain::kLargeMagnitude));
    }
}

}  // namespace

int main() {
    std::printf("=== Crunchy sweep (global, all six banks), 10s hold at max ===\n");
    const SweepResult crunchyResult = RunSweepAndReport(
        "crunchy", synth_froggers::kFroggersCrunchySlot, "crunchy", std::nullopt, /*extraHoldSeconds=*/10.0);
    ReportSweep("Crunchy sweep", crunchyResult);

    std::printf("\n=== Crispy sweep (Filter bank only), 10s hold at max ===\n");
    const SweepResult crispyResult =
        RunSweepAndReport("crispy-filter", synth_froggers::kFroggersCrispySlot, "crispy-filter",
                           synth_froggers::FroggersBankId::Filter, /*extraHoldSeconds=*/10.0);
    ReportSweep("Crispy sweep", crispyResult);

    std::printf("\n=== Compound: Randomize All, then Crunchy sweep to max, 60s hold ===\n");
    const SweepResult compoundResult =
        RunSweepAndReport("randomize-then-crunchy", synth_froggers::kFroggersCrunchySlot, "randomize-then-crunchy",
                           std::nullopt, /*extraHoldSeconds=*/60.0, /*randomizeAllFirst=*/true);
    ReportSweep("Randomize-All-then-Crunchy sweep", compoundResult);

    std::printf("\n=== Legitimate-silence control ===\n");
    RunLegitimateSilenceCheck();

    return 0;
}
