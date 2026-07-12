#include "MidiCvSettingsComponent.h"

#include "V2ParamDisplayNames.hpp"
#include "control/FroggersV2ControlCore.hpp"
#include "manifest/FroggersV2AppManifest.hpp"

#include <array>
#include <cstring>

namespace
{
constexpr int kChannelControlWidth = 50;
constexpr int kCcControlWidth = 80;
constexpr int kCcTextBoxWidth = 44;
constexpr int kRowControlHeight = 24;
constexpr int kBindingLabelWidth = 120;
constexpr int kBindingEnableWidth = 40;
constexpr int kBindingChLabelWidth = 24;
constexpr int kBindingKindWidth = 72;
constexpr int kReadbackWidth = 140;

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

MidiCvButtonBinding* buttonBindingForTargetId(MidiCvAssignmentTable& table, const char* targetId)
{
    const MidiCvControllerTargetIds& ids = midiCvControllerTargetIds();
    if (targetId == nullptr)
    {
        return nullptr;
    }
    if (std::strcmp(targetId, ids.shiftButton) == 0)
    {
        return &table.shiftButton;
    }
    if (std::strcmp(targetId, ids.scene1) == 0)
    {
        return &table.sceneButtons[0];
    }
    if (std::strcmp(targetId, ids.scene2) == 0)
    {
        return &table.sceneButtons[1];
    }
    if (std::strcmp(targetId, ids.scene3) == 0)
    {
        return &table.sceneButtons[2];
    }
    return nullptr;
}

MidiCvBindingRole bindingRoleForTargetId(const char* targetId)
{
    const MidiCvControllerTargetIds& ids = midiCvControllerTargetIds();
    if (targetId == nullptr)
    {
        return MidiCvBindingRole::None;
    }
    if (std::strcmp(targetId, ids.shiftButton) == 0)
    {
        return MidiCvBindingRole::HeldModifier;
    }
    if (std::strcmp(targetId, ids.scene1) == 0)
    {
        return MidiCvBindingRole::SceneOrdinal0;
    }
    if (std::strcmp(targetId, ids.scene2) == 0)
    {
        return MidiCvBindingRole::SceneOrdinal1;
    }
    if (std::strcmp(targetId, ids.scene3) == 0)
    {
        return MidiCvBindingRole::SceneOrdinal2;
    }
    return MidiCvBindingRole::None;
}

MidiCvTargetRowKind rowKindForIndex(size_t index)
{
    switch (index)
    {
        case 0:
            return MidiCvTargetRowKind::Pitch;
        case 1:
            return MidiCvTargetRowKind::Gate;
        case 2:
        case 3:
            return MidiCvTargetRowKind::CcBinding;
        case 4:
        case 5:
        case 6:
        case 7:
            return MidiCvTargetRowKind::ButtonBinding;
        case 8:
            return MidiCvTargetRowKind::QwertyChannel;
        case 9:
            return MidiCvTargetRowKind::ExternalClock;
        default:
            return MidiCvTargetRowKind::ButtonBinding;
    }
}
} // namespace

