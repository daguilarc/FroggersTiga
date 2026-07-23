#include "manifest/FroggersV2AppManifest.hpp"
#include "HostParameterInventoryV2.hpp"
#include "V2DesktopPageDisplayNames.hpp"
#include "V2ParamDisplayNames.hpp"
#include "control/MidiCvAssignmentTable.hpp"
#include "SequencerState.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#ifndef FROGGERS_V2_REPO_ROOT
#define FROGGERS_V2_REPO_ROOT "."
#endif

namespace
{
struct CheckResult
{
    const char* id = "";
    const char* title = "";
    bool passed = false;
    std::string detail;
};

std::filesystem::path repoRoot()
{
    return std::filesystem::path(FROGGERS_V2_REPO_ROOT);
}

std::string readTextFile(const std::filesystem::path& relativePath)
{
    const std::filesystem::path path = repoRoot() / relativePath;
    std::ifstream in(path, std::ios::binary);
    if (!in)
    {
        std::ostringstream out;
        out << "missing file: " << path.string();
        return out.str();
    }
    std::ostringstream out;
    out << in.rdbuf();
    return out.str();
}

template <size_t N>
std::string joinList(const std::array<std::string, N>& values)
{
    std::ostringstream out;
    bool first = true;
    for (const std::string& value : values)
    {
        if (!first)
        {
            out << ", ";
        }
        first = false;
        out << value;
    }
    return out.str();
}

std::string joinList(const std::vector<std::string>& values)
{
    std::ostringstream out;
    bool first = true;
    for (const std::string& value : values)
    {
        if (!first)
        {
            out << ", ";
        }
        first = false;
        out << value;
    }
    return out.str();
}

bool contains(const std::string& text, const std::string& needle)
{
    return text.find(needle) != std::string::npos;
}

bool containsAny(const std::string& text, std::initializer_list<const char*> needles)
{
    for (const char* needle : needles)
    {
        if (needle != nullptr && contains(text, needle))
        {
            return true;
        }
    }
    return false;
}

template <typename EntryT, typename StableIdGetter>
void sortByStableId(std::vector<EntryT>& entries, StableIdGetter stableIdOf)
{
    std::sort(entries.begin(), entries.end(), [&](const EntryT& lhs, const EntryT& rhs) {
        const std::string lhsId = stableIdOf(lhs) != nullptr ? stableIdOf(lhs) : "";
        const std::string rhsId = stableIdOf(rhs) != nullptr ? stableIdOf(rhs) : "";
        return lhsId < rhsId;
    });
}

std::vector<froggers_v2::manifest::HostedParameterEntry> makeManifestHostedEntries()
{
    const auto entries = froggers_v2::manifest::buildHostedParameterEntries();
    return std::vector<froggers_v2::manifest::HostedParameterEntry>(entries.begin(), entries.end());
}

std::vector<HostParameterInventoryV2::RuntimeDescriptor> makeInventoryEntries()
{
    std::vector<HostParameterInventoryV2::RuntimeDescriptor> entries;
    entries.reserve(HostParameterInventoryV2::kCount);
    for (size_t i = 0; i < HostParameterInventoryV2::kCount; ++i)
    {
        entries.push_back(HostParameterInventoryV2::buildDescriptorAt(i));
    }
    return entries;
}

CheckResult checkHostedParameterInventory()
{
    CheckResult result{"2.1", "HostParameterInventoryV2 vs manifest hosted entries", false, {}};
    if (!HostParameterInventoryV2::validateInventory())
    {
        result.detail = "HostParameterInventoryV2::validateInventory() failed.";
        return result;
    }

    auto manifestEntries = makeManifestHostedEntries();
    if (manifestEntries.size() != HostParameterInventoryV2::kCount)
    {
        std::ostringstream out;
        out << "count mismatch manifest=" << manifestEntries.size()
            << " inventory=" << HostParameterInventoryV2::kCount;
        result.detail = out.str();
        return result;
    }

    for (size_t i = 0; i < manifestEntries.size(); ++i)
    {
        const auto& lhs = manifestEntries[i];
        const HostParameterInventoryV2::RuntimeDescriptor rhs = HostParameterInventoryV2::buildDescriptorAt(i);
        const bool sameAxis = lhs.axis == rhs.axis;
        const bool sameGeometry = lhs.page == rhs.page && lhs.row == rhs.row;
        const auto nearlyEqual = [](float a, float b) {
            return std::fabs(a - b) <= 1.0e-6f;
        };
        const bool sameRange = nearlyEqual(lhs.minNorm, rhs.minNorm) && nearlyEqual(lhs.maxNorm, rhs.maxNorm)
                            && nearlyEqual(lhs.defaultNorm, rhs.defaultNorm);
        if (lhs.stableId != (rhs.stableId != nullptr ? rhs.stableId : "")
            || lhs.displayName != (rhs.displayName != nullptr ? rhs.displayName : "") || !sameAxis || !sameGeometry
            || !sameRange)
        {
            std::ostringstream out;
            out << "entry mismatch at index=" << i << " stableId=" << lhs.stableId << " manifest(page="
                << static_cast<unsigned>(lhs.page)
                << ", row=" << static_cast<unsigned>(lhs.row) << ", axis=" << static_cast<unsigned>(lhs.axis)
                << ", default=" << lhs.defaultNorm << ") vs inventory(page="
                << static_cast<unsigned>(rhs.page) << ", row=" << static_cast<unsigned>(rhs.row)
                << ", axis=" << static_cast<unsigned>(rhs.axis) << ", default=" << rhs.defaultNorm
                << ") manifestDisplay=\"" << lhs.displayName << "\" inventoryDisplay=\""
                << (rhs.displayName != nullptr ? rhs.displayName : "") << "\"";
            result.detail = out.str();
            return result;
        }
    }

    std::ostringstream out;
    out << "validated " << manifestEntries.size() << " hosted parameters";
    result.passed = true;
    result.detail = out.str();
    return result;
}

CheckResult checkCarouselPageRows()
{
    CheckResult result{"2.2", "Carousel page counts vs manifest page rows", false, {}};
    const size_t expectedPages = HostParameterInventoryV2::kNumUiPages;
    const size_t expectedRows = HostParameterInventoryV2::kPageRowCount;
    const size_t manifestRows = froggers_v2::manifest::productRowCount();
    size_t computedRows = 0;
    std::vector<std::string> rowLabels;

    if (HostParameterInventoryV2::kNumUiPages != V2DesktopPageDisplayNames::kV2NumHostPages)
    {
        std::ostringstream out;
        out << "page count mismatch inventory=" << static_cast<unsigned>(HostParameterInventoryV2::kNumUiPages)
            << " desktop-labels=" << static_cast<unsigned>(V2DesktopPageDisplayNames::kV2NumHostPages);
        result.detail = out.str();
        return result;
    }

    for (uint8_t page = 0; page < HostParameterInventoryV2::kNumUiPages; ++page)
    {
        const uint8_t rows = HostParameterInventoryV2::rowsForUiPage(page);
        computedRows += rows;
        const uint8_t crispyRow = V2DesktopPageDisplayNames::CrispyRowForPage(page);
        if (rows == 0 || crispyRow >= rows)
        {
            std::ostringstream out;
            out << "invalid page geometry for page " << static_cast<unsigned>(page) << " rows="
                << static_cast<unsigned>(rows) << " crispyRow=" << static_cast<unsigned>(crispyRow);
            result.detail = out.str();
            return result;
        }

        const char* pageName = V2DesktopPageDisplayNames::forHostPage(page);
        if (pageName == nullptr || pageName[0] == '\0')
        {
            std::ostringstream out;
            out << "missing page name for page " << static_cast<unsigned>(page);
            result.detail = out.str();
            return result;
        }
        for (uint8_t row = 0; row < rows; ++row)
        {
            const char* rowName = V2DesktopPageDisplayNames::forHostPageRow(page, row);
            if (rowName == nullptr || rowName[0] == '\0')
            {
                std::ostringstream out;
                out << "missing row label for page " << static_cast<unsigned>(page) << " row "
                    << static_cast<unsigned>(row);
                result.detail = out.str();
                return result;
            }
            rowLabels.emplace_back(rowName);
        }
    }

    if (computedRows != expectedRows || manifestRows != expectedRows)
    {
        std::ostringstream out;
        out << "row count mismatch expected=" << expectedRows << " computed=" << computedRows
            << " manifest=" << manifestRows;
        result.detail = out.str();
        return result;
    }

    const std::string pageCarousel = readTextFile("desktop-v2/Source/ui/PageCarouselComponent.cpp");
    const std::string submodule = readTextFile("desktop-v2/Source/ui/SubmodulePagePanel.cpp");
    const std::string adsr = readTextFile("desktop-v2/Source/ui/AdsrPagePanel.cpp");
    if (!contains(pageCarousel, "froggers_v2::kNumHostPages") || !contains(submodule, "V2ParamDisplayNames::kV2ExpandedNumRows")
        || !contains(adsr, "V2ParamDisplayNames::kV2ExpandedNumRows"))
    {
        result.detail = "carousel sources do not consistently consume the shared page-row constants.";
        return result;
    }

    result.passed = true;
    std::ostringstream out;
    out << "validated " << expectedPages << " pages and " << expectedRows << " rows";
    result.detail = out.str();
    return result;
}

CheckResult checkModSourceAuthority()
{
    CheckResult result{"2.3", "Permanent modulation catalog and route eligibility", false, {}};
    const auto& manifestSources = froggers_v2::manifest::kPermanentModulationSources;
    if (manifestSources.size() != 15)
    {
        result.detail = "manifest permanent modulation source catalog is not fixed at 15 lanes.";
        return result;
    }

    const std::string simCatalog = readTextFile("sim/V2ModSourceCatalog.hpp");
    const std::string simTapBank = readTextFile("sim/V2ModTapBank.hpp");
    const std::string simModSource = readTextFile("src/core/SimModSource.hpp");
    const std::string controlCoreH = readTextFile("desktop-v2/Source/control/FroggersV2ControlCore.hpp");
    const std::string controlCoreCpp = readTextFile("desktop-v2/Source/control/FroggersV2ControlCore.cpp");
    const std::string modSourceCell = readTextFile("desktop-v2/Source/ui/ModSourceCell.cpp");

    std::vector<std::string> missingManifestSources;
    for (const auto& source : manifestSources)
    {
        const bool visibleInConsumers = contains(simCatalog, source.displayName) || contains(simTapBank, source.displayName)
                                     || contains(simModSource, source.displayName) || contains(controlCoreCpp, source.displayName)
                                     || contains(modSourceCell, source.displayName);
        if (!visibleInConsumers)
        {
            missingManifestSources.emplace_back(source.displayName);
        }
    }

    const bool legacyCatalogStillOwned = contains(controlCoreH, "kNumModSources = 10")
                                      || contains(controlCoreH, "kModSourceMidiCcA")
                                      || contains(controlCoreH, "kModSourceMidiCcB")
                                      || contains(modSourceCell, "MIDI CC A")
                                      || contains(modSourceCell, "MIDI CC B")
                                      || contains(simCatalog, "V2ModSourceLabel")
                                      || contains(simTapBank, "kNumTaps = 8")
                                      || contains(simModSource, "IsV2ModSourceIndex(uint8_t modIndex)")
                                      || contains(simModSource, "return modIndex >= 7 && modIndex <= 14")
                                      || contains(controlCoreCpp, "kMaxSteps")
                                      || contains(controlCoreCpp, "kNumModSources = 10");

    if (!missingManifestSources.empty() || legacyCatalogStillOwned)
    {
        std::ostringstream out;
        out << "missing consumer representations for: " << joinList(missingManifestSources);
        if (legacyCatalogStillOwned)
        {
            out << "; legacy authority markers still present in current consumers (kNumModSources=10, MIDI CC A/B, "
                   "V2ModSourceLabel, kNumTaps=8, V2 source-index pool)";
        }
        result.detail = out.str();
        return result;
    }

    result.passed = true;
    result.detail = "validated permanent source catalog against current desktop/UI consumers";
    return result;
}

CheckResult checkSequencerAuthority()
{
    CheckResult result{"2.4", "Fixed 16 sequencer snapshots and lock fields", false, {}};
    const bool manifestCountIsFixed = froggers_v2::manifest::kSequencerSlotCount == 16;
    const bool manifestSlotsComplete = froggers_v2::manifest::kSequencerSlots.size() == froggers_v2::manifest::kSequencerSlotCount;
    const std::string sequencerState = readTextFile("sim/SequencerState.hpp");
    const std::string hostInventory = readTextFile("desktop-v2/Source/HostParameterInventoryV2.hpp");
    const std::string sequencerPanel = readTextFile("desktop-v2/Source/ui/SequencerPanelComponent.hpp");

    if (!manifestCountIsFixed || !manifestSlotsComplete)
    {
        result.detail = "manifest sequencer slot declarations are incomplete.";
        return result;
    }

    const bool currentStateStillVariable = contains(sequencerState, "kMaxSteps = 64")
                                        || contains(sequencerState, "m_patternLength = 16")
                                        || contains(hostInventory, "sequencer_pattern_length")
                                        || contains(hostInventory, "sequencer_playhead")
                                        || contains(sequencerPanel, "PatternLength")
                                        || contains(sequencerPanel, "m_patternScope")
                                        || contains(sequencerState, "SequencerStepSnapshot")
                                        || !contains(sequencerState, "kSlotCount = 16")
                                        || !contains(sequencerState, "bool written")
                                        || !contains(sequencerState, "bool locked");

    if (currentStateStillVariable)
    {
        result.detail = "current sequencer consumer state still exposes variable-length / playhead-led authority "
                        "(`SequencerState::kMaxSteps = 64`, `sequencer_pattern_length`, and no explicit lock-field "
                        "surface yet).";
        return result;
    }

    result.passed = true;
    result.detail = "validated 16 manifest slots and current sequencer fields";
    return result;
}

CheckResult checkControllerTargets()
{
    CheckResult result{"2.5", "Desktop MIDI/controller target declarations", false, {}};
    const auto& targets = froggers_v2::manifest::controllerTargetDeclarations();
    const std::string midiCvSettingsCpp = readTextFile("desktop-v2/Source/MidiCvSettingsComponent.cpp");
    const std::string midiCvSettingsH = readTextFile("desktop-v2/Source/MidiCvSettingsComponent.h");
    const std::string midiCvTableCpp = readTextFile("desktop-v2/Source/control/MidiCvAssignmentTable.cpp");
    const std::string audioEngineCpp = readTextFile("desktop-v2/Source/AudioEngine.cpp");
    const MidiCvControllerTargetIds& ids = midiCvControllerTargetIds();
    const std::array<const char*, froggers_v2::manifest::kBaseControllerTargetCount> actualIds{{
        ids.pitch,
        ids.gate,
        ids.externalModA,
        ids.externalModB,
        ids.scene1,
        ids.scene2,
        ids.scene3,
        ids.qwertyVirtual,
        ids.externalClock}};

    std::vector<std::string> missingTargetLabels;
    std::vector<std::string> mismatchedTargetIds;
    if (targets.size() != froggers_v2::manifest::kControllerTargetCount)
    {
        std::ostringstream out;
        out << "controller target count mismatch manifest=" << targets.size()
            << " expected=" << froggers_v2::manifest::kControllerTargetCount;
        result.detail = out.str();
        return result;
    }
    const bool manifestBackedLabels = contains(midiCvSettingsCpp, "controllerTargetDeclarations")
                                   && contains(midiCvTableCpp, "buildTargetMappingRows")
                                   && contains(midiCvTableCpp, "controllerTargetDeclarations");
    size_t encoderTargetCount = 0;
    for (const auto& target : targets)
    {
        if (froggers_v2::manifest::isEncoderTurnBindingRole(target.bindingRole)
            || froggers_v2::manifest::isEncoderModDrillInBindingRole(target.bindingRole))
        {
            ++encoderTargetCount;
            continue;
        }
        const bool visible = contains(midiCvSettingsCpp, target.displayName) || contains(midiCvSettingsH, target.displayName)
                           || contains(midiCvTableCpp, target.displayName)
                           || contains(audioEngineCpp, target.displayName)
                           || manifestBackedLabels;
        if (!visible)
        {
            missingTargetLabels.emplace_back(target.displayName);
        }
    }
    if (encoderTargetCount != froggers_v2::manifest::kEncoderControllerTargetCount)
    {
        std::ostringstream out;
        out << "encoder target count mismatch actual=" << encoderTargetCount
            << " expected=" << froggers_v2::manifest::kEncoderControllerTargetCount;
        result.detail = out.str();
        return result;
    }
    for (size_t i = 0; i < actualIds.size(); ++i)
    {
        const char* actual = actualIds[i] != nullptr ? actualIds[i] : "";
        const char* expected = targets[i].stableId != nullptr ? targets[i].stableId : "";
        if (std::string(actual) != std::string(expected))
        {
            std::ostringstream out;
            out << "index " << i << " expected " << expected << " actual " << actual;
            mismatchedTargetIds.emplace_back(out.str());
        }
    }

    const std::string midiCvSettingsText = midiCvSettingsCpp + midiCvSettingsH;
    const bool targetSurfaceVisible = contains(midiCvSettingsText, "Pitch")
                                   && contains(midiCvSettingsText, "Gate")
                                   && contains(midiCvSettingsText, "MIDI CC A")
                                   && contains(midiCvSettingsText, "MIDI CC B")
                                   && contains(midiCvSettingsText, "QWERTY virtual MIDI")
                                   && contains(midiCvSettingsText, "External MIDI clock")
                                   && contains(midiCvSettingsCpp, "controllerTargetDeclarations")
                                   && contains(midiCvSettingsCpp, "rowKindForBindingRole")
                                   && contains(midiCvSettingsCpp, "manifestTargetIdForBindingRole")
                                   && contains(midiCvSettingsCpp, "MidiCvBindingRole::SceneOrdinal0")
                                   && !contains(midiCvSettingsCpp, "HeldModifier")
                                   && !contains(midiCvSettingsCpp, "shiftButton")
                                   && contains(midiCvTableCpp, "manifestTargetIdForBindingRole")
                                   && contains(midiCvTableCpp, "midiCvControllerTargetIds")
                                   && contains(midiCvTableCpp, "ParamTurn")
                                   && contains(midiCvTableCpp, "ModDrillIn");

    if (!missingTargetLabels.empty() || !mismatchedTargetIds.empty() || !targetSurfaceVisible)
    {
        std::ostringstream out;
        if (!missingTargetLabels.empty())
        {
            out << "missing target labels: " << joinList(missingTargetLabels);
        }
        if (!mismatchedTargetIds.empty())
        {
            if (!missingTargetLabels.empty())
            {
                out << "; ";
            }
            out << "mismatched target ids: " << joinList(mismatchedTargetIds);
        }
        if (!targetSurfaceVisible)
        {
            out << "; one or more controller binding surfaces are not visible in the current MIDI/controller code";
        }
        result.detail = out.str();
        return result;
    }

    result.passed = true;
    result.detail = "validated controller target declarations against current MIDI/controller UI";
    return result;
}

CheckResult checkHostedProjectionOverlay()
{
    CheckResult result{"2.6", "Hosted projection overlays vs plugin editor behavior", false, {}};
    const std::string pluginEditorH = readTextFile("desktop-v2/Source/PluginEditorV2.h");
    const std::string pluginEditorCpp = readTextFile("desktop-v2/Source/PluginEditorV2.cpp");
    const std::string hostedMainH = readTextFile("desktop-v2/Source/HostedMainComponentV2.h");
    const std::string hostedMainCpp = readTextFile("desktop-v2/Source/HostedMainComponentV2.cpp");
    const auto& pluginProjection = froggers_v2::manifest::kProjections[2];

    if (pluginProjection.hidden || pluginProjection.readOnly || !pluginProjection.interactive)
    {
        result.detail = "plugin projection overlay visibility flags do not match the expected hosted-editor contract.";
        return result;
    }

    const bool pluginUsesHostedShell = contains(pluginEditorH, "HostedMainComponentV2")
                                     && contains(pluginEditorCpp, "setResizable(true, true)");
    const bool hostedShellAvoidsStandaloneHardwareUi = !contains(hostedMainCpp, "AudioSettingsComponent")
                                                    && !contains(hostedMainCpp, "MidiSettingsComponent")
                                                    && !contains(hostedMainCpp, "RecordButton");
    const bool hostedShellKeepsReadOnlyStatus = contains(hostedMainH, "GlobalOscilloscopeDisplay")
                                             && contains(hostedMainH, "PerformanceBandV2")
                                             && contains(hostedMainH, "PageCarouselComponent")
                                             && contains(hostedMainH, "SequencerPanelComponent");

    if (!pluginUsesHostedShell || !hostedShellAvoidsStandaloneHardwareUi || !hostedShellKeepsReadOnlyStatus)
    {
        std::ostringstream out;
        out << "pluginUsesHostedShell=" << (pluginUsesHostedShell ? "true" : "false")
            << ", hostedShellAvoidsStandaloneHardwareUi="
            << (hostedShellAvoidsStandaloneHardwareUi ? "true" : "false")
            << ", hostedShellKeepsReadOnlyStatus=" << (hostedShellKeepsReadOnlyStatus ? "true" : "false");
        result.detail = out.str();
        return result;
    }

    result.passed = true;
    result.detail = "validated hosted editor shell, read-only status projection, and host-parameter-backed controls";
    return result;
}

std::array<CheckResult, 6> runChecks()
{
    return {checkHostedParameterInventory(),
            checkCarouselPageRows(),
            checkModSourceAuthority(),
            checkSequencerAuthority(),
            checkControllerTargets(),
            checkHostedProjectionOverlay()};
}
} // namespace

int main()
{
    const auto checks = runChecks();
    bool anyFailure = false;
    for (const CheckResult& check : checks)
    {
        std::printf("%s: [%s] %s\n", check.passed ? "PASS" : "FAIL", check.id, check.title);
        std::printf("      %s\n", check.detail.c_str());
        if (!check.passed)
        {
            anyFailure = true;
        }
    }

    if (anyFailure)
    {
        std::printf("FAIL: one or more projection validators found current-state drift\n");
        return 1;
    }

    std::printf("PASS: Froggers v2 projection validators\n");
    return 0;
}
