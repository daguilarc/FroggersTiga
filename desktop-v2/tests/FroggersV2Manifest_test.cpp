#include "manifest/FroggersV2AppManifest.hpp"

#include <array>
#include <cstdio>
#include <string>

namespace
{
bool contains(const std::string& haystack, const char* needle)
{
    return haystack.find(needle) != std::string::npos;
}

bool hasInvalidJsonControlCharacter(const std::string& text)
{
    for (const unsigned char c : text)
    {
        if (c < 0x20 && c != '\n' && c != '\r' && c != '\t')
        {
            return true;
        }
    }
    return false;
}

bool appearsBefore(const std::string& haystack, const char* lhs, const char* rhs)
{
    const size_t lhsPos = haystack.find(lhs);
    const size_t rhsPos = haystack.find(rhs);
    return lhsPos != std::string::npos && rhsPos != std::string::npos && lhsPos < rhsPos;
}
} // namespace

int main()
{
    const auto validation = froggers_v2::manifest::buildValidationSummary();
    if (validation.duplicateStableIds || validation.invalidPageRowRefs || validation.missingDisplayNames
        || validation.invalidRanges || validation.invalidDefaults || validation.invalidProjectionOverlays
        || validation.missingOscilloscopeTaps || validation.invalidRandomizationScopes
        || validation.invalidSequencerStepCounts || validation.missingWrittenUnwrittenState
        || validation.invalidSequencerDirectionSpeed || validation.invalidSourceLaneCounts
        || validation.duplicateSourceIds || validation.missingSourceColorsGroups
        || validation.invalidExternalAudioAvailability)
    {
        std::printf("FAIL: validation flags dup=%d page=%d names=%d ranges=%d defaults=%d proj=%d osc=%d rand=%d seq=%d write=%d dir=%d lanes=%d dupsrc=%d colors=%d ext=%d\n",
                    validation.duplicateStableIds,
                    validation.invalidPageRowRefs,
                    validation.missingDisplayNames,
                    validation.invalidRanges,
                    validation.invalidDefaults,
                    validation.invalidProjectionOverlays,
                    validation.missingOscilloscopeTaps,
                    validation.invalidRandomizationScopes,
                    validation.invalidSequencerStepCounts,
                    validation.missingWrittenUnwrittenState,
                    validation.invalidSequencerDirectionSpeed,
                    validation.invalidSourceLaneCounts,
                    validation.duplicateSourceIds,
                    validation.missingSourceColorsGroups,
                    validation.invalidExternalAudioAvailability);
        std::printf("FAIL: manifest validation summary contains an unexpected failure flag\n");
        return 1;
    }
    if (froggers_v2::manifest::productRowCount() != HostParameterInventoryV2::kPageRowCount)
    {
        std::printf("FAIL: product row count does not match current v2 row inventory\n");
        return 1;
    }
    if (froggers_v2::manifest::kPermanentModulationSources.size() != 15)
    {
        std::printf("FAIL: permanent modulation source catalog is not 15 lanes\n");
        return 1;
    }
    if (froggers_v2::manifest::kSequencerSlotCount != 16)
    {
        std::printf("FAIL: manifest sequencer slot count is not fixed at 16\n");
        return 1;
    }
    if (froggers_v2::manifest::kRandomizationScopeControls.size() != 2)
    {
        std::printf("FAIL: manifest does not declare the two global randomization scope controls\n");
        return 1;
    }
    if (!contains(froggers_v2::manifest::kRandomizationScopeControls[0].stableId, "scene")
        || !contains(froggers_v2::manifest::kRandomizationScopeControls[0].choices[0].displayName, "All Scenes")
        || !contains(froggers_v2::manifest::kRandomizationScopeControls[0].choices[1].displayName, "Current Scene")
        || !froggers_v2::manifest::kRandomizationScopeControls[0].consumedByRandomizeAll
        || !froggers_v2::manifest::kRandomizationScopeControls[0].consumedByRandomizeMod)
    {
        std::printf("FAIL: scene randomization scope control is not fully declared\n");
        return 1;
    }
    if (!contains(froggers_v2::manifest::kRandomizationScopeControls[1].stableId, "step")
        || !contains(froggers_v2::manifest::kRandomizationScopeControls[1].choices[0].displayName, "All Steps")
        || !contains(froggers_v2::manifest::kRandomizationScopeControls[1].choices[1].displayName, "Current Step")
        || !froggers_v2::manifest::kRandomizationScopeControls[1].consumedByRandomizeAll
        || !froggers_v2::manifest::kRandomizationScopeControls[1].consumedByRandomizeMod)
    {
        std::printf("FAIL: step randomization scope control is not fully declared\n");
        return 1;
    }
    if (froggers_v2::manifest::kSequencerSlots.size() != froggers_v2::manifest::kSequencerSlotCount)
    {
        std::printf("FAIL: manifest does not declare all fixed sequencer slots\n");
        return 1;
    }
    {
        const std::array<froggers_v2::manifest::OscilloscopeTap, 2> duplicateTaps{{
            {"dup_tap", "Tap A", 0, true},
            {"dup_tap", "Tap B", 1, true},
        }};
        if (!froggers_v2::manifest::hasDuplicateStableIds(duplicateTaps,
                                                          [](const froggers_v2::manifest::OscilloscopeTap& tap) {
                                                              return tap.stableId;
                                                          }))
        {
            std::printf("FAIL: oscilloscope tap duplicate stable ID helper did not detect duplicates\n");
            return 1;
        }
    }
    {
        const std::array<froggers_v2::manifest::RandomizationScopeControl, 2> duplicateControls{{
            {"dup_scope", "Scope A", {{{"First", true}, {"Second", false}}}, true, true},
            {"dup_scope", "Scope B", {{{"First", true}, {"Second", false}}}, true, true},
        }};
        if (!froggers_v2::manifest::hasDuplicateStableIds(
                duplicateControls, [](const froggers_v2::manifest::RandomizationScopeControl& control) {
                    return control.stableId;
                }))
        {
            std::printf("FAIL: randomization scope duplicate stable ID helper did not detect duplicates\n");
            return 1;
        }
    }
    {
        const std::array<std::string, 6> globalStableIds{{
            "product:page0_row0",
            "host:page0_row0",
            "mod:vco_1_ef",
            "tap:vco_1_ef",
            "scope:scene",
            "product:page0_row0",
        }};
        if (!froggers_v2::manifest::hasDuplicateStableIds(globalStableIds,
                                                          [](const std::string& stableId) { return stableId.c_str(); }))
        {
            std::printf("FAIL: global duplicate stable ID helper did not detect a cross-family collision\n");
            return 1;
        }
    }
    {
        const std::array<froggers_v2::manifest::ModulationSource, 3> syntheticSources{{
            {"z_last", "Last", "z-group", "audio", "always", 0x010203u, true},
            {"a_second", "Second", "a-group", "audio", "always", 0x040506u, true},
            {"a_first", "First", "a-group", "audio", "always", 0x070809u, true},
        }};
        const auto sortedSources = froggers_v2::manifest::sortedByGroupThenStableId(
            syntheticSources,
            [](const froggers_v2::manifest::ModulationSource& source) { return source.group; },
            [](const froggers_v2::manifest::ModulationSource& source) { return source.stableId; });
        if (!contains(sortedSources[0].stableId, "a_first") || !contains(sortedSources[1].stableId, "a_second")
            || !contains(sortedSources[2].stableId, "z_last"))
        {
            std::printf("FAIL: group/stable modulation source sorting helper is not ordering by group first\n");
            return 1;
        }
    }
    {
        const auto& projections = froggers_v2::manifest::kProjections;
        if (projections.size() != 4 || std::string(projections[0].id) != "product"
            || projections[0].hidden || projections[0].readOnly || !projections[0].interactive
            || projections[0].reserved || std::string(projections[1].id) != "desktop"
            || projections[1].hidden || projections[1].readOnly || !projections[1].interactive
            || projections[1].reserved || std::string(projections[2].id) != "plugin"
            || projections[2].hidden || projections[2].readOnly || !projections[2].interactive
            || projections[2].reserved || std::string(projections[3].id) != "vcv"
            || !projections[3].hidden || !projections[3].readOnly || projections[3].interactive
            || !projections[3].reserved)
        {
            std::printf("FAIL: projection overlay declarations do not match the expected visibility contract\n");
            return 1;
        }
    }
    for (size_t i = 0; i < froggers_v2::manifest::kSequencerSlots.size(); ++i)
    {
        const auto& slot = froggers_v2::manifest::kSequencerSlots[i];
        if (slot.index != i || slot.writtenStateField == nullptr || slot.writtenStateField[0] == '\0'
            || !slot.sequencerPersistent || !slot.sequencerLockable)
        {
            std::printf("FAIL: manifest sequencer slot %zu lacks written/snapshot/lock declaration\n", i);
            return 1;
        }
    }

    const std::string snapshot = froggers_v2::manifest::buildSnapshotJson();
    if (hasInvalidJsonControlCharacter(snapshot))
    {
        std::printf("FAIL: generated manifest snapshot contains an invalid JSON control character\n");
        return 1;
    }
    if (!contains(snapshot, "\"authority\": \"cpp\"") || !contains(snapshot, "\"vcv\"")
        || !contains(snapshot, "\"external_audio_rate\"") || !contains(snapshot, "\"hostParameters\""))
    {
        std::printf("FAIL: generated manifest snapshot is missing required review sections\n");
        return 1;
    }
    if (!contains(snapshot, "\"randomizationScopeControls\"") || !contains(snapshot, "\"All Scenes\"")
        || !contains(snapshot, "\"Current Scene\"") || !contains(snapshot, "\"All Steps\"")
        || !contains(snapshot, "\"Current Step\"") || !contains(snapshot, "\"sequencerSlots\"")
        || !contains(snapshot, "\"writtenStateField\"") || !contains(snapshot, "\"sequencerPersistent\"")
        || !contains(snapshot, "\"sequencerLockable\""))
    {
        std::printf("FAIL: generated manifest snapshot is missing declaration-driven scope/slot sections\n");
        return 1;
    }
    if (contains(snapshot, "\"vco_1_audio\"") || contains(snapshot, "\"vco_2_audio\"")
        || contains(snapshot, "\"vco_3_audio\"") || contains(snapshot, "\"vco_123_ef\"")
        || contains(snapshot, "\"midi_cc_a\"") || contains(snapshot, "\"midi_cc_b\""))
    {
        std::printf("FAIL: manifest snapshot contains rejected duplicate source lanes\n");
        return 1;
    }
    if (!appearsBefore(snapshot, "\"vco_pair_13\"", "\"vco_pair_23\"")
        || !appearsBefore(snapshot, "\"global_crunchy\"", "\"global_gesture_weight_0\"")
        || !appearsBefore(snapshot, "\"vco_1_ef\"", "\"vco_pair_12\""))
    {
        std::printf("FAIL: generated manifest snapshot entries are not sorted by group/stable ID inside projection groups\n");
        return 1;
    }
    if (!contains(snapshot, "\"oscilloscopeTaps\"") || !contains(snapshot, "\"stableId\": \"oscilloscope_vco_1_ef\"")
        || !contains(snapshot, "\"stableId\": \"oscilloscope_vco_2_ef\"")
        || !contains(snapshot, "\"stableId\": \"oscilloscope_vco_3_ef\""))
    {
        std::printf("FAIL: generated manifest snapshot is missing oscilloscope tap declarations\n");
        return 1;
    }
    if (!contains(snapshot, "\"schema\"") || !contains(snapshot, "\"productControls\"")
        || !contains(snapshot, "\"projectionOverlays\"") || !contains(snapshot, "\"runtimeControls\"")
        || !contains(snapshot, "\"hostParameterMapping\"") || !contains(snapshot, "\"hardwareControls\"")
        || !contains(snapshot, "\"layoutGroups\"") || !contains(snapshot, "\"reservedSchemaOnly\""))
    {
        std::printf("FAIL: manifest snapshot is missing manifest-family schema sections\n");
        return 1;
    }
    if (!contains(snapshot, "\"hidden\"") || !contains(snapshot, "\"readOnly\"")
        || !contains(snapshot, "\"interactive\""))
    {
        std::printf("FAIL: projection overlay schema is missing visibility fields\n");
        return 1;
    }
    if (!contains(snapshot,
                  "\"id\": \"vcv\", \"displayName\": \"VCV Reserved\", \"hidden\": true, \"readOnly\": true, "
                  "\"interactive\": false, \"reserved\": true"))
    {
        std::printf("FAIL: generated manifest snapshot is missing the declared VCV overlay visibility\n");
        return 1;
    }
    if (!contains(snapshot, "\"defaultModulePage\": \"audio_vco\"")
        || !contains(snapshot, "\"envelopePage\"")
        || !contains(snapshot, "\"vcoAttackReleasePairs\"") || !contains(snapshot, "\"waveformMorphControls\"")
        || !contains(snapshot, "\"lfoModule\"") || !contains(snapshot, "\"globalRandomizationScopes\"")
        || !contains(snapshot, "\"deviceNeutralClearStep\"") || !contains(snapshot, "\"sequencerLocks\"")
        || !contains(snapshot, "\"optionalMidiClockSync\"") || !contains(snapshot, "\"heldGestures\": false"))
    {
        std::printf("FAIL: manifest snapshot is missing required Froggers v2 product contract entries\n");
        return 1;
    }
    // Task 7.4 (D11/D14): the coupler is removed entirely for the V2 host -- the
    // manifest must NOT emit a crossCouplers declaration (or its vco_12/vco_23 IDs)
    // any more.
    if (contains(snapshot, "\"crossCouplers\"") || contains(snapshot, "\"vco_12\"")
        || contains(snapshot, "\"vco_23\""))
    {
        std::printf("FAIL: manifest snapshot still emits removed crossCouplers declaration\n");
        return 1;
    }
    if (!contains(snapshot, "\"validation\"") || !contains(snapshot, "\"duplicateStableIds\"")
        || !contains(snapshot, "\"invalidPageRowRefs\"") || !contains(snapshot, "\"missingDisplayNames\"")
        || !contains(snapshot, "\"invalidRanges\"") || !contains(snapshot, "\"invalidDefaults\"")
        || !contains(snapshot, "\"invalidProjectionOverlays\"") || !contains(snapshot, "\"missingOscilloscopeTaps\"")
        || !contains(snapshot, "\"invalidRandomizationScopes\"") || !contains(snapshot, "\"invalidSequencerStepCounts\"")
        || !contains(snapshot, "\"missingWrittenUnwrittenState\"") || !contains(snapshot, "\"invalidSequencerDirectionSpeed\"")
        || !contains(snapshot, "\"invalidSourceLaneCounts\"") || !contains(snapshot, "\"duplicateSourceIds\"")
        || !contains(snapshot, "\"missingSourceColorsGroups\"") || !contains(snapshot, "\"invalidExternalAudioAvailability\""))
    {
        std::printf("FAIL: manifest snapshot is missing validation coverage entries\n");
        return 1;
    }

    const std::string report = froggers_v2::manifest::buildReviewerReportMarkdown();
    if (!contains(report, "Froggers v2 Manifest Report") || !contains(report, "C++ declarations")
        || !contains(report, "Manifest Family Schema"))
    {
        std::printf("FAIL: generated manifest report is missing required review text\n");
        return 1;
    }

    std::printf("PASS: Froggers v2 manifest foundation (%zu rows, %zu host parameters, %zu sources)\n",
                froggers_v2::manifest::productRowCount(),
                HostParameterInventoryV2::kCount,
                froggers_v2::manifest::kPermanentModulationSources.size());
    return 0;
}
