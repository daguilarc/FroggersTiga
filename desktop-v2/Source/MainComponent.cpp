#include "MainComponent.h"

#include "DesktopV2HostCallbacks.hpp"
#include "ui/DesktopV2ChromeLayout.hpp"
#include "ui/DesktopV2TransportLayout.hpp"
#include "PermanentModTapRack.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

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
    : m_audio(std::in_place)
    , m_facade(m_audio.value())
    , m_filePatchPage(*m_audio)
    , m_audioPage(*m_audio)
    , m_controllersPage(*m_audio)
    , m_hostCallbacks(
          m_facade.controlCore(),
          m_facade.hostBridge(),
          m_facade.host(),
          m_carousel,
          m_lastModRoutesVersion)
{
    m_facade.initialize();

    m_globalOscilloscope.bindHost(&m_facade.host());
    m_globalStrip.bind(&m_facade.host(), &m_facade.controlCore());
    m_carousel.bindHost(&m_facade.host(), &m_facade.controlCore());
    m_performanceBand.bind(&m_facade.controlCore());
    m_performanceBand.bindHost(&m_facade.host());
    m_globalStrip.resolveRandSeqScope = [this]() {
        return m_sequencerPanel.getRandSeqScope();
    };
    // Carryover C1: let the sequencer's Rand-seq dice read the global
    // All Steps / Current Step scope so it can resolve Pattern scope instead
    // of being stuck at Step (mirrors resolveRandSeqScope above).
    m_sequencerPanel.resolveAllStepsScope = [this]() { return m_globalStrip.allStepsScope(); };
    m_sequencerPanel.bind(
        &m_audio->getSequencer(), &m_facade.controlCore(), &m_facade.hostBridge());

    addAndMakeVisible(m_globalOscilloscope);
    addAndMakeVisible(m_globalStrip);
    addAndMakeVisible(m_performanceBand);
    addAndMakeVisible(m_carousel);
    addAndMakeVisible(m_sequencerPanel);

    m_play.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2ea043));
    m_play.onClick = [this]() {
        m_audio->startAudio();
        updateTransportUi();
        grabKeyboardFocus();
    };
    addAndMakeVisible(m_play);

    m_stop.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffda3633));
    m_stop.onClick = [this]() {
        m_audio->stopAudio();
        updateTransportUi();
    };
    addAndMakeVisible(m_stop);

    m_recordButton.onClick = [this]() { handleRecordClick(); };
    addAndMakeVisible(m_recordButton);

    m_fileButton.onClick = [this]() {
        selectRuntimePage(m_activeRuntimePage == RuntimePageKind::FilePatch ? RuntimePageKind::None
                                                                            : RuntimePageKind::FilePatch);
    };
    m_audioButton.onClick = [this]() {
        selectRuntimePage(m_activeRuntimePage == RuntimePageKind::Audio ? RuntimePageKind::None
                                                                        : RuntimePageKind::Audio);
    };
    m_midiButton.onClick = [this]() {
        selectRuntimePage(m_activeRuntimePage == RuntimePageKind::Controllers
                              ? RuntimePageKind::None
                              : RuntimePageKind::Controllers);
    };

    addAndMakeVisible(m_fileButton);
    addAndMakeVisible(m_audioButton);
    addAndMakeVisible(m_midiButton);
    addChildComponent(m_filePatchPage);
    addChildComponent(m_audioPage);
    addChildComponent(m_controllersPage);

    m_audio->setTransportChangedCallback([this]() { updateTransportUi(); });

    wireCallbacks();
    wireMidiCvCallbacks();
    m_carousel.selectPage(m_facade.config().defaultPage, false);

    setWantsKeyboardFocus(true);
    setSize(DesktopV2ChromeLayout::kDefaultWidth, DesktopV2ChromeLayout::kDefaultHeight);
    updateTransportUi();
    updateRuntimePageVisibility();
    startTimerHz(15);
}

void MainComponent::wireCallbacks()
{
    desktop_v2::refreshAndWireHostCallbacks(m_hostCallbacks,
                                            m_facade.controlCore(),
                                            m_facade.hostBridge(),
                                            m_facade.host(),
                                            m_carousel,
                                            m_lastModRoutesVersion);
}

