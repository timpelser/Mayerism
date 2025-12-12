#pragma once

/**
 * Klon Centaur Precompiled Header Replacement
 *
 * This header replaces the original pch.h from ChowCentaur.
 * It provides all necessary includes and brings JUCE types into scope
 * so the original Klon DSP code can compile without modifications.
 */

// JUCE includes
#include <JuceHeader.h>

// Disable foleys_gui_magic dependency before including chowdsp
#define CHOWDSP_USE_FOLEYS_CLASSES 0

// Klon's chowdsp DSP only (no GUI, no PluginUtils, no foleys)
#include "dependencies/chowdsp_utils/DSP/chowdsp_DSP.h"

// Klon-specific toms917 (Wright Omega function)
#include "dependencies/toms917/toms917.hpp"

// Bring JUCE into global scope (matches original pch.h behavior exactly)
// This is required because the Klon DSP files were written expecting this
using namespace juce;

// Note: chowdsp namespaces are already accessible via chowdsp_dsp.h
