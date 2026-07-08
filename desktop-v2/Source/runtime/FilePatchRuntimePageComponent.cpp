#include "runtime/FilePatchRuntimePageComponent.h"

FilePatchRuntimePageComponent::FilePatchRuntimePageComponent(AudioEngine& engine)
    : m_engine(engine)
{
    m_title.setText("File / Patch", juce::dontSendNotification);
    m_title.setFont(juce::Font(16.0f, juce::Font::bold));
    m_log.setMultiLine(true);
    m_log.setReadOnly(true);

    m_save.onClick = [this]() { handleSave(); };
    m_load.onClick = [this]() { handleLoad(); };
    m_revert.onClick = [this]() { handleRevert(); };

    for (juce::Component* component :
         {static_cast<juce::Component*>(&m_title),
          static_cast<juce::Component*>(&m_identityLabel),
          static_cast<juce::Component*>(&m_dirtyLabel),
          static_cast<juce::Component*>(&m_saveResultLabel),
          static_cast<juce::Component*>(&m_loadResultLabel),
          static_cast<juce::Component*>(&m_revertResultLabel),
          static_cast<juce::Component*>(&m_mappingResultLabel),
          static_cast<juce::Component*>(&m_save),
          static_cast<juce::Component*>(&m_load),
          static_cast<juce::Component*>(&m_revert),
          static_cast<juce::Component*>(&m_log)})
    {
        addAndMakeVisible(component);
    }

    refreshState();
}

void FilePatchRuntimePageComponent::refreshState()
{
    const FilePatchRuntimeState& state = m_engine.filePatchState();
    m_identityLabel.setText("Patch: " + state.patchIdentity, juce::dontSendNotification);
    m_dirtyLabel.setText(state.dirty ? "Dirty" : "Clean", juce::dontSendNotification);
    m_saveResultLabel.setText("Last save: " + state.lastSaveResult, juce::dontSendNotification);
    m_loadResultLabel.setText("Last load: " + state.lastLoadResult, juce::dontSendNotification);
    m_revertResultLabel.setText("Last revert: " + state.lastRevertResult, juce::dontSendNotification);
    m_mappingResultLabel.setText(
        "Controller mappings: " + state.controllerMappingPersistenceResult, juce::dontSendNotification);

    juce::String logText;
    for (const auto& line : state.logMessages())
    {
        logText << line << juce::newLine;
    }
    m_log.setText(logText);
}

void FilePatchRuntimePageComponent::handleSave()
{
    auto chooser = std::make_shared<juce::FileChooser>(
        "Save patch",
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
        "*.frogv2");
    chooser->launchAsync(
        juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
        [this, chooser](const juce::FileChooser& fc) {
            juce::File file = fc.getResult();
            if (file == juce::File())
            {
                return;
            }
            if (!file.hasFileExtension("frogv2"))
            {
                file = file.withFileExtension("frogv2");
            }
            juce::String error;
            if (m_engine.savePatchToFile(file, error))
            {
                refreshState();
                return;
            }
            m_engine.filePatchState().appendLog("Save failed: " + error);
            refreshState();
        });
}

void FilePatchRuntimePageComponent::handleLoad()
{
    auto chooser = std::make_shared<juce::FileChooser>(
        "Load patch",
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
        "*.frogv2");
    chooser->launchAsync(
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this, chooser](const juce::FileChooser& fc) {
            const juce::File file = fc.getResult();
            if (file == juce::File())
            {
                return;
            }
            juce::String error;
            if (m_engine.loadPatchFromFile(file, error))
            {
                refreshState();
                return;
            }
            m_engine.filePatchState().appendLog("Load failed: " + error);
            refreshState();
        });
}

void FilePatchRuntimePageComponent::handleRevert()
{
    juce::String message;
    m_engine.revertPatch(message);
    m_engine.filePatchState().appendLog(message);
    refreshState();
}

void FilePatchRuntimePageComponent::resized()
{
    auto area = getLocalBounds().reduced(8);
    m_title.setBounds(area.removeFromTop(22));
    area.removeFromTop(6);
    m_identityLabel.setBounds(area.removeFromTop(18));
    m_dirtyLabel.setBounds(area.removeFromTop(18));
    area.removeFromTop(6);

    auto buttonRow = area.removeFromTop(28);
    m_save.setBounds(buttonRow.removeFromLeft(80));
    buttonRow.removeFromLeft(8);
    m_load.setBounds(buttonRow.removeFromLeft(80));
    buttonRow.removeFromLeft(8);
    m_revert.setBounds(buttonRow.removeFromLeft(80));
    area.removeFromTop(8);

    m_saveResultLabel.setBounds(area.removeFromTop(16));
    m_loadResultLabel.setBounds(area.removeFromTop(16));
    m_revertResultLabel.setBounds(area.removeFromTop(16));
    m_mappingResultLabel.setBounds(area.removeFromTop(16));
    area.removeFromTop(8);
    m_log.setBounds(area);
}
