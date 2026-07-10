#pragma once

#include <juce_core/juce_core.h>

struct FilePatchRuntimeState
{
    juce::String patchIdentity{"Untitled"};
    bool dirty = false;
    juce::String lastSaveResult;
    juce::String lastLoadResult;
    juce::String lastRevertResult;
    juce::String controllerMappingPersistenceResult{"Not saved"};

    void appendLog(const juce::String& message);
    const juce::StringArray& logMessages() const;

    void markDirty();
    void markClean();
    void markSaved(const juce::String& identity);
    void setControllerMappingPersistenceResult(const juce::String& result);

private:
    juce::StringArray m_logMessages;
};
