---
description: Implementation plan for adding independent input/output controls
---

# Implementation Plan: Independent Input/Output Controls

## Overview
Add two new parameters (Independent Input Gain and Independent Output Gain) that are separate from the NAM processor's internal input/output controls. This provides better gain staging and matches the Neural DSP workflow.

## Signal Flow (After Implementation)
```
Input Signal 
→ [NEW] Independent Input Gain
→ Input Meter
→ Noise Gate
→ NAM Input Gain (existing, part of NAM model)
→ NAM Model Processing
→ Tone Stack
→ NAM Output Gain (existing, part of NAM model)
→ Doubler
→ [NEW] Independent Output Gain
→ Output Meter
```

---

## File Modifications Required

### 1. **PluginProcessor.h**
**Location:** `/Users/timpelser/Documents/SideProjects/Mayerism/codebase/nam-juce/Source/PluginProcessor.h`

**Changes:**
- Add two new member variables to store parameter pointers:
  ```cpp
  std::atomic<float>* pluginInputGain;
  std::atomic<float>* pluginOutputGain;
  ```

**Why:** Need to store references to the new parameters for use in processBlock.

---

### 2. **PluginProcessor.cpp**
**Location:** `/Users/timpelser/Documents/SideProjects/Mayerism/codebase/nam-juce/Source/PluginProcessor.cpp`

#### **Change A: createParameters() method (around line 200-213)**
**Add two new parameters:**
```cpp
// After NAM parameters, before DOUBLER_SPREAD_ID
parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
    "PLUGIN_INPUT_ID", "PLUGIN_INPUT", -20.0f, 20.0f, 0.0f));
parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
    "PLUGIN_OUTPUT_ID", "PLUGIN_OUTPUT", -40.0f, 40.0f, 0.0f));
```

**Why:** Creates the audio parameters in the plugin's parameter tree.

#### **Change B: prepareToPlay() method (around line 89)**
**Hook the parameter pointers:**
```cpp
// After myNAM.hookParameters(apvts);
pluginInputGain = apvts.getRawParameterValue("PLUGIN_INPUT_ID");
pluginOutputGain = apvts.getRawParameterValue("PLUGIN_OUTPUT_ID");
```

**Why:** Gets direct access to parameter values for efficient real-time processing.

#### **Change C: processBlock() method (around line 157-185)**
**Current flow:**
```cpp
Line 159: meterInSource.measureBlock(buffer);
Line 172: myNAM.processBlock(buffer);
Line 179-182: // Doubler processing
Line 184: meterOutSource.measureBlock(buffer);
```

**New flow:**
```cpp
// Apply independent input gain BEFORE input metering
buffer.applyGain(dB_to_linear(pluginInputGain->load()));

meterInSource.measureBlock(buffer);

// ... existing NAM processing ...
myNAM.processBlock(buffer);

// ... existing doubler processing ...
if (*apvts.getRawParameterValue("DOUBLER_SPREAD_ID") > 0.0) {
  doubler.setDelayMs(*apvts.getRawParameterValue("DOUBLER_SPREAD_ID"));
  doubler.process(buffer);
}

// Apply independent output gain AFTER doubler, BEFORE output metering
buffer.applyGain(dB_to_linear(pluginOutputGain->load()));

meterOutSource.measureBlock(buffer);
```

**Add helper method:**
```cpp
// In PluginProcessor.h private section or as inline in .cpp
double dB_to_linear(double db_value) {
  return std::pow(10.0, db_value / 20.0);
}
```

**Why:** 
- Input gain is the very first thing applied (before meters show level)
- Doubler processes the stereo effect
- Output gain is the final level control after all processing
- Meters show the actual levels at input and output stages

---

### 3. **NamEditor.h**
**Location:** `/Users/timpelser/Documents/SideProjects/Mayerism/codebase/nam-juce/Source/NamEditor.h`

#### **Change A: Update NUM_SLIDERS define (line 10)**
```cpp
#define NUM_SLIDERS 9  // Changed from 7
```

**Why:** We're adding 2 more sliders.

#### **Change B: Update PluginKnobs enum (lines 27-35)**
```cpp
enum PluginKnobs {
  PluginInput = 0,    // NEW - Independent input
  Input,              // NAM amp input (was 0, now 1)
  NoiseGate,          // (was 1, now 2)
  Bass,               // (was 2, now 3)
  Middle,             // (was 3, now 4)
  Treble,             // (was 4, now 5)
  Output,             // NAM amp output (was 5, now 6)
  PluginOutput,       // NEW - Independent output
  Doubler             // (was 6, now 8)
};
```

**Why:** Adds the two new knobs to the enum, maintaining logical signal flow order.

#### **Change C: Update sliderIDs array (lines 42-44)**
```cpp
juce::String sliderIDs[NUM_SLIDERS]{
    "PLUGIN_INPUT_ID",  // NEW
    "INPUT_ID", 
    "NGATE_ID", 
    "BASS_ID",
    "MIDDLE_ID", 
    "TREBLE_ID", 
    "OUTPUT_ID",
    "PLUGIN_OUTPUT_ID", // NEW
    "DOUBLER_ID"
};
```

**Why:** Maps UI sliders to parameter IDs in APVTS.

---

### 4. **NamEditor.cpp**
**Location:** `/Users/timpelser/Documents/SideProjects/Mayerism/codebase/nam-juce/Source/NamEditor.cpp`

#### **Change A: Update positioning logic (lines 21-71)**

**UI Layout Strategy:**
- **Row 1 (utility/effects)**: PluginInput, NoiseGate, Doubler, PluginOutput - positioned manually
- **Row 2 (NAM amp controls)**: Input, Bass, Middle, Treble, Output - positioned in loop

