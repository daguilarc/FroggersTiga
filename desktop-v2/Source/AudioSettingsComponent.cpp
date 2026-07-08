#include "AudioSettingsComponent.h"

namespace
{
class InputLevelMeter : public juce::Component,
                        private juce::Timer
{
public:
    InputLevelMeter(AudioEngine& engine, juce::AudioDeviceManager& deviceManager)
        : m_engine(engine)
        , m_levelGetter(deviceManager.getInputLevelGetter())
    {
        startTimerHz(20);
    }

    void paint(juce::Graphics& g) override
    {
        float level = 0.0f;
        if (m_levelGetter != nullptr)
        {
            level = static_cast<float>(m_levelGetter->getCurrentLevel());
        }
        if (level <= 0.0f && m_engine.isExternalInputEnabled() && m_engine.isAudioRunning())
        {
            level = m_engine.getInputPeakLevel();
        }
        const float skewed = level > 0.0f ? std::exp(std::log(level) / 3.0f) : 0.0f;
        getLookAndFeel().drawLevelMeter(g, getWidth(), getHeight(), skewed);
    }

private:
    void timerCallback() override
    {
        if (isShowing())
        {
            repaint();
        }
    }

    AudioEngine& m_engine;
    juce::AudioDeviceManager::LevelMeter::Ptr m_levelGetter;
};

void initExportFormatToggle(juce::ToggleButton& btn, int radioGroupId)
{
    btn.setRadioGroupId(radioGroupId);
    btn.setClickingTogglesState(true);
}
} // namespace

AudioSettingsComponent::AudioSettingsComponent(AudioEngine& engine,
                                               std::function<void()> onClose,
                                               AudioSettingsPresentation presentation)
    : m_engine(engine)
    , m_onClose(std::move(onClose))
    , m_presentation(presentation)
{
    m_rateLabel.setText("Sample rate", juce::dontSendNotification);
    m_outLabel.setText("Output", juce::dontSendNotification);
    m_inLabel.setText("Input", juce::dontSendNotification);
    m_exportFormatLabel.setText("Export format", juce::dontSendNotification);
    m_inHelp.setText(
        "None = no microphone access. Pick a device only when you need external audio in.",
        juce::dontSendNotification);
    m_inHelp.setJustificationType(juce::Justification::topLeft);
    m_inMeterLabel.setText("Level", juce::dontSendNotification);
    m_status.setJustificationType(juce::Justification::centredLeft);

    for (int i = 0; i < HostAudioConfig::kNumSupportedRates; ++i)
    {
        const int rateHz = static_cast<int>(HostAudioConfig::kSupportedRates[i]);
        m_sampleRate.addItem(juce::String(rateHz) + " Hz", i + 1);
    }

    m_testButton.setTooltip("Play a test tone on the selected output device");
    m_testButton.onClick = [this]() { m_engine.getDeviceManager().playTestSound(); };

    m_sampleRate.onChange = [this]() { applySampleRate(); };
    m_outDevice.onChange = [this]() { applyOutputDevice(); };
    m_inDevice.onChange = [this]() { applyInputDevice(); };
    m_refresh.onClick = [this]() { refreshDeviceLists(); };
    m_close.onClick = [this]() {
        if (m_onClose)
        {
            m_onClose();
        }
    };

    constexpr int kExportFormatRadioGroup = 42;
    initExportFormatToggle(m_fmtWav, kExportFormatRadioGroup);
    initExportFormatToggle(m_fmtMp3, kExportFormatRadioGroup);
    initExportFormatToggle(m_fmtFlac, kExportFormatRadioGroup);
    initExportFormatToggle(m_fmtOgg, kExportFormatRadioGroup);

    for (juce::ToggleButton* btn : {&m_fmtWav, &m_fmtMp3, &m_fmtFlac, &m_fmtOgg})
    {
        btn->onClick = [this]() { applyExportFormat(); };
    }
    syncExportFormatUi();

    m_inMeter = std::make_unique<InputLevelMeter>(engine, m_engine.getDeviceManager());
    addAndMakeVisible(*m_inMeter);

    for (juce::Component* c : {static_cast<juce::Component*>(&m_rateLabel),
                               static_cast<juce::Component*>(&m_sampleRate),
                               static_cast<juce::Component*>(&m_outLabel),
                               static_cast<juce::Component*>(&m_outDevice),
                               static_cast<juce::Component*>(&m_testButton),
                               static_cast<juce::Component*>(&m_inLabel),
                               static_cast<juce::Component*>(&m_inDevice),
                               static_cast<juce::Component*>(&m_inHelp),
                               static_cast<juce::Component*>(&m_inMeterLabel),
                               static_cast<juce::Component*>(&m_exportFormatLabel),
                               static_cast<juce::Component*>(&m_fmtWav),
                               static_cast<juce::Component*>(&m_fmtMp3),
                               static_cast<juce::Component*>(&m_fmtFlac),
                               static_cast<juce::Component*>(&m_fmtOgg),
                               static_cast<juce::Component*>(&m_refresh),
                               static_cast<juce::Component*>(&m_status),
                               static_cast<juce::Component*>(&m_close)})
    {
        addAndMakeVisible(c);
    }

    refreshDeviceLists();
    updateStatus();
    startTimerHz(4);
    if (m_presentation == AudioSettingsPresentation::RuntimePage)
    {
        m_close.setVisible(false);
    }
    setSize(480, m_presentation == AudioSettingsPresentation::RuntimePage ? 360 : 292);
}

