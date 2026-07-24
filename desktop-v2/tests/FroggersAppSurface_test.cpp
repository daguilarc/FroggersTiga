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
// Does NOT wire into Main.cpp / MainComponent (shell cutover is tasks.md
// section 10, a later packet). Does not build the other five modules' real
// grids yet (later increments).

#include "ui/FroggersAppSurface.hpp"

#include <array>
#include <cstdio>
#include <string>

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
    const synth::ui::Node* stub = findNode(tree, "filter_grid_stub");
    if (stub == nullptr || stub->kind != synth::ui::NodeKind::Label)
    {
        std::printf("FAIL: Filter module should render a labeled stub while active\n");
        return false;
    }
    if (stub->text.find("Filter") == std::string::npos)
    {
        std::printf("FAIL: Filter stub text does not name the module ('%s')\n", stub->text.c_str());
        return false;
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

    if (!ok)
    {
        std::printf("FAIL: FroggersAppSurface_test\n");
        return 1;
    }
    std::printf("PASS: FroggersAppSurface_test\n");
    return 0;
}
