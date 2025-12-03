#include "NamEditor.h"

NamEditor::NamEditor(NamJUCEAudioProcessor &p)
    : AudioProcessorEditor(&p), audioProcessor(p), topBar(p),
      pmc(p.getPresetManager(), [&]() { updateAfterPresetLoad(); }) {
  assetManager.reset(new AssetManager());

  // Meters
  meterIn.setMeterSource(&audioProcessor.getMeterInSource());
  addAndMakeVisible(meterIn);

  meterOut.setMeterSource(&audioProcessor.getMeterOutSource());
  addAndMakeVisible(meterOut);

  meterIn.setAlpha(0.8);
  meterOut.setAlpha(0.8);

  meterIn.setSelectedChannel(0);
  meterOut.setSelectedChannel(0);

  int knobSize = 51;
  int knobSizeMinimal = 54; // Larger size for minimal knobs (top row)
  int xStart = 266;
  int xOffsetMultiplier = 74;

  lnf.setColour(Slider::textBoxOutlineColourId,
                juce::Colours::transparentBlack);
  lnf.setColour(Slider::textBoxBackgroundColourId,
                juce::Colours::transparentBlack);
  lnf.setColour(Slider::textBoxTextColourId, juce::Colours::ivory);

  lnfMinimal.setColour(Slider::textBoxOutlineColourId,
                       juce::Colours::transparentBlack);
  lnfMinimal.setColour(Slider::textBoxBackgroundColourId,
                       juce::Colours::transparentBlack);
  lnfMinimal.setColour(Slider::textBoxTextColourId, juce::Colours::ivory);

  // Setup sliders
  int positionIndex = 0; // Separate counter for NAM amp controls (Row 2)
  for (int slider = 0; slider < NUM_SLIDERS; ++slider) {
    sliders[slider].reset(new CustomSlider());
    addAndMakeVisible(sliders[slider].get());

    // Apply different LookAndFeel based on slider type
    // Top row controls use minimal style, NAM controls use main style
    if (slider == PluginKnobs::PluginInput ||
        slider == PluginKnobs::NoiseGate || slider == PluginKnobs::Doubler ||
        slider == PluginKnobs::PluginOutput) {
      sliders[slider]->setLookAndFeel(&lnfMinimal);
    } else {
      sliders[slider]->setLookAndFeel(&lnf);
    }

    sliders[slider]->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    sliders[slider]->setTextBoxStyle(juce::Slider::NoTextBox, false, 80, 20);
    // sliders[slider]->setPopupDisplayEnabled(true, true,
    // getTopLevelComponent());

    // Position only NAM amp controls in Row 2 (main row)
    // Exclude Row 1 controls: PluginInput, NoiseGate, Doubler, PluginOutput
    if (slider != PluginKnobs::PluginInput &&
        slider != PluginKnobs::NoiseGate && slider != PluginKnobs::Doubler &&
        slider != PluginKnobs::PluginOutput) {
      sliders[slider]->setBounds(xStart + (positionIndex * xOffsetMultiplier),
                                 450, knobSize, knobSize);
      positionIndex++;
    }
  }

  // ========== Row 1 Controls (Utility/Effects) - Manual Positioning ==========

  // PluginInput - leftmost position in Row 1
  sliders[PluginKnobs::PluginInput]->setBounds(88, 89, knobSizeMinimal,
                                               knobSizeMinimal);
  sliders[PluginKnobs::PluginInput]->setCustomSlider(
      CustomSlider::SliderTypes::PluginInput);
  sliders[PluginKnobs::PluginInput]->addListener(this);

  // NoiseGate - second position in Row 1
  sliders[PluginKnobs::NoiseGate]->setBounds(218, 89, knobSizeMinimal,
                                             knobSizeMinimal);
  // sliders[PluginKnobs::NoiseGate]->setPopupDisplayEnabled(
  //     true, true, getTopLevelComponent());
  sliders[PluginKnobs::NoiseGate]->setCustomSlider(
      CustomSlider::SliderTypes::Gate);
  sliders[PluginKnobs::NoiseGate]->addListener(this);

  // Doubler - third position in Row 1
  // sliders[PluginKnobs::Doubler]->setPopupDisplayEnabled(true, true,
  //                                                       getTopLevelComponent());
  sliders[PluginKnobs::Doubler]->setCustomSlider(
      CustomSlider::SliderTypes::Doubler);
  sliders[PluginKnobs::Doubler]->setTextBoxStyle(juce::Slider::NoTextBox, false,
                                                 89, 20);
  sliders[PluginKnobs::Doubler]->setBounds(674, 89, knobSizeMinimal,
                                           knobSizeMinimal);
  sliders[PluginKnobs::Doubler]->addListener(this);

  // PluginOutput - rightmost position in Row 1
  sliders[PluginKnobs::PluginOutput]->setBounds(804, 89, knobSizeMinimal,
                                                knobSizeMinimal);
  sliders[PluginKnobs::PluginOutput]->setCustomSlider(
      CustomSlider::SliderTypes::PluginOutput);
  sliders[PluginKnobs::PluginOutput]->addListener(this);

  // Hook slider and button attachments
  for (int slider = 0; slider < NUM_SLIDERS; ++slider)
    sliderAttachments[slider] =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.apvts, sliderIDs[slider], *sliders[slider]);

  sliderAttachments[PluginKnobs::Doubler] =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          audioProcessor.apvts, "DOUBLER_SPREAD_ID",
          *sliders[PluginKnobs::Doubler]);

  meterIn.toFront(true);
  meterOut.toFront(true);

  // Initialize meter positions and styling
  setMeterPosition();

  // Initialize knob labels
  drawKnobLabels();

  // Initialize knob value labels
  drawKnobValueLabels();
  updateKnobValueLabels(); // Show initial values

  addAndMakeVisible(&pmc);
  pmc.setColour(juce::Colours::transparentWhite, 0.0f);

  addAndMakeVisible(&topBar);
}

