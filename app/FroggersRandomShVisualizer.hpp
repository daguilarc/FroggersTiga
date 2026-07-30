#pragma once

// synth_froggers::RandomShLaneVisualizer -- packet 8 (tasks.md section
// "8. Marbles: clock + visualizer", task 8.3; design D8a). The five X-style
// Random S&H sources' visualizer: "a new Visualizer rendering the
// remembered loop as a waveform with a playhead at the current index."
// Sheaf ships nothing for this -- DspRandomLfo has no loop concept, and
// GangedRandomLfoVisualizer draws an *extrapolated future* path, not
// remembered history (design D8a) -- so this is new UI code, reading
// dsp::RandomShLane::UiState (app/dsp/RandomShLane.hpp, populated once per
// block via RandomShLane::PopulateUiState()).
//
// This is the ONLY bespoke drawing code this change adds anywhere in the
// app -- the VCO scopes (packet 7) and the bump/comb transfer-function
// plots (packet 9, FroggersTransferFunctionVisualizer.hpp) are both
// generic Visualizer subclasses sampling a Sheaf-shaped contract
// (ScopeVisualizer<UIState> and a from-scratch TransferFunction sampler,
// respectively) -- but no ported/Sheaf equivalent exists for "remembered
// loop with a playhead," so per design D8a this is authored, not reused.

#include "dsp/RandomShLane.hpp"

#include "synth/Color.hpp"
#include "synth/PortableUI.hpp"

#include <algorithm>
#include <cstddef>
#include <vector>

namespace synth_froggers {

class RandomShLaneVisualizer final : public synth::ui::Visualizer {
public:
    RandomShLaneVisualizer(const dsp::RandomShLane::UiState& uiState, synth::Color color)
        : uiState_(&uiState), color_(color) {}

protected:
    std::vector<synth::ui::DrawCommand> DrawVisible() const override {
        const std::size_t rawSize = uiState_->size.load(std::memory_order_relaxed);
        const std::size_t size = std::clamp<std::size_t>(rawSize, 1, dsp::RandomShLane::kNumSlots);
        const std::size_t currentIndex = uiState_->currentIndex.load(std::memory_order_relaxed) % size;
        const synth::ui::Bounds bounds = GetBounds();

        std::vector<synth::ui::Point> points;
        points.reserve(size);
        for (std::size_t i = 0; i < size; ++i) {
            const float value =
                std::clamp(uiState_->slots[i].load(std::memory_order_relaxed), 0.0f, 1.0f);
            const float xFrac =
                size > 1 ? static_cast<float>(i) / static_cast<float>(size - 1) : 0.0f;
            const float x = bounds.x + bounds.width * xFrac;
            const float y = bounds.y + bounds.height * (1.0f - value);
            points.push_back({x, y});
        }

        std::vector<synth::ui::DrawCommand> commands;
        if (points.empty()) {
            return commands;
        }
        commands.push_back(synth::ui::DrawCommand::Polyline(points, color_, 1.4f));

        // Playhead marker at the current index -- the same "small filled
        // circle at the live position" convention
        // GangedRandomLfoVisualizer uses for source #6 (design D8a's
        // requirement that both visualizer kinds read as related).
        constexpr float kDotRadius = 3.0f;
        const synth::ui::Point playhead = points[std::min(currentIndex, points.size() - 1)];
        commands.push_back(synth::ui::DrawCommand::FillEllipse(
            synth::ui::Bounds{playhead.x - kDotRadius, playhead.y - kDotRadius,
                               kDotRadius * 2.0f, kDotRadius * 2.0f},
            color_));
        return commands;
    }

private:
    const dsp::RandomShLane::UiState* uiState_;
    synth::Color color_;
};

}  // namespace synth_froggers
