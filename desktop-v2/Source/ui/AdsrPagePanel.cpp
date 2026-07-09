#include "ui/AdsrPagePanel.hpp"

#include "V2ParamDisplayNames.hpp"
#include "manifest/FroggersV2AppManifest.hpp"
#include "ui/ModDetailGridLayout.hpp"

AdsrPagePanel::AdsrPagePanel()
{
    m_randomize.onClick = [this]() {
        if (onRandomize)
        {
            onRandomize();
        }
    };
    m_randomizeMod.onClick = [this]() {
        if (onRandomizeMod)
        {
            onRandomizeMod();
        }
    };

    addAndMakeVisible(m_randomize);
    addAndMakeVisible(m_randomizeMod);

    m_encoderViewport.setViewedComponent(&m_encoderContent, false);
    m_encoderViewport.setScrollBarsShown(false, false);
    addAndMakeVisible(m_encoderViewport);

    m_modColumnViewport.setViewedComponent(&m_modColumnContent, false);
    m_modColumnViewport.setScrollBarsShown(false, false);
    addAndMakeVisible(m_modColumnViewport);

    for (int i = 0; i < kCellCapacity; ++i)
    {
        m_rowLabels[static_cast<size_t>(i)].setJustificationType(juce::Justification::centredLeft);
        m_encoderContent.addAndMakeVisible(m_rowLabels[static_cast<size_t>(i)]);
        m_encoderContent.addAndMakeVisible(m_rings[static_cast<size_t>(i)]);
        m_modColumnContent.addAndMakeVisible(m_modCells[static_cast<size_t>(i)]);

        const uint8_t slot = static_cast<uint8_t>(i);
        m_rings[static_cast<size_t>(i)].setSlot(slot);
        m_rings[static_cast<size_t>(i)].onTurn = [this](uint8_t slotIn, float delta) { pushTurn(slotIn, delta); };
        m_rings[static_cast<size_t>(i)].onPress = [this](uint8_t slotIn) { pushPress(slotIn); };
        m_modCells[static_cast<size_t>(i)].onAssign = [this](uint8_t row, uint8_t source) {
            pushModAssign(row, source);
        };
    }
}

void AdsrPagePanel::bindCore(froggers_v2::FroggersV2ControlCore* core)
{
    m_core = core;
    refresh();
}

int AdsrPagePanel::documentRowCount() const
{
    if (m_core)
    {
        return static_cast<int>(m_core->visibleCount());
    }
    return static_cast<int>(V2ParamDisplayNames::kV2ExpandedNumRows);
}

bool AdsrPagePanel::detailGridOpen() const
{
    return m_core != nullptr && m_core->visibleCount() == froggers_v2::kUiSlots;
}

void AdsrPagePanel::pushTurn(uint8_t slot, float delta)
{
    if (!m_core)
    {
        return;
    }
    m_core->bus().push(froggers_v2::MessageIn::ParamTurn(kAdsrPage, slot, delta));
    m_core->processBus();
}

void AdsrPagePanel::pushPress(uint8_t slot)
{
    if (!m_core)
    {
        return;
    }
    froggers_v2::MessageIn message;
    message.type = froggers_v2::MessageIn::Type::ParamPress;
    message.page = kAdsrPage;
    message.slot = slot;
    m_core->bus().push(message);
    m_core->processBus();
}

void AdsrPagePanel::pushModAssign(uint8_t row, uint8_t internalSource)
{
    if (!m_core)
    {
        return;
    }
    froggers_v2::MessageIn message;
    message.type = froggers_v2::MessageIn::Type::ModSourceAssign;
    message.page = kAdsrPage;
    message.slot = row;
    message.index = internalSource;
    m_core->bus().push(message);
    m_core->processBus();
}

void AdsrPagePanel::refresh()
{
    if (!m_core)
    {
        return;
    }
    const auto& state = m_core->uiState();
    const uint8_t visible = state.visibleCount.load(std::memory_order_acquire);
    const bool detail = detailGridOpen();
    for (int i = 0; i < kCellCapacity; ++i)
    {
        const bool active = i < static_cast<int>(visible);
        m_rowLabels[static_cast<size_t>(i)].setVisible(active);
        m_rings[static_cast<size_t>(i)].setVisible(active);
        m_modCells[static_cast<size_t>(i)].setVisible(active && !detail);
        if (!active)
        {
            continue;
        }
        m_rings[static_cast<size_t>(i)].refreshFromState(state);
        if (detail)
        {
            const bool isTarget = m_core->visibleSlotIsTarget(static_cast<uint8_t>(i));
            const uint8_t lane = m_core->visibleModIndexForSlot(static_cast<uint8_t>(i));
            const char* label = isTarget || lane >= froggers_v2::kNumModSources
                ? "Target"
                : froggers_v2::manifest::kPermanentModulationSources[lane].displayName;
            m_rowLabels[static_cast<size_t>(i)].setText(label, juce::dontSendNotification);
            continue;
        }
        const uint8_t row = m_core->visibleRowForSlot(static_cast<uint8_t>(i));
        m_rowLabels[static_cast<size_t>(i)].setText(
            V2ParamDisplayNames::forHostPageRow(kAdsrPage, row),
            juce::dontSendNotification);
        m_modCells[static_cast<size_t>(i)].setPage(kAdsrPage);
        m_modCells[static_cast<size_t>(i)].setRow(row);
        m_modCells[static_cast<size_t>(i)].setExternalAudioAvailable(m_core->externalAudioAvailable());
        m_modCells[static_cast<size_t>(i)].setAssignedSource(m_core->assignedModSource(kAdsrPage, row));
        m_modCells[static_cast<size_t>(i)].refresh();
    }
    layoutRows();
    repaint();
}

