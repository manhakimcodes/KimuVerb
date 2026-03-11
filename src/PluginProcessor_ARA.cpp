#include "PluginProcessor.h"
#if JucePlugin_Enable_ARA
#include <ARA_API/ARAVST3.h>

namespace juce {

const ARA::ARAPlugInExtensionInstance* KimuVerbAudioProcessor::bindToARA (ARA::ARADocumentControllerRef documentControllerRef,
                                                                           ARA::ARAPlugInInstanceRoleFlags knownRoles,
                                                                           ARA::ARAPlugInInstanceRoleFlags assignedRoles)
{
    // ARA is not implemented for this processor. Return nullptr to avoid leaking
    // an incomplete extension instance and to signal no ARA binding.
    (void) documentControllerRef;
    (void) knownRoles;
    (void) assignedRoles;
    return nullptr;
}

} // namespace juce
#endif