void MidiCvSettingsComponent::initTargetRows()
{
    const MidiCvControllerTargetIds& ids = midiCvControllerTargetIds();
    const std::array<const char*, froggers_v2::manifest::kControllerTargetDeclarations.size()> targetIds{{
        ids.pitch,
        ids.gate,
        ids.externalModA,
        ids.externalModB,
        ids.shiftButton,
        ids.scene1,
        ids.scene2,
        ids.scene3,
        ids.qwertyVirtual,
        ids.externalClock,
    }};

    for (size_t i = 0; i < m_targetRows.size(); ++i)
    {
        TargetRowUi& row = m_targetRows[i];
        const auto& decl = froggers_v2::manifest::kControllerTargetDeclarations[i];
        row.targetId = targetIds[i];
        row.kind = rowKindForIndex(i);
        row.label.setText(
            juce::String(decl.displayName) + " [" + juce::String(row.targetId) + "]",
            juce::dontSendNotification);
        row.readbackLabel.setJustificationType(juce::Justification::centredLeft);
        row.fanOutLabel.setJustificationType(juce::Justification::centredLeft);
        row.fanOutLabel.setFont(juce::Font(11.0f));

        if (row.kind == MidiCvTargetRowKind::Pitch)
        {
            row.enable.setButtonText("On");
            row.pageLabel.setText("Page", juce::dontSendNotification);
            row.rowLabel.setText("Row", juce::dontSendNotification);
            row.row.setRange(0, 9, 1);
            row.row.setSliderStyle(juce::Slider::LinearHorizontal);
            row.row.setTextBoxStyle(juce::Slider::TextBoxRight, false, 36, kRowControlHeight - 4);
            for (uint8_t page = 0; page < froggers_v2::kNumHostPages; ++page)
            {
                row.page.addItem(V2ParamDisplayNames::forHostPage(page), static_cast<int>(page) + 1);
            }
        }
        else if (row.kind == MidiCvTargetRowKind::Gate)
        {
            row.enable.setButtonText("On");
            row.help.setText(
                "Note on/off drives ADSR and sequencer gates.", juce::dontSendNotification);
            row.help.setJustificationType(juce::Justification::topLeft);
        }
        else if (row.kind == MidiCvTargetRowKind::CcBinding)
        {
            row.enable.setButtonText("On");
            row.chLabel.setText("Ch", juce::dontSendNotification);
            row.label.setTooltip(
                "Incoming CC level routed through the controller model to eligible targets.");
            row.number.setName("CC");
            configureAnyChannelSlider(row.channel);
            configureNumberSlider(row.number, 127);
        }
        else if (row.kind == MidiCvTargetRowKind::ButtonBinding)
        {
            row.enable.setButtonText("On");
            row.chLabel.setText("Ch", juce::dontSendNotification);
            configureAnyChannelSlider(row.channel);
            row.kindCombo.addItem("Note", 1);
            row.kindCombo.addItem("CC", 2);
            row.kindCombo.setSelectedId(1, juce::dontSendNotification);
            configureNumberSlider(row.number, 127);
            const MidiCvBindingRole role = bindingRoleForTargetId(row.targetId);
            row.label.setText(
                juce::String(decl.displayName) + " ["
                    + juce::String(manifestTargetIdForBindingRole(role)) + "]",
                juce::dontSendNotification);
        }
        else if (row.kind == MidiCvTargetRowKind::QwertyChannel)
        {
            row.enable.setButtonText("QWERTY virtual MIDI");
            row.chLabel.setText("Ch", juce::dontSendNotification);
            configureChannelSlider(row.channel);
            row.channel.setRange(1, 16, 1);
        }
        else if (row.kind == MidiCvTargetRowKind::ExternalClock)
        {
            row.enable.setButtonText("External MIDI clock");
        }
    }
}

void MidiCvSettingsComponent::addTargetRowComponents(TargetRowUi& row)
{
    addAndMakeVisible(row.label);
    addAndMakeVisible(row.readbackLabel);
    addAndMakeVisible(row.fanOutLabel);
    addAndMakeVisible(row.enable);
    if (row.kind == MidiCvTargetRowKind::Pitch)
    {
        addAndMakeVisible(row.pageLabel);
        addAndMakeVisible(row.page);
        addAndMakeVisible(row.rowLabel);
        addAndMakeVisible(row.row);
    }
    if (row.kind == MidiCvTargetRowKind::Gate)
    {
        addAndMakeVisible(row.help);
    }
    if (row.kind == MidiCvTargetRowKind::CcBinding || row.kind == MidiCvTargetRowKind::ButtonBinding)
    {
        addAndMakeVisible(row.chLabel);
        addAndMakeVisible(row.channel);
    }
    if (row.kind == MidiCvTargetRowKind::ButtonBinding)
    {
        addAndMakeVisible(row.kindCombo);
        addAndMakeVisible(row.number);
    }
    if (row.kind == MidiCvTargetRowKind::CcBinding)
    {
        addAndMakeVisible(row.number);
    }
    if (row.kind == MidiCvTargetRowKind::QwertyChannel)
    {
        addAndMakeVisible(row.chLabel);
        addAndMakeVisible(row.channel);
    }
}

