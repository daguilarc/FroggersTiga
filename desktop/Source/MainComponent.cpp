#include "MainComponent.h"

#include "DesktopChromeLayout.hpp"
#include "ParamDisplayNames.hpp"

namespace
{
struct KeyNote
{
    int keyCode;
    int note;
};

constexpr KeyNote kQwertyPianoKeys[] = {
    {'A', 60}, {'W', 61}, {'S', 62}, {'E', 63}, {'D', 64}, {'F', 65}, {'T', 66}, {'G', 67},
    {'Y', 68}, {'H', 69}, {'U', 70}, {'J', 71}, {'K', 72}, {'O', 73}, {'L', 74}, {'P', 75},
};

constexpr int kQwertyPitchLow = 60;
constexpr float kQwertyPitchSpan = 16.0f;

int noteForKeyCode(int keyCode)
{
    for (const KeyNote& keyNote : kQwertyPianoKeys)
    {
        if (keyNote.keyCode == keyCode)
        {
            return keyNote.note;
        }
    }
    return -1;
}

bool isModalDialogOpen()
{
    return juce::ModalComponentManager::getInstance()->getNumModalComponents() > 0;
}

bool isTextEntryFocused()
{
    juce::Component* focused = juce::Component::getCurrentlyFocusedComponent();
    return focused != nullptr && dynamic_cast<juce::TextEditor*>(focused) != nullptr;
}
} // namespace

AudioEngine& MainComponent::audioEngine()
{
    return *m_audioEngine;
}

const AudioEngine& MainComponent::audioEngine() const
{
    return *m_audioEngine;
}

MainComponent::MainComponent()
    : m_ownedAudio(std::in_place)
    , m_audioEngine(&*m_ownedAudio)
    , m_modRack(audioEngine().getHost())
    , m_cableOverlay(audioEngine().getHost(), audioEngine().getDelay())
    , m_strip(audioEngine().getHost(), audioEngine().getDelay())
{
    initFromEngine();
}

MainComponent::MainComponent(AudioEngine& externalEngine)
    : m_audioEngine(&externalEngine)
    , m_modRack(audioEngine().getHost())
    , m_cableOverlay(audioEngine().getHost(), audioEngine().getDelay())
    , m_strip(audioEngine().getHost(), audioEngine().getDelay())
{
    initFromEngine();
}

void MainComponent::initFromEngine()
{
    const char* titles[] = {
        ParamDisplayNames::forHostPage(0),
        ParamDisplayNames::forHostPage(1),
        ParamDisplayNames::forHostPage(2),
        ParamDisplayNames::forHostPage(3),
        ParamDisplayNames::forHostPage(4),
        ParamDisplayNames::forHostPage(5),
    };
    for (int i = 0; i < 5; i++)
    {
        m_coreBackends[static_cast<size_t>(i)] =
            std::make_unique<DesktopPanelBackend>(static_cast<uint8_t>(i), audioEngine().getHost());
        m_panels[static_cast<size_t>(i)] = std::make_unique<SubModulePanel>(
            static_cast<uint8_t>(i), titles[i], *m_coreBackends[static_cast<size_t>(i)]);
        addAndMakeVisible(m_panels[static_cast<size_t>(i)].get());
    }
    m_delayBackend = std::make_unique<DelayHostBackend>(audioEngine().getDelay(), audioEngine().getHost());
    m_panels[5] = std::make_unique<SubModulePanel>(
        DelayState::kDelayPageIndex, titles[5], *m_delayBackend);
    addAndMakeVisible(m_panels[5].get());

    addAndMakeVisible(m_modRack);
    addAndMakeVisible(m_strip);
    addAndMakeVisible(m_cableOverlay);

    m_play.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2ea043));
    m_play.onClick = [this]() {
        audioEngine().startAudio();
        updateTransportUi();
        grabKeyboardFocus();
    };
    addAndMakeVisible(m_play);

    m_stop.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffda3633));
    m_stop.onClick = [this]() {
        audioEngine().stopAudio();
        updateTransportUi();
    };
    addAndMakeVisible(m_stop);

    m_externalInput.setToggleState(false, juce::dontSendNotification);
    m_externalInput.setTooltip(
        "Ext. In.: route line/mic to engine (off = VCO-only). Ring mod opens above Schmidt gate.");
    m_externalInput.onClick = [this]() {
        audioEngine().setExternalInputEnabled(m_externalInput.getToggleState());
        const bool active = audioEngine().isExternalInputEnabled() && audioEngine().isAudioRunning();
        m_inputEnvelope.setActive(active);
        m_inputEnvelope.setLevel(audioEngine().getInputPeakLevel());
    };
    addAndMakeVisible(m_externalInput);

    m_inputEnvelope.setTooltip(
        "Input peak (Ext. In. on + Play). Ring mod opens above Schmidt threshold.");
    m_inputEnvelope.setInterceptsMouseClicks(false, false);
    m_inputEnvelope.setActive(false);
    addAndMakeVisible(m_inputEnvelope);

    m_routeHint.setJustificationType(juce::Justification::centredLeft);
    m_routeHint.setColour(juce::Label::textColourId, juce::Colour(0xffffa657));
    m_routeHint.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(m_routeHint);

    audioEngine().setTransportChangedCallback([this]() { updateTransportUi(); });

    m_audioSettings.onClick = [this]() { audioEngine().showAudioSettings(this); };
    m_audioSettings.setTooltip("Audio device settings");
    addAndMakeVisible(m_audioSettings);

    m_recordCluster.onRecordClick = [this]() { handleRecordClick(); };
    addAndMakeVisible(m_recordCluster);

    m_midiSettings.onClick = [this]() { audioEngine().showMidiSettings(this); };
    m_midiSettings.setTooltip("MIDI In → mod rack; MIDI Out (VCO Env) to hardware");
    addAndMakeVisible(m_midiSettings);

    if (audioEngine().isPluginHosted())
    {
        m_audioSettings.setVisible(false);
        m_play.setVisible(false);
        m_stop.setVisible(false);
    }

    m_cableOverlay.setConnectionChangedCallback([this](uint8_t page) {
        if (page < m_panels.size() && m_panels[static_cast<size_t>(page)])
        {
            m_panels[static_cast<size_t>(page)]->refresh();
        }
    });

    setWantsKeyboardFocus(true);

    setSize(DesktopChromeLayout::kDefaultWidth, DesktopChromeLayout::kDefaultHeight);
    updateTransportUi();
    startTimerHz(15);
}

