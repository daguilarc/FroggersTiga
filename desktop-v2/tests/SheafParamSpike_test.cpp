// SheafParamSpike_test -- desktop-v2-sheaf-runtime-harmonization tasks.md
// §13.1 (bounded spike, PROOF OF SHAPE, additive only).
//
// GOAL: prove a Sheaf-native synth::ParameterManager / synth::ParameterGroup
// / synth::Bank can drive the existing Froggers DSP through the existing
// bridge boundary -- specifically that a Sheaf synth::Parameter value change
// reaches Page::GetParam() via DesktopHostIO::SetPageKnob (unmodified; see
// Source/control/SheafParamBridgeSpike.hpp for the glue and its read-method
// rationale).
//
// This is NOT a replacement for FroggersV2ControlCore / FroggersV2HostBridge
// and does not touch either. It builds a minimal, self-contained Sheaf
// ParameterManager + one ParameterGroup + one Bank for three representative
// rows of the real Audio page layout (verified against the vendored
// V2ParamDisplayNames.hpp, not the tasks.md packet description -- see the
// note below), registers real synth::ParameterConfigs, and drives each
// param from its Sheaf default to a distinct absolute value, asserting the
// changed value reaches Page::GetParam(position) through DesktopHostIO
// unchanged from what a direct DesktopHostIO::SetPageKnob call with the same
// numeric value would produce (independent host instance, same page/
// position/value) -- proving the glue's terminus is exactly
// DesktopHostIO::SetPageKnob and nothing else, without needing to duplicate
// Froggers' internal V2Fuego/ModMgr transform math in this test.
//
// Row-count note: tasks.md's packet description assumed "10 params post-D11:
// 3 pitch, 3 Shape, 3 PM, Crispy" for the Audio page. The vendored
// V2ParamDisplayNames.hpp (kAudioRowLabels, Source/V2ParamDisplayNames.hpp)
// instead shows Audio is 7 rows post-D11: VCO1/VCO2/VCO3 (pitch),
// Phase mod 1/2/3 (PM), Crispy -- there is no separate "Shape" row on the
// Audio page (Shape lives on the Drive page). This test follows the
// vendored ground truth per the spike's own grounding instructions and
// exercises one pitch row, one PM row, and the Crispy row.

#include "synth/ParameterModulation.hpp"

#include "DesktopHostIO.hpp"
#include "control/SheafParamBridgeSpike.hpp"

#include <cmath>
#include <cstdio>
#include <cstdint>