void MidiCvSettingsComponent::wireTargetRowCallbacks()
{
    auto syncOnChange = [this]() {
        syncTableFromUi();
        updateMappingReadbacks();
    };

    for (TargetRowUi& row : m_targetRows)
    {
        row.enable.onClick = syncOnChange;
        if (row.kind == MidiCvTargetRowKind::Pitch)
        {
            row.page.onChange = syncOnChange;
            row.row.onValueChange = syncOnChange;
        }
        if (row.kind == MidiCvTargetRowKind::CcBinding || row.kind == MidiCvTargetRowKind::ButtonBinding)
        {
            row.channel.onValueChange = syncOnChange;
            row.number.onValueChange = syncOnChange;
        }
        if (row.kind == MidiCvTargetRowKind::ButtonBinding)
        {
            row.kindCombo.onChange = syncOnChange;
        }
        if (row.kind == MidiCvTargetRowKind::QwertyChannel)
        {
            row.channel.onValueChange = syncOnChange;
        }
        if (row.kind == MidiCvTargetRowKind::ExternalClock)
        {
            row.enable.onClick = [this]() {
                m_engine.getSequencer().m_externalClock = m_targetRows[9].enable.getToggleState();
                updateMappingReadbacks();
            };
        }
    }
}

MidiCvSettingsComponent::MidiCvSettingsComponent(AudioEngine& engine,
                                                 std::function<void()> onClose,
                                                 MidiCvSettingsPresentation presentation)
    : m_engine(engine)
    , m_onClose(std::move(onClose))
    , m_presentation(presentation)
{
    m_inSectionLabel.setText("MIDI In", juce::dontSendNotification);
    m_inSectionLabel.setFont(juce::Font(juce::FontOptions{}.withHeight(14.0f).withStyle("Bold")));
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
    m_assignSectionLabel.setFont(juce::Font(juce::FontOptions{}.withHeight(14.0f).withStyle("Bold")));
    m_assignHelp.setText(
        "Rows project manifest controller targets via buildTargetMappingRows(). "
        "Pitch and Gate are performance targets. MIDI CC A and MIDI CC B route through the controller model. "
        "Shift is a dedicated mapping row, not the Gate row.",
        juce::dontSendNotification);
    m_assignHelp.setJustificationType(juce::Justification::topLeft);

    initTargetRows();
    wireTargetRowCallbacks();

    m_outSectionLabel.setText("MIDI Out (VCO Env)", juce::dontSendNotification);
    m_outSectionLabel.setFont(juce::Font(juce::FontOptions{}.withHeight(14.0f).withStyle("Bold")));
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

    for (juce::Component* c : {static_cast<juce::Component*>(&m_inSectionLabel),
                               static_cast<juce::Component*>(&m_inHelp),
                               static_cast<juce::Component*>(&m_inLabel),
                               static_cast<juce::Component*>(&m_inDevice),
                               static_cast<juce::Component*>(&m_refresh),
                               static_cast<juce::Component*>(&m_inStatus),
                               static_cast<juce::Component*>(&m_assignSectionLabel),
                               static_cast<juce::Component*>(&m_assignHelp),
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
    for (TargetRowUi& row : m_targetRows)
    {
        addTargetRowComponents(row);
    }

    syncUiFromTable();
    refreshDeviceLists();
    updateStatus();
    if (m_presentation == MidiCvSettingsPresentation::RuntimePage)
    {
        m_close.setVisible(false);
    }
    startTimerHz(4);
    setSize(560, m_presentation == MidiCvSettingsPresentation::RuntimePage ? 720 : 900);
}

void MidiCvSettingsComponent::configureCcSlider(juce::Slider& slider)
{
    slider.setSliderStyle(juce::Slider::LinearHorizontal);
    slider.setTextBoxStyle(
        juce::Slider::TextBoxRight, false, kCcTextBoxWidth, kRowControlHeight - 4);
    slider.setNumDecimalPlacesToDisplay(0);
    slider.setRange(0, 127, 1);
}

void MidiCvSettingsComponent::updateMappingReadbacks()
{
    const MidiCvAssignmentTable& table = m_engine.getMidiCvTable();
    const std::vector<ControllerTargetMappingRow> rows = table.buildTargetMappingRows();
    for (TargetRowUi& uiRow : m_targetRows)
    {
        for (const ControllerTargetMappingRow& modelRow : rows)
        {
            if (modelRow.targetId == nullptr || uiRow.targetId == nullptr)
            {
                continue;
            }
            if (std::strcmp(modelRow.targetId, uiRow.targetId) != 0)
            {
                continue;
            }
            uiRow.readbackLabel.setText(modelRow.targetReadback, juce::dontSendNotification);
            if (modelRow.fanOutCount > 1)
            {
                uiRow.fanOutLabel.setText(
                    "Fan-out x" + juce::String(static_cast<int>(modelRow.fanOutCount)),
                    juce::dontSendNotification);
            }
            else
            {
                uiRow.fanOutLabel.setText({}, juce::dontSendNotification);
            }
            break;
        }
    }

    TargetRowUi& pitchRow = m_targetRows[0];
    const uint8_t page = static_cast<uint8_t>(pitchRow.page.getSelectedId() - 1);
    const uint8_t row = static_cast<uint8_t>(pitchRow.row.getValue());
    pitchRow.readbackLabel.setText(
        V2ParamDisplayNames::forHostPageRow(page, row), juce::dontSendNotification);
}

void MidiCvSettingsComponent::syncUiFromTable()
{
    MidiCvAssignmentTable& table = m_engine.getMidiCvTable();

    m_targetRows[0].enable.setToggleState(table.pitchEnabled, juce::dontSendNotification);
    m_targetRows[0].page.setSelectedId(static_cast<int>(table.pitchPage) + 1, juce::dontSendNotification);
    m_targetRows[0].row.setValue(table.pitchRow, juce::dontSendNotification);

    m_targetRows[1].enable.setToggleState(table.gateEnabled, juce::dontSendNotification);

    m_targetRows[2].enable.setToggleState(table.externalModA.enabled, juce::dontSendNotification);
    m_targetRows[2].channel.setValue(table.externalModA.channel, juce::dontSendNotification);
    m_targetRows[2].number.setValue(table.externalModA.cc, juce::dontSendNotification);

    m_targetRows[3].enable.setToggleState(table.externalModB.enabled, juce::dontSendNotification);
    m_targetRows[3].channel.setValue(table.externalModB.channel, juce::dontSendNotification);
    m_targetRows[3].number.setValue(table.externalModB.cc, juce::dontSendNotification);

    for (size_t i = 4; i <= 7; ++i)
    {
        MidiCvButtonBinding* binding = buttonBindingForTargetId(table, m_targetRows[i].targetId);
        if (binding == nullptr)
        {
            continue;
        }
        TargetRowUi& uiRow = m_targetRows[i];
        uiRow.enable.setToggleState(binding->enabled, juce::dontSendNotification);
        setTriggerKindCombo(uiRow.kindCombo, binding->kind);
        uiRow.number.setValue(binding->number, juce::dontSendNotification);
        uiRow.channel.setValue(binding->channel, juce::dontSendNotification);
    }

    m_targetRows[8].enable.setToggleState(
        table.qwertyVirtualChannelEnabled, juce::dontSendNotification);
    m_targetRows[8].channel.setValue(table.qwertyMidiChannel, juce::dontSendNotification);

    m_targetRows[9].enable.setToggleState(
        m_engine.getSequencer().m_externalClock, juce::dontSendNotification);

    updateMappingReadbacks();
}

void MidiCvSettingsComponent::syncTableFromUi()
{
    m_engine.getMidiCvTable().markMappingsDirty();
    MidiCvAssignmentTable& table = m_engine.getMidiCvTable();

    table.pitchEnabled = m_targetRows[0].enable.getToggleState();
    table.gateEnabled = m_targetRows[1].enable.getToggleState();
    table.pitchPage = static_cast<uint8_t>(m_targetRows[0].page.getSelectedId() - 1);
    table.pitchRow = static_cast<uint8_t>(m_targetRows[0].row.getValue());

    table.externalModA.enabled = m_targetRows[2].enable.getToggleState();
    table.externalModA.channel = static_cast<uint8_t>(m_targetRows[2].channel.getValue());
    table.externalModA.cc = static_cast<uint8_t>(m_targetRows[2].number.getValue());

    table.externalModB.enabled = m_targetRows[3].enable.getToggleState();
    table.externalModB.channel = static_cast<uint8_t>(m_targetRows[3].channel.getValue());
    table.externalModB.cc = static_cast<uint8_t>(m_targetRows[3].number.getValue());

    for (size_t i = 4; i <= 7; ++i)
    {
        MidiCvButtonBinding* binding = buttonBindingForTargetId(table, m_targetRows[i].targetId);
        if (binding == nullptr)
        {
            continue;
        }
        const TargetRowUi& uiRow = m_targetRows[i];
        binding->enabled = uiRow.enable.getToggleState();
        binding->kind = triggerKindFromCombo(uiRow.kindCombo);
        binding->number = static_cast<uint8_t>(uiRow.number.getValue());
        binding->channel = static_cast<uint8_t>(uiRow.channel.getValue());
        binding->target = bindingRoleForTargetId(uiRow.targetId);
    }

    table.qwertyVirtualChannelEnabled = m_targetRows[8].enable.getToggleState();
    table.qwertyMidiChannel = static_cast<uint8_t>(m_targetRows[8].channel.getValue());
}

void MidiCvSettingsComponent::timerCallback()
{
    updateStatus();
    updateMappingReadbacks();
}

void MidiCvSettingsComponent::updateStatus()
{
    MidiCvAssignmentTable& table = m_engine.getMidiCvTable();
    if (m_engine.isHardwareMidiInputOpenFailed())
    {
        m_inStatus.setText("Could not open selected MIDI input device.", juce::dontSendNotification);
        m_inStatus.setColour(juce::Label::textColourId, juce::Colours::orange);
        table.setSelectedInputLabel("Error opening device");
        return;
    }
    if (m_engine.isComputerKeyboardMidiEnabled())
    {
        m_inStatus.setText("Computer keyboard -> virtual MIDI channel (pitch/gate/CC).",
                           juce::dontSendNotification);
        m_inStatus.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
        table.setSelectedInputLabel("QWERTY virtual MIDI");
        return;
    }
    if (m_engine.getMidiInputDeviceIdentifier().isEmpty())
    {
        m_inStatus.setText("No MIDI input device selected.", juce::dontSendNotification);
        m_inStatus.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
        table.setSelectedInputLabel("None");
        return;
    }
    m_inStatus.setText("Hardware MIDI in active.", juce::dontSendNotification);
    m_inStatus.setColour(juce::Label::textColourId, juce::Colours::lightgreen);
    table.setSelectedInputLabel(m_engine.getMidiInputDeviceIdentifier().toStdString());
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
        m_engine.setMidiInputDevice(devices[deviceIndex].identifier);
        updateStatus();
    }
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
        m_engine.setMidiOutputDevice(AudioEngine::kNoMidiOutId);
        return;
    }
    const auto devices = juce::MidiOutput::getAvailableDevices();
    const int deviceIndex = id - 2;
    if (deviceIndex >= 0 && deviceIndex < devices.size())
    {
        m_engine.setMidiOutputDevice(devices[deviceIndex].identifier);
    }
}

