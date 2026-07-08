#include "runtime/AudioRuntimePageComponent.h"

AudioRuntimePageComponent::AudioRuntimePageComponent(AudioEngine& engine)
    : m_engine(engine)
    , m_settings(engine, {}, AudioSettingsPresentation::RuntimePage)
{
    m_title.setText("Audio", juce::dontSendNotification);
    m_title.setFont(juce::Font(16.0f, juce::Font::bold));

    for (juce::Label* label :
         {&m_outputChannelsLabel, &m_inputChannelsLabel, &m_blockSizeLabel, &m_busLayoutLabel,
          &m_outputStateLabel, &m_inputStateLabel})
    {
        label->setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(label);
    }

    addAndMakeVisible(m_title);
    addAndMakeVisible(m_settings);
    refreshProjection();
    startTimerHz(4);
}

void AudioRuntimePageComponent::refreshProjection()
{
    const DesktopV2AudioStateProjection projection = buildStandaloneAudioStateProjection(m_engine);
    m_outputChannelsLabel.setText(
        "Output channels: " + juce::String(projection.activeOutputChannels), juce::dontSendNotification);
    m_inputChannelsLabel.setText(
        "Input channels: " + juce::String(projection.activeInputChannels), juce::dontSendNotification);
    m_blockSizeLabel.setText("Block size: " + juce::String(projection.blockSize), juce::dontSendNotification);
    m_busLayoutLabel.setText("Bus layout: " + projection.busLayout, juce::dontSendNotification);
    m_outputStateLabel.setText("Output state: " + projection.outputState, juce::dontSendNotification);
    m_inputStateLabel.setText("Input state: " + projection.inputState, juce::dontSendNotification);
}

void AudioRuntimePageComponent::timerCallback()
{
    refreshProjection();
}

void AudioRuntimePageComponent::layoutProjectionLabels(juce::Rectangle<int>& area)
{
    m_title.setBounds(area.removeFromTop(22));
    area.removeFromTop(6);
    m_outputChannelsLabel.setBounds(area.removeFromTop(16));
    m_inputChannelsLabel.setBounds(area.removeFromTop(16));
    m_blockSizeLabel.setBounds(area.removeFromTop(16));
    m_busLayoutLabel.setBounds(area.removeFromTop(16));
    m_outputStateLabel.setBounds(area.removeFromTop(16));
    m_inputStateLabel.setBounds(area.removeFromTop(16));
    area.removeFromTop(8);
}

void AudioRuntimePageComponent::resized()
{
    auto area = getLocalBounds().reduced(8);
    layoutProjectionLabels(area);
    m_settings.setBounds(area);
}