void AudioSettingsComponent::timerCallback()
{
    updateStatus();
}

double AudioSettingsComponent::selectedSampleRate() const
{
    const int id = m_sampleRate.getSelectedId();
    if (id >= 1 && id <= HostAudioConfig::kNumSupportedRates)
    {
        return HostAudioConfig::kSupportedRates[id - 1];
    }
    return HostAudioConfig::kDefaultSampleRate;
}

ExportFormat AudioSettingsComponent::selectedExportFormat() const
{
    if (m_fmtMp3.getToggleState())
    {
        return ExportFormat::Mp3;
    }
    if (m_fmtFlac.getToggleState())
    {
        return ExportFormat::Flac;
    }
    if (m_fmtOgg.getToggleState())
    {
        return ExportFormat::Ogg;
    }
    return ExportFormat::Wav;
}

void AudioSettingsComponent::syncExportFormatUi()
{
    const ExportFormat format = m_engine.exportFormat();
    m_fmtWav.setToggleState(format == ExportFormat::Wav, juce::dontSendNotification);
    m_fmtMp3.setToggleState(format == ExportFormat::Mp3, juce::dontSendNotification);
    m_fmtFlac.setToggleState(format == ExportFormat::Flac, juce::dontSendNotification);
    m_fmtOgg.setToggleState(format == ExportFormat::Ogg, juce::dontSendNotification);
}

void AudioSettingsComponent::applyExportFormat()
{
    m_engine.setExportFormat(selectedExportFormat());
}

juce::AudioIODeviceType* AudioSettingsComponent::getDeviceType() const
{
    auto& types = m_engine.getDeviceManager().getAvailableDeviceTypes();
    if (types.isEmpty())
    {
        return nullptr;
    }
    if (auto* current = m_engine.getDeviceManager().getCurrentDeviceTypeObject())
    {
        return current;
    }
    return types.getFirst();
}