void MainComponent::pushSelectPage(uint8_t page)
{
    desktop_v2::pushSelectPage(m_hostCallbacks, page);
}

void MainComponent::wireMidiCvCallbacks()
{
    MidiCvAssignmentTable& table = m_audio->getMidiCvTable();
    table.setUiShiftCallback([this](bool held) {
        juce::MessageManager::callAsync([this, held]() {
            m_globalStrip.setShiftHeld(held);
        });
    });
    table.setUiSceneCallback([this](uint8_t ordinal) {
        juce::MessageManager::callAsync([this, ordinal]() {
            froggers_v2::MessageIn message;
            message.type = froggers_v2::MessageIn::Type::SceneSelect;
            message.index = ordinal;
            m_facade.ingestMessage(message);
            m_carousel.refresh();
        });
    });
}

void MainComponent::drainMidiUiActions()
{
    m_audio->getMidiCvTable().drainPendingUiActions();
}

bool MainComponent::shouldCaptureQwertyMidi() const
{
    if (!m_audio->isComputerKeyboardMidiEnabled())
    {
        return false;
    }
    if (isModalDialogOpen() || isTextEntryFocused())
    {
        return false;
    }
    return true;
}

int MainComponent::noteForKeyCode(int keyCode) const
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

void MainComponent::syncQwertyKey(int note, bool down)
{
    if (note < 0 || note >= static_cast<int>(m_qwertyHeldVelocity.size()))
    {
        return;
    }
    const uint8_t channel = m_audio->getMidiCvTable().qwertyMidiChannel;
    if (down)
    {
        m_qwertyHeldVelocity[static_cast<size_t>(note)] = 127;
        m_audio->feedVirtualMidiMessage(juce::MidiMessage::noteOn(
            channel, note, static_cast<juce::uint8>(127)));
        return;
    }
    m_qwertyHeldVelocity[static_cast<size_t>(note)] = 0;
    m_audio->feedVirtualMidiMessage(juce::MidiMessage::noteOff(channel, note));
}

void MainComponent::updateShiftFromKeyboard()
{
    const bool shift = juce::ModifierKeys::getCurrentModifiers().isShiftDown();
    m_globalStrip.setShiftHeld(shift);
}

bool MainComponent::keyPressed(const juce::KeyPress& key)
{
    updateShiftFromKeyboard();
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
    syncQwertyKey(note, true);
    return true;
}

bool MainComponent::keyStateChanged(bool /*isKeyDown*/)
{
    updateShiftFromKeyboard();
    if (!shouldCaptureQwertyMidi())
    {
        return false;
    }
    bool handled = false;
    for (const KeyNote& keyNote : kQwertyPianoKeys)
    {
        const bool down = juce::KeyPress::isKeyCurrentlyDown(keyNote.keyCode);
        const uint8_t prev = m_qwertyHeldVelocity[static_cast<size_t>(keyNote.note)];
        if (down && prev == 0)
        {
            syncQwertyKey(keyNote.note, true);
            handled = true;
        }
        else if (!down && prev != 0)
        {
            syncQwertyKey(keyNote.note, false);
            handled = true;
        }
    }
    return handled;
}

void MainComponent::updateTransportUi()
{
    const bool running = m_audio->isAudioRunning();
    m_play.setEnabled(!running);
    m_stop.setEnabled(running);
}

void MainComponent::handleRecordClick()
{
    if (!m_audio->isRecording())
    {
        if (!m_audio->startRecording())
        {
            juce::AlertWindow::showMessageBoxAsync(
                juce::AlertWindow::WarningIcon,
                "Record",
                "Press Play before recording.");
            return;
        }
        m_recordButton.setRecording(true);
        return;
    }

    m_audio->stopRecording();
    m_recordButton.setRecording(false);
    if (!m_audio->hasCapturedAudio())
    {
        return;
    }
    if (m_audio->wasLastCaptureTruncated())
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon,
            "Record",
            "Recording stopped at the 30-minute limit.");
    }
    m_audio->exportCapturedAudio(this, [](bool success, const juce::String& message) {
        juce::AlertWindow::showMessageBoxAsync(
            success ? juce::AlertWindow::InfoIcon : juce::AlertWindow::WarningIcon,
            "Export",
            message);
    });
}

