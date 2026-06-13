#include "MainComponent.h"

#include "DesktopChromeLayout.hpp"

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

MainComponent::MainComponent()
    : m_modRack(m_audio.getHost())
    , m_cableOverlay(m_audio.getHost(), m_audio.getDelay())
    , m_strip(m_audio.getHost(), m_audio.getDelay())
{
    const char* titles[] = {"Audio", "Marbles", "Reverb", "Filter", "Drive", "Delay"};
    for (int i = 0; i < 5; i++)
    {
        m_coreBackends[static_cast<size_t>(i)] =
            std::make_unique<DesktopPanelBackend>(static_cast<uint8_t>(i), m_audio.getHost());
        m_panels[static_cast<size_t>(i)] = std::make_unique<SubModulePanel>(
            static_cast<uint8_t>(i), titles[i], *m_coreBackends[static_cast<size_t>(i)]);
        addAndMakeVisible(m_panels[static_cast<size_t>(i)].get());
    }
    m_delayBackend = std::make_unique<DelayHostBackend>(m_audio.getDelay(), m_audio.getHost());
    m_panels[5] = std::make_unique<SubModulePanel>(
        DelayState::kDelayPageIndex, titles[5], *m_delayBackend);
    addAndMakeVisible(m_panels[5].get());

    addAndMakeVisible(m_modRack);
    addAndMakeVisible(m_strip);
    addAndMakeVisible(m_cableOverlay);

    m_play.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2ea043));
    m_play.onClick = [this]() {
        m_audio.startAudio();
        updateTransportUi();
        grabKeyboardFocus();
    };
    addAndMakeVisible(m_play);

    m_stop.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffda3633));
    m_stop.onClick = [this]() {
        m_audio.stopAudio();
        updateTransportUi();
    };
    addAndMakeVisible(m_stop);

    m_externalInput.setToggleState(false, juce::dontSendNotification);
    m_externalInput.setTooltip(
        "Ext. In.: route line/mic to engine (off = VCO-only). Ring mod opens above Schmidt gate.");
    m_externalInput.onClick = [this]() {
        m_audio.setExternalInputEnabled(m_externalInput.getToggleState());
        const bool active = m_audio.isExternalInputEnabled() && m_audio.isAudioRunning();
        m_inputEnvelope.setActive(active);
        m_inputEnvelope.setLevel(m_audio.getInputPeakLevel());
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

    m_audio.setTransportChangedCallback([this]() { updateTransportUi(); });

    m_audioSettings.onClick = [this]() { m_audio.showAudioSettings(this); };
    m_audioSettings.setTooltip("Audio device settings");
    addAndMakeVisible(m_audioSettings);

    m_recordCluster.onRecordClick = [this]() { handleRecordClick(); };
    addAndMakeVisible(m_recordCluster);

    m_midiSettings.onClick = [this]() { m_audio.showMidiSettings(this); };
    m_midiSettings.setTooltip("MIDI In → mod rack; MIDI Out (VCO Env) to hardware");
    addAndMakeVisible(m_midiSettings);

    m_cableOverlay.setConnectionChangedCallback([this](uint8_t page) {
        if (page < m_panels.size() && m_panels[static_cast<size_t>(page)])
        {
            m_panels[static_cast<size_t>(page)]->refresh();
        }
    });

    m_keyboardState.addListener(this);
    setWantsKeyboardFocus(true);

    setSize(DesktopChromeLayout::kDefaultWidth, DesktopChromeLayout::kDefaultHeight);
    updateTransportUi();
    startTimerHz(15);
}

void MainComponent::updateTransportUi()
{
    const bool running = m_audio.isAudioRunning();
    m_play.setEnabled(!running);
    m_stop.setEnabled(running);
}

bool MainComponent::shouldCaptureQwertyMidi() const
{
    if (!m_audio.isComputerKeyboardMidiEnabled())
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

void MainComponent::handleRecordClick()
{
    if (!m_audio.isRecording())
    {
        if (!m_audio.startRecording())
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

    m_audio.stopRecording();
    m_recordCluster.setRecording(false);
    if (!m_audio.hasCapturedAudio())
    {
        return;
    }
    if (m_audio.wasLastCaptureTruncated())
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon,
            "Record",
            "Recording stopped at the 30-minute limit.");
    }
    const ExportFormat format = m_recordCluster.selectedFormat();
    m_audio.exportCapturedAudio(format, this, [](bool success, const juce::String& message) {
        juce::AlertWindow::showMessageBoxAsync(
            success ? juce::AlertWindow::InfoIcon : juce::AlertWindow::WarningIcon,
            "Export",
            message);
    });
}

void MainComponent::handleNoteOn(juce::MidiKeyboardState*,
                                 int,
                                 int midiNoteNumber,
                                 float velocity)
{
    if (!shouldCaptureQwertyMidi())
    {
        return;
    }
    const uint8_t ch = m_audio.getHost().m_midiBridge.m_inChannel;
    const uint8_t vel = static_cast<uint8_t>(juce::jlimit(1, 127, static_cast<int>(velocity * 127.0f)));
    m_audio.feedMidiInNote(ch, static_cast<uint8_t>(midiNoteNumber), vel, true);
}

void MainComponent::handleNoteOff(juce::MidiKeyboardState*,
                                  int,
                                  int midiNoteNumber,
                                  float)
{
    if (!shouldCaptureQwertyMidi())
    {
        return;
    }
    const uint8_t ch = m_audio.getHost().m_midiBridge.m_inChannel;
    m_audio.feedMidiInNote(ch, static_cast<uint8_t>(midiNoteNumber), 0, false);
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
    m_keyboardState.noteOn(1, note, 1.0f);
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
            m_keyboardState.noteOn(1, keyNote.note, 1.0f);
            handled = true;
        }
        else
        {
            m_keyboardState.noteOff(1, keyNote.note, 1.0f);
        }
    }
    return handled;
}

void MainComponent::mouseDown(const juce::MouseEvent& event)
{
    juce::Component::mouseDown(event);
    grabKeyboardFocus();
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
        m_panels[static_cast<size_t>(i)]->setBounds(area.removeFromLeft(panelW).reduced(2));
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
    const bool running = m_audio.isAudioRunning();
    if (!running)
    {
        m_audio.getHost().DrainPendingMutations();
    }
    m_modRack.refresh(running);
    for (auto& panel : m_panels)
    {
        panel->refresh();
    }

    const bool extOn = m_audio.isExternalInputEnabled();
    m_inputEnvelope.setActive(extOn && running);
    m_inputEnvelope.setLevel(extOn && running ? m_audio.getInputPeakLevel() : 0.0f);

    if (extOn && running && m_audio.getInputRouteStatus() != InputRouteStatus::Ok)
    {
        m_routeHint.setText(m_audio.getInputRouteMessage(), juce::dontSendNotification);
    }
    else
    {
        m_routeHint.setText({}, juce::dontSendNotification);
    }

    syncPatchPorts();
    m_cableOverlay.repaint();
}
