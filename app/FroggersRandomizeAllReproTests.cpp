// FroggersRandomizeAllReproTests.cpp -- headless repro for the
// operator-confirmed "audio peters out after Randomize All and never
// returns" bug. NOT part of the regular suite (not wired into app/Makefile's
// `test` target) -- this is a one-off investigation binary built directly
// with the same flags app/Makefile's AUDIO_ROUTING_BIN target uses:
//
//   clang++ -std=c++20 -Wall -Wextra -Wpedantic -O2 \
//     -IExternal/Sheaf/projects/synth/include -IExternal/Sheaf/projects/synth/tests \
//     app/FroggersRandomizeAllReproTests.cpp External/Sheaf/projects/synth/build/libsynth.a \
//     -o app/build/froggers_randomize_all_repro
//
// Repro steps, exactly as the operator described:
//   1. Start the transport the real way (SynthRig::StartAt pushes the same
//      synth::MessageIn::Start(...) through the uiBus that the real Play
//      button does).
//   2. Render ~1s of steady-state audio (default patch) first.
//   3. Fire Randomize All the real way: FroggersApp::RequestRandomizeAll()
//      is the exact method FroggersUiSurface::HandleAction calls for the
//      "Randomize All" button (app/FroggersUiSurface.hpp:656-658) -- no
//      uiBus message is involved for this action (only Start/Stop go
//      through uiBus); the request is a plain atomic flag consumed on the
//      very next audio-thread ProcessFrame() (app/FroggersAppCore.hpp:
//      344-346), so calling it directly on rig.Application() mirrors the UI
//      exactly.
//   4. Render 25 more seconds, recording per-second RMS/peak and watching
//      for SawNaN() / permanent silence.
//
// Note: FroggersApp::SanitizeOutputSample (app/FroggersAppCore.hpp) replaces
// any non-finite sample with 0.0f before it ever reaches the output buffer,
// so SawNaN() (which checks the OUTPUT samples) can never observe an
// internal NaN directly here -- it stays false even when the bug fires. The
// per-second RMS/peak series is therefore the signal to watch: a sustained
// drop to (and permanent stay at) 0.0 after a loud/clipped stretch is this
// bug's externally-observable signature.

#include "Froggers.hpp"
#include "FroggersModulation.hpp"
#include "FroggersParameters.hpp"
#include "support/SynthRig.hpp"

#ifdef JUCE_MAJOR_VERSION
#error "Froggers randomize-all repro test must not see JUCE headers"
#endif

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <vector>

using Rig = synth_rig::SynthRig<synth_froggers::FroggersApp>;

namespace {

synth::RuntimeDataPaths ScratchPaths() {
    const std::filesystem::path dataRoot =
        std::filesystem::temp_directory_path() / "froggers-randomize-all-repro";
    std::filesystem::remove_all(dataRoot);
    synth::RuntimeDataPaths paths = synth::RuntimeDataPaths::FromDataRoot(dataRoot);
    std::filesystem::create_directories(paths.patchesRoot);
    return paths;
}

}  // namespace

int main() {
    constexpr double kSampleRate = 48000.0;
    constexpr std::size_t kBlockSize = 512;
    Rig rig(/*patchPumpBudgetBlocks=*/64, ScratchPaths(),
            Rig::AudioSettings{kSampleRate, static_cast<int>(kBlockSize)});

    // Step 1: start the transport the real way (same MessageIn::Start the
    // Play button and SynthRig::StartAt both push through the uiBus).
    rig.StartAt(0);

    std::vector<double> perSecondRms;
    std::vector<float> perSecondPeak;

    auto runOneSecondAndRecord = [&](const char* label) {
        // 48000 / 512 = 93.75; round up so each bucket covers >= 1.0s.
        const std::size_t blocksThisSecond =
            static_cast<std::size_t>(std::ceil(kSampleRate / static_cast<double>(kBlockSize)));
        rig.ClearOutput();
        rig.RunBlocks(blocksThisSecond);

        double sumSquares = 0.0;
        float peak = 0.0f;
        std::size_t sampleCount = 0;
        for (const auto& frame : rig.Output()) {
            for (float s : frame.channels) {
                sumSquares += static_cast<double>(s) * static_cast<double>(s);
                peak = std::max(peak, std::fabs(s));
                ++sampleCount;
            }
        }
        const double rms = sampleCount > 0 ? std::sqrt(sumSquares / static_cast<double>(sampleCount)) : 0.0;
        perSecondRms.push_back(rms);
        perSecondPeak.push_back(peak);
        std::fprintf(stderr, "[%s] second %zu: rms=%.6f peak=%.6f sawNaN=%d\n", label, perSecondRms.size(), rms,
                     peak, rig.SawNaN());
    };

    // Step 2: ~1s of steady-state default-patch audio before touching
    // Randomize All.
    runOneSecondAndRecord("pre");

    // Step 3: fire Randomize All the real way.
    rig.Application().RequestRandomizeAll();

    // Step 4: render 25 more seconds and watch for the death.
    for (int i = 0; i < 25; ++i) {
        runOneSecondAndRecord("post");
    }

    std::printf("\n=== per-second RMS series (index 0 = pre-Randomize-All, 1..25 = post) ===\n");
    for (double v : perSecondRms) {
        std::printf("%.6f ", v);
    }
    std::printf("\n=== per-second peak series ===\n");
    for (float v : perSecondPeak) {
        std::printf("%.6f ", v);
    }
    std::printf("\nSawNaN() at end: %d\n", rig.SawNaN());

    return 0;
}