namespace
{

constexpr uint8_t kAudioPage = 0;
constexpr uint8_t kVco1Row = 0;      // "VCO1" (pitch)
constexpr uint8_t kPhaseMod1Row = 3; // "Phase mod 1" (PM)
constexpr uint8_t kCrispyRow = 6;    // "Crispy"
constexpr float kEps = 1.0e-5f;

bool nearlyEqual(float a, float b, float eps = kEps)
{
    return std::fabs(a - b) <= eps;
}

// Builds a minimal Sheaf ParameterManager + one ParameterGroup + one Bank
// covering three representative Audio-page rows (see file header for why
// these three and not the tasks.md-assumed 10), registers real
// synth::ParameterConfigs, and returns the manager plus the registered
// Parameter pointers in Audio-page-row order (VCO1, Phase mod 1, Crispy).
struct SheafSpikeRig
{
    synth::ParameterManager manager;
    synth::ParameterGroup* group = nullptr;
    synth::Bank* bank = nullptr;
    synth::Parameter* vco1 = nullptr;
    synth::Parameter* phaseMod1 = nullptr;
    synth::Parameter* crispy = nullptr;
};

bool buildSheafSpikeRig(SheafSpikeRig& rig)
{
    synth::ParameterGroupConfig groupConfig;
    groupConfig.numVoices = 1;
    groupConfig.numModulators = 0;
    groupConfig.numScenes = 1;
    groupConfig.maxParameters = 8;
    if (!groupConfig.IsValid())
    {
        std::printf("FAIL: ParameterGroupConfig invalid\n");
        return false;
    }

    rig.group = &rig.manager.CreateGroup(groupConfig);

    rig.vco1 = &rig.manager.CreateParameter(*rig.group, synth::ParameterConfig{
        .name = "VCO1 Pitch",
        .shortName = "VCO1",
        .defaultValue = 0.5f,
        .range = synth::RangeKind::Unipolar,
    });
    rig.phaseMod1 = &rig.manager.CreateParameter(*rig.group, synth::ParameterConfig{
        .name = "Phase Mod 1",
        .shortName = "PM1",
        .defaultValue = 0.0f,
        .range = synth::RangeKind::Unipolar,
    });
    rig.crispy = &rig.manager.CreateParameter(*rig.group, synth::ParameterConfig{
        .name = "Crispy",
        .shortName = "CRSP",
        .defaultValue = 0.0f,
        .range = synth::RangeKind::Unipolar,
    });

    if (!rig.manager.SetSceneEndpoints(0, 0))
    {
        std::printf("FAIL: SetSceneEndpoints(0, 0) rejected\n");
        return false;
    }

    // Bank + slot construction: proves the Bank/BankSlot registration shape
    // from tasks.md §13.1's deliverable list compiles and links against the
    // three real parameters, even though this test drives values through
    // HandleSetAbsolute directly rather than through Bank::HandleSetAbsolute
    // (see the "does NOT yet prove" note at the bottom of this file).
    rig.bank = &rig.manager.CreateBank();
    synth::BankSlot& slot = rig.manager.CreateBankSlot();
    slot.AddPhysicalEncoder(0);
    slot.AddPhysicalEncoder(1);
    slot.AddPhysicalEncoder(2);
    slot.SelectBank(rig.bank);
    rig.bank->AddMapping(0, *rig.vco1);
    rig.bank->AddMapping(1, *rig.phaseMod1);
    rig.bank->AddMapping(2, *rig.crispy);

    return true;
}

// Drives one Sheaf Parameter to targetValue via the same absolute-set path
// UI encoder gestures use (HandleSetAbsolute), then pumps
// ParameterManager::ComputeAllParameters() -- the non-steady-state
// (init/patch-load/revert) recompute-and-snap pump documented on
// ParameterManager::ComputeAllParameters() in ParameterModulation.hpp --
// so CachedKnobValue reflects the change immediately without needing a
// per-sample ProcessSample() pump loop.
void driveSheafParamAbsolute(synth::ParameterManager& manager, synth::Parameter& param, float targetValue)
{
    param.HandleSetAbsolute(manager.Scene(), targetValue);
    manager.ComputeAllParameters();
}

// Core assertion: for one (Sheaf param, Froggers row) pair, prove
// (a) the Sheaf-resolved value differs from Froggers' Audio-page default at
//     that row (the "default -> changed" transition tasks.md §13.1 asks for),
//     and
// (b) pushing that Sheaf value through
//     froggers_v2::spike::PushSheafParamToPageKnob lands on Page::GetParam
//     exactly what an independent DesktopHostIO::SetPageKnob call with the
//     identical numeric value would produce -- i.e. the glue's terminus is
//     DesktopHostIO::SetPageKnob and nothing else.
bool assertParamReachesPageViaGlue(DesktopHostIO& host, synth::Parameter& param, uint8_t position,
                                   const char* label)
{
    const float beforeGlue = host.GetPageParam(kAudioPage, position);

    froggers_v2::spike::PushSheafParamToPageKnob(param, host, kAudioPage, position);
    const float afterGlue = host.GetPageParam(kAudioPage, position);

    const float sheafKnobValue = param.CachedKnobValue(0);

    DesktopHostIO directHost;
    directHost.Init();
    directHost.SetPageKnob(kAudioPage, position, sheafKnobValue);
    const float expected = directHost.GetPageParam(kAudioPage, position);

    if (nearlyEqual(afterGlue, beforeGlue, 1.0e-3f))
    {
        std::printf("FAIL: %s (row %u) did not change after glue push (before=%f after=%f)\n",
                    label, position, static_cast<double>(beforeGlue), static_cast<double>(afterGlue));
        return false;
    }
    if (!nearlyEqual(afterGlue, expected))
    {
        std::printf("FAIL: %s (row %u) glue result %f != direct SetPageKnob result %f (knobValue=%f)\n",
                    label, position, static_cast<double>(afterGlue), static_cast<double>(expected),
                    static_cast<double>(sheafKnobValue));
        return false;
    }
    return true;
}

bool test_sheaf_param_value_reaches_page_get_param()
{
    SheafSpikeRig rig;
    if (!buildSheafSpikeRig(rig))
    {
        return false;
    }

    DesktopHostIO host;
    host.Init();

    bool ok = true;

    driveSheafParamAbsolute(rig.manager, *rig.vco1, 0.85f);
    ok = assertParamReachesPageViaGlue(host, *rig.vco1, kVco1Row, "VCO1 Pitch") && ok;

    driveSheafParamAbsolute(rig.manager, *rig.phaseMod1, 0.2f);
    ok = assertParamReachesPageViaGlue(host, *rig.phaseMod1, kPhaseMod1Row, "Phase Mod 1") && ok;

    driveSheafParamAbsolute(rig.manager, *rig.crispy, 0.7f);
    ok = assertParamReachesPageViaGlue(host, *rig.crispy, kCrispyRow, "Crispy") && ok;

    return ok;
}

bool test_cached_knob_value_matches_default_before_any_drive()
{
    SheafSpikeRig rig;
    if (!buildSheafSpikeRig(rig))
    {
        return false;
    }

    // Parameter::CachedKnobValue is seeded at construction time
    // (Parameter::SeedCachedKnobAndUiDisplayState(), called from the
    // Parameter constructor) -- no ComputeAllParameters() pump is required
    // just to read the registered default back out.
    if (!nearlyEqual(rig.vco1->CachedKnobValue(0), 0.5f))
    {
        std::printf("FAIL: VCO1 default CachedKnobValue expected 0.5, got %f\n",
                    static_cast<double>(rig.vco1->CachedKnobValue(0)));
        return false;
    }
    if (!nearlyEqual(rig.crispy->CachedKnobValue(0), 0.0f))
    {
        std::printf("FAIL: Crispy default CachedKnobValue expected 0.0, got %f\n",
                    static_cast<double>(rig.crispy->CachedKnobValue(0)));
        return false;
    }
    return true;
}

} // namespace

int main()
{
    bool ok = true;
    ok = test_cached_knob_value_matches_default_before_any_drive() && ok;
    ok = test_sheaf_param_value_reaches_page_get_param() && ok;

    if (!ok)
    {
        std::printf("FAIL: SheafParamSpike_test\n");
        return 1;
    }
    std::printf("PASS: SheafParamSpike_test\n");
    return 0;
}
