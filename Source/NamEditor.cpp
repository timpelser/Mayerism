#include "NamEditor.h"

NamEditor::NamEditor(NamJUCEAudioProcessor &p)
    : AudioProcessorEditor(&p), audioProcessor(p), topBar(p),
      pmc(p.getPresetManager(), [&]() { updateAfterPresetLoad(); }) {

  // Load page background images
  backgroundPreEffects = juce::ImageFileFormat::loadFrom(
      BinaryData::backgroundpre_png, BinaryData::backgroundpre_pngSize);
  backgroundPreEffectsNoKnobs = juce::ImageFileFormat::loadFrom(
      BinaryData::background_pre_no_knobs_png,
      BinaryData::background_pre_no_knobs_pngSize);
  backgroundAmp = juce::ImageFileFormat::loadFrom(
      BinaryData::backgroundamp_png, BinaryData::backgroundamp_pngSize);
  backgroundPostEffects = juce::ImageFileFormat::loadFrom(
      BinaryData::backgroundpost_png, BinaryData::backgroundpost_pngSize);

  // Load pedal button images for TS toggle
  pedalButtonOn = juce::ImageFileFormat::loadFrom(
      BinaryData::PedalButtonOn_png, BinaryData::PedalButtonOn_pngSize);
  pedalButtonOff = juce::ImageFileFormat::loadFrom(
      BinaryData::PedalButtonOff_png, BinaryData::PedalButtonOff_pngSize);

  // Meters
  meterIn.setMeterSource(&audioProcessor.getMeterInSource());
  addAndMakeVisible(meterIn);

  meterOut.setMeterSource(&audioProcessor.getMeterOutSource());
  addAndMakeVisible(meterOut);

  meterIn.setAlpha(0.8);
  meterOut.setAlpha(0.8);

  meterIn.setSelectedChannel(0);
  meterOut.setSelectedChannel(0);

  meterIn.toFront(true);
  meterOut.toFront(true);

  // Initialize meter positions and styling
  setMeterPosition();

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

  initializeTopRow();

  initializeAmpSliders();

  initializeTSSliders();

  initializeCompSliders();

  initializeBoostSliders();

  initializeSliderAttachments();

  // Update TS toggle appearance to match initial state
  updateTSToggleAppearance();

  // Update Compressor toggle appearance to match initial state
  updateCompToggleAppearance();

  // Update Boost toggle appearance to match initial state
  updateBoostToggleAppearance();

  addAndMakeVisible(&pmc);
  pmc.setColour(juce::Colours::transparentWhite, 0.0f);

  addAndMakeVisible(&topBar);

  setupPageTabs();
  setKnobVisibility(); // Set initial visibility based on default page
}

NamEditor::~NamEditor() {
  for (int sliderAtt = 0; sliderAtt < NUM_SLIDERS; ++sliderAtt)
    sliderAttachments[sliderAtt] = nullptr;
}

