#include "ui/SubmodulePagePanel.hpp"

#include "V2DesktopPageDisplayNames.hpp"
#include "V2ParamDisplayNames.hpp"
#include "manifest/FroggersV2AppManifest.hpp"
#include "ui/ModDetailGridLayout.hpp"
#include "ui/ModDetailUnderlayBind.hpp"

SubmodulePagePanel::SubmodulePagePanel()
{
    // Per-page Randomize/Randmod header buttons removed (desktop-v2
    // "Randomization affordances are not duplicated"): global Rand All /
    // Rand Mods in the command band are the single surface now.

    m_encoderViewport.setViewedComponent(&m_encoderContent, false);
    m_encoderViewport.setScrollBarsShown(false, false);
    addAndMakeVisible(m_encoderViewport);

    for (int i = 0; i < kCellCapacity; ++i)
    {
        m_rowLabels[static_cast<size_t>(i)].setJustificationType(juce::Justification::centredLeft);
        m_encoderContent.addAndMakeVisible(m_rowLabels[static_cast<size_t>(i)]);
        m_encoderContent.addAndMakeVisible(m_rings[static_cast<size_t>(i)]);
    }

    wireEncoderCallbacks();
}

bool SubmodulePagePanel::detailGridOpen() const
{
    // The parameter-detail grid is the only state that produces the full
    // kUiSlots (16) visible-cell count; module pages have at most 10 rows.
    return m_core != nullptr && m_core->visibleCount() == froggers_v2::kUiSlots;
}

void SubmodulePagePanel::setPage(uint8_t page)
{
    m_page = page;
    refresh();
}

void SubmodulePagePanel::bindCore(froggers_v2::FroggersV2ControlCore* core)
{
    m_core = core;
    refresh();
}

void SubmodulePagePanel::bindLaneHistory(const CvLaneHistoryStore* store)
{
    m_laneHistory = store;
    refresh();
}

uint8_t SubmodulePagePanel::rowCountForPage(uint8_t page) const
{
    if (page == 0)
    {
        return 8;
    }
    if (page < froggers_v2::kNumHostPages)
    {
        return V2ParamDisplayNames::kV2ExpandedNumRows;
    }
    return 0;
}

int SubmodulePagePanel::documentRowCount() const
{
    if (m_core)
    {
        return static_cast<int>(m_core->visibleCount());
    }
    return static_cast<int>(rowCountForPage(m_page));
}

void SubmodulePagePanel::wireEncoderCallbacks()
{
    for (int i = 0; i < kCellCapacity; ++i)
    {
        const uint8_t slot = static_cast<uint8_t>(i);
        m_rings[static_cast<size_t>(i)].setSlot(slot);
        m_rings[static_cast<size_t>(i)].onTurn = [this](uint8_t slotIn, float delta) { pushTurn(slotIn, delta); };
        m_rings[static_cast<size_t>(i)].onPress = [this](uint8_t slotIn) { pushPress(slotIn); };
        m_rings[static_cast<size_t>(i)].onModDrillIn = [this](uint8_t slotIn) { pushModDrillIn(slotIn); };
    }
}

void SubmodulePagePanel::pushTurn(uint8_t slot, float delta)
{
    if (!m_core)
    {
        return;
    }
    froggers_v2::MessageIn message = froggers_v2::MessageIn::ParamTurn(m_page, slot, delta);
    m_core->bus().push(message);
    m_core->processBus();
}

void SubmodulePagePanel::pushPress(uint8_t slot)
{
    if (!m_core)
    {
        return;
    }
    froggers_v2::MessageIn message;
    message.type = froggers_v2::MessageIn::Type::ParamPress;
    message.page = m_page;
    message.slot = slot;
    m_core->bus().push(message);
    m_core->processBus();
}

void SubmodulePagePanel::pushModDrillIn(uint8_t row)
{
    if (!m_core)
    {
        return;
    }
    m_core->bus().push(froggers_v2::MessageIn::ModDrillIn(m_page, row));
    m_core->processBus();
}

