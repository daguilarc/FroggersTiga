#include "ui/SubmodulePagePanel.hpp"

#include "V2ParamDisplayNames.hpp"

SubmodulePagePanel::SubmodulePagePanel()
{
    m_randomize.onClick = [this]() {
        if (onRandomize)
        {
            onRandomize(m_page);
        }
    };
    m_randomizeMod.onClick = [this]() {
        if (onRandomizeMod)
        {
            onRandomizeMod(m_page);
        }
    };

    addAndMakeVisible(m_randomize);
    addAndMakeVisible(m_randomizeMod);

    m_encoderViewport.setViewedComponent(&m_encoderContent, false);
    m_encoderViewport.setScrollBarsShown(true, false);
    addAndMakeVisible(m_encoderViewport);

    for (int i = 0; i < DesktopV2ChromeLayout::kVisibleEncoderSlots; ++i)
    {
        m_rowLabels[static_cast<size_t>(i)].setJustificationType(juce::Justification::centredLeft);
        m_encoderContent.addAndMakeVisible(m_rowLabels[static_cast<size_t>(i)]);
        m_encoderContent.addAndMakeVisible(m_rings[static_cast<size_t>(i)]);
        m_encoderContent.addAndMakeVisible(m_modCells[static_cast<size_t>(i)]);
    }

    wireEncoderCallbacks();
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
    for (int i = 0; i < DesktopV2ChromeLayout::kVisibleEncoderSlots; ++i)
    {
        const uint8_t slot = static_cast<uint8_t>(i);
        m_rings[static_cast<size_t>(i)].setSlot(slot);
        m_rings[static_cast<size_t>(i)].onTurn = [this](uint8_t slotIn, float delta) { pushTurn(slotIn, delta); };
        m_rings[static_cast<size_t>(i)].onPress = [this](uint8_t slotIn) { pushPress(slotIn); };
        m_modCells[static_cast<size_t>(i)].onAssign = [this](uint8_t row, uint8_t source) {
            pushModAssign(row, source);
        };
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

void SubmodulePagePanel::pushModAssign(uint8_t row, uint8_t internalSource)
{
    if (!m_core)
    {
        return;
    }
    froggers_v2::MessageIn message;
    message.type = froggers_v2::MessageIn::Type::ModSourceAssign;
    message.page = m_page;
    message.slot = row;
    message.index = internalSource;
    m_core->bus().push(message);
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
    for (int i = 0; i < DesktopV2ChromeLayout::kVisibleEncoderSlots; ++i)
    {
        const bool active = i < static_cast<int>(visible);
        m_rowLabels[static_cast<size_t>(i)].setVisible(active);
        m_rings[static_cast<size_t>(i)].setVisible(active);
        m_modCells[static_cast<size_t>(i)].setVisible(active);
        if (!active)
        {
            continue;
        }
        const uint8_t row = m_core->visibleRowForSlot(static_cast<uint8_t>(i));
        m_rowLabels[static_cast<size_t>(i)].setText(
            V2ParamDisplayNames::forHostPageRow(m_page, row),
            juce::dontSendNotification);
        m_rings[static_cast<size_t>(i)].refreshFromState(state);
        m_modCells[static_cast<size_t>(i)].setRow(row);
        m_modCells[static_cast<size_t>(i)].setAssignedSource(m_core->assignedModSource(m_page, row));
        m_modCells[static_cast<size_t>(i)].refresh();
    }
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

void SubmodulePagePanel::layoutRows()
{
    auto area = getLocalBounds().reduced(DesktopV2ChromeLayout::kChromePad);
    auto btnRow = area.removeFromTop(DesktopV2ChromeLayout::kTextButtonH);
    m_randomize.setBounds(btnRow.removeFromLeft(DesktopV2ChromeLayout::kRandomizeButtonW));
    btnRow.removeFromLeft(DesktopV2ChromeLayout::kSectionGap);
    m_randomizeMod.setBounds(btnRow.removeFromLeft(DesktopV2ChromeLayout::kRandModButtonW));
    area.removeFromTop(DesktopV2ChromeLayout::kSectionGap);

    m_encoderViewport.setBounds(area);

    const int rows = documentRowCount();
    const int contentW = m_encoderViewport.getWidth();
    const int contentH = DesktopV2ChromeLayout::encoderDocumentHeight(rows);
    m_encoderContent.setSize(contentW, contentH);

    auto rowArea = m_encoderContent.getLocalBounds();
    for (int i = 0; i < rows && i < DesktopV2ChromeLayout::kVisibleEncoderSlots; ++i)
    {
        auto rowBounds = rowArea.removeFromTop(DesktopV2ChromeLayout::kEncoderRowH);
        m_rowLabels[static_cast<size_t>(i)].setBounds(
            rowBounds.removeFromLeft(DesktopV2ChromeLayout::kRowLabelW));
        m_modCells[static_cast<size_t>(i)].setBounds(rowBounds.removeFromRight(DesktopV2ChromeLayout::kModCellW));
        rowBounds.removeFromRight(DesktopV2ChromeLayout::kSectionGap);
        const int ringSide = juce::jmin(DesktopV2ChromeLayout::kEncoderRingSize, rowBounds.getWidth());
        m_rings[static_cast<size_t>(i)].setBounds(rowBounds.withSizeKeepingCentre(ringSide, ringSide));
    }
}