**Update the main row loop (around line 32-48):**
```cpp
// Setup sliders
int positionIndex = 0; // Counter for main row (NAM controls) positioning
for (int slider = 0; slider < NUM_SLIDERS; ++slider) {
  sliders[slider].reset(new CustomSlider());
  addAndMakeVisible(sliders[slider].get());
  sliders[slider]->setLookAndFeel(&lnf);
  sliders[slider]->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  sliders[slider]->setTextBoxStyle(juce::Slider::NoTextBox, false, 80, 20);

  // Position only NAM amp controls in main row
  // Exclude: PluginInput, NoiseGate, Doubler, PluginOutput
  if (slider != PluginKnobs::PluginInput && 
      slider != PluginKnobs::NoiseGate && 
      slider != PluginKnobs::Doubler &&
      slider != PluginKnobs::PluginOutput) {
    sliders[slider]->setBounds(xStart + (positionIndex * xOffsetMultiplier),
                               450, knobSize, knobSize);
    positionIndex++;
  }
}
```

**Add manual positioning for Row 1 controls (after the loop, before line 51):**
```cpp
// Position Row 1 controls manually (PluginInput, Gate, Doubler, PluginOutput)

// PluginInput - leftmost position
sliders[PluginKnobs::PluginInput]->setBounds(
    sliders[PluginKnobs::Input]->getX() - 140,  // Align with row structure
    80,  // Same Y as NoiseGate/Doubler
    knobSize, knobSize
);

// NoiseGate - already positioned (keep existing code at lines 60-64)
sliders[PluginKnobs::NoiseGate]->setBounds(
    sliders[PluginKnobs::Output]->getX() - 140,
    80,
    knobSize, knobSize
);
sliders[PluginKnobs::NoiseGate]->setPopupDisplayEnabled(true, true, getTopLevelComponent());
sliders[PluginKnobs::NoiseGate]->setCustomSlider(CustomSlider::SliderTypes::Gate);
sliders[PluginKnobs::NoiseGate]->addListener(this);

// Doubler - already positioned (keep existing code at lines 51-58)
sliders[PluginKnobs::Doubler]->setPopupDisplayEnabled(true, true, getTopLevelComponent());
sliders[PluginKnobs::Doubler]->setCustomSlider(CustomSlider::SliderTypes::Doubler);
sliders[PluginKnobs::Doubler]->setTextBoxStyle(juce::Slider::NoTextBox, false, 80, 20);
sliders[PluginKnobs::Doubler]->setBounds(
    sliders[PluginKnobs::Output]->getX(),
    80,
    knobSize, knobSize
);

// PluginOutput - rightmost position (after Doubler)
sliders[PluginKnobs::PluginOutput]->setBounds(
    sliders[PluginKnobs::Doubler]->getX() + 140,  // To the right of Doubler
    80,  // Same Y as other Row 1 controls
    knobSize, knobSize
);
```

**Why:** 
- All sliders remain in one array
- Use enum indexes to differentiate positioning
- Row 1 (utility) controls are manually positioned together at Y=80
- Row 2 (NAM amp) controls use the automatic main row loop at Y=450
- Maintains existing NoiseGate and Doubler positioning logic while adding PluginInput and PluginOutput

---

## Testing Checklist

After implementation, verify:

- [ ] **Parameters created**: Check plugin parameters in DAW - should see PLUGIN_INPUT and PLUGIN_OUTPUT
- [ ] **UI renders**: Both new knobs visible and positioned correctly
- [ ] **Parameter linking**: Moving knobs changes corresponding parameter values
- [ ] **Signal flow**: 
  - Increasing PluginInput makes signal louder into amp (meters reflect this)
  - Increasing PluginOutput makes final output louder
  - NAM Input/Output still function as expected
- [ ] **Automation**: Both new parameters can be automated in DAW
- [ ] **Preset saving**: Parameters save/load with presets
- [ ] **No clipping**: Verify meters don't clip unexpectedly
- [ ] **CPU performance**: No significant CPU increase

---

## Parameter Ranges (Proposed)

- **PLUGIN_INPUT_ID**: -20.0 dB to +20.0 dB (default: 0.0 dB)
  - Enough range to accommodate different pickup outputs
  - Matches NAM's INPUT_ID range
  
- **PLUGIN_OUTPUT_ID**: -40.0 dB to +40.0 dB (default: 0.0 dB)
  - Wider range for final output matching
  - Matches NAM's OUTPUT_ID range

---

## Estimated Implementation Time
- **Code changes**: 30-45 minutes
- **UI positioning/testing**: 15-30 minutes (depends on background image alignment needs)
- **Testing**: 15-20 minutes
- **Total**: ~1-1.5 hours

---

## Notes

1. **Signal flow order**: The final processing order is:
   - PluginInput gain → Input meter → NAM processing → Doubler → PluginOutput gain → Output meter
   - This ensures the output gain controls the final level after all effects

2. **Gain staging workflow**:
   - User adjusts PluginInput to match their guitar/pickup to desired level
   - User adjusts NAM Input to control amp drive/saturation
   - User adjusts NAM Output as part of amp tone
   - Doubler adds stereo width
   - User adjusts PluginOutput to match final level in mix

3. **UI Layout**:
   - **Row 1 (Y=80)**: PluginInput, NoiseGate, Doubler, PluginOutput - utility and effects controls
   - **Row 2 (Y=450)**: Input, Bass, Mid, Treble, Output - NAM amp model controls
   - All sliders in one array, different positioning based on enum index

4. **Metering**: 
   - Input meter shows level after PluginInput gain is applied
   - Output meter shows final output level after all processing including PluginOutput gain

5. **No labeling yet**: Labels for the knobs will be added in a future update