void AudioSettingsComponent::refreshDeviceLists()
{
    auto* type = getDeviceType();
    if (type == nullptr)
    {
        return;
    }
    type->scanForDevices();

    juce::AudioDeviceManager::AudioDeviceSetup setup;
    m_engine.getDeviceManager().getAudioDeviceSetup(setup);
    const juce::String prevOut = setup.outputDeviceName;
    const juce::String prevIn = setup.inputDeviceName;
    const double prevRate = setup.sampleRate;

    m_outDevice.clear();
    m_inDevice.clear();
    m_inDevice.addItem("None", 1);

    const juce::StringArray outputs = type->getDeviceNames(false);
    int outIdx = 1;
    for (const auto& name : outputs)
    {
        m_outDevice.addItem(name, outIdx++);
    }

    int inIdx = 2;
    for (const auto& name : type->getDeviceNames(true))
    {
        m_inDevice.addItem(name, inIdx++);
    }

    int selectOut = 1;
    for (int i = 0; i < outputs.size(); ++i)
    {
        if (outputs[i] == prevOut)
        {
            selectOut = i + 1;
            break;
        }
    }
    m_outDevice.setSelectedId(selectOut, juce::dontSendNotification);

    if (prevIn.isEmpty())
    {
        m_inDevice.setSelectedId(1, juce::dontSendNotification);
    }
    else
    {
        int selectIn = 1;
        const juce::StringArray inputs = type->getDeviceNames(true);
        for (int i = 0; i < inputs.size(); ++i)
        {
            if (inputs[i] == prevIn)
            {
                selectIn = i + 2;
                break;
            }
        }
        m_inDevice.setSelectedId(selectIn, juce::dontSendNotification);
    }

    const double nearestRate = HostAudioConfig::nearestSupportedRate(prevRate);
    m_sampleRate.setSelectedId(HostAudioConfig::indexForRate(nearestRate) + 1, juce::dontSendNotification);

    updateStatus();
}

void AudioSettingsComponent::applySampleRate()
{
    juce::AudioDeviceManager::AudioDeviceSetup setup;
    m_engine.getDeviceManager().getAudioDeviceSetup(setup);
    setup.sampleRate = selectedSampleRate();

    const juce::String error = m_engine.getDeviceManager().setAudioDeviceSetup(setup, true);
    if (error.isNotEmpty())
    {
        m_status.setText(error, juce::dontSendNotification);
        m_status.setColour(juce::Label::textColourId, juce::Colours::orange);
        return;
    }
    updateStatus();
}

void AudioSettingsComponent::applyOutputDevice()
{
    auto* type = getDeviceType();
    if (type == nullptr)
    {
        return;
    }

    const int id = m_outDevice.getSelectedId();
    if (id < 1)
    {
        return;
    }

    const juce::StringArray outputs = type->getDeviceNames(false);
    const int index = id - 1;
    if (index < 0 || index >= outputs.size())
    {
        return;
    }

    juce::AudioDeviceManager::AudioDeviceSetup setup;
    m_engine.getDeviceManager().getAudioDeviceSetup(setup);
    setup.outputDeviceName = outputs[index];
    setup.outputChannels.clear();
    setup.outputChannels.setBit(0);
    setup.outputChannels.setBit(1);
    setup.useDefaultOutputChannels = false;
    setup.sampleRate = selectedSampleRate();

    const juce::String error = m_engine.getDeviceManager().setAudioDeviceSetup(setup, true);
    if (error.isNotEmpty())
    {
        m_status.setText(error, juce::dontSendNotification);
        m_status.setColour(juce::Label::textColourId, juce::Colours::orange);
        return;
    }
    updateStatus();
}

