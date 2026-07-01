#include "MidiCvSettingsComponent.h"

#include "V2ParamDisplayNames.hpp"
#include "control/FroggersV2ControlCore.hpp"

namespace
{
constexpr int kChannelControlWidth = 50;
constexpr int kCcControlWidth = 80;
constexpr int kCcTextBoxWidth = 44;
constexpr int kRowControlHeight = 24;
constexpr int kBindingLabelWidth = 80;
constexpr int kBindingEnableWidth = 40;
constexpr int kBindingChLabelWidth = 24;
constexpr int kBindingKindWidth = 72;

void configureChannelSlider(juce::Slider& slider)
{
    slider.setRange(0, 16, 1);
    slider.setSliderStyle(juce::Slider::LinearHorizontal);
    slider.setTextBoxStyle(
        juce::Slider::TextBoxRight, false, kCcTextBoxWidth, kRowControlHeight - 4);
    slider.setNumDecimalPlacesToDisplay(0);
}

void configureAnyChannelSlider(juce::Slider& slider)
{
    configureChannelSlider(slider);
    slider.textFromValueFunction = [](double value) {
        if (value < 0.5)
        {
            return juce::String("Any");
        }
        return juce::String(static_cast<int>(value));
    };
    slider.valueFromTextFunction = [](const juce::String& text) {
        if (text.equalsIgnoreCase("Any"))
        {
            return 0.0;
        }
        return text.getDoubleValue();
    };
}

void configureNumberSlider(juce::Slider& slider, int maxValue)
{
    slider.setRange(0, maxValue, 1);
    slider.setSliderStyle(juce::Slider::LinearHorizontal);
    slider.setTextBoxStyle(
        juce::Slider::TextBoxRight, false, kCcTextBoxWidth, kRowControlHeight - 4);
    slider.setNumDecimalPlacesToDisplay(0);
}

MidiCvTriggerKind triggerKindFromCombo(const juce::ComboBox& combo)
{
    return combo.getSelectedId() == 2 ? MidiCvTriggerKind::Cc : MidiCvTriggerKind::Note;
}

void setTriggerKindCombo(juce::ComboBox& combo, MidiCvTriggerKind kind)
{
    combo.setSelectedId(kind == MidiCvTriggerKind::Cc ? 2 : 1, juce::dontSendNotification);
}

MidiCvButtonBinding* bindingForRow(MidiCvAssignmentTable& table, size_t index)
{
    if (index == 0)
    {
        return &table.shiftButton;
    }
    return &table.sceneButtons[index - 1];
}

MidiCvButtonTarget targetForRow(size_t index)
{
    if (index == 0)
    {
        return MidiCvButtonTarget::Shift;
    }
    return static_cast<MidiCvButtonTarget>(static_cast<uint8_t>(MidiCvButtonTarget::Scene1) + index - 1);
}
} // namespace

