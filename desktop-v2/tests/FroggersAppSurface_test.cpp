// FroggersAppSurface_test -- packet 5 (openspec/changes/desktop-v2-sheaf-
// runtime-harmonization, tasks.md 5.1-5.4).
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
// Does NOT wire into Main.cpp / MainComponent (shell cutover is tasks.md
// section 10, a later packet).

#include "ui/FroggersAppSurface.hpp"

#include <array>
#include <cstdio>

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

} // namespace

int main()
{
    bool ok = true;
    ok = test_surface_exposes_dual_scope_panels() && ok;
    ok = test_surface_exposes_ganged_visualizer_on_random_sh_cells() && ok;
    ok = test_surface_tree_has_a_single_root_parenting_all_four_nodes() && ok;
    ok = test_repeated_build_tree_is_stable() && ok;

    if (!ok)
    {
        std::printf("FAIL: FroggersAppSurface_test\n");
        return 1;
    }
    std::printf("PASS: FroggersAppSurface_test\n");
    return 0;
}
