// FroggersAppSurface_test -- packet 5 (openspec/changes/desktop-v2-sheaf-
// runtime-harmonization, tasks.md 5.1-5.4) extended by packet 7 increment 1
// (tasks.md 7.2, design.md "Layout addendum" Candidate A).
//
// Portable-visualizer presence coverage (task 5.4a): proves the FroggersApp
// portable surface (Source/ui/FroggersAppSurface.hpp, packet 3's
// FroggersAppSurface relocated + extended here) exposes the two dual
// ScopeVisualizer panels and the GangedRandomLfoVisualizer on the Random
// S&H mod-depth cells through its actual synth::ui::Surface::BuildTree()
// contract -- not just as private members. Built standalone: no JUCE, no
// AudioEngine (FroggersAppSurface owns only Sheaf-vendored + manifest
// headers), matching the "portable" (JUCE-free) surface requirement.
//
// Packet 7 increment 1 coverage added here: the 6-tab module selector (all
// six module names, single active-module authority via DispatchAction), and
// the active module's 4x4 grid -- Audio ported fully (real
// V2DesktopPageDisplayNames labels, one Draw ring per slot), the other five
// modules a labeled stub while active. Selecting a different tab is the only
// way the active-module index moves (no parallel page-state).
//
// Packet 7 increment 2 extends this: Filter, Drive, Reverb, and Delay are now
// ported to the same real reference pattern as Audio (same id-prefix
// convention: "<moduleId>_grid_label_N"/"<moduleId>_grid_ring_N"), reading
// real labels/values from the same shared FroggersV2ControlCore + manifest
// source.
//
// Packet 7 increment 3 (tasks.md 7.5, "ASR Envelope") ports the sixth and
// last module, Envelope, to the same reference pattern (real grid, not a
// stub) and covers task 7.5's naming retirement: the desktop-v2 page-5 label
// authority (V2DesktopPageDisplayNames.hpp) no longer says "Pair-AR" anywhere
// -- test_no_pair_ar_label_remains_anywhere_in_the_surface scans every
// Label/Draw-text node in the built tree for that substring. Sustain rows are
// deliberately NOT added (grid stays 7 slots: Attack/Release per VCO +
// Crispy) -- see V2DesktopPageDisplayNames.hpp's file-header note; that gap
// is reported, not fixed, in this increment.
//
// Does NOT wire into Main.cpp / MainComponent (shell cutover is tasks.md
// section 10, a later packet).

#include "ui/FroggersAppSurface.hpp"

#include <array>
#include <cstdio>
#include <string>
#include <vector>

using froggers_v2::FroggersAppSurface;

