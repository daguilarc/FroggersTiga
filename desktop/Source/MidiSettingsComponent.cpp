#include "MidiSettingsComponent.h"

namespace
{
constexpr int kChannelControlWidth = 50;
constexpr int kCcControlWidth = 80;
constexpr int kCcTextBoxWidth = 44;
constexpr int kRowControlHeight = 24;
} // namespace

MidiSettingsComponent::MidiSettingsComponent(AudioEngine& engine, std::function<void()> onClose)
    : m_engine(engine)
    , m_onClose(std::move(onClose))
{
    m_inSectionLabel.setText("MIDI In", juce::dontSendNotification);
    m_inSectionLabel.setFont(juce::Font(14.0f, juce::Font::bold));
    m_inLabel.setText("Device", juce::dontSendNotification);
    m_inCc1GroupLabel.setText("MIDI CC 1", juce::dontSendNotification);
    m_inCc1GroupLabel.setFont(juce::Font(12.0f, juce::Font::bold));
    m_inCh1Label.setText("Ch", juce::dontSendNotification);
    m_inCc1Label.setText("CC", juce::dontSendNotification);
    m_inCc2GroupLabel.setText("MIDI CC 2", juce::dontSendNotification);
    m_inCc2GroupLabel.setFont(juce::Font(12.0f, juce::Font::bold));
    m_inCh2Label.setText("Ch", juce::dontSendNotification);
    m_inCc2Label.setText("CC", juce::dontSendNotification);
    m_inStatus.setJustificationType(juce::Justification::centredLeft);

    m_outSectionLabel.setText("MIDI Out (VCO Env)", juce::dontSendNotification);
    m_outSectionLabel.setFont(juce::Font(14.0f, juce::Font::bold));
    m_outLabel.setText("Device", juce::dontSendNotification);
    m_outHelp.setText(
        "Sends VCO envelope level to a physical MIDI port when a device is selected.",
        juce::dontSendNotification);
    m_outHelp.setJustificationType(juce::Justification::topLeft);
    m_outCcLabel.setText("CC", juce::dontSendNotification);
    m_outChLabel.setText("Channel", juce::dontSendNotification);

    auto& bridge = m_engine.getHost().m_midiBridge;

    m_inChannel1.setRange(1, 16, 1);
    m_inChannel1.setValue(bridge.m_inChannel1 + 1, juce::dontSendNotification);
    m_inChannel1.onValueChange = [this]() {
        m_engine.getHost().m_midiBridge.m_inChannel1 =
            static_cast<uint8_t>(m_inChannel1.getValue() - 1);
    };

    m_inCc1.setRange(0, 127, 1);
    m_inCc1.setValue(bridge.m_inCc1, juce::dontSendNotification);
    m_inCc1.onValueChange = [this]() {
        m_engine.getHost().m_midiBridge.m_inCc1 = static_cast<uint8_t>(m_inCc1.getValue());
    };

    m_inChannel2.setRange(1, 16, 1);
    m_inChannel2.setValue(bridge.m_inChannel2 + 1, juce::dontSendNotification);
    m_inChannel2.onValueChange = [this]() {
        m_engine.getHost().m_midiBridge.m_inChannel2 =
            static_cast<uint8_t>(m_inChannel2.getValue() - 1);
    };

    m_inCc2.setRange(0, 127, 1);
    m_inCc2.setValue(bridge.m_inCc2, juce::dontSendNotification);
    m_inCc2.onValueChange = [this]() {
        m_engine.getHost().m_midiBridge.m_inCc2 = static_cast<uint8_t>(m_inCc2.getValue());
    };

    configureCcSlider(m_inCc1);
    configureCcSlider(m_inCc2);

    m_outCc.setRange(0, 127, 1);
    m_outCc.setValue(bridge.m_outCc, juce::dontSendNotification);
    m_outCc.onValueChange = [this]() {
        m_engine.getHost().m_midiBridge.m_outCc = static_cast<uint8_t>(m_outCc.getValue());
    };
    configureCcSlider(m_outCc);

    m_outChannel.setRange(1, 16, 1);
    m_outChannel.setValue(bridge.m_outChannel + 1, juce::dontSendNotification);
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
                               static_cast<juce::Component*>(&m_inCc1GroupLabel),
                               static_cast<juce::Component*>(&m_inCh1Label),
                               static_cast<juce::Component*>(&m_inChannel1),
                               static_cast<juce::Component*>(&m_inCc1Label),
                               static_cast<juce::Component*>(&m_inCc1),
                               static_cast<juce::Component*>(&m_inCc2GroupLabel),
                               static_cast<juce::Component*>(&m_inCh2Label),
                               static_cast<juce::Component*>(&m_inChannel2),
                               static_cast<juce::Component*>(&m_inCc2Label),
                               static_cast<juce::Component*>(&m_inCc2),
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
    setSize(520, 380);
}

