#include "SequencerState.hpp"

#include <cmath>
#include <cstdio>

int main()
{
    SequencerState seq;
    seq.setBpm(120.0f);
    seq.setPatternLength(4);
    seq.m_playing = true;

    SequencerStepSnapshot snap{};
    snap.gate = true;
    snap.sceneCenter[0][0][0] = 0.25f;
    seq.captureStep(0, snap);

    const bool stepped = seq.advanceOnSamples(22050, 44100.0f);
    if (!stepped)
    {
        std::printf("FAIL: expected step advance after half second at 120 BPM\n");
        return 1;
    }
    if (seq.m_playhead != 1)
    {
        std::printf("FAIL: playhead expected 1 got %u\n", seq.m_playhead);
        return 1;
    }

    seq.m_playhead = 0;
    if (!seq.stepGate())
    {
        std::printf("FAIL: step 0 gate expected true\n");
        return 1;
    }

    seq.setPatternLength(16);
    seq.m_editStep = 0;
    seq.prevEditStep();
    if (seq.m_editStep != 15)
    {
        std::printf("FAIL: prevEditStep wrap expected 15 got %u\n", seq.m_editStep);
        return 1;
    }
    seq.m_editStep = 15;
    seq.nextEditStep();
    if (seq.m_editStep != 0)
    {
        std::printf("FAIL: nextEditStep wrap expected 0 got %u\n", seq.m_editStep);
        return 1;
    }

    if (!seq.activeStepGate())
    {
        std::printf("FAIL: activeStepGate expected true when playing with lit step\n");
        return 1;
    }

    seq.m_playing = false;
    if (seq.activeStepGate())
    {
        std::printf("FAIL: activeStepGate expected false when stopped (lit step ignored)\n");
        return 1;
    }

    snap.gate = false;
    seq.captureStep(0, snap);
    seq.m_playing = true;
    if (seq.activeStepGate())
    {
        std::printf("FAIL: activeStepGate expected false when step gate is off\n");
        return 1;
    }

    seq.setPatternLength(64);
    if (seq.m_patternLength != 64)
    {
        std::printf("FAIL: pattern length clamp\n");
        return 1;
    }

    seq.m_playhead = 0;
    seq.m_externalClock = true;
    seq.advanceOnExternalClock();
    if (seq.m_playhead != 1)
    {
        std::printf("FAIL: external clock advance expected playhead 1\n");
        return 1;
    }

    std::printf("SequencerState_test OK\n");
    return 0;
}