MidiCvSettingsComponent::MidiCvSettingsComponent(AudioEngine& engine, std::function<void()> onClose)
    : m_engine(engine)
    , m_onClose(std::move(onClose))
{
    m_inSectionLabel.setText("MIDI In", juce::dontSendNotification);
    m_inSectionLabel.setFont(juce::Font(14.0f, juce::Font::bold));
    m_inHelp.setText(
        "Select one MIDI input stream: computer keyboard, none, or a hardware port.",
        juce::dontSendNotification);
    m_inHelp.setJustificationType(juce::Justification::topLeft);
    m_inLabel.setText("Device", juce::dontSendNotification);
    m_inDevice.setTooltip(
        "Computer keyboard: QWERTY keys send notes on the virtual MIDI channel. "
        "Select a hardware device for external MIDI instead.");
    m_inStatus.setJustificationType(juce::Justification::centredLeft);

    m_assignSectionLabel.setText("CV Assignments", juce::dontSendNotification);
    m_assignSectionLabel.setFont(juce::Font(14.0f, juce::Font::bold));
    m_assignHelp.setText(
        "Rows below map messages from that input to pitch, gate, CC modulators, and performance "
        "triggers.",
        juce::dontSendNotification);
    m_assignHelp.setJustificationType(juce::Justification::topLeft);
    m_pitchPageLabel.setText("Page", juce::dontSendNotification);
    m_pitchRowLabel.setText("Row", juce::dontSendNotification);
    m_pitchRow.setRange(0, 9, 1);
    m_pitchRow.setSliderStyle(juce::Slider::LinearHorizontal);
    m_pitchRow.setTextBoxStyle(juce::Slider::TextBoxRight, false, 36, kRowControlHeight - 4);

    for (uint8_t page = 0; page < froggers_v2::kNumHostPages; ++page)
    {
        m_pitchPage.addItem(V2ParamDisplayNames::forHostPage(page), static_cast<int>(page) + 1);
    }

    m_gateHelp.setText(
        "Note on/off drives ADSR and sequencer gates.", juce::dontSendNotification);
    m_gateHelp.setJustificationType(juce::Justification::topLeft);

    m_extModALabel.setText("MIDI CC A", juce::dontSendNotification);
    m_extModBLabel.setText("MIDI CC B", juce::dontSendNotification);
    m_extModALabel.setTooltip("Incoming CC level assignable as MIDI CC A in module mod menus.");
    m_extModBLabel.setTooltip("Incoming CC level assignable as MIDI CC B in module mod menus.");
    m_extModAChLabel.setText("Ch", juce::dontSendNotification);
    m_extModACcLabel.setText("CC", juce::dontSendNotification);
    m_extModBChLabel.setText("Ch", juce::dontSendNotification);
    m_extModBCcLabel.setText("CC", juce::dontSendNotification);
    configureAnyChannelSlider(m_extModAChannel);
    configureAnyChannelSlider(m_extModBChannel);
    configureCcSlider(m_extModACc);
    configureCcSlider(m_extModBCc);

    static constexpr const char* kBindingNames[] = {
        "Shift button", "Scene S1", "Scene S2", "Scene S3"};
    for (size_t i = 0; i < m_bindingRows.size(); ++i)
    {
        BindingRowUi& row = m_bindingRows[i];
        row.label.setText(kBindingNames[i], juce::dontSendNotification);
        row.chLabel.setText("Ch", juce::dontSendNotification);
        configureAnyChannelSlider(row.channel);
        row.kind.addItem("Note", 1);
        row.kind.addItem("CC", 2);
        row.kind.setSelectedId(1, juce::dontSendNotification);
        configureNumberSlider(row.number, 127);
    }

    m_qwertyChLabel.setText("Ch", juce::dontSendNotification);
    configureChannelSlider(m_qwertyChannel);
    m_qwertyChannel.setRange(1, 16, 1);

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
        syncTableFromUi();
        if (m_onClose)
        {
            m_onClose();
        }
    };

    auto syncOnChange = [this]() {
        syncTableFromUi();
        updatePitchTargetLabel();
    };
    m_pitchEnable.onClick = syncOnChange;
    m_gateEnable.onClick = syncOnChange;
    m_pitchPage.onChange = syncOnChange;
    m_pitchRow.onValueChange = syncOnChange;
    m_extModAEnable.onClick = syncOnChange;
    m_extModAChannel.onValueChange = syncOnChange;
    m_extModACc.onValueChange = syncOnChange;
    m_extModBEnable.onClick = syncOnChange;
    m_extModBChannel.onValueChange = syncOnChange;
    m_extModBCc.onValueChange = syncOnChange;
    for (BindingRowUi& row : m_bindingRows)
    {
        row.enable.onClick = syncOnChange;
        row.kind.onChange = syncOnChange;
        row.number.onValueChange = syncOnChange;
        row.channel.onValueChange = syncOnChange;
    }
    m_qwertyEnable.onClick = syncOnChange;
    m_qwertyChannel.onValueChange = syncOnChange;
    m_externalClock.onClick = [this]() {
        m_engine.getSequencer().m_externalClock = m_externalClock.getToggleState();
    };

    for (juce::Component* c : {static_cast<juce::Component*>(&m_inSectionLabel),
                               static_cast<juce::Component*>(&m_inHelp),
                               static_cast<juce::Component*>(&m_inLabel),
                               static_cast<juce::Component*>(&m_inDevice),
                               static_cast<juce::Component*>(&m_refresh),
                               static_cast<juce::Component*>(&m_inStatus),
                               static_cast<juce::Component*>(&m_assignSectionLabel),
                               static_cast<juce::Component*>(&m_assignHelp),
                               static_cast<juce::Component*>(&m_pitchEnable),
                               static_cast<juce::Component*>(&m_pitchPageLabel),
                               static_cast<juce::Component*>(&m_pitchPage),
                               static_cast<juce::Component*>(&m_pitchRowLabel),
                               static_cast<juce::Component*>(&m_pitchRow),
                               static_cast<juce::Component*>(&m_pitchTargetLabel),
                               static_cast<juce::Component*>(&m_gateEnable),
                               static_cast<juce::Component*>(&m_gateHelp),
                               static_cast<juce::Component*>(&m_extModALabel),
                               static_cast<juce::Component*>(&m_extModAEnable),
                               static_cast<juce::Component*>(&m_extModAChLabel),
                               static_cast<juce::Component*>(&m_extModAChannel),
                               static_cast<juce::Component*>(&m_extModACcLabel),
                               static_cast<juce::Component*>(&m_extModACc),
                               static_cast<juce::Component*>(&m_extModBLabel),
                               static_cast<juce::Component*>(&m_extModBEnable),
                               static_cast<juce::Component*>(&m_extModBChLabel),
                               static_cast<juce::Component*>(&m_extModBChannel),
                               static_cast<juce::Component*>(&m_extModBCcLabel),
                               static_cast<juce::Component*>(&m_extModBCc),
                               static_cast<juce::Component*>(&m_qwertyEnable),
                               static_cast<juce::Component*>(&m_qwertyChLabel),
                               static_cast<juce::Component*>(&m_qwertyChannel),
                               static_cast<juce::Component*>(&m_externalClock),
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
    for (BindingRowUi& row : m_bindingRows)
    {
        for (juce::Component* c :
             {static_cast<juce::Component*>(&row.label),
              static_cast<juce::Component*>(&row.enable),
              static_cast<juce::Component*>(&row.chLabel),
              static_cast<juce::Component*>(&row.channel),
              static_cast<juce::Component*>(&row.kind),
              static_cast<juce::Component*>(&row.number)})
        {
            addAndMakeVisible(c);
        }
    }

    syncUiFromTable();
    updatePitchTargetLabel();
    refreshDeviceLists();
    updateStatus();
    startTimerHz(4);
    setSize(560, 820);
}

void MidiCvSettingsComponent::configureCcSlider(juce::Slider& slider)
{
    slider.setSliderStyle(juce::Slider::LinearHorizontal);
    slider.setTextBoxStyle(
        juce::Slider::TextBoxRight, false, kCcTextBoxWidth, kRowControlHeight - 4);
    slider.setNumDecimalPlacesToDisplay(0);
    slider.setRange(0, 127, 1);
}

void MidiCvSettingsComponent::updatePitchTargetLabel()
{
    const uint8_t page = static_cast<uint8_t>(m_pitchPage.getSelectedId() - 1);
    const uint8_t row = static_cast<uint8_t>(m_pitchRow.getValue());
    m_pitchTargetLabel.setText(
        V2ParamDisplayNames::forHostPageRow(page, row), juce::dontSendNotification);
}

void MidiCvSettingsComponent::syncUiFromTable()
{
    MidiCvAssignmentTable& table = m_engine.getMidiCvTable();
    m_pitchEnable.setToggleState(table.pitchEnabled, juce::dontSendNotification);
    m_gateEnable.setToggleState(table.gateEnabled, juce::dontSendNotification);
    m_pitchPage.setSelectedId(static_cast<int>(table.pitchPage) + 1, juce::dontSendNotification);
    m_pitchRow.setValue(table.pitchRow, juce::dontSendNotification);
    m_extModAEnable.setToggleState(table.externalModA.enabled, juce::dontSendNotification);
    m_extModAChannel.setValue(table.externalModA.channel, juce::dontSendNotification);
    m_extModACc.setValue(table.externalModA.cc, juce::dontSendNotification);
    m_extModBEnable.setToggleState(table.externalModB.enabled, juce::dontSendNotification);
    m_extModBChannel.setValue(table.externalModB.channel, juce::dontSendNotification);
    m_extModBCc.setValue(table.externalModB.cc, juce::dontSendNotification);
    for (size_t i = 0; i < m_bindingRows.size(); ++i)
    {
        const MidiCvButtonBinding& binding = *bindingForRow(table, i);
        BindingRowUi& row = m_bindingRows[i];
        row.enable.setToggleState(binding.enabled, juce::dontSendNotification);
        setTriggerKindCombo(row.kind, binding.kind);
        row.number.setValue(binding.number, juce::dontSendNotification);
        row.channel.setValue(binding.channel, juce::dontSendNotification);
    }
    m_qwertyEnable.setToggleState(table.qwertyVirtualChannelEnabled, juce::dontSendNotification);
    m_qwertyChannel.setValue(table.qwertyMidiChannel, juce::dontSendNotification);
    m_externalClock.setToggleState(
        m_engine.getSequencer().m_externalClock, juce::dontSendNotification);
    updatePitchTargetLabel();
}

void MidiCvSettingsComponent::syncTableFromUi()
{
    MidiCvAssignmentTable& table = m_engine.getMidiCvTable();
    table.pitchEnabled = m_pitchEnable.getToggleState();
    table.gateEnabled = m_gateEnable.getToggleState();
    table.pitchPage = static_cast<uint8_t>(m_pitchPage.getSelectedId() - 1);
    table.pitchRow = static_cast<uint8_t>(m_pitchRow.getValue());
    table.externalModA.enabled = m_extModAEnable.getToggleState();
    table.externalModA.channel = static_cast<uint8_t>(m_extModAChannel.getValue());
    table.externalModA.cc = static_cast<uint8_t>(m_extModACc.getValue());
    table.externalModB.enabled = m_extModBEnable.getToggleState();
    table.externalModB.channel = static_cast<uint8_t>(m_extModBChannel.getValue());
    table.externalModB.cc = static_cast<uint8_t>(m_extModBCc.getValue());
    for (size_t i = 0; i < m_bindingRows.size(); ++i)
    {
        MidiCvButtonBinding& binding = *bindingForRow(table, i);
        const BindingRowUi& row = m_bindingRows[i];
        binding.enabled = row.enable.getToggleState();
        binding.kind = triggerKindFromCombo(row.kind);
        binding.number = static_cast<uint8_t>(row.number.getValue());
        binding.channel = static_cast<uint8_t>(row.channel.getValue());
        binding.target = targetForRow(i);
    }
    table.qwertyVirtualChannelEnabled = m_qwertyEnable.getToggleState();
    table.qwertyMidiChannel = static_cast<uint8_t>(m_qwertyChannel.getValue());
}

void MidiCvSettingsComponent::timerCallback()
{
    updateStatus();
}

void MidiCvSettingsComponent::updateStatus()
{
    if (m_engine.isHardwareMidiInputOpenFailed())
    {
        m_inStatus.setText("Could not open selected MIDI input device.", juce::dontSendNotification);
        m_inStatus.setColour(juce::Label::textColourId, juce::Colours::orange);
        return;
    }
    if (m_engine.isComputerKeyboardMidiEnabled())
    {
        m_inStatus.setText("Computer keyboard -> virtual MIDI channel (pitch/gate/CC).",
                           juce::dontSendNotification);
        m_inStatus.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
        return;
    }
    if (m_engine.getMidiInputDeviceIdentifier().isEmpty())
    {
        m_inStatus.setText("No MIDI input device selected.", juce::dontSendNotification);
        m_inStatus.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
        return;
    }
    m_inStatus.setText("Hardware MIDI in active.", juce::dontSendNotification);
    m_inStatus.setColour(juce::Label::textColourId, juce::Colours::lightgreen);
}

void MidiCvSettingsComponent::refreshDeviceLists()
{
    const juce::String prevIn = m_engine.getMidiInputDeviceIdentifier();
    const juce::String prevOut = m_engine.getMidiOutputDeviceIdentifier();

    m_inDevice.clear();
    m_outDevice.clear();
    m_inDevice.addItem("Computer keyboard", 1);
    m_inDevice.addItem("None", 2);
    int inIdx = 3;
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
    else if (prevIn.isEmpty())
    {
        m_inDevice.setSelectedId(2, juce::dontSendNotification);
    }
    else
    {
        int selectId = 2;
        int id = 3;
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

void MidiCvSettingsComponent::applyInputDevice()
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
    if (id == 2)
    {
        m_engine.setMidiInputDevice({});
        updateStatus();
        return;
    }
    const auto devices = juce::MidiInput::getAvailableDevices();
    const int deviceIndex = id - 3;
    if (deviceIndex >= 0 && deviceIndex < devices.size())
    {
        m_engine.setMidiInputDevice(devices[static_cast<size_t>(deviceIndex)].identifier);
    }
    updateStatus();
}

void MidiCvSettingsComponent::applyOutputDevice()
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

void MidiCvSettingsComponent::layoutBindingRow(juce::Rectangle<int>& area, BindingRowUi& row)
{
    auto bindingRow = area.removeFromTop(24);
    row.label.setBounds(bindingRow.removeFromLeft(kBindingLabelWidth));
    row.enable.setBounds(bindingRow.removeFromLeft(kBindingEnableWidth));
    row.chLabel.setBounds(bindingRow.removeFromLeft(kBindingChLabelWidth));
    row.channel.setBounds(bindingRow.removeFromLeft(kChannelControlWidth));
    bindingRow.removeFromLeft(8);
    row.kind.setBounds(bindingRow.removeFromLeft(kBindingKindWidth));
    row.number.setBounds(bindingRow.removeFromLeft(kCcControlWidth));
    area.removeFromTop(4);
}

void MidiCvSettingsComponent::resized()
{
    auto area = getLocalBounds().reduced(12);
    m_inSectionLabel.setBounds(area.removeFromTop(20));
    area.removeFromTop(4);
    m_inHelp.setBounds(area.removeFromTop(32));
    area.removeFromTop(4);
    m_inLabel.setBounds(area.removeFromTop(16));
    auto inDevRow = area.removeFromTop(24);
    m_inDevice.setBounds(inDevRow.removeFromLeft(inDevRow.getWidth() - 120));
    inDevRow.removeFromLeft(6);
    m_refresh.setBounds(inDevRow);
    area.removeFromTop(6);
    m_inStatus.setBounds(area.removeFromTop(18));
    area.removeFromTop(10);

    m_assignSectionLabel.setBounds(area.removeFromTop(20));
    area.removeFromTop(4);
    m_assignHelp.setBounds(area.removeFromTop(32));
    area.removeFromTop(4);

    auto pitchRow = area.removeFromTop(24);
    m_pitchEnable.setBounds(pitchRow.removeFromLeft(64));
    m_pitchPageLabel.setBounds(pitchRow.removeFromLeft(40));
    m_pitchPage.setBounds(pitchRow.removeFromLeft(120));
    pitchRow.removeFromLeft(8);
    m_pitchRowLabel.setBounds(pitchRow.removeFromLeft(32));
    m_pitchRow.setBounds(pitchRow.removeFromLeft(80));
    pitchRow.removeFromLeft(8);
    m_pitchTargetLabel.setBounds(pitchRow);
    area.removeFromTop(4);

    auto gateRow = area.removeFromTop(24);
    m_gateEnable.setBounds(gateRow.removeFromLeft(64));
    area.removeFromTop(2);
    m_gateHelp.setBounds(area.removeFromTop(18));
    area.removeFromTop(4);

    auto extARow = area.removeFromTop(24);
    m_extModALabel.setBounds(extARow.removeFromLeft(80));
    m_extModAEnable.setBounds(extARow.removeFromLeft(40));
    m_extModAChLabel.setBounds(extARow.removeFromLeft(24));
    m_extModAChannel.setBounds(extARow.removeFromLeft(kChannelControlWidth));
    extARow.removeFromLeft(8);
    m_extModACcLabel.setBounds(extARow.removeFromLeft(24));
    m_extModACc.setBounds(extARow.removeFromLeft(kCcControlWidth));
    area.removeFromTop(4);

    auto extBRow = area.removeFromTop(24);
    m_extModBLabel.setBounds(extBRow.removeFromLeft(80));
    m_extModBEnable.setBounds(extBRow.removeFromLeft(40));
    m_extModBChLabel.setBounds(extBRow.removeFromLeft(24));
    m_extModBChannel.setBounds(extBRow.removeFromLeft(kChannelControlWidth));
    extBRow.removeFromLeft(8);
    m_extModBCcLabel.setBounds(extBRow.removeFromLeft(24));
    m_extModBCc.setBounds(extBRow.removeFromLeft(kCcControlWidth));
    area.removeFromTop(6);

    for (BindingRowUi& row : m_bindingRows)
    {
        layoutBindingRow(area, row);
    }
    area.removeFromTop(2);

    auto qwertyRow = area.removeFromTop(24);
    m_qwertyEnable.setBounds(qwertyRow.removeFromLeft(180));
    m_qwertyChLabel.setBounds(qwertyRow.removeFromLeft(24));
    m_qwertyChannel.setBounds(qwertyRow.removeFromLeft(kChannelControlWidth));
    area.removeFromTop(4);
    m_externalClock.setBounds(area.removeFromTop(24));
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
