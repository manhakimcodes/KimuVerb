#include "PluginProcessor.h"
#include <JuceHeader.h>

#if JucePlugin_Enable_ARA

namespace juce
{
    // Provide a minimal/defensive implementation for AudioProcessorARAExtension::bindToARA.
    // This resolves the "Function definition for 'bindToARA' not found" diagnostic/link error
    // when ARA support is enabled but the concrete AudioProcessor doesn't implement the method.
    //
    // The stub returns nullptr (no ARA binding). Replace with a real implementation if your
    // processor actually needs to bind into ARA.
    const ARA::ARAPlugInExtensionInstance* AudioProcessorARAExtension::bindToARA (ARA::ARADocumentControllerRef documentControllerRef,
                                                                                  ARA::ARAPlugInInstanceRoleFlags knownRoles,
                                                                                  ARA::ARAPlugInInstanceRoleFlags assignedRoles)
    {
        (void) documentControllerRef;
        (void) knownRoles;
        (void) assignedRoles;
        return nullptr;
    }
} // namespace juce

#endif // JucePlugin_Enable_ARA