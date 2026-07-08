#include "runtime/FilePatchRuntimeState.hpp"

void FilePatchRuntimeState::appendLog(const juce::String& message)
{
    m_logMessages.add(message);
    while (m_logMessages.size() > 32)
    {
        m_logMessages.remove(0);
    }
}

const juce::StringArray& FilePatchRuntimeState::logMessages() const
{
    return m_logMessages;
}

void FilePatchRuntimeState::markDirty()
{
    dirty = true;
}

void FilePatchRuntimeState::markSaved(const juce::String& identity)
{
    patchIdentity = identity;
    dirty = false;
}

void FilePatchRuntimeState::setControllerMappingPersistenceResult(const juce::String& result)
{
    controllerMappingPersistenceResult = result;
}