namespace
{

const synth::ui::Node* findNode(const synth::ui::NodeTree& tree, const char* id)
{
    for (const auto& node : tree.nodes)
    {
        if (node.id.value == id)
        {
            return &node;
        }
    }
    return nullptr;
}

bool test_surface_exposes_dual_scope_panels()
{
    FroggersAppSurface surface;
    const synth::ui::NodeTree tree = surface.BuildTree();

    const synth::ui::Node* vcoPanel = findNode(tree, "vco_scope_panel");
    const synth::ui::Node* lfoEfPanel = findNode(tree, "lfo_ef_scope_panel");
    if (vcoPanel == nullptr || lfoEfPanel == nullptr)
    {
        std::printf("FAIL: surface tree missing one or both dual scope panel nodes\n");
        return false;
    }
    if (vcoPanel->kind != synth::ui::NodeKind::Draw || lfoEfPanel->kind != synth::ui::NodeKind::Draw)
    {
        std::printf("FAIL: scope panel nodes must be NodeKind::Draw\n");
        return false;
    }
    if (vcoPanel->drawCommands.empty() || lfoEfPanel->drawCommands.empty())
    {
        std::printf("FAIL: scope panel nodes drew no commands\n");
        return false;
    }
    if (vcoPanel->id.value == lfoEfPanel->id.value)
    {
        std::printf("FAIL: VCO and LFO EF panels must be distinct nodes\n");
        return false;
    }
    return true;
}

bool test_surface_exposes_ganged_visualizer_on_random_sh_cells()
{
    FroggersAppSurface surface;
    const synth::ui::NodeTree tree = surface.BuildTree();

    const synth::ui::Node* cell1 = findNode(tree, "mod_depth_random_sh_1");
    const synth::ui::Node* cell2 = findNode(tree, "mod_depth_random_sh_2");
    if (cell1 == nullptr || cell2 == nullptr)
    {
        std::printf("FAIL: surface tree missing one or both Random S&H mod-depth cell nodes\n");
        return false;
    }
    if (cell1->kind != synth::ui::NodeKind::Draw || cell2->kind != synth::ui::NodeKind::Draw)
    {
        std::printf("FAIL: Random S&H mod-depth cell nodes must be NodeKind::Draw\n");
        return false;
    }
    if (cell1->drawCommands.empty() || cell2->drawCommands.empty())
    {
        std::printf("FAIL: Random S&H mod-depth cell nodes drew no commands\n");
        return false;
    }
    return true;
}

bool test_surface_tree_has_a_single_root_parenting_all_four_nodes()
{
    FroggersAppSurface surface;
    const synth::ui::NodeTree tree = surface.BuildTree();

    std::size_t rootCount = 0;
    const synth::ui::Node* root = nullptr;
    for (const auto& node : tree.nodes)
    {
        if (node.kind == synth::ui::NodeKind::Root)
        {
            ++rootCount;
            root = &node;
        }
    }
    if (rootCount != 1 || root == nullptr)
    {
        std::printf("FAIL: expected exactly one Root node, found %zu\n", rootCount);
        return false;
    }

    static constexpr std::array<const char*, 4> kExpectedChildIds{
        {"vco_scope_panel", "lfo_ef_scope_panel", "mod_depth_random_sh_1", "mod_depth_random_sh_2"}};
    for (const char* expectedId : kExpectedChildIds)
    {
        bool found = false;
        for (const auto& childId : root->children)
        {
            if (childId.value == expectedId)
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            std::printf("FAIL: root does not parent expected child '%s'\n", expectedId);
            return false;
        }
    }
    return true;
}

bool test_repeated_build_tree_is_stable()
{
    // BuildTree() may be called every UI frame by a future shell (tasks.md
    // section 10); it must not accumulate nodes or duplicate IDs run over
    // run.
    FroggersAppSurface surface;
    const synth::ui::NodeTree first = surface.BuildTree();
    const synth::ui::NodeTree second = surface.BuildTree();
    if (first.nodes.size() != second.nodes.size())
    {
        std::printf("FAIL: BuildTree() node count changed across calls (%zu vs %zu)\n",
                    first.nodes.size(),
                    second.nodes.size());
        return false;
    }
    return true;
}

bool test_surface_exposes_six_module_tabs()
{
    FroggersAppSurface surface;
    const synth::ui::NodeTree tree = surface.BuildTree();

    static constexpr std::array<const char*, 6> kExpectedIds{
        {"tab_audio", "tab_envelope", "tab_filter", "tab_drive", "tab_reverb", "tab_delay"}};
    static constexpr std::array<const char*, 6> kExpectedLabels{
        {"Audio", "Envelope", "Filter", "Drive", "Reverb", "Delay"}};

    for (std::size_t i = 0; i < kExpectedIds.size(); ++i)
    {
        const synth::ui::Node* tab = findNode(tree, kExpectedIds[i]);
        if (tab == nullptr)
        {
            std::printf("FAIL: surface tree missing module tab '%s'\n", kExpectedIds[i]);
            return false;
        }
        if (tab->kind != synth::ui::NodeKind::Toggle)
        {
            std::printf("FAIL: module tab '%s' must be NodeKind::Toggle\n", kExpectedIds[i]);
            return false;
        }
        if (tab->label != kExpectedLabels[i])
        {
            std::printf("FAIL: module tab '%s' label mismatch ('%s' != '%s')\n",
                        kExpectedIds[i],
                        tab->label.c_str(),
                        kExpectedLabels[i]);
            return false;
        }
    }

    const synth::ui::Node* audioTab = findNode(tree, "tab_audio");
    if (audioTab == nullptr || !audioTab->checked)
    {
        std::printf("FAIL: Audio tab should be the default active module\n");
        return false;
    }
    return true;
}

bool test_default_active_module_grid_shows_real_audio_labels()
{
    FroggersAppSurface surface;
    const synth::ui::NodeTree tree = surface.BuildTree();

    static constexpr std::array<const char*, 7> kExpectedAudioLabels{
        {"VCO1", "VCO2", "VCO3", "Phase mod 1", "Phase mod 2", "Phase mod 3", "Crispy"}};

    for (std::size_t slot = 0; slot < kExpectedAudioLabels.size(); ++slot)
    {
        const std::string labelId = "audio_grid_label_" + std::to_string(slot);
        const std::string ringId = "audio_grid_ring_" + std::to_string(slot);
        const synth::ui::Node* label = findNode(tree, labelId.c_str());
        const synth::ui::Node* ring = findNode(tree, ringId.c_str());
        if (label == nullptr || ring == nullptr)
        {
            std::printf("FAIL: audio grid slot %zu missing label or ring node\n", slot);
            return false;
        }
        if (label->text != kExpectedAudioLabels[slot])
        {
            std::printf("FAIL: audio grid slot %zu label mismatch ('%s' != '%s')\n",
                        slot,
                        label->text.c_str(),
                        kExpectedAudioLabels[slot]);
            return false;
        }
        if (ring->kind != synth::ui::NodeKind::Draw || ring->drawCommands.empty())
        {
            std::printf("FAIL: audio grid slot %zu ring drew no commands\n", slot);
            return false;
        }
    }

    if (findNode(tree, "audio_grid_stub") != nullptr)
    {
        std::printf("FAIL: Audio module must not render a stub placeholder\n");
        return false;
    }
    return true;
}

bool test_selecting_a_different_tab_moves_active_module_and_grid()
{
    FroggersAppSurface surface;
    surface.DispatchAction(synth::ui::Action::WithValue("select_module", "filter"));
    const synth::ui::NodeTree tree = surface.BuildTree();

    const synth::ui::Node* audioTab = findNode(tree, "tab_audio");
    const synth::ui::Node* filterTab = findNode(tree, "tab_filter");
    if (audioTab == nullptr || filterTab == nullptr)
    {
        std::printf("FAIL: expected both tab_audio and tab_filter nodes present\n");
        return false;
    }
    if (audioTab->checked || !filterTab->checked)
    {
        std::printf("FAIL: DispatchAction(select_module, filter) did not move active-module authority\n");
        return false;
    }
    if (findNode(tree, "audio_grid_label_0") != nullptr)
    {
        std::printf("FAIL: Audio grid must not render once Filter is active\n");
        return false;
    }
    // Packet 7 increment 2: Filter is now ported to the real reference
    // pattern, so it must NOT fall back to the "Coming next" stub anymore.
    if (findNode(tree, "filter_grid_stub") != nullptr)
    {
        std::printf("FAIL: Filter module must not render the stub placeholder anymore\n");
        return false;
    }
    const synth::ui::Node* filterLabel0 = findNode(tree, "filter_grid_label_0");
    if (filterLabel0 == nullptr || filterLabel0->text != "Comb offset")
    {
        std::printf("FAIL: Filter grid slot 0 should show real label 'Comb offset'\n");
        return false;
    }
    return true;
}

// Shared helper: builds the surface, selects moduleId's tab, and checks that
// every row in expectedLabels shows up as "<idPrefix>_grid_label_N" (text
// match) with a paired "<idPrefix>_grid_ring_N" Draw node that drew
// something -- the exact id-prefix convention BuildAudioModuleGrid
// established in increment 1 (tasks.md 7.2), now mechanically reused for
// Filter/Drive/Reverb/Delay (tasks.md 7.2 increment 2) reading the same
// FroggersV2ControlCore + V2DesktopPageDisplayNames source as Audio.
bool checkModuleGridRealLabels(const char* moduleId,
                                const char* idPrefix,
                                const std::vector<const char*>& expectedLabels)
{
    FroggersAppSurface surface;
    surface.DispatchAction(synth::ui::Action::WithValue("select_module", moduleId));
    const synth::ui::NodeTree tree = surface.BuildTree();

    for (std::size_t slot = 0; slot < expectedLabels.size(); ++slot)
    {
        const std::string labelId = std::string(idPrefix) + "_grid_label_" + std::to_string(slot);
        const std::string ringId = std::string(idPrefix) + "_grid_ring_" + std::to_string(slot);
        const synth::ui::Node* label = findNode(tree, labelId.c_str());
        const synth::ui::Node* ring = findNode(tree, ringId.c_str());
        if (label == nullptr || ring == nullptr)
        {
            std::printf("FAIL: %s grid slot %zu missing label or ring node\n", moduleId, slot);
            return false;
        }
        if (label->text != expectedLabels[slot])
        {
            std::printf("FAIL: %s grid slot %zu label mismatch ('%s' != '%s')\n",
                        moduleId,
                        slot,
                        label->text.c_str(),
                        expectedLabels[slot]);
            return false;
        }
        if (ring->kind != synth::ui::NodeKind::Draw || ring->drawCommands.empty())
        {
            std::printf("FAIL: %s grid slot %zu ring drew no commands\n", moduleId, slot);
            return false;
        }
    }

    const std::string stubId = std::string(idPrefix) + "_grid_stub";
    if (findNode(tree, stubId.c_str()) != nullptr)
    {
        std::printf("FAIL: %s module must not render a stub placeholder\n", moduleId);
        return false;
    }
    return true;
}

bool test_filter_module_grid_shows_real_labels()
{
    return checkModuleGridRealLabels(
        "filter",
        "filter",
        {"Comb offset", "Peak freq", "Peak gain", "Peak Q", "Comb delay", "Comb feedback",
         "Comb LP", "Comb/Peak", "Scoop", "Crispy"});
}

bool test_drive_module_grid_shows_real_labels()
{
    return checkModuleGridRealLabels(
        "drive",
        "drive",
        {"Drive", "Shape", "SRR 1", "SRR 2", "XOR", "Bit depth", "Fuzz", "Blend", "Phase", "Crispy"});
}

bool test_reverb_module_grid_shows_real_labels()
{
    return checkModuleGridRealLabels(
        "reverb",
        "reverb",
        {"Wet/dry", "Room size", "Decay", "Pre-delay", "Damping", "Stereo width", "Diffusion",
         "Mod depth", "Hold", "Crispy"});
}

bool test_delay_module_grid_shows_real_labels()
{
    return checkModuleGridRealLabels(
        "delay",
        "delay",
        {"Delay time", "Send", "Feedback", "Stereo width", "Detune", "Mod depth", "Wet mix",
         "Color", "Halo", "Crispy"});
}

bool test_envelope_module_grid_shows_real_labels()
{
    // Packet 7 increment 3 (tasks.md 7.5): Envelope is now ported to the real
    // reference pattern, same as the other five modules. Rows are per-VCO
    // Attack/Sustain/Release triplets using the task 7.5 full-word labels
    // ("Attack VCO1", not "Atk1"). Sustain rows were added same-day (D15
    // follow-up) once the shared-engine VcoAdsrState gained real sustain-level
    // semantics -- see V2DesktopPageDisplayNames.hpp's file-header note.
    return checkModuleGridRealLabels(
        "envelope",
        "envelope",
        {"Attack VCO1", "Sustain VCO1", "Release VCO1", "Attack VCO2", "Sustain VCO2",
         "Release VCO2", "Attack VCO3", "Sustain VCO3", "Release VCO3", "Crispy"});
}

bool test_no_pair_ar_label_remains_anywhere_in_the_surface()
{
    // Task 7.5 retires the "Pair-AR" naming in the desktop-v2 surface. Scans
    // every node's label/text and every Draw node's text draw commands (the
    // encoder rings' shortLabel is rendered through a Text draw command) for
    // the substring, across every module tab, not just Envelope's own grid.
    FroggersAppSurface surface;
    static constexpr std::array<const char*, 6> kAllModules{
        {"audio", "envelope", "filter", "drive", "reverb", "delay"}};

    for (const char* moduleId : kAllModules)
    {
        surface.DispatchAction(synth::ui::Action::WithValue("select_module", moduleId));
        const synth::ui::NodeTree tree = surface.BuildTree();
        for (const synth::ui::Node& node : tree.nodes)
        {
            if (node.label.find("Pair-AR") != std::string::npos ||
                node.text.find("Pair-AR") != std::string::npos)
            {
                std::printf("FAIL: node '%s' still carries a 'Pair-AR' label/text while '%s' active\n",
                            node.id.value.c_str(),
                            moduleId);
                return false;
            }
            for (const synth::ui::DrawCommand& command : node.drawCommands)
            {
                if (command.text.find("Pair-AR") != std::string::npos)
                {
                    std::printf(
                        "FAIL: node '%s' draw command still carries 'Pair-AR' text while '%s' active\n",
                        node.id.value.c_str(),
                        moduleId);
                    return false;
                }
            }
        }
    }
    return true;
}

// Packet 7 increment 4 (tasks.md 7.x, "mod-detail drill-in swap"): the
// layer-0<->layer-1 choice is driven entirely by FroggersV2ControlCore's own
// 2-deep gate (m_modView, packet 6's onModDrillIn/onParamPress) -- these
// tests push the same MessageIn::ModDrillIn / ParamPress messages
// SubmodulePagePanel::pushModDrillIn/pushPress push today, through the
// surface's already-exposed AudioControlCore() test accessor, and assert the
// swap follows without any new surface-owned flag.

bool test_mod_detail_grid_closed_shows_layer0_labels()
{
    FroggersAppSurface surface;
    const synth::ui::NodeTree tree = surface.BuildTree();

    const synth::ui::Node* label0 = findNode(tree, "audio_grid_label_0");
    if (label0 == nullptr || label0->text != "VCO1")
    {
        std::printf("FAIL: audio grid slot 0 should show layer-0 label 'VCO1' while mod view closed\n");
        return false;
    }
    if (findNode(tree, "audio_grid_label_15") != nullptr)
    {
        std::printf("FAIL: layer-0 audio grid must not render a 16th slot\n");
        return false;
    }
    return true;
}

bool test_mod_detail_grid_opens_via_control_core_drill_in()
{
    FroggersAppSurface surface;
    // Drill into Audio's page 0 (kModuleHostPage[kAudioModuleIndex] == 0), row
    // 0 (VCO1) -- same MessageIn::ModDrillIn the ring's centre-dot hit-zone
    // pushes via SubmodulePagePanel::pushModDrillIn.
    surface.AudioControlCore().bus().push(froggers_v2::MessageIn::ModDrillIn(0, 0));
    surface.AudioControlCore().processBus();

    const synth::ui::NodeTree tree = surface.BuildTree();

    const synth::ui::Node* lane0Label = findNode(tree, "audio_grid_label_0");
    if (lane0Label == nullptr)
    {
        std::printf("FAIL: mod-detail grid slot 0 missing\n");
        return false;
    }
    const char* expectedLane0 = froggers_v2::manifest::kPermanentModulationSources[0].displayName;
    if (lane0Label->text != expectedLane0)
    {
        std::printf("FAIL: mod-detail grid slot 0 should show lane 0's display name ('%s'), got '%s'\n",
                    expectedLane0,
                    lane0Label->text.c_str());
        return false;
    }
    const synth::ui::Node* lane0Ring = findNode(tree, "audio_grid_ring_0");
    if (lane0Ring == nullptr || lane0Ring->kind != synth::ui::NodeKind::Draw || lane0Ring->drawCommands.empty())
    {
        std::printf("FAIL: mod-detail grid slot 0 ring drew no commands\n");
        return false;
    }

    // kNumModSources (15) permanent lanes precede the dedicated Target/Back
    // cell, so it lands at slot 15 -- the 16th (and last) of the fixed
    // kUiSlots (4x4) grid.
    const synth::ui::Node* targetLabel = findNode(tree, "audio_grid_label_15");
    if (targetLabel == nullptr || targetLabel->text != std::string("Target (Back)"))
    {
        std::printf("FAIL: mod-detail grid slot 15 should be the Target/Back cell\n");
        return false;
    }
    const synth::ui::Node* targetRing = findNode(tree, "audio_grid_ring_15");
    if (targetRing == nullptr || targetRing->kind != synth::ui::NodeKind::Draw
        || targetRing->drawCommands.empty())
    {
        std::printf("FAIL: mod-detail grid slot 15 (Target/Back) ring drew no commands\n");
        return false;
    }

    // Tab selector + scope/chrome band are unaffected by the layer swap.
    const synth::ui::Node* audioTab = findNode(tree, "tab_audio");
    if (audioTab == nullptr || !audioTab->checked)
    {
        std::printf("FAIL: Audio tab should remain the checked active module while its mod view is open\n");
        return false;
    }
    if (findNode(tree, "vco_scope_panel") == nullptr)
    {
        std::printf("FAIL: scope panel must still render while the mod-detail grid is open\n");
        return false;
    }
    return true;
}

bool test_mod_detail_grid_closes_back_to_layer0_on_target_press()
{
    FroggersAppSurface surface;
    surface.AudioControlCore().bus().push(froggers_v2::MessageIn::ModDrillIn(0, 0));
    surface.AudioControlCore().processBus();

    const synth::ui::NodeTree treeOpen = surface.BuildTree();
    if (findNode(treeOpen, "audio_grid_label_15") == nullptr)
    {
        std::printf("FAIL: expected mod-detail grid open before Target/Back press\n");
        return false;
    }

    // Press the Target/Back cell (slot 15) -- same MessageIn::ParamPress
    // SubmodulePagePanel::pushPress pushes on a ring's centre-dot press;
    // onParamPress's isTarget branch is the only way back to layer 0.
    froggers_v2::MessageIn press;
    press.type = froggers_v2::MessageIn::Type::ParamPress;
    press.page = 0;
    press.slot = 15;
    surface.AudioControlCore().bus().push(press);
    surface.AudioControlCore().processBus();

    const synth::ui::NodeTree treeClosed = surface.BuildTree();
    const synth::ui::Node* label0 = findNode(treeClosed, "audio_grid_label_0");
    if (label0 == nullptr || label0->text != "VCO1")
    {
        std::printf("FAIL: Target/Back press did not return the grid to layer 0\n");
        return false;
    }
    if (findNode(treeClosed, "audio_grid_label_15") != nullptr)
    {
        std::printf("FAIL: layer-0 audio grid must not render a 16th slot after closing mod view\n");
        return false;
    }
    return true;
}

// Mod-view/tab-switch stale-state bug (FroggersV2ControlCore::SelectPage,
// control/FroggersV2ControlCore.cpp): switching module tabs while a
// mod-detail view is open used to leave m_modView open, so the newly
// selected module's grid kept rendering the stale 16-cell detail (with the
// old page's targetRow) instead of its own layer-0 grid. SelectPage now
// closes m_modView (open=false, targetRow=kNoSelection) alongside
// m_activePage, exactly like the dedicated Target/Back press already does.
bool test_selecting_a_different_tab_closes_stale_mod_detail_view()
{
    FroggersAppSurface surface;
    // Open Audio's mod-detail view (page 0, row 0 -- VCO1), same drill-in
    // path test_mod_detail_grid_opens_via_control_core_drill_in uses.
    surface.AudioControlCore().bus().push(froggers_v2::MessageIn::ModDrillIn(0, 0));
    surface.AudioControlCore().processBus();

    const synth::ui::NodeTree treeOpen = surface.BuildTree();
    if (findNode(treeOpen, "audio_grid_label_15") == nullptr)
    {
        std::printf("FAIL: expected Audio's mod-detail grid open before switching tabs\n");
        return false;
    }

    // Switch to Filter (kModuleHostPage[filter] == 2) while the mod view is
    // still open -- the same tab-selection path
    // test_selecting_a_different_tab_moves_active_module_and_grid uses.
    surface.DispatchAction(synth::ui::Action::WithValue("select_module", "filter"));
    const synth::ui::NodeTree treeAfterSwitch = surface.BuildTree();

    // Without the fix, Filter's grid would still render the stale 16-slot
    // mod-detail layout (a 16th slot, and/or the Target/Back label) carried
    // over from Audio's now-irrelevant targetRow, instead of Filter's own
    // <=10-slot layer-0 grid.
    if (findNode(treeAfterSwitch, "filter_grid_label_15") != nullptr)
    {
        std::printf(
            "FAIL: Filter must not render a stale 16-slot mod-detail grid after tab switch\n");
        return false;
    }
    const synth::ui::Node* filterLabel0 = findNode(treeAfterSwitch, "filter_grid_label_0");
    if (filterLabel0 == nullptr || filterLabel0->text != "Comb offset")
    {
        std::printf(
            "FAIL: Filter grid slot 0 should show real layer-0 label 'Comb offset' after tab "
            "switch, got '%s'\n",
            filterLabel0 == nullptr ? "<missing>" : filterLabel0->text.c_str());
        return false;
    }
    return true;
}

bool test_switching_between_modules_clears_previous_grid_nodes()
{
    // Tab selection renders each module's grid in turn, and only that
    // module's nodes: single active-module authority means no leftover
    // nodes from a previously active module's grid.
    FroggersAppSurface surface;
    static constexpr std::array<const char*, 6> kRealModules{
        {"audio", "envelope", "filter", "drive", "reverb", "delay"}};

    for (const char* moduleId : kRealModules)
    {
        surface.DispatchAction(synth::ui::Action::WithValue("select_module", moduleId));
        const synth::ui::NodeTree tree = surface.BuildTree();

        const std::string ownFirstLabelId = std::string(moduleId) + "_grid_label_0";
        if (findNode(tree, ownFirstLabelId.c_str()) == nullptr)
        {
            std::printf("FAIL: expected '%s' present once '%s' tab is active\n",
                        ownFirstLabelId.c_str(),
                        moduleId);
            return false;
        }
        for (const char* otherModuleId : kRealModules)
        {
            if (otherModuleId == moduleId)
            {
                continue;
            }
            const std::string otherFirstLabelId = std::string(otherModuleId) + "_grid_label_0";
            if (findNode(tree, otherFirstLabelId.c_str()) != nullptr)
            {
                std::printf("FAIL: '%s' still present while '%s' tab is active\n",
                            otherFirstLabelId.c_str(),
                            moduleId);
                return false;
            }
        }
    }
    return true;
}

} // namespace

