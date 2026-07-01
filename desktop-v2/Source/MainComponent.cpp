#include "MainComponent.h"

#include "DesktopV2HostCallbacks.hpp"
#include "ui/DesktopV2ChromeLayout.hpp"
#include "V2ModTapBank.hpp"

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
    , m_bridge(m_core, m_audio->getHost())
    , m_hostCallbacks(m_core, m_bridge, m_audio->getHost(), m_carousel, m_lastModRoutesVersion)
{
    m_vcoEfScope.bindHost(&m_audio->getHost());
    m_carousel.bindCore(&m_core);
    m_performanceBand.bind(&m_core, &m_audio->getSequencer());
    m_performanceBand.bindHost(&m_audio->getHost());
    m_globalStrip.bind(&m_audio->getHost(), &m_core);
    m_sequencerPanel.bind(&m_audio->getSequencer(), &m_core, &m_bridge);

    addAndMakeVisible(m_vcoEfScope);
    addAndMakeVisible(m_performanceBand);
    addAndMakeVisible(m_carousel);
    addAndMakeVisible(m_globalStrip);
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

    m_audioSettings.onClick = [this]() { m_audio->showAudioSettings(this); };
    addAndMakeVisible(m_audioSettings);

    m_midiSettings.onClick = [this]() { m_audio->showMidiSettings(this); };
    addAndMakeVisible(m_midiSettings);

    m_audio->setTransportChangedCallback([this]() { updateTransportUi(); });

    wireCallbacks();
    wireMidiCvCallbacks();
    pushSelectPage(0);
    m_carousel.selectPage(0, false);

    setWantsKeyboardFocus(true);
    setSize(DesktopV2ChromeLayout::kDefaultWidth, DesktopV2ChromeLayout::kDefaultHeight);
    updateTransportUi();
    startTimerHz(15);
}

void MainComponent::wireCallbacks()
{
    desktop_v2::refreshAndWireHostCallbacks(
        m_hostCallbacks, m_core, m_bridge, m_audio->getHost(), m_carousel, m_lastModRoutesVersion);
}

void MainComponent::pushRandomizeMod(uint8_t page)
{
    desktop_v2::pushRandomizeMod(m_hostCallbacks, page);
}

void MainComponent::syncHostModRoutesIfNeeded()
{
    DesktopHostIO& host = m_audio->getHost();
    const uint32_t version = host.modRoutesVersion();
    if (version == m_lastModRoutesVersion)
    {
        return;
    }
    m_lastModRoutesVersion = version;
    m_bridge.syncFromHostModRoutes();
    m_carousel.refresh();
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
            m_core.bus().push(message);
            m_core.processBus();
            m_bridge.syncToHost();
            m_globalStrip.refresh();
        });
    });
}

void MainComponent::pushExternalMidiMods()
{
    MidiCvAssignmentTable& table = m_audio->getMidiCvTable();
    for (uint8_t slot = 0; slot < 2; ++slot)
    {
        if (slot == 0 && !table.externalModA.enabled)
        {
            continue;
        }
        if (slot == 1 && !table.externalModB.enabled)
        {
            continue;
        }
        froggers_v2::MessageIn message;
        message.type = froggers_v2::MessageIn::Type::Clock;
        message.index = slot;
        message.value = table.externalModLevel(slot);
        m_core.bus().push(message);
    }
    m_core.processBus();
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

void MainComponent::pushModSourceSamples()
{
    DesktopHostIO& host = m_audio->getHost();
    for (uint8_t engine = V2ModTapBank::kFirstIndex; engine <= V2ModTapBank::kLastIndex; ++engine)
    {
        froggers_v2::MessageIn message;
        message.type = froggers_v2::MessageIn::Type::Clock;
        message.index = static_cast<uint8_t>(engine - V2ModTapBank::kFirstIndex + 6);
        message.value = host.GetCvOut(engine);
        m_core.bus().push(message);
    }
    m_core.processBus();
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

void MainComponent::timerCallback()
{
    if (m_audio->shouldDrainPendingUiMutations())
    {
        m_audio->getHost().DrainPendingMutations();
    }
    syncHostModRoutesIfNeeded();

    drainMidiUiActions();
    pushExternalMidiMods();
    pushModSourceSamples();
    m_bridge.syncToHost();

    const uint32_t version = m_core.uiState().version.load(std::memory_order_acquire);
    if (version != m_lastUiVersion)
    {
        m_lastUiVersion = version;
        m_carousel.refresh();
        m_performanceBand.refresh();
        m_globalStrip.refresh();
    }

    const bool running = m_audio->isAudioRunning();
    m_vcoEfScope.refresh(running);
    m_performanceBand.refreshMarbles(running);
    m_sequencerPanel.refresh();
}

void MainComponent::resized()
{
    using namespace DesktopV2ChromeLayout;

    auto area = getLocalBounds().reduced(kChromePad);
    auto transport = area.removeFromTop(kTransportRowH);
    m_audioSettings.setBounds(transport.removeFromRight(72));
    transport.removeFromRight(6);
    m_midiSettings.setBounds(transport.removeFromRight(72));
    transport.removeFromRight(8);
    m_play.setBounds(transport.removeFromLeft(64));
    transport.removeFromLeft(6);
    m_stop.setBounds(transport.removeFromLeft(64));
    m_vcoEfScope.setBounds(transport);

    area.removeFromTop(kSectionGap);
    m_globalStrip.setBounds(area.removeFromBottom(kGlobalStripH));
    area.removeFromBottom(kSectionGap);

    if (m_sequencerVisible)
    {
        m_sequencerPanel.setBounds(area.removeFromBottom(kSequencerH));
        area.removeFromBottom(kSectionGap);
    }

    m_performanceBand.setBounds(area.removeFromTop(kPerformanceBandH));
    area.removeFromTop(kSectionGap);
    m_carousel.setBounds(area);
}
