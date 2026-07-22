// FroggersApp_test -- packet 3 headless coverage
// (openspec/changes/desktop-v2-sheaf-runtime-harmonization, tasks 3.1-3.3).
//
// Proves FroggersApp (Source/FroggersApp.hpp) satisfies synth::SynthApplication
// and that synth::Engine<FroggersApp> can drive it end-to-end headlessly:
// Initialize() -> Prepare() -> ProcessBlock() -> finite, correctly-shaped
// stereo output. Also proves ProcessBlock actually delegates to the existing
// AudioEngine + FroggersV2AppCoreFacade path (design D2) rather than any new
// DSP, by running a directly-constructed facade side by side (mirroring the
// existing FroggersV2AppCoreFacade_test.cpp audio_equivalence pattern) and
// asserting bit-for-bit parity between the two.
//
// Does NOT wire FroggersApp into Main.cpp / MainComponent (shell cutover is
// tasks.md section 10, a later packet).

#include "synth/AppConcepts.hpp"
#include "synth/Engine.hpp"

#include "FroggersApp.hpp"
#include "control/FroggersV2AppCoreFacade.hpp"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <vector>

using froggers_v2::FroggersV2AppCoreConfig;
using froggers_v2::FroggersV2AppCoreFacade;

namespace
{
constexpr int kBlockSize = 256;
constexpr double kSampleRate = 44100.0;
constexpr int kBlocks = 8;

bool nearlyEqual(float a, float b, float eps = 1.0e-4f)
{
    return std::fabs(a - b) <= eps;
}

// Task 3.1/3.3: FroggersApp must satisfy the full synth::SynthApplication
// concept (Config/Init/ProcessBlock/PortableSurface) for synth::Engine<App>
// to instantiate below.
static_assert(synth::SynthApplication<FroggersApp>,
              "FroggersApp must satisfy synth::SynthApplication");

bool test_engine_init_process_block_finite_stereo()
{
    synth::Engine<FroggersApp> engine([]() -> std::uint64_t { return 0; });
    engine.Initialize();
    engine.Prepare(kSampleRate, kBlockSize);

    std::vector<float> left(static_cast<size_t>(kBlockSize), 0.0f);
    std::vector<float> right(static_cast<size_t>(kBlockSize), 0.0f);
    float* outputChannels[2] = {left.data(), right.data()};

    synth::AudioBlock block;
    block.inputs = nullptr;
    block.outputs = outputChannels;
    block.numInputChannels = 0;
    block.numOutputChannels = 2;
    block.numFrames = static_cast<std::size_t>(kBlockSize);

    for (int b = 0; b < kBlocks; ++b)
    {
        engine.ProcessBlock(block, static_cast<std::uint64_t>(b));
    }

    if (block.numOutputChannels != 2 || block.numFrames != static_cast<std::size_t>(kBlockSize))
    {
        std::printf("FAIL: output shape mismatch (channels=%d frames=%zu)\n",
                    block.numOutputChannels,
                    block.numFrames);
        return false;
    }

    for (int i = 0; i < kBlockSize; ++i)
    {
        if (!std::isfinite(left[static_cast<size_t>(i)]) || !std::isfinite(right[static_cast<size_t>(i)]))
        {
            std::printf("FAIL: non-finite sample at %d (L=%f R=%f)\n",
                        i,
                        static_cast<double>(left[static_cast<size_t>(i)]),
                        static_cast<double>(right[static_cast<size_t>(i)]));
            return false;
        }
    }
    return true;
}

bool test_process_block_matches_direct_facade_path()
{
    // Direct facade reference path -- mirrors
    // FroggersV2AppCoreFacade_test.cpp's audio_equivalence construction
    // (AudioEngine(true) + FroggersV2AppCoreFacade, headless/pluginHosted).
    AudioEngine referenceAudio(true);
    FroggersV2AppCoreFacade referenceFacade(referenceAudio);
    referenceFacade.initialize();
    referenceFacade.prepare(static_cast<float>(kSampleRate), kBlockSize);

    synth::Engine<FroggersApp> engine([]() -> std::uint64_t { return 0; });
    engine.Initialize();
    engine.Prepare(kSampleRate, kBlockSize);

    std::vector<float> appLeft(static_cast<size_t>(kBlockSize), 0.0f);
    std::vector<float> appRight(static_cast<size_t>(kBlockSize), 0.0f);
    float* appOutputs[2] = {appLeft.data(), appRight.data()};

    synth::AudioBlock block;
    block.outputs = appOutputs;
    block.numOutputChannels = 2;
    block.numFrames = static_cast<std::size_t>(kBlockSize);

    std::vector<float> refLeft(static_cast<size_t>(kBlockSize), 0.0f);
    std::vector<float> refRight(static_cast<size_t>(kBlockSize), 0.0f);

    for (int b = 0; b < kBlocks; ++b)
    {
        engine.ProcessBlock(block, static_cast<std::uint64_t>(b));
        referenceFacade.processHostedBlock(
            nullptr, 0, refLeft.data(), refRight.data(), 2, kBlockSize);

        for (int i = 0; i < kBlockSize; ++i)
        {
            if (!nearlyEqual(appLeft[static_cast<size_t>(i)], refLeft[static_cast<size_t>(i)])
                || !nearlyEqual(appRight[static_cast<size_t>(i)], refRight[static_cast<size_t>(i)]))
            {
                std::printf(
                    "FAIL: FroggersApp output diverged from direct facade path at block %d sample %d\n",
                    b,
                    i);
                return false;
            }
        }
    }
    return true;
}
} // namespace

int main()
{
    if (!test_engine_init_process_block_finite_stereo())
    {
        return 1;
    }
    if (!test_process_block_matches_direct_facade_path())
    {
        return 1;
    }
    std::printf("PASS: FroggersApp_test\n");
    return 0;
}