int main()
{
    bool ok = true;
    ok = test_surface_exposes_dual_scope_panels() && ok;
    ok = test_surface_exposes_ganged_visualizer_on_random_sh_cells() && ok;
    ok = test_surface_tree_has_a_single_root_parenting_all_four_nodes() && ok;
    ok = test_repeated_build_tree_is_stable() && ok;
    ok = test_surface_exposes_six_module_tabs() && ok;
    ok = test_default_active_module_grid_shows_real_audio_labels() && ok;
    ok = test_selecting_a_different_tab_moves_active_module_and_grid() && ok;
    ok = test_filter_module_grid_shows_real_labels() && ok;
    ok = test_drive_module_grid_shows_real_labels() && ok;
    ok = test_reverb_module_grid_shows_real_labels() && ok;
    ok = test_delay_module_grid_shows_real_labels() && ok;
    ok = test_envelope_module_grid_shows_real_labels() && ok;
    ok = test_no_pair_ar_label_remains_anywhere_in_the_surface() && ok;
    ok = test_switching_between_modules_clears_previous_grid_nodes() && ok;
    ok = test_mod_detail_grid_closed_shows_layer0_labels() && ok;
    ok = test_mod_detail_grid_opens_via_control_core_drill_in() && ok;
    ok = test_mod_detail_grid_closes_back_to_layer0_on_target_press() && ok;
    ok = test_selecting_a_different_tab_closes_stale_mod_detail_view() && ok;

    if (!ok)
    {
        std::printf("FAIL: FroggersAppSurface_test\n");
        return 1;
    }
    std::printf("PASS: FroggersAppSurface_test\n");
    return 0;
}
