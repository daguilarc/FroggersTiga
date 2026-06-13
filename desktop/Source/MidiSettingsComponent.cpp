#include "MidiSettingsComponent.h"

MidiSettingsComponent::MidiSettingsComponent(AudioEngine& engine, std::function<void()> onClose)
    : m_engine(engine)
    , m_onClose(std::move(onClose))
{
    m_inSectionLabel.setText("MIDI In", juce::dontSendNotification);
    m_inSectionLabel.setFont(juce::Font(14.0f, juce::Font::bold));
    m_inLabel.setText("Device", juce::dontSendNotification);
    m_inDevice.setTooltip(
        "Computer keyboard: QWERTY piano keys drive the MIDI mod jack (patch cables). "
        "Select a hardware device to use an external keyboard instead.");
    m_inChLabel.setText("Channel", juce::dontSendNotification);
    m_inCcLabel.setText("In CC", juce::dontSendNotification);
    m_inCcLabel.setTooltip(
        "Notes drive pitch CV x velocity on the MIDI mod jack (highest held note wins). "
        "CC controllers on this channel override when sent.");
    m_inLegend.setText(
        "Piano: A W S E D F T G Y H U J K O L P  (white + black keys)\n"
        "Each key steps pitch CV on the scope when the MIDI jack is patched (A = lowest step).",
        juce::dontSendNotification);
    m_inLegend.setJustificationType(juce::Justification::topLeft);
    m_inStatus.setJustificationType(juce::Justification::centredLeft);

    m_outSectionLabel.setText("MIDI Out (VCO Env)", juce::dontSendNotification);
    m_outSectionLabel.setFont(juce::Font(14.0f, juce::Font::bold));
    m_outLabel.setText("Device", juce::dontSendNotification);
    m_outHelp.setText(
        "Sends VCO envelope level to a physical MIDI port when a device is selected. "
        "Keyboard notes never leave this port.",
        juce::dontSendNotification);
    m_outHelp.setJustificationType(juce::Justification::topLeft);
    m_outCcLabel.setText("CC", juce::dontSendNotification);
    m_outChLabel.setText("Channel", juce::dontSendNotification);

    m_inChannel.setRange(1, 16, 1);
    m_inChannel.setValue(m_engine.getHost().m_midiBridge.m_inChannel + 1, juce::dontSendNotification);
    m_inChannel.onValueChange = [this]() {
        m_engine.getHost().m_midiBridge.m_inChannel =
            static_cast<uint8_t>(m_inChannel.getValue() - 1);
    };

    m_inCc.setRange(0, 127, 1);
    m_inCc.setValue(m_engine.getHost().m_midiBridge.m_inCc, juce::dontSendNotification);
    m_inCc.onValueChange = [this]() {
        m_engine.getHost().m_midiBridge.m_inCc = static_cast<uint8_t>(m_inCc.getValue());
    };

    m_outCc.setRange(0, 127, 1);
    m_outCc.setValue(m_engine.getHost().m_midiBridge.m_outCc, juce::dontSendNotification);
    m_outCc.onValueChange = [this]() {
        m_engine.getHost().m_midiBridge.m_outCc = static_cast<uint8_t>(m_outCc.getValue());
    };

    m_outChannel.setRange(1, 16, 1);
    m_outChannel.setValue(m_engine.getHost().m_midiBridge.m_outChannel + 1, juce::dontSendNotification);
    m_outChannel.onValueChange = [this]() {
        m_engine.getHost().m_midiBridge.m_outChannel =
            static_cast<uint8_t>(m_outChannel.getValue() - 1);
    };

    m_inDevice.onChange = [this]() { applyInputDevice(); };
    m_outDevice.onChange = [this]() { applyOutputDevice(); };
    m_refresh.onClick = [this]() { refreshDeviceLists(); };
    m_close.onClick = [this]() {
        if (m_onClose)
        {
            m_onClose();
        }
    };

    for (juce::Component* c : {static_cast<juce::Component*>(&m_inSectionLabel),
                               static_cast<juce::Component*>(&m_inLabel),
                               static_cast<juce::Component*>(&m_inDevice),
                               static_cast<juce::Component*>(&m_refresh),
                               static_cast<juce::Component*>(&m_inChLabel),
                               static_cast<juce::Component*>(&m_inChannel),
                               static_cast<juce::Component*>(&m_inCcLabel),
                               static_cast<juce::Component*>(&m_inCc),
                               static_cast<juce::Component*>(&m_inLegend),
                               static_cast<juce::Component*>(&m_inStatus),
                               static_cast<juce::Component*>(&m_outSectionLabel),
                               static_cast<juce::Component*>(&m_outLabel),
                               static_cast<juce::Component*>(&m_outDevice),
                               static_cast<juce::Component*>(&m_outHelp),
                               static_cast<juce::Component*>(&m_outCcLabel),
                               static_cast<juce::Component*>(&m_outCc),
                               static_cast<juce::Component*>(&m_outChLabel),
                               static_cast<juce::Component*>(&m_outChannel),
                               static_cast<juce::Component*>(&m_close)})
    {
        addAndMakeVisible(c);
    }

    refreshDeviceLists();
    updateStatus();
    startTimerHz(4);
    setSize(480, 420);
}

void MidiSettingsComponent::timerCallback()
{
    updateStatus();
}

