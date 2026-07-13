#include "runtime/ControllersRuntimePageComponent.h"

#include "control/MidiCvAssignmentTable.hpp"

namespace
{
juce::String connectionStateLabel(ControllerConnectionState state)
{
    switch (state)
    {
        case ControllerConnectionState::Connected:
            return "Connected";
        case ControllerConnectionState::Receiving:
            return "Receiving";
        case ControllerConnectionState::Error:
            return "Error";
        case ControllerConnectionState::Disconnected:
        default:
            return "Disconnected";
    }
}
} // namespace

ControllersRuntimePageComponent::ControllersRuntimePageComponent(AudioEngine& engine)
    : m_engine(engine)
    , m_settings(engine, {}, MidiCvSettingsPresentation::RuntimePage)
{
    m_title.setText("MIDI / Controllers", juce::dontSendNotification);
    m_title.setFont(juce::Font(16.0f, juce::Font::bold));
    m_inputStateLabel.setJustificationType(juce::Justification::centredLeft);
    m_persistenceLabel.setJustificationType(juce::Justification::centredLeft);
    m_fanOutLabel.setJustificationType(juce::Justification::centredLeft);

    m_viewport.setViewedComponent(&m_settings, false);
    m_viewport.setScrollBarsShown(true, false);

    addAndMakeVisible(m_title);
    addAndMakeVisible(m_inputStateLabel);
    addAndMakeVisible(m_persistenceLabel);
    addAndMakeVisible(m_fanOutLabel);
    addAndMakeVisible(m_viewport);

    refreshStatus();
    startTimerHz(4);
}

void ControllersRuntimePageComponent::refreshStatus()
{
    const MidiCvAssignmentTable& table = m_engine.getMidiCvTable();
    m_inputStateLabel.setText(
        "Input: " + juce::String(table.selectedInputLabel()) + " — "
            + connectionStateLabel(table.inputConnectionState()),
        juce::dontSendNotification);
    m_persistenceLabel.setText(
        "Persistence: " + juce::String(table.mappingPersistenceMessage()), juce::dontSendNotification);

    size_t fanOutRows = 0;
    for (const ControllerTargetMappingRow& row : table.buildTargetMappingRows())
    {
        if (row.fanOutCount > 1)
        {
            ++fanOutRows;
        }
    }
    m_fanOutLabel.setText("Multi-target fan-out rows: " + juce::String(static_cast<int>(fanOutRows)),
                          juce::dontSendNotification);
}

void ControllersRuntimePageComponent::timerCallback()
{
    refreshStatus();
}

void ControllersRuntimePageComponent::resized()
{
    auto area = getLocalBounds().reduced(8);
    m_title.setBounds(area.removeFromTop(22));
    area.removeFromTop(4);
    m_inputStateLabel.setBounds(area.removeFromTop(16));
    m_persistenceLabel.setBounds(area.removeFromTop(16));
    m_fanOutLabel.setBounds(area.removeFromTop(16));
    area.removeFromTop(6);
    m_viewport.setBounds(area);
    m_settings.setSize(area.getWidth(),
                       juce::jmax(area.getHeight(), m_settings.preferredContentHeight()));
}