void MainComponent::updateTransportUi()
{
    const bool running = audioEngine().isAudioRunning();
    m_play.setEnabled(!running);
    m_stop.setEnabled(running);
}

void MainComponent::handleRecordClick()
{
    if (!audioEngine().isRecording())
    {
        if (!audioEngine().startRecording())
        {
            juce::AlertWindow::showMessageBoxAsync(
                juce::AlertWindow::WarningIcon,
                "Record",
                "Press Play before recording.");
            return;
        }
        m_recordCluster.setRecording(true);
        return;
    }

    audioEngine().stopRecording();
    m_recordCluster.setRecording(false);
    if (!audioEngine().hasCapturedAudio())
    {
        return;
    }
    if (audioEngine().wasLastCaptureTruncated())
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon,
            "Record",
            "Recording stopped at the 30-minute limit.");
    }
    const ExportFormat format = m_recordCluster.selectedFormat();
    audioEngine().exportCapturedAudio(format, this, [](bool success, const juce::String& message) {
        juce::AlertWindow::showMessageBoxAsync(
            success ? juce::AlertWindow::InfoIcon : juce::AlertWindow::WarningIcon,
            "Export",
            message);
    });
}

void MainComponent::mouseDown(const juce::MouseEvent& event)
{
    juce::Component::mouseDown(event);
    grabKeyboardFocus();
}

bool MainComponent::shouldCaptureQwertyMidi() const
{
    if (!audioEngine().isComputerKeyboardMidiEnabled())
    {
        return false;
    }
    if (isModalDialogOpen())
    {
        return false;
    }
    if (isTextEntryFocused())
    {
        return false;
    }
    return true;
}

void MainComponent::recomputeQwertyCcFromHeldKeys()
{
    uint8_t maxVel = 0;
    int highestNote = -1;
    for (size_t i = 0; i < m_qwertyHeldVelocity.size(); i++)
    {
        const uint8_t vel = m_qwertyHeldVelocity[i];
        if (vel == 0)
        {
            continue;
        }
        if (vel > maxVel)
        {
            maxVel = vel;
        }
        if (highestNote < static_cast<int>(i))
        {
            highestNote = static_cast<int>(i);
        }
    }

    uint8_t ccValue = 0;
    if (highestNote >= 0 && maxVel > 0)
    {
        float pitchStep =
            static_cast<float>(highestNote - kQwertyPitchLow + 1) / kQwertyPitchSpan;
        pitchStep = juce::jlimit(0.0f, 1.0f, pitchStep);
        ccValue = static_cast<uint8_t>(juce::jlimit(0, 127, static_cast<int>(pitchStep * maxVel)));
    }
    audioEngine().feedComputerKeyboardCc1(ccValue);
}

bool MainComponent::keyPressed(const juce::KeyPress& key)
{
    if (!shouldCaptureQwertyMidi())
    {
        return false;
    }
    const int note = noteForKeyCode(key.getKeyCode());
    if (note < 0)
    {
        return false;
    }
    m_qwertyHeldVelocity[static_cast<size_t>(note)] = 127;
    recomputeQwertyCcFromHeldKeys();
    return true;
}