void MainComponent::selectRuntimePage(RuntimePageKind page)
{
    m_activeRuntimePage = page;
    updateRuntimePageVisibility();
    resized();
}

void MainComponent::updateRuntimePageVisibility()
{
    const bool showCarousel = m_activeRuntimePage == RuntimePageKind::None;
    m_carousel.setVisible(showCarousel);
    m_performanceBand.setVisible(showCarousel);
    m_filePatchPage.setVisible(m_activeRuntimePage == RuntimePageKind::FilePatch);
    m_audioPage.setVisible(m_activeRuntimePage == RuntimePageKind::Audio);
    m_controllersPage.setVisible(m_activeRuntimePage == RuntimePageKind::Controllers);

    const juce::Colour activeColour(0xff388bfd);
    const juce::Colour idleColour = findColour(juce::TextButton::buttonColourId);
    m_fileButton.setColour(juce::TextButton::buttonColourId,
                           m_activeRuntimePage == RuntimePageKind::FilePatch ? activeColour
                                                                             : idleColour);
    m_audioButton.setColour(juce::TextButton::buttonColourId,
                            m_activeRuntimePage == RuntimePageKind::Audio ? activeColour : idleColour);
    m_midiButton.setColour(juce::TextButton::buttonColourId,
                           m_activeRuntimePage == RuntimePageKind::Controllers ? activeColour
                                                                               : idleColour);
}

void MainComponent::timerCallback()
{
    drainMidiUiActions();
    const uint32_t prevModRoutesVersion = m_lastModRoutesVersion;
    m_facade.publishUiFrame();
    if (m_facade.controlCore().consumeContentMutated())
    {
        m_audio->filePatchState().markDirty();
    }
    m_lastModRoutesVersion = m_facade.lastModRoutesVersion();
    if (m_lastModRoutesVersion != prevModRoutesVersion)
    {
        m_carousel.refresh();
    }

    const uint32_t version = m_facade.uiState().version.load(std::memory_order_acquire);
    if (version != m_lastUiVersion)
    {
        m_lastUiVersion = version;
        m_carousel.refresh();
        m_globalStrip.refresh();
        m_performanceBand.refresh();
    }

    const bool running = m_audio->isAudioRunning();
    m_globalOscilloscope.setAudioRunning(running);
    m_globalOscilloscope.setExternalAudioAvailable(
        m_audio->isExternalInputEnabled() && running);
    m_performanceBand.refreshMarbles(running);
    m_sequencerPanel.refresh();
}

void MainComponent::resized()
{
    using namespace DesktopV2ChromeLayout;

    auto bounds = getLocalBounds().reduced(kChromePad);
    auto rail = bounds.removeFromRight(kRuntimePageRailW);
    layoutRuntimePageRail(rail, m_fileButton, m_audioButton, m_midiButton);

    auto area = bounds;
    auto transport = area.removeFromTop(kTransportRowH);
    layoutStandaloneTransportRow(transport, m_play, m_stop, m_globalOscilloscope, &m_recordButton);

    area.removeFromTop(kSectionGap);
    m_globalStrip.setBounds(area.removeFromTop(kGlobalCommandBandH));
    area.removeFromTop(kSectionGap);

    if (m_sequencerVisible)
    {
        m_sequencerPanel.setBounds(area.removeFromBottom(kSequencerH));
        area.removeFromBottom(kSectionGap);
    }

    if (m_activeRuntimePage == RuntimePageKind::None)
    {
        m_performanceBand.setBounds(area.removeFromTop(kPerformanceBandH));
        area.removeFromTop(kSectionGap);
        m_carousel.setBounds(area);
        return;
    }

    if (m_activeRuntimePage == RuntimePageKind::FilePatch)
    {
        m_filePatchPage.setBounds(area);
        return;
    }
    if (m_activeRuntimePage == RuntimePageKind::Audio)
    {
        m_audioPage.setBounds(area);
        return;
    }
    m_controllersPage.setBounds(area);
}
