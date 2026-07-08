#include "runtime/HostedRuntimeStatusPanel.h"

HostedRuntimeStatusPanel::HostedRuntimeStatusPanel(AudioEngine& engine, juce::AudioProcessor* processor)
    : m_engine(engine)
    , m_processor(processor)
{
    m_toggle.onClick = [this]() { toggleExpanded(); };
    m_audioTitle.setText("Hosted Audio", juce::dontSendNotification);
    m_midiTitle.setText("Hosted MIDI", juce::dontSendNotification);
    m_audioTitle.setFont(juce::Font(13.0f, juce::Font::bold));
    m_midiTitle.setFont(juce::Font(13.0f, juce::Font::bold));

    addAndMakeVisible(m_toggle);
    for (juce::Label* label :
         {&m_audioTitle, &m_audioBusLabel, &m_audioChannelsLabel, &m_audioRateLabel, &m_audioBlockLabel,
          &m_audioInputStateLabel, &m_audioOutputStateLabel, &m_midiTitle, &m_midiSummaryLabel})
    {
        label->setJustificationType(juce::Justification::centredLeft);
        addChildComponent(label);
    }

    refreshLabels();
    startTimerHz(4);
}

void HostedRuntimeStatusPanel::setProcessor(juce::AudioProcessor* processor)
{
    m_processor = processor;
    refreshLabels();
}

void HostedRuntimeStatusPanel::toggleExpanded()
{
    m_expanded = !m_expanded;
    for (juce::Component* component :
         {static_cast<juce::Component*>(&m_audioTitle),
          static_cast<juce::Component*>(&m_audioBusLabel),
          static_cast<juce::Component*>(&m_audioChannelsLabel),
          static_cast<juce::Component*>(&m_audioRateLabel),
          static_cast<juce::Component*>(&m_audioBlockLabel),
          static_cast<juce::Component*>(&m_audioInputStateLabel),
          static_cast<juce::Component*>(&m_audioOutputStateLabel),
          static_cast<juce::Component*>(&m_midiTitle),
          static_cast<juce::Component*>(&m_midiSummaryLabel)})
    {
        component->setVisible(m_expanded);
    }
    resized();
}

void HostedRuntimeStatusPanel::refreshLabels()
{
    if (m_processor == nullptr)
    {
        m_audioBusLabel.setText("Host buses: unavailable", juce::dontSendNotification);
        m_midiSummaryLabel.setText("MIDI mapping: host parameters only", juce::dontSendNotification);
        return;
    }

    const DesktopV2AudioStateProjection projection =
        buildHostedAudioStateProjection(m_engine, *m_processor);
    m_audioBusLabel.setText("Bus layout: " + projection.busLayout, juce::dontSendNotification);
    m_audioChannelsLabel.setText(
        "Channels: in " + juce::String(projection.activeInputChannels) + ", out "
            + juce::String(projection.activeOutputChannels),
        juce::dontSendNotification);
    m_audioRateLabel.setText("Sample rate: " + juce::String(static_cast<int>(projection.sampleRate)) + " Hz",
                             juce::dontSendNotification);
    m_audioBlockLabel.setText("Block size: " + juce::String(projection.blockSize), juce::dontSendNotification);
    m_audioInputStateLabel.setText("Input: " + projection.inputState, juce::dontSendNotification);
    m_audioOutputStateLabel.setText("Output: " + projection.outputState, juce::dontSendNotification);
    m_midiSummaryLabel.setText("MIDI mapping: DAW host-parameter semantics only",
                               juce::dontSendNotification);
}

void HostedRuntimeStatusPanel::timerCallback()
{
    refreshLabels();
}

void HostedRuntimeStatusPanel::resized()
{
    auto area = getLocalBounds();
    m_toggle.setBounds(area.removeFromTop(24));
    if (!m_expanded)
    {
        return;
    }
    area.removeFromTop(4);
    m_audioTitle.setBounds(area.removeFromTop(18));
    m_audioBusLabel.setBounds(area.removeFromTop(16));
    m_audioChannelsLabel.setBounds(area.removeFromTop(16));
    m_audioRateLabel.setBounds(area.removeFromTop(16));
    m_audioBlockLabel.setBounds(area.removeFromTop(16));
    m_audioInputStateLabel.setBounds(area.removeFromTop(16));
    m_audioOutputStateLabel.setBounds(area.removeFromTop(16));
    area.removeFromTop(6);
    m_midiTitle.setBounds(area.removeFromTop(18));
    m_midiSummaryLabel.setBounds(area.removeFromTop(16));
}