bool MainComponent::keyStateChanged(bool /*isKeyDown*/)
{
    if (!shouldCaptureQwertyMidi())
    {
        return false;
    }
    bool handled = false;
    for (const KeyNote& keyNote : kQwertyPianoKeys)
    {
        if (juce::KeyPress::isKeyCurrentlyDown(keyNote.keyCode))
        {
            m_qwertyHeldVelocity[static_cast<size_t>(keyNote.note)] = 127;
            handled = true;
        }
        else
        {
            m_qwertyHeldVelocity[static_cast<size_t>(keyNote.note)] = 0;
        }
    }
    recomputeQwertyCcFromHeldKeys();
    return handled;
}

void MainComponent::syncPatchPorts()
{
    std::vector<PatchCableOverlay::OutputPort> outputs;
    m_modRack.collectOutputPorts(outputs);
    std::vector<PatchCableOverlay::InputPort> inputs;
    for (auto& panel : m_panels)
    {
        panel->collectInputPorts(inputs);
    }
    m_cableOverlay.setOutputPorts(std::move(outputs));
    m_cableOverlay.setInputPorts(std::move(inputs));
}

void MainComponent::resized()
{
    using namespace DesktopChromeLayout;

    auto area = getLocalBounds().reduced(8);
    const int kHeaderHeight = kTransportRowH + kModRackRowH;
    const auto header = area.removeFromTop(kHeaderHeight);

    constexpr int kRecordW = 88;
    auto transportGlobal =
        juce::Rectangle<int>(header.getX(), header.getY(), header.getWidth(), kTransportRowH);
    auto topBar = transportGlobal;
    const auto recordGlobal = topBar.removeFromRight(kRecordW);
    m_audioSettings.setBounds(topBar.removeFromRight(80));
    topBar.removeFromRight(8);
    m_midiSettings.setBounds(topBar.removeFromRight(80));
    topBar.removeFromRight(12);
    m_play.setBounds(topBar.removeFromLeft(72));
    topBar.removeFromLeft(6);
    m_stop.setBounds(topBar.removeFromLeft(72));
    topBar.removeFromLeft(12);
    m_externalInput.setBounds(topBar.removeFromLeft(72));
    topBar.removeFromLeft(6);
    m_inputEnvelope.setBounds(topBar.removeFromLeft(80));
    topBar.removeFromLeft(8);
    m_routeHint.setBounds(topBar.removeFromLeft(juce::jmin(topBar.getWidth(), 280)));

    const auto rackGlobal = juce::Rectangle<int>(
        header.getX(),
        header.getY() + kTransportRowH,
        header.getWidth() - kRecordClusterW,
        kModRackRowH);
    m_modRack.setBounds(rackGlobal);

    const auto formatGlobal = juce::Rectangle<int>(
        header.getRight() - kRecordClusterW,
        header.getY() + kTransportRowH,
        kRecordClusterW,
        kModRackRowH);
    const auto clusterGlobal = recordGlobal.getUnion(formatGlobal);
    m_recordCluster.setBounds(clusterGlobal);
    const auto recordLocal = recordGlobal.translated(-clusterGlobal.getX(), -clusterGlobal.getY());
    const auto formatLocal = formatGlobal.translated(-clusterGlobal.getX(), -clusterGlobal.getY());
    m_recordCluster.layoutChrome(recordLocal, formatLocal);

    area.removeFromTop(6);

    m_strip.setBounds(area.removeFromBottom(40));
    area.removeFromBottom(6);

    const int panelW = area.getWidth() / 6;
    for (int i = 0; i < 6; i++)
    {
        const int w = (i == 5) ? area.getWidth() : panelW;
        m_panels[static_cast<size_t>(i)]->setBounds(area.removeFromLeft(w));
    }

    m_cableOverlay.setBounds(getLocalBounds());
    juce::Component* transport[] = {&m_play,
                                    &m_stop,
                                    &m_externalInput,
                                    &m_inputEnvelope,
                                    &m_routeHint,
                                    &m_audioSettings,
                                    &m_midiSettings,
                                    &m_recordCluster};
    for (juce::Component* component : transport)
    {
        component->toFront(false);
    }
    m_cableOverlay.toFront(false);
    syncPatchPorts();
}

void MainComponent::timerCallback()
{
    const bool running = audioEngine().isAudioRunning();
    if (!running)
    {
        audioEngine().getHost().DrainPendingMutations();
    }
    m_modRack.refresh(running);
    for (auto& panel : m_panels)
    {
        panel->refresh();
    }

    const bool extOn = audioEngine().isExternalInputEnabled();
    m_inputEnvelope.setActive(extOn && running);
    m_inputEnvelope.setLevel(extOn && running ? audioEngine().getInputPeakLevel() : 0.0f);

    if (extOn && running && audioEngine().getInputRouteStatus() != InputRouteStatus::Ok)
    {
        m_routeHint.setText(audioEngine().getInputRouteMessage(), juce::dontSendNotification);
    }
    else
    {
        m_routeHint.setText({}, juce::dontSendNotification);
    }

    syncPatchPorts();
    m_cableOverlay.repaint();
}