void MidiCvSettingsComponent::layoutTargetRow(juce::Rectangle<int>& area, TargetRowUi& row)
{
    if (row.kind == MidiCvTargetRowKind::Pitch)
    {
        auto pitchLine = area.removeFromTop(24);
        row.label.setBounds(pitchLine.removeFromLeft(kBindingLabelWidth));
        row.enable.setBounds(pitchLine.removeFromLeft(kBindingEnableWidth));
        row.pageLabel.setBounds(pitchLine.removeFromLeft(40));
        row.page.setBounds(pitchLine.removeFromLeft(120));
        pitchLine.removeFromLeft(8);
        row.rowLabel.setBounds(pitchLine.removeFromLeft(32));
        row.row.setBounds(pitchLine.removeFromLeft(80));
        area.removeFromTop(4);
        auto readbackLine = area.removeFromTop(18);
        row.readbackLabel.setBounds(readbackLine);
        area.removeFromTop(4);
        return;
    }

    if (row.kind == MidiCvTargetRowKind::Gate)
    {
        auto gateLine = area.removeFromTop(24);
        row.label.setBounds(gateLine.removeFromLeft(kBindingLabelWidth));
        row.enable.setBounds(gateLine.removeFromLeft(kBindingEnableWidth));
        row.readbackLabel.setBounds(gateLine);
        area.removeFromTop(2);
        row.help.setBounds(area.removeFromTop(18));
        area.removeFromTop(4);
        return;
    }

    if (row.kind == MidiCvTargetRowKind::CcBinding)
    {
        auto ccLine = area.removeFromTop(24);
        row.label.setBounds(ccLine.removeFromLeft(kBindingLabelWidth));
        row.enable.setBounds(ccLine.removeFromLeft(kBindingEnableWidth));
        row.chLabel.setBounds(ccLine.removeFromLeft(kBindingChLabelWidth));
        row.channel.setBounds(ccLine.removeFromLeft(kChannelControlWidth));
        ccLine.removeFromLeft(8);
        row.number.setBounds(ccLine.removeFromLeft(kCcControlWidth));
        ccLine.removeFromLeft(8);
        row.readbackLabel.setBounds(ccLine.removeFromLeft(kReadbackWidth));
        row.fanOutLabel.setBounds(ccLine);
        area.removeFromTop(4);
        return;
    }

    if (row.kind == MidiCvTargetRowKind::ButtonBinding)
    {
        auto buttonLine = area.removeFromTop(24);
        row.label.setBounds(buttonLine.removeFromLeft(kBindingLabelWidth));
        row.enable.setBounds(buttonLine.removeFromLeft(kBindingEnableWidth));
        row.chLabel.setBounds(buttonLine.removeFromLeft(kBindingChLabelWidth));
        row.channel.setBounds(buttonLine.removeFromLeft(kChannelControlWidth));
        buttonLine.removeFromLeft(8);
        row.kindCombo.setBounds(buttonLine.removeFromLeft(kBindingKindWidth));
        row.number.setBounds(buttonLine.removeFromLeft(kCcControlWidth));
        area.removeFromTop(2);
        auto metaLine = area.removeFromTop(16);
        row.readbackLabel.setBounds(metaLine.removeFromLeft(kReadbackWidth));
        row.fanOutLabel.setBounds(metaLine);
        area.removeFromTop(4);
        return;
    }

    if (row.kind == MidiCvTargetRowKind::QwertyChannel)
    {
        auto qwertyLine = area.removeFromTop(24);
        row.enable.setBounds(qwertyLine.removeFromLeft(180));
        row.chLabel.setBounds(qwertyLine.removeFromLeft(kBindingChLabelWidth));
        row.channel.setBounds(qwertyLine.removeFromLeft(kChannelControlWidth));
        qwertyLine.removeFromLeft(8);
        row.readbackLabel.setBounds(qwertyLine);
        area.removeFromTop(4);
        return;
    }

    if (row.kind == MidiCvTargetRowKind::ExternalClock)
    {
        auto clockLine = area.removeFromTop(24);
        row.enable.setBounds(clockLine.removeFromLeft(220));
        row.readbackLabel.setBounds(clockLine);
        area.removeFromTop(4);
    }
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
    m_assignHelp.setBounds(area.removeFromTop(48));
    area.removeFromTop(4);

    for (TargetRowUi& row : m_targetRows)
    {
        layoutTargetRow(area, row);
    }
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
    area.removeFromTop(10);

    if (m_close.isVisible())
    {
        m_close.setBounds(area.removeFromTop(28).removeFromRight(100));
    }
}