void MidiSettingsComponent::updateStatus()
{
    if (m_engine.isHardwareMidiInputOpenFailed())
    {
        m_inStatus.setText("Could not open selected MIDI input device.", juce::dontSendNotification);
        m_inStatus.setColour(juce::Label::textColourId, juce::Colours::orange);
    }
    else if (m_engine.isComputerKeyboardMidiEnabled())
    {
        m_inStatus.setText("Computer keyboard → MIDI mod jack (not physical MIDI out).",
                           juce::dontSendNotification);
        m_inStatus.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    }
    else
    {
        m_inStatus.setText("Hardware MIDI in active.", juce::dontSendNotification);
        m_inStatus.setColour(juce::Label::textColourId, juce::Colours::lightgreen);
    }
}

void MidiSettingsComponent::refreshDeviceLists()
{
    const juce::String prevIn = m_engine.getMidiInputDeviceIdentifier();
    const juce::String prevOut = m_engine.getMidiOutputDeviceIdentifier();

    m_inDevice.clear();
    m_outDevice.clear();
    m_inDevice.addItem("Computer keyboard", 1);
    int inIdx = 2;
    for (const auto& dev : juce::MidiInput::getAvailableDevices())
    {
        m_inDevice.addItem(dev.name, inIdx++);
    }

    m_outDevice.addItem("None", 1);
    int outIdx = 2;
    for (const auto& dev : juce::MidiOutput::getAvailableDevices())
    {
        m_outDevice.addItem(dev.name, outIdx++);
    }

    if (m_engine.isComputerKeyboardMidiEnabled())
    {
        m_inDevice.setSelectedId(1, juce::dontSendNotification);
    }
    else
    {
        int selectId = 1;
        int id = 2;
        for (const auto& dev : juce::MidiInput::getAvailableDevices())
        {
            if (dev.identifier == prevIn)
            {
                selectId = id;
                break;
            }
            id++;
        }
        m_inDevice.setSelectedId(selectId, juce::dontSendNotification);
    }

    if (prevOut.isEmpty() || prevOut == AudioEngine::kNoMidiOutId)
    {
        m_outDevice.setSelectedId(1, juce::dontSendNotification);
    }
    else
    {
        int selectId = 1;
        int id = 2;
        for (const auto& dev : juce::MidiOutput::getAvailableDevices())
        {
            if (dev.identifier == prevOut)
            {
                selectId = id;
                break;
            }
            id++;
        }
        m_outDevice.setSelectedId(selectId, juce::dontSendNotification);
    }

    updateStatus();
}

void MidiSettingsComponent::applyInputDevice()
{
    const int id = m_inDevice.getSelectedId();
    if (id < 1)
    {
        return;
    }
    if (id == 1)
    {
        m_engine.setMidiInputDevice(juce::String(AudioEngine::kComputerKeyboardMidiId));
        updateStatus();
        return;
    }
    const auto devices = juce::MidiInput::getAvailableDevices();
    const int deviceIndex = id - 2;
    if (deviceIndex >= 0 && deviceIndex < devices.size())
    {
        m_engine.setMidiInputDevice(devices[static_cast<size_t>(deviceIndex)].identifier);
    }
    updateStatus();
}

void MidiSettingsComponent::applyOutputDevice()
{
    const int id = m_outDevice.getSelectedId();
    if (id < 1)
    {
        return;
    }
    if (id == 1)
    {
        m_engine.setMidiOutputDevice(juce::String(AudioEngine::kNoMidiOutId));
        return;
    }
    const auto devices = juce::MidiOutput::getAvailableDevices();
    const int deviceIndex = id - 2;
    if (deviceIndex >= 0 && deviceIndex < devices.size())
    {
        m_engine.setMidiOutputDevice(devices[static_cast<size_t>(deviceIndex)].identifier);
    }
}

void MidiSettingsComponent::resized()
{
    auto area = getLocalBounds().reduced(12);
    m_inSectionLabel.setBounds(area.removeFromTop(20));
    area.removeFromTop(4);
    m_inLabel.setBounds(area.removeFromTop(16));
    auto inDevRow = area.removeFromTop(24);
    m_inDevice.setBounds(inDevRow.removeFromLeft(inDevRow.getWidth() - 120));
    inDevRow.removeFromLeft(6);
    m_refresh.setBounds(inDevRow);
    area.removeFromTop(6);

    auto inChRow = area.removeFromTop(24);
    m_inChLabel.setBounds(inChRow.removeFromLeft(70));
    m_inChannel.setBounds(inChRow.removeFromLeft(50));
    inChRow.removeFromLeft(8);
    m_inCcLabel.setBounds(inChRow.removeFromLeft(50));
    m_inCc.setBounds(inChRow.removeFromLeft(50));

    area.removeFromTop(6);
    m_inLegend.setBounds(area.removeFromTop(36));
    area.removeFromTop(4);
    m_inStatus.setBounds(area.removeFromTop(18));
    area.removeFromTop(10);

    m_outSectionLabel.setBounds(area.removeFromTop(20));
    area.removeFromTop(4);
    m_outLabel.setBounds(area.removeFromTop(16));
    m_outDevice.setBounds(area.removeFromTop(24));
    area.removeFromTop(4);
    m_outHelp.setBounds(area.removeFromTop(32));
    area.removeFromTop(6);

    auto outRow = area.removeFromTop(24);
    m_outChLabel.setBounds(outRow.removeFromLeft(60));
    m_outChannel.setBounds(outRow.removeFromLeft(50));
    outRow.removeFromLeft(8);
    m_outCcLabel.setBounds(outRow.removeFromLeft(30));
    m_outCc.setBounds(outRow.removeFromLeft(50));

    area.removeFromTop(12);
    m_close.setBounds(area.removeFromTop(28).removeFromRight(80));
}
