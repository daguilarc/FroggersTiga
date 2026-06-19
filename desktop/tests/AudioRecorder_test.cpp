#include "AudioRecorder.h"
#include "OwnedAllocationGuard.hpp"

#include <cstdio>
#include <vector>

static bool fillPattern(float* left, float* right, size_t count, float base)
{
    for (size_t i = 0; i < count; ++i)
    {
        left[i] = base + static_cast<float>(i);
        right[i] = base + static_cast<float>(i) + 0.5f;
    }
    return true;
}

static bool test_stop_drains_partial_chunk()
{
    AudioRecorder recorder;
    recorder.start();

    std::vector<float> left(100);
    std::vector<float> right(100);
    fillPattern(left.data(), right.data(), left.size(), 10.0f);
    if (!recorder.appendStereo(left.data(), right.data(), left.size()))
    {
        std::printf("FAIL: appendStereo returned false for partial chunk\n");
        return false;
    }

    recorder.stop();
    if (recorder.getSampleCount() != 100)
    {
        std::printf("FAIL: expected 100 samples after stop, got %zu\n", recorder.getSampleCount());
        return false;
    }
    if (recorder.wasTruncated())
    {
        std::printf("FAIL: partial chunk drain should not truncate\n");
        return false;
    }

    const auto& interleaved = recorder.getInterleaved();
    if (interleaved.size() != 200 || interleaved[0] != 10.0f || interleaved[1] != 10.5f)
    {
        std::printf("FAIL: interleaved payload mismatch after drain\n");
        return false;
    }
    return true;
}

static bool test_max_samples_truncation()
{
    AudioRecorder recorder;
    constexpr size_t kTestMax = 5000;
    recorder.start();
    recorder.setMaxSamplesForTest(kTestMax);

    std::vector<float> left(256, 1.0f);
    std::vector<float> right(256, -1.0f);
    size_t appended = 0;
    while (recorder.isActive())
    {
        if (!recorder.appendStereo(left.data(), right.data(), left.size()))
        {
            break;
        }
        appended += left.size();
        if (appended > kTestMax + left.size())
        {
            std::printf("FAIL: append loop did not stop at test max samples\n");
            return false;
        }
    }

    recorder.stop();
    if (!recorder.wasTruncated())
    {
        std::printf("FAIL: expected truncation at test max samples\n");
        return false;
    }
    if (recorder.getSampleCount() != kTestMax)
    {
        std::printf("FAIL: expected %zu stored samples, got %zu\n",
                    kTestMax,
                    recorder.getSampleCount());
        return false;
    }
    return true;
}

static bool test_pool_overflow_truncates()
{
    AudioRecorder recorder;
    recorder.setConsumerBlockedForTest(true);
    recorder.start();

    std::vector<float> left(AudioRecorder::kPoolChunkFrames, 0.25f);
    std::vector<float> right(AudioRecorder::kPoolChunkFrames, -0.25f);
    const size_t framesToOverflow =
        AudioRecorder::kPoolChunkFrames * (AudioRecorder::kPoolChunkCount + 1);

    size_t appended = 0;
    while (recorder.isActive())
    {
        if (!recorder.appendStereo(left.data(), right.data(), left.size()))
        {
            break;
        }
        appended += left.size();
        if (appended > framesToOverflow)
        {
            std::printf("FAIL: append exceeded overflow budget without stopping\n");
            recorder.setConsumerBlockedForTest(false);
            recorder.stop();
            return false;
        }
    }

    recorder.setConsumerBlockedForTest(false);
    recorder.stop();

    if (!recorder.wasTruncated())
    {
        std::printf("FAIL: expected pool overflow truncation\n");
        return false;
    }
    if (recorder.getSampleCount() == 0)
    {
        std::printf("FAIL: overflow path should retain drained samples\n");
        return false;
    }
    return true;
}

static bool test_append_steady_state_no_owned_allocation()
{
    AudioRecorder recorder;
    recorder.start();

    std::vector<float> left(256);
    std::vector<float> right(256);
    fillPattern(left.data(), right.data(), left.size(), 0.0f);

    for (int pass = 0; pass < 32; ++pass)
    {
        FROGGERS_OWNED_ALLOCATION_GUARD();
        if (!recorder.appendStereo(left.data(), right.data(), left.size()))
        {
            std::printf("FAIL: appendStereo stopped unexpectedly during allocation test\n");
            recorder.stop();
            return false;
        }
    }

    recorder.stop();
    if (recorder.getSampleCount() != 256 * 32)
    {
        std::printf("FAIL: expected %zu samples after steady append, got %zu\n",
                    static_cast<size_t>(256 * 32),
                    recorder.getSampleCount());
        return false;
    }
    return true;
}

int main()
{
    if (!test_stop_drains_partial_chunk())
    {
        return 1;
    }
    if (!test_max_samples_truncation())
    {
        return 1;
    }
    if (!test_pool_overflow_truncates())
    {
        return 1;
    }
    if (!test_append_steady_state_no_owned_allocation())
    {
        return 1;
    }

    std::printf("PASS: AudioRecorder drain/overflow/allocation tests\n");
    return 0;
}