NamEditor::~NamEditor() {
  for (int sliderAtt = 0; sliderAtt < NUM_SLIDERS; ++sliderAtt)
    sliderAttachments[sliderAtt] = nullptr;
}

void NamEditor::paint(juce::Graphics &g) {
  g.fillAll(juce::Colour::fromString("FF121212"));

  g.setColour(juce::Colours::white);
  g.setFont(15.0f);

  g.drawImageAt(assetManager->getBackground(), 0, 0);
  g.drawImageAt(assetManager->getScreens(), 0, 0);
}

void NamEditor::resized() {
  topBar.setBounds(0, 0, getWidth(), 40);

  // Preset Manager - centered between GATE and DOUBLER knobs
  // GATE ends at x=272, DOUBLER starts at x=674, centered at x=343
  pmc.setBounds(343, 82, 260, 65);
}

void NamEditor::timerCallback() {}

void NamEditor::sliderValueChanged(juce::Slider *slider) {
  // Update value labels whenever any slider changes
  updateKnobValueLabels();
}

void NamEditor::setMeterPosition() {
  // Meter bar color (the moving part) - 717171
  meterlnf.setColour(foleys::LevelMeter::lmMeterGradientLowColour,
                     juce::Colour::fromString("FF717171"));
  // Set high gradient to same color to avoid gradient effect
  meterlnf.setColour(foleys::LevelMeter::lmMeterGradientMidColour,
                     juce::Colour::fromString("FF717171"));
  // Outline - transparent for clean look
  meterlnf.setColour(foleys::LevelMeter::lmMeterOutlineColour,
                     juce::Colours::transparentWhite);
  // Background color - D7D7D7
  meterlnf.setColour(foleys::LevelMeter::lmMeterBackgroundColour,
                     juce::Colour::fromString("FFD7D7D7"));
  meterIn.setLookAndFeel(&meterlnf);
  meterOut.setLookAndFeel(&meterlnf);

  int meterHeight = 115;
  int meterWidth = 14;
  meterIn.setBounds(juce::Rectangle<int>(48, 68, meterWidth, meterHeight));
  meterOut.setBounds(juce::Rectangle<int>(891, 68, meterWidth, meterHeight));
}

void NamEditor::updateAfterPresetLoad() {}