void SubmodulePagePanel::refresh()
{
    if (!m_core)
    {
        return;
    }
    const auto& state = m_core->uiState();
    const uint8_t visible = state.visibleCount.load(std::memory_order_acquire);
    const bool detail = detailGridOpen();
    const uint8_t detailRow = state.modViewTargetRow.load(std::memory_order_acquire);
    const bool externalAudio = m_core->externalAudioAvailable();
    const juce::Colour labelAvailable(0xffe6edf3);
    const juce::Colour labelUnavailable = labelAvailable.withAlpha(0.35f);
    for (int i = 0; i < kCellCapacity; ++i)
    {
        const bool active = i < static_cast<int>(visible);
        m_rowLabels[static_cast<size_t>(i)].setVisible(active);
        m_rings[static_cast<size_t>(i)].setVisible(active);
        if (!active)
        {
            continue;
        }
        m_rings[static_cast<size_t>(i)].refreshFromState(state);
        if (detail)
        {
            const bool isTarget = m_core->visibleSlotIsTarget(static_cast<uint8_t>(i));
            const uint8_t lane = m_core->visibleModIndexForSlot(static_cast<uint8_t>(i));
            const bool laneAvailable = isTarget
                || (lane < froggers_v2::kNumModSources
                    && froggers_v2::manifest::isModLaneAssignable(
                        m_page, detailRow, lane, externalAudio));
            m_rings[static_cast<size_t>(i)].setLaneAvailable(laneAvailable);
            m_rowLabels[static_cast<size_t>(i)].setColour(
                juce::Label::textColourId, laneAvailable ? labelAvailable : labelUnavailable);
            const char* label = isTarget || lane >= froggers_v2::kNumModSources
                ? "Target (Back)"
                : froggers_v2::manifest::kPermanentModulationSources[lane].displayName;
            m_rowLabels[static_cast<size_t>(i)].setText(label, juce::dontSendNotification);
            continue;
        }
        m_rings[static_cast<size_t>(i)].setLaneAvailable(true);
        m_rowLabels[static_cast<size_t>(i)].setColour(juce::Label::textColourId, labelAvailable);
        const uint8_t row = m_core->visibleRowForSlot(static_cast<uint8_t>(i));
        m_rowLabels[static_cast<size_t>(i)].setText(
            V2DesktopPageDisplayNames::forHostPageRow(m_page, row),
            juce::dontSendNotification);
    }
    desktop_v2::bindDetailUnderlays(m_rings, m_core, m_laneHistory, detail);
    layoutRows();
    repaint();
}

void SubmodulePagePanel::paint(juce::Graphics& g)
{
    g.setColour(juce::Colour(0xff3d444d));
    g.drawRect(getLocalBounds(), 1);
}

void SubmodulePagePanel::resized()
{
    layoutRows();
}

void SubmodulePagePanel::layoutDetailGrid(juce::Rectangle<int> area)
{
    using namespace froggers_v2::ui;
    // The whole encoder region becomes the 4x4 grid; any scroll is
    // suppressed in detail mode.
    m_scrollBarsVisible = false;
    m_encoderViewport.setScrollBarsShown(false, false);
    m_encoderViewport.setBounds(area);
    m_encoderContent.setSize(area.getWidth(), area.getHeight());
    m_encoderViewport.setViewPosition(0, 0);

    const juce::Rectangle<int> gridArea(0, 0, area.getWidth(), area.getHeight());
    for (int i = 0; i < kCellCapacity; ++i)
    {
        auto cell = modDetailCellBounds(gridArea, i).reduced(kModDetailCellPad);
        auto labelStrip = cell.removeFromTop(kModDetailLabelStripH);
        m_rowLabels[static_cast<size_t>(i)].setBounds(labelStrip);
        m_rowLabels[static_cast<size_t>(i)].setJustificationType(juce::Justification::centred);
        const int side = juce::jmin(cell.getWidth(), cell.getHeight());
        m_rings[static_cast<size_t>(i)].setBounds(cell.withSizeKeepingCentre(side, side));
    }
}

void SubmodulePagePanel::layoutRows()
{
    auto area = getLocalBounds();
    area.removeFromLeft(DesktopV2ChromeLayout::kChromePad);
    area.removeFromRight(DesktopV2ChromeLayout::kChromePad);
    const auto columns = DesktopV2ChromeLayout::moduleRowColumns(area.getWidth());

    if (detailGridOpen())
    {
        layoutDetailGrid(area);
        return;
    }

    m_encoderViewport.setBounds(area.withWidth(columns.labelEncoderW));

    const int rows = documentRowCount();
    const int docH = DesktopV2ChromeLayout::encoderDocumentHeight(rows);
    m_encoderContent.setSize(columns.labelEncoderW, docH);

    const bool needsScroll = docH > m_encoderViewport.getHeight();
    m_scrollBarsVisible = needsScroll;
    m_encoderViewport.setScrollBarsShown(needsScroll, false);
    if (!needsScroll)
    {
        m_encoderViewport.setViewPosition(0, 0);
    }

    int rowY = 0;
    for (int i = 0; i < rows && i < DesktopV2ChromeLayout::kVisibleEncoderSlots; ++i)
    {
        const int rowH = DesktopV2ChromeLayout::kEncoderRowH;
        const int ringY = rowY + (rowH - DesktopV2ChromeLayout::kEncoderRingSize) / 2;

        m_rowLabels[static_cast<size_t>(i)].setBounds(
            DesktopV2ChromeLayout::kModuleRowLabelOffset, rowY, columns.labelW, rowH);
        m_rings[static_cast<size_t>(i)].setBounds(
            DesktopV2ChromeLayout::kModuleRowEncoderOffset,
            ringY,
            columns.encoderW,
            DesktopV2ChromeLayout::kEncoderRingSize);

        rowY += rowH;
    }
}

bool SubmodulePagePanel::encoderViewportShowsVerticalScrollbar() const
{
    return m_scrollBarsVisible;
}

juce::Rectangle<int> SubmodulePagePanel::modCellBoundsInPanel(int) const
{
    // Packet 15-C2 (D12): the mod-source dropdown column is retired, so
    // there is no mod cell to report bounds for. Callers (LayoutBounds_test
    // via PageCarouselComponent::modCellBoundsInCarousel) already treat an
    // empty rectangle as "nothing to check here".
    return {};
}