void AudioSettingsComponent::applyInputDevice()
{
    auto* type = getDeviceType();
    if (type == nullptr)
    {
        return;
    }

    const int id = m_inDevice.getSelectedId();
    if (id < 1)
    {
        return;
    }

    juce::AudioDeviceManager::AudioDeviceSetup setup;
    m_engine.getDeviceManager().getAudioDeviceSetup(setup);
    setup.sampleRate = selectedSampleRate();

    if (id == 1)
    {
        setup.inputDeviceName.clear();
        setup.inputChannels.clear();
        setup.useDefaultInputChannels = false;
    }
    else
    {
        const juce::StringArray inputs = type->getDeviceNames(true);
        const int index = id - 2;
        if (index < 0 || index >= inputs.size())
        {
            return;
        }
        setup.inputDeviceName = inputs[index];
        setup.inputChannels.clear();
        setup.inputChannels.setBit(0);
        setup.useDefaultInputChannels = false;
    }

    const juce::String error = m_engine.getDeviceManager().setAudioDeviceSetup(setup, true);
    if (error.isNotEmpty())
    {
        m_status.setText(error, juce::dontSendNotification);
        m_status.setColour(juce::Label::textColourId, juce::Colours::orange);
        return;
    }
    updateStatus();
}

void AudioSettingsComponent::updateStatus()
{
    juce::AudioDeviceManager::AudioDeviceSetup setup;
    m_engine.getDeviceManager().getAudioDeviceSetup(setup);

    const int rateHz = static_cast<int>(m_engine.getHostSampleRate());
    juce::String line = "Audio: " + juce::String(rateHz) + " Hz";

    if (setup.inputDeviceName.isEmpty())
    {
        line += " — Input: none (no microphone permission requested).";
        m_status.setText(line, juce::dontSendNotification);
        m_status.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
        return;
    }

    if (auto* device = m_engine.getDeviceManager().getCurrentAudioDevice())
    {
        const int inCh = device->getActiveInputChannels().countNumberOfSetBits();
        if (inCh == 0)
        {
            line += " — Input device selected but no channels active.";
            m_status.setText(line, juce::dontSendNotification);
            m_status.setColour(juce::Label::textColourId, juce::Colours::orange);
            return;
        }
    }

    line += " — Input active. Enable Ext. In. on the main bar to route audio.";
    m_status.setText(line, juce::dontSendNotification);
    m_status.setColour(juce::Label::textColourId, juce::Colours::lightgreen);
}

void AudioSettingsComponent::resized()
{
    auto area = getLocalBounds().reduced(12);

    m_rateLabel.setBounds(area.removeFromTop(16));
    m_sampleRate.setBounds(area.removeFromTop(26));
    area.removeFromTop(10);

    m_outLabel.setBounds(area.removeFromTop(16));
    auto outRow = area.removeFromTop(26);
    m_testButton.setBounds(outRow.removeFromRight(64));
    outRow.removeFromRight(8);
    m_outDevice.setBounds(outRow);
    area.removeFromTop(10);

    m_inLabel.setBounds(area.removeFromTop(16));
    auto inRow = area.removeFromTop(26);
    m_inMeter->setBounds(inRow.removeFromRight(72));
    inRow.removeFromRight(6);
    m_inMeterLabel.setBounds(inRow.removeFromRight(40));
    inRow.removeFromRight(6);
    m_inDevice.setBounds(inRow);
    area.removeFromTop(4);
    m_inHelp.setBounds(area.removeFromTop(32));
    area.removeFromTop(8);

    m_exportFormatLabel.setBounds(area.removeFromTop(16));
    auto formatRow = area.removeFromTop(26);
    const int fmtW = formatRow.getWidth() / 4;
    m_fmtWav.setBounds(formatRow.removeFromLeft(fmtW).reduced(0, 0));
    m_fmtMp3.setBounds(formatRow.removeFromLeft(fmtW));
    m_fmtFlac.setBounds(formatRow.removeFromLeft(fmtW));
    m_fmtOgg.setBounds(formatRow);
    area.removeFromTop(8);

    auto refreshRow = area.removeFromTop(26);
    m_refresh.setBounds(refreshRow.removeFromLeft(140));
    area.removeFromTop(4);
    m_status.setBounds(area.removeFromTop(36));
    if (m_presentation == AudioSettingsPresentation::Dialog)
    {
        area.removeFromTop(8);
        m_close.setBounds(area.removeFromTop(28).removeFromRight(80));
    }
}