void NamEditor::drawKnobLabels() {
  // Define label texts and corresponding knob indices
  struct LabelConfig {
    int arrayIndex;    // Index in the knobLabels array
    int knobIndex;     // Index of the slider (PluginKnobs enum)
    juce::String text; // Label text
  };

  const LabelConfig labelConfigs[] = {{0, PluginKnobs::PluginInput, "INPUT"},
                                      {1, PluginKnobs::NoiseGate, "GATE"},
                                      {2, PluginKnobs::Doubler, "DOUBLER"},
                                      {3, PluginKnobs::PluginOutput, "OUTPUT"}};

  for (const auto &config : labelConfigs) {
    // Get the knob bounds to position label above it
    auto knobBounds = sliders[config.knobIndex]->getBounds();

    // Make the label visible
    addAndMakeVisible(knobLabels[config.arrayIndex]);

    // Set the text
    knobLabels[config.arrayIndex].setText(config.text,
                                          juce::dontSendNotification);

    // Set text color to ivory (matching slider text)
    knobLabels[config.arrayIndex].setColour(juce::Label::textColourId,
                                            juce::Colours::black);

    // Set transparent background
    knobLabels[config.arrayIndex].setColour(juce::Label::backgroundColourId,
                                            juce::Colours::transparentBlack);

    // Set font - 12pt, bold
    knobLabels[config.arrayIndex].setFont(juce::Font(14.0f, juce::Font::plain));

    // Center the text
    knobLabels[config.arrayIndex].setJustificationType(
        juce::Justification::centred);

    // Position the label above the knob
    // Label is centered on the knob with a fixed width to prevent text
    // squashing
    int labelWidth = 80; // Wide enough for "DOUBLER"
    int labelHeight = 20;
    int labelX = knobBounds.getCentreX() - (labelWidth / 2); // Center on knob
    int labelY = knobBounds.getY() - labelHeight - 5;
    knobLabels[config.arrayIndex].setBounds(labelX, labelY, labelWidth,
                                            labelHeight);
  }
}

void NamEditor::drawKnobValueLabels() {
  // Define label configuration for value labels
  struct LabelConfig {
    int arrayIndex; // Index in the knobValueLabels array
    int knobIndex;  // Index of the slider (PluginKnobs enum)
  };

  const LabelConfig labelConfigs[] = {{0, PluginKnobs::PluginInput},
                                      {1, PluginKnobs::NoiseGate},
                                      {2, PluginKnobs::Doubler},
                                      {3, PluginKnobs::PluginOutput}};

  for (const auto &config : labelConfigs) {
    // Get the knob bounds to position value label below it
    auto knobBounds = sliders[config.knobIndex]->getBounds();

    // Make the label visible
    addAndMakeVisible(knobValueLabels[config.arrayIndex]);

    // Set initial placeholder text
    knobValueLabels[config.arrayIndex].setText("--",
                                               juce::dontSendNotification);

    // Set text color to black (matching name labels)
    knobValueLabels[config.arrayIndex].setColour(juce::Label::textColourId,
                                                 juce::Colours::black);

    // Set transparent background
    knobValueLabels[config.arrayIndex].setColour(
        juce::Label::backgroundColourId, juce::Colours::transparentBlack);

    // Set font - 14pt, plain (matching name labels)
    knobValueLabels[config.arrayIndex].setFont(
        juce::Font(14.0f, juce::Font::plain));

    // Center the text
    knobValueLabels[config.arrayIndex].setJustificationType(
        juce::Justification::centred);

    // Position the label below the knob
    // Label is centered on the knob with a fixed width
    int labelWidth = 80; // Wide enough for values with units
    int labelHeight = 20;
    int labelX = knobBounds.getCentreX() - (labelWidth / 2); // Center on knob
    int labelY = knobBounds.getBottom() + 5; // 5 pixels below knob
    knobValueLabels[config.arrayIndex].setBounds(labelX, labelY, labelWidth,
                                                 labelHeight);
  }
}

void NamEditor::updateKnobValueLabels() {
  // Define the mapping between label array positions and knob indices
  const int knobIndices[] = {PluginKnobs::PluginInput, PluginKnobs::NoiseGate,
                             PluginKnobs::Doubler, PluginKnobs::PluginOutput};

  // Update each value label with the current formatted slider value
  for (int i = 0; i < 4; ++i) {
    // Get the current value from the slider
    double currentValue = sliders[knobIndices[i]]->getValue();

    // Use the slider's getTextFromValue() method to format the value
    // This reuses the same formatting logic as the popup displays
    juce::String formattedValue =
        sliders[knobIndices[i]]->getTextFromValue(currentValue);

    // Update the label text
    knobValueLabels[i].setText(formattedValue, juce::dontSendNotification);
  }
}