void AdsrPagePanel::paint(juce::Graphics& g)
{
    g.setColour(juce::Colour(0xff3d444d));
    g.drawRect(getLocalBounds(), 1);
}

void AdsrPagePanel::resized()
{
    layoutRows();
}

void AdsrPagePanel::layoutDetailGrid(juce::Rectangle<int> area)
{
    using namespace froggers_v2::ui;
    m_scrollBarsVisible = false;
    m_encoderViewport.setScrollBarsShown(false, false);
    m_modColumnViewport.setScrollBarsShown(false, false);
    m_modColumnViewport.setBounds({});
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

void AdsrPagePanel::layoutRows()
{
    auto area = getLocalBounds();
    area.removeFromLeft(DesktopV2ChromeLayout::kChromePad);
    area.removeFromRight(DesktopV2ChromeLayout::kChromePad);
    const auto columns = DesktopV2ChromeLayout::moduleRowColumns(area.getWidth());

    auto btnRow = area.removeFromTop(DesktopV2ChromeLayout::kTextButtonH);
    m_randomize.setBounds(btnRow.removeFromLeft(DesktopV2ChromeLayout::kRandomizeButtonW));
    btnRow.removeFromLeft(DesktopV2ChromeLayout::kSectionGap);
    m_randomizeMod.setBounds(btnRow.removeFromLeft(DesktopV2ChromeLayout::kRandModButtonW));
    area.removeFromTop(DesktopV2ChromeLayout::kSectionGap);

    if (detailGridOpen())
    {
        layoutDetailGrid(area);
        return;
    }

    auto modColumnArea = area.removeFromRight(columns.modW);
    m_encoderViewport.setBounds(area.withWidth(columns.labelEncoderW));
    m_modColumnViewport.setBounds(modColumnArea);

    const int rows = documentRowCount();
    const int docH = DesktopV2ChromeLayout::encoderDocumentHeight(rows);
    m_encoderContent.setSize(columns.labelEncoderW, docH);
    m_modColumnContent.setSize(columns.modW, docH);

    const bool needsScroll = docH > m_encoderViewport.getHeight();
    m_scrollBarsVisible = needsScroll;
    m_encoderViewport.setScrollBarsShown(needsScroll, false);
    m_modColumnViewport.setScrollBarsShown(needsScroll, false);
    if (!needsScroll)
    {
        m_encoderViewport.setViewPosition(0, 0);
        m_modColumnViewport.setViewPosition(0, 0);
    }

    int rowY = 0;
    for (int i = 0; i < rows && i < DesktopV2ChromeLayout::kVisibleEncoderSlots; ++i)
    {
        const int rowH = DesktopV2ChromeLayout::kEncoderRowH;
        const int ringY = rowY + (rowH - DesktopV2ChromeLayout::kEncoderRingSize) / 2;
        const int modY = rowY + (rowH - DesktopV2ChromeLayout::kModCellHeight) / 2;

        m_rowLabels[static_cast<size_t>(i)].setBounds(
            DesktopV2ChromeLayout::kModuleRowLabelOffset, rowY, columns.labelW, rowH);
        m_rings[static_cast<size_t>(i)].setBounds(
            DesktopV2ChromeLayout::kModuleRowEncoderOffset,
            ringY,
            columns.encoderW,
            DesktopV2ChromeLayout::kEncoderRingSize);
        m_modCells[static_cast<size_t>(i)].setBounds(0, modY, columns.modW, DesktopV2ChromeLayout::kModCellHeight);

        rowY += rowH;
    }
}

bool AdsrPagePanel::encoderViewportShowsVerticalScrollbar() const
{
    return m_scrollBarsVisible;
}

juce::Rectangle<int> AdsrPagePanel::modCellBoundsInPanel(int rowIndex) const
{
    if (rowIndex < 0 || rowIndex >= DesktopV2ChromeLayout::kVisibleEncoderSlots)
    {
        return {};
    }
    auto bounds = m_modCells[static_cast<size_t>(rowIndex)].getBounds();
    bounds = bounds.translated(m_modColumnContent.getX(), m_modColumnContent.getY());
    bounds = m_modColumnViewport.getLocalArea(&m_modColumnContent, bounds);
    return getLocalArea(&m_modColumnViewport, bounds);
}