void MidiSettingsComponent::configureCcSlider(juce::Slider& slider)
{
    slider.setSliderStyle(juce::Slider::LinearHorizontal);
    slider.setTextBoxStyle(
        juce::Slider::TextBoxRight, false, kCcTextBoxWidth, kRowControlHeight - 4);
    slider.setNumDecimalPlacesToDisplay(0);
}

void MidiSettingsComponent::timerCallback()
{
    updateStatus();
}

void MidiSettingsComponent::updateStatus()
{
    if (m_engine.getMidiInputDeviceIdentifier().isEmpty())
    {
        m_inStatus.setText("No MIDI input device selected.", juce::dontSendNotification);
        m_inStatus.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
        return;
    }
    if (m_engine.isHardwareMidiInputOpenFailed())
    {
        m_inStatus.setText("Could not open selected MIDI input device.", juce::dontSendNotification);
        m_inStatus.setColour(juce::Label::textColourId, juce::Colours::orange);
        return;
    }
    m_inStatus.setText("Hardware MIDI in active.", juce::dontSendNotification);
    m_inStatus.setColour(juce::Label::textColourId, juce::Colours::lightgreen);
}

void MidiSettingsComponent::refreshDeviceLists()
{
    const juce::String prevIn = m_engine.getMidiInputDeviceIdentifier();
    const juce::String prevOut = m_engine.getMidiOutputDeviceIdentifier();

    m_inDevice.clear();
    m_outDevice.clear();
    m_inDevice.addItem("None", 1);
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

    if (prevIn.isEmpty())
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
        m_engine.setMidiInputDevice({});
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

    auto cc1Row = area.removeFromTop(24);
    m_inCc1GroupLabel.setBounds(cc1Row.removeFromLeft(80));
    m_inCh1Label.setBounds(cc1Row.removeFromLeft(24));
    m_inChannel1.setBounds(cc1Row.removeFromLeft(kChannelControlWidth));
    cc1Row.removeFromLeft(8);
    m_inCc1Label.setBounds(cc1Row.removeFromLeft(24));
    m_inCc1.setBounds(cc1Row.removeFromLeft(kCcControlWidth));
    area.removeFromTop(4);

    auto cc2Row = area.removeFromTop(24);
    m_inCc2GroupLabel.setBounds(cc2Row.removeFromLeft(80));
    m_inCh2Label.setBounds(cc2Row.removeFromLeft(24));
    m_inChannel2.setBounds(cc2Row.removeFromLeft(kChannelControlWidth));
    cc2Row.removeFromLeft(8);
    m_inCc2Label.setBounds(cc2Row.removeFromLeft(24));
    m_inCc2.setBounds(cc2Row.removeFromLeft(kCcControlWidth));

    area.removeFromTop(6);
    m_inStatus.setBounds(area.removeFromTop(18));
    area.removeFromTop(10);

    m_outSectionLabel.setBounds(area.removeFromTop(20));
    area.removeFromTop(4);
    m_outLabel.setBounds(area.removeFromTop(16));
    m_outDevice.setBounds(area.removeFromTop(24));
    area.removeFromTop(4);
    m_outHelp.setBounds(area.removeFromTop(24));
    area.removeFromTop(6);

    auto outRow = area.removeFromTop(24);
    m_outChLabel.setBounds(outRow.removeFromLeft(60));
    m_outChannel.setBounds(outRow.removeFromLeft(kChannelControlWidth));
    outRow.removeFromLeft(8);
    m_outCcLabel.setBounds(outRow.removeFromLeft(30));
    m_outCc.setBounds(outRow.removeFromLeft(kCcControlWidth));

    area.removeFromTop(12);
    m_close.setBounds(area.removeFromTop(28).removeFromRight(80));
}