void NamEditor::paint(juce::Graphics &g) {
  g.fillAll(juce::Colour::fromString("FF121212"));

  g.setColour(juce::Colours::white);
  g.setFont(15.0f);

  // Draw the background for the current page
  switch (currentPage) {
  case PRE_EFFECTS:
    // Use debug background (no knobs) if flag is set, otherwise use normal
    if (useDebugPreBackground)
      g.drawImageAt(backgroundPreEffectsNoKnobs, 0, 0);
    else
      g.drawImageAt(backgroundPreEffects, 0, 0);
    break;
  case AMP:
    g.drawImageAt(backgroundAmp, 0, 0);
    break;
  case POST_EFFECTS:
    g.drawImageAt(backgroundPostEffects, 0, 0);
    break;
  }
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

void NamEditor::updatePageTabHighlight() {
  preEffectsPage->setAlpha(currentPage == PRE_EFFECTS ? 1.0f : 0.5f);
  ampPage->setAlpha(currentPage == AMP ? 1.0f : 0.5f);
  postEffectsPage->setAlpha(currentPage == POST_EFFECTS ? 1.0f : 0.5f);
}

void NamEditor::setKnobVisibility() {
  switch (currentPage) {
  case PRE_EFFECTS:
    // Hide amp controls on pre-effects page
    sliders[PluginKnobs::Input]->setVisible(false);
    sliders[PluginKnobs::Bass]->setVisible(false);
    sliders[PluginKnobs::Middle]->setVisible(false);
    sliders[PluginKnobs::Treble]->setVisible(false);
    sliders[PluginKnobs::Output]->setVisible(false);

    // Show Tube Screamer controls
    sliders[PluginKnobs::TSDrive]->setVisible(true);
    sliders[PluginKnobs::TSTone]->setVisible(true);
    sliders[PluginKnobs::TSLevel]->setVisible(true);
    tsEnabledToggle->setVisible(true);

    // Show Compressor controls
    sliders[PluginKnobs::CompVolume]->setVisible(true);
    sliders[PluginKnobs::CompAttack]->setVisible(true);
    sliders[PluginKnobs::CompSustain]->setVisible(true);
    compEnabledToggle->setVisible(true);

    // Show Boost controls
    sliders[PluginKnobs::BoostVolume]->setVisible(true);
    boostEnabledToggle->setVisible(true);
    break;

  case AMP:
    // Show amp controls on amp page
    sliders[PluginKnobs::Input]->setVisible(true);
    sliders[PluginKnobs::Bass]->setVisible(true);
    sliders[PluginKnobs::Middle]->setVisible(true);
    sliders[PluginKnobs::Treble]->setVisible(true);
    sliders[PluginKnobs::Output]->setVisible(true);

    // Hide Tube Screamer controls
    sliders[PluginKnobs::TSDrive]->setVisible(false);
    sliders[PluginKnobs::TSTone]->setVisible(false);
    sliders[PluginKnobs::TSLevel]->setVisible(false);
    tsEnabledToggle->setVisible(false);

    // Hide Compressor controls
    sliders[PluginKnobs::CompVolume]->setVisible(false);
    sliders[PluginKnobs::CompAttack]->setVisible(false);
    sliders[PluginKnobs::CompSustain]->setVisible(false);
    compEnabledToggle->setVisible(false);

    // Hide Boost controls
    sliders[PluginKnobs::BoostVolume]->setVisible(false);
    boostEnabledToggle->setVisible(false);
    break;

  case POST_EFFECTS:
    // Hide amp controls on post-effects page
    sliders[PluginKnobs::Input]->setVisible(false);
    sliders[PluginKnobs::Bass]->setVisible(false);
    sliders[PluginKnobs::Middle]->setVisible(false);
    sliders[PluginKnobs::Treble]->setVisible(false);
    sliders[PluginKnobs::Output]->setVisible(false);

    // Hide Tube Screamer controls
    sliders[PluginKnobs::TSDrive]->setVisible(false);
    sliders[PluginKnobs::TSTone]->setVisible(false);
    sliders[PluginKnobs::TSLevel]->setVisible(false);
    tsEnabledToggle->setVisible(false);

    // Hide Compressor controls
    sliders[PluginKnobs::CompVolume]->setVisible(false);
    sliders[PluginKnobs::CompAttack]->setVisible(false);
    sliders[PluginKnobs::CompSustain]->setVisible(false);
    compEnabledToggle->setVisible(false);

    // Hide Boost controls
    sliders[PluginKnobs::BoostVolume]->setVisible(false);
    boostEnabledToggle->setVisible(false);
    break;
  }
}

void NamEditor::switchToPage(int pageIndex) {
  if (pageIndex < 0 || pageIndex > 2)
    return;

  currentPage = pageIndex;
  updatePageTabHighlight();
  setKnobVisibility();
  repaint();
}

void NamEditor::setupPageTabs() {
  // Page navigation tabs
  preEffectsPage = std::make_unique<juce::ImageButton>("PreEffectsPage");
  ampPage = std::make_unique<juce::ImageButton>("AmpPage");
  postEffectsPage = std::make_unique<juce::ImageButton>("PostEffectsPage");

  // Load and set tab icons
  juce::Image preIcon = juce::ImageFileFormat::loadFrom(
      BinaryData::PreIcon_png, BinaryData::PreIcon_pngSize);
  juce::Image ampIcon = juce::ImageFileFormat::loadFrom(
      BinaryData::AmpIcon_png, BinaryData::AmpIcon_pngSize);
  juce::Image postIcon = juce::ImageFileFormat::loadFrom(
      BinaryData::PostIcon_png, BinaryData::PostIcon_pngSize);

  preEffectsPage->setImages(false, true, true, preIcon, 1.0f,
                            juce::Colours::transparentBlack, preIcon, 0.8f,
                            juce::Colours::transparentBlack, preIcon, 0.6f,
                            juce::Colours::transparentBlack);
  ampPage->setImages(false, true, true, ampIcon, 1.0f,
                     juce::Colours::transparentBlack, ampIcon, 0.8f,
                     juce::Colours::transparentBlack, ampIcon, 0.6f,
                     juce::Colours::transparentBlack);
  postEffectsPage->setImages(false, true, true, postIcon, 1.0f,
                             juce::Colours::transparentBlack, postIcon, 0.8f,
                             juce::Colours::transparentBlack, postIcon, 0.6f,
                             juce::Colours::transparentBlack);

  addAndMakeVisible(preEffectsPage.get());
  addAndMakeVisible(ampPage.get());
  addAndMakeVisible(postEffectsPage.get());

  // Attach onClick handlers
  preEffectsPage->onClick = [this] { switchToPage(PRE_EFFECTS); };
  ampPage->onClick = [this] { switchToPage(AMP); };
  postEffectsPage->onClick = [this] { switchToPage(POST_EFFECTS); };

  // Position page navigation tabs
  int tabSize = 31;
  int tabSpacing = 22;
  int startX = 412;
  int startY = 19;

  preEffectsPage->setBounds(startX, startY, tabSize, tabSize);
  ampPage->setBounds(startX + tabSize + tabSpacing, startY, tabSize, tabSize);
  postEffectsPage->setBounds(startX + (tabSize + tabSpacing) * 2, startY,
                             tabSize, tabSize);

  // Set initial highlight state (AMP page is default)
  updatePageTabHighlight();
}

void NamEditor::initializeTopRow() {

  // --- initialize sliders ---

  const int knobSizeMinimal = 54;

  // Define top row slider configurations
  struct TopRowSliderConfig {
    PluginKnobs knobId;
    int x;
    int y;
    CustomSlider::SliderTypes sliderType;
  };

  const TopRowSliderConfig configs[] = {
      {PluginKnobs::PluginInput, 88, 89,
       CustomSlider::SliderTypes::PluginInput},
      {PluginKnobs::NoiseGate, 218, 89, CustomSlider::SliderTypes::Gate},
      {PluginKnobs::Doubler, 674, 89, CustomSlider::SliderTypes::Doubler},
      {PluginKnobs::PluginOutput, 804, 89,
       CustomSlider::SliderTypes::PluginOutput}};

  for (const auto &config : configs) {
    auto &slider = sliders[config.knobId];

    // Basic setup
    slider.reset(new CustomSlider());
    addAndMakeVisible(slider.get());

    // Apply minimal look and feel
    slider->setLookAndFeel(&lnfMinimal);
    slider->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider->setTextBoxStyle(juce::Slider::NoTextBox, false, 80, 20);

    // Position and customize
    slider->setBounds(config.x, config.y, knobSizeMinimal, knobSizeMinimal);
    slider->setCustomSlider(config.sliderType);
    slider->addListener(this);
  }

  // Initialize top row knob labels
  drawKnobLabels();

  // Initialize top row knob value labels
  drawKnobValueLabels();
  updateKnobValueLabels(); // Show initial values
}

void NamEditor::initializeAmpSliders() {
  const int knobSize = 51;
  const int xStart = 266;
  const int xOffsetMultiplier = 74;
  const int yPosition = 450;

  // Collect all amp control knob IDs (excluding top row)
  std::vector<int> ampKnobs;
  for (int slider = 0; slider < NUM_SLIDERS; ++slider) {
    if (slider != PluginKnobs::PluginInput &&
        slider != PluginKnobs::NoiseGate && slider != PluginKnobs::Doubler &&
        slider != PluginKnobs::PluginOutput) {
      ampKnobs.push_back(slider);
    }
  }

  // Initialize amp sliders
  int positionIndex = 0;
  for (int knobId : ampKnobs) {
    auto &slider = sliders[knobId];

    // Basic setup
    slider.reset(new CustomSlider());
    addAndMakeVisible(slider.get());

    // Apply main look and feel
    slider->setLookAndFeel(&lnf);
    slider->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider->setTextBoxStyle(juce::Slider::NoTextBox, false, 80, 20);

    // Position in main row
    slider->setBounds(xStart + (positionIndex * xOffsetMultiplier), yPosition,
                      knobSize, knobSize);

    slider->addListener(this);
    positionIndex++;
  }
}

void NamEditor::initializeTSSliders() {
  const int knobSize = 53;

  const int xStart = 497;
  const int xOffsetMultiplier = 65;
  const int yPosition = 261;

  // TS slider IDs
  const int tsSliders[] = {PluginKnobs::TSDrive, PluginKnobs::TSTone,
                           PluginKnobs::TSLevel};

  for (int i = 0; i < 3; ++i) {
    auto &slider = sliders[tsSliders[i]];

    // Basic setup
    slider.reset(new CustomSlider());
    addAndMakeVisible(slider.get());

    // Apply pre-effects look and feel
    slider->setLookAndFeel(&lnfPreEffects);
    slider->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider->setTextBoxStyle(juce::Slider::NoTextBox, false, 80, 20);

    // Position in row
    slider->setBounds(xStart + (i * xOffsetMultiplier), yPosition, knobSize,
                      knobSize);

    slider->addListener(this);

    // NO setCustomSlider() call - use default (same as amp sliders)
  }

  // TS Enable toggle
  tsEnabledToggle = std::make_unique<juce::ImageButton>("TS Enable");
  addAndMakeVisible(tsEnabledToggle.get());

  // Set up the button to act as a toggle button
  tsEnabledToggle->setClickingTogglesState(true);

  // Set initial images (will be updated by updateTSToggleAppearance)
  tsEnabledToggle->setImages(
      false, true, true, pedalButtonOff, 1.0f,
      juce::Colours::transparentBlack,                        // normal
      pedalButtonOff, 0.8f, juce::Colours::transparentBlack,  // over
      pedalButtonOff, 1.0f, juce::Colours::transparentBlack); // down

  // Add onClick handler to update appearance when toggled
  tsEnabledToggle->onClick = [this]() { updateTSToggleAppearance(); };

  tsEnabledToggle->setMouseCursor(juce::MouseCursor::PointingHandCursor);

  tsEnabledToggle->setBounds(561, 433, 53, 53);
}

void NamEditor::initializeCompSliders() {
  const int knobSize = 53;

  const int xStart = 48;
  const int xOffsetMultiplier = 57;
  const int yPosition = 261;

  // Compressor slider IDs
  const int compSliders[] = {PluginKnobs::CompVolume, PluginKnobs::CompAttack,
                             PluginKnobs::CompSustain};

  for (int i = 0; i < 3; ++i) {
    auto &slider = sliders[compSliders[i]];

    // Basic setup
    slider.reset(new CustomSlider());
    addAndMakeVisible(slider.get());

    // Apply pre-effects look and feel (same as TS)
    slider->setLookAndFeel(&lnfPreEffects);
    slider->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider->setTextBoxStyle(juce::Slider::NoTextBox, false, 80, 20);

    // Position in row
    slider->setBounds(xStart + (i * xOffsetMultiplier), yPosition, knobSize,
                      knobSize);

    slider->addListener(this);
  }

  // Compressor Enable toggle
  compEnabledToggle = std::make_unique<juce::ImageButton>("Comp Enable");
  addAndMakeVisible(compEnabledToggle.get());

  // Set up the button to act as a toggle button
  compEnabledToggle->setClickingTogglesState(true);

  // Use same pedal button images as TS
  compEnabledToggle->setImages(
      false, true, true, pedalButtonOff, 1.0f,
      juce::Colours::transparentBlack,                        // normal
      pedalButtonOff, 0.8f, juce::Colours::transparentBlack,  // over
      pedalButtonOff, 1.0f, juce::Colours::transparentBlack); // down

  // Add onClick handler to update appearance when toggled
  compEnabledToggle->onClick = [this]() { updateCompToggleAppearance(); };

  compEnabledToggle->setMouseCursor(juce::MouseCursor::PointingHandCursor);

  // Placeholder position
  compEnabledToggle->setBounds(104, 433, 53, 53);
}

void NamEditor::initializeBoostSliders() {
  const int knobSize = 62;

  const int xStart = 324;
  const int yPosition = 260; // Same Y as other pedals

  auto &slider = sliders[PluginKnobs::BoostVolume];

  // Basic setup
  slider.reset(new CustomSlider());
  addAndMakeVisible(slider.get());

  // Apply pre-effects look and feel
  slider->setLookAndFeel(&lnfPreEffects);
  slider->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  slider->setTextBoxStyle(juce::Slider::NoTextBox, false, 80, 20);

  // Position
  slider->setBounds(xStart, yPosition, knobSize, knobSize);

  slider->addListener(this);

  // Boost Enable toggle
  boostEnabledToggle = std::make_unique<juce::ImageButton>("Boost Enable");
  addAndMakeVisible(boostEnabledToggle.get());

  // Set up the button to act as a toggle button
  boostEnabledToggle->setClickingTogglesState(true);

  // Use same pedal button images
  boostEnabledToggle->setImages(
      false, true, true, pedalButtonOff, 1.0f,
      juce::Colours::transparentBlack,                        // normal
      pedalButtonOff, 0.8f, juce::Colours::transparentBlack,  // over
      pedalButtonOff, 1.0f, juce::Colours::transparentBlack); // down

  // Add onClick handler to update appearance when toggled
  boostEnabledToggle->onClick = [this]() { updateBoostToggleAppearance(); };

  boostEnabledToggle->setMouseCursor(juce::MouseCursor::PointingHandCursor);

  // Position below knob (roughly centered)
  // Comp toggle is at x=104 (knobs at 48, 105, 162) -> toggle ~middle knob
  // TS toggle is at x=561 (knobs at 497, 562, 627) -> toggle ~middle knob
  // Boost knob is at 310. Toggle should be centered below it.
  boostEnabledToggle->setBounds(327, 433, 53, 53);
}

void NamEditor::updateTSToggleAppearance() {
  // Update the button images based on toggle state
  bool isToggled = tsEnabledToggle->getToggleState();

  if (isToggled) {
    // Show ON image when enabled
    tsEnabledToggle->setImages(
        false, true, true, pedalButtonOn, 1.0f,
        juce::Colours::transparentBlack,                       // normal
        pedalButtonOn, 1.0f, juce::Colours::transparentBlack,  // over
        pedalButtonOn, 1.0f, juce::Colours::transparentBlack); // down
  } else {
    // Show OFF image when disabled
    tsEnabledToggle->setImages(
        false, true, true, pedalButtonOff, 1.0f,
        juce::Colours::transparentBlack,                        // normal
        pedalButtonOff, 1.0f, juce::Colours::transparentBlack,  // over
        pedalButtonOff, 1.0f, juce::Colours::transparentBlack); // down
  }
}

void NamEditor::updateCompToggleAppearance() {
  // Update the button images based on toggle state
  bool isToggled = compEnabledToggle->getToggleState();

  if (isToggled) {
    // Show ON image when enabled
    compEnabledToggle->setImages(
        false, true, true, pedalButtonOn, 1.0f,
        juce::Colours::transparentBlack,                       // normal
        pedalButtonOn, 1.0f, juce::Colours::transparentBlack,  // over
        pedalButtonOn, 1.0f, juce::Colours::transparentBlack); // down
  } else {
    // Show OFF image when disabled
    compEnabledToggle->setImages(
        false, true, true, pedalButtonOff, 1.0f,
        juce::Colours::transparentBlack,                        // normal
        pedalButtonOff, 1.0f, juce::Colours::transparentBlack,  // over
        pedalButtonOff, 1.0f, juce::Colours::transparentBlack); // down
  }
}

void NamEditor::updateBoostToggleAppearance() {
  // Update the button images based on toggle state
  bool isToggled = boostEnabledToggle->getToggleState();

  if (isToggled) {
    // Show ON image when enabled
    boostEnabledToggle->setImages(
        false, true, true, pedalButtonOn, 1.0f,
        juce::Colours::transparentBlack,                       // normal
        pedalButtonOn, 1.0f, juce::Colours::transparentBlack,  // over
        pedalButtonOn, 1.0f, juce::Colours::transparentBlack); // down
  } else {
    // Show OFF image when disabled
    boostEnabledToggle->setImages(
        false, true, true, pedalButtonOff, 1.0f,
        juce::Colours::transparentBlack,                        // normal
        pedalButtonOff, 1.0f, juce::Colours::transparentBlack,  // over
        pedalButtonOff, 1.0f, juce::Colours::transparentBlack); // down
  }
}

void NamEditor::initializeSliderAttachments() {
  // Hook slider attachments
  for (int slider = 0; slider < NUM_SLIDERS; ++slider) {
    sliderAttachments[slider] =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.apvts, sliderIDs[slider], *sliders[slider]);
  }

  // Special case: Doubler uses different parameter ID
  sliderAttachments[PluginKnobs::Doubler] =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          audioProcessor.apvts, "DOUBLER_SPREAD_ID",
          *sliders[PluginKnobs::Doubler]);

  // Attach TS Enable toggle button
  tsEnabledAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
          audioProcessor.apvts, "TS_ENABLED_ID", *tsEnabledToggle);

  // Attach Compressor Enable toggle button
  compEnabledAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
          audioProcessor.apvts, "COMP_ENABLED_ID", *compEnabledToggle);

  // Attach Boost Enable toggle button
  boostEnabledAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
          audioProcessor.apvts, "BOOST_ENABLED_ID", *boostEnabledToggle);
}