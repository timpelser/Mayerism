#include "NamEditor.h"

NamEditor::NamEditor(NamJUCEAudioProcessor &p)
    : AudioProcessorEditor(&p), audioProcessor(p),
      topBar(p, [&]() { updateAfterPresetLoad(); }) {
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

  int knobSize = 98;
  int xStart = 75;
  int xOffsetMultiplier = 140;

  lnf.setColour(Slider::textBoxOutlineColourId,
                juce::Colours::transparentBlack);
  lnf.setColour(Slider::textBoxBackgroundColourId,
                juce::Colours::transparentBlack);
  lnf.setColour(Slider::textBoxTextColourId, juce::Colours::ivory);

  // Setup sliders
  for (int slider = 0; slider < NUM_SLIDERS; ++slider) {
    sliders[slider].reset(new CustomSlider());
    addAndMakeVisible(sliders[slider].get());
    sliders[slider]->setLookAndFeel(&lnf);
    sliders[slider]->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    sliders[slider]->setTextBoxStyle(juce::Slider::NoTextBox, false, 80, 20);
    // sliders[slider]->setPopupDisplayEnabled(true, true,
    // getTopLevelComponent());

    sliders[slider]->setBounds(xStart + (slider * xOffsetMultiplier), 204,
                               knobSize, knobSize);
  }

  sliders[PluginKnobs::Doubler]->setPopupDisplayEnabled(true, true,
                                                        getTopLevelComponent());
  sliders[PluginKnobs::Doubler]->setCustomSlider(
      CustomSlider::SliderTypes::Doubler);
  sliders[PluginKnobs::Doubler]->setTextBoxStyle(juce::Slider::NoTextBox, false,
                                                 80, 20);
  sliders[PluginKnobs::Doubler]->setBounds(sliders[PluginKnobs::Output]->getX(),
                                           435, knobSize, knobSize);
  sliders[PluginKnobs::NoiseGate]->setPopupDisplayEnabled(
      true, true, getTopLevelComponent());
  sliders[PluginKnobs::NoiseGate]->setCustomSlider(
      CustomSlider::SliderTypes::Gate);
  sliders[PluginKnobs::NoiseGate]->setPopupDisplayEnabled(
      true, true, getTopLevelComponent());
  sliders[PluginKnobs::NoiseGate]->addListener(this);

  // Tone Stack Toggle
  toneStackToggle.reset(new juce::ToggleButton("ToneStackToggleButton"));
  addAndMakeVisible(toneStackToggle.get());
  toneStackToggle->setBounds(sliders[PluginKnobs::Bass]->getX() + 30,
                             sliders[PluginKnobs::Bass]->getY() + knobSize + 50,
                             90, 40);
  toneStackToggle->setButtonText("Tone Stack");
  toneStackToggle->onClick = [this] {
    setToneStackEnabled(
        bool(*audioProcessor.apvts.getRawParameterValue("TONE_STACK_ON_ID")));
  };
  toneStackToggle->setVisible(false);

  // Rerunning this for GUI Recustrunction upon reopning the plugin
  setToneStackEnabled(
      bool(*audioProcessor.apvts.getRawParameterValue("TONE_STACK_ON_ID")));

  // Normalize Toggle
  normalizeToggle.reset(new juce::ToggleButton("NormalizeToggleButton"));
  addAndMakeVisible(normalizeToggle.get());
  normalizeToggle->setBounds(toneStackToggle->getX() + 120,
                             toneStackToggle->getY(), 90, 40);
  normalizeToggle->setButtonText("Normalize");
  normalizeToggle->setVisible(false);

  // Hook slider and button attacments
  for (int slider = 0; slider < NUM_SLIDERS; ++slider)
    sliderAttachments[slider] =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.apvts, sliderIDs[slider], *sliders[slider]);

  sliderAttachments[PluginKnobs::Doubler] =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          audioProcessor.apvts, "DOUBLER_SPREAD_ID",
          *sliders[PluginKnobs::Doubler]);

  toneStackToggleAttachment.reset(
      new juce::AudioProcessorValueTreeState::ButtonAttachment(
          audioProcessor.apvts, "TONE_STACK_ON_ID", *toneStackToggle));
  normalizeToggleAttachment.reset(
      new juce::AudioProcessorValueTreeState::ButtonAttachment(
          audioProcessor.apvts, "NORMALIZE_ID", *normalizeToggle));

  assetManager->initializeButton(toneStackButton,
                                 AssetManager::Buttons::TONESTACK_BUTTON);
  addAndMakeVisible(toneStackButton.get());
  toneStackButton->setBounds(
      sliders[PluginKnobs::Middle]->getX() +
          (sliders[PluginKnobs::Middle]->getWidth() / 2) - 45,
      sliders[PluginKnobs::Middle]->getY() +
          sliders[PluginKnobs::Middle]->getHeight() + 15,
      90, 40);
  toneStackButton->setLedState(
      *audioProcessor.apvts.getRawParameterValue("TONE_STACK_ON_ID"));
  toneStackButton->onClick = [this] {
    toneStackToggle->setToggleState(!toneStackToggle->getToggleState(), true);
    toneStackButton->setLedState(
        *audioProcessor.apvts.getRawParameterValue("TONE_STACK_ON_ID"));
  };

  assetManager->initializeButton(normalizeButton,
                                 AssetManager::Buttons::NORMALIZE_BUTTON);
  addAndMakeVisible(normalizeButton.get());
  normalizeButton->setBounds(
      sliders[PluginKnobs::Treble]->getX() +
          (sliders[PluginKnobs::Treble]->getWidth() / 2) - 45,
      sliders[PluginKnobs::Treble]->getY() +
          sliders[PluginKnobs::Treble]->getHeight() + 15,
      90, 40);
  normalizeButton->setLedState(
      *audioProcessor.apvts.getRawParameterValue("NORMALIZE_ID"));
  normalizeButton->onClick = [this] {
    normalizeToggle->setToggleState(!normalizeToggle->getToggleState(), true);
    normalizeButton->setLedState(
        *audioProcessor.apvts.getRawParameterValue("NORMALIZE_ID"));
  };

  meterIn.toFront(true);
  meterOut.toFront(true);

  addAndMakeVisible(&topBar);

  addAndMakeVisible(debugLabel);
  debugLabel.setColour(juce::Label::textColourId, juce::Colours::white);
  debugLabel.setJustificationType(juce::Justification::centred);

  startTimer(30);
}

NamEditor::~NamEditor() {
  for (int sliderAtt = 0; sliderAtt < NUM_SLIDERS; ++sliderAtt)
    sliderAttachments[sliderAtt] = nullptr;

  toneStackToggleAttachment = nullptr;
  normalizeToggleAttachment = nullptr;
}

void NamEditor::paint(juce::Graphics &g) {
  g.fillAll(juce::Colour::fromString("FF121212"));

  g.setColour(juce::Colours::white);
  g.setFont(15.0f);

  g.drawImageAt(assetManager->getBackground(), 0, 0);
  g.drawImageAt(assetManager->getScreens(), 0, 0);

  //// TODO: Move this into a dedicated component with its own timer
  g.drawImageAt(led_to_draw, 296, 168);
}

void NamEditor::resized() {
  topBar.setBounds(0, 0, getWidth(), 40);
  debugLabel.setBounds(0, 40, getWidth(), 30);
}

void NamEditor::timerCallback() {
  //// TODO: Move this into a dedicated component with its own timer

  if (audioProcessor.getTriggerStatus() &&
      static_cast<float>(
          *audioProcessor.apvts.getRawParameterValue("NGATE_ID")) > -101.0)
    led_to_draw = led_on;
  else
    led_to_draw = led_off;

  debugLabel.setText(
      "Model Loaded: " +
          juce::String(audioProcessor.isNamModelLoaded() ? "True" : "False"),
      juce::dontSendNotification);

  repaint();
}

void NamEditor::sliderValueChanged(juce::Slider *slider) {}

void NamEditor::setToneStackEnabled(bool toneStackEnabled) {
  for (int slider = PluginKnobs::Bass; slider <= PluginKnobs::Treble;
       ++slider) {
    sliders[slider]->setEnabled(toneStackEnabled);
    toneStackEnabled ? sliders[slider]->setAlpha(1.0f)
                     : sliders[slider]->setAlpha(0.3f);
  }
}

void NamEditor::initializeTextBox(const juce::String label,
                                  std::unique_ptr<juce::TextEditor> &textBox,
                                  int x, int y, int width, int height) {
  textBox.reset(new juce::TextEditor(label));
  addAndMakeVisible(textBox.get());
  textBox->setMultiLine(false);
  textBox->setReturnKeyStartsNewLine(false);
  textBox->setReadOnly(true);
  textBox->setScrollbarsShown(true);
  textBox->setCaretVisible(true);
  textBox->setPopupMenuEnabled(true);
  textBox->setAlpha(0.9f);
  textBox->setColour(juce::TextEditor::backgroundColourId,
                     juce::Colours::transparentBlack);
  textBox->setColour(juce::TextEditor::outlineColourId,
                     juce::Colours::transparentBlack);
  juce::Font textBoxFont;
  textBoxFont.setHeight(18.0f);
  // textBoxFont.setBold(true);
  textBox->setFont(textBoxFont);
  textBox->setAlpha(0.8f);
  textBox->setBounds(x, y, width, height);
}

void NamEditor::initializeButton(const juce::String label,
                                 const juce::String buttonText,
                                 std::unique_ptr<juce::ImageButton> &button,
                                 int x, int y, int width, int height) {
  button.reset(new juce::ImageButton(label));
  addAndMakeVisible(button.get());
  button->setBounds(x, y, width, height);
}

void NamEditor::setMeterPosition(bool isOnMainScreen) {
  if (isOnMainScreen) {
    meterlnf.setColour(foleys::LevelMeter::lmMeterGradientLowColour,
                       juce::Colours::ivory);
    meterlnf.setColour(foleys::LevelMeter::lmMeterOutlineColour,
                       juce::Colours::transparentWhite);
    meterlnf.setColour(foleys::LevelMeter::lmMeterBackgroundColour,
                       juce::Colours::transparentWhite);
    meterIn.setLookAndFeel(&meterlnf);
    meterOut.setLookAndFeel(&meterlnf);

    int meterHeight = 172;
    int meterWidth = 18;
    meterIn.setBounds(juce::Rectangle<int>(26, 174, meterWidth, meterHeight));
    meterOut.setBounds(juce::Rectangle<int>(getWidth() - meterWidth - 21, 174,
                                            meterWidth, meterHeight));
  } else {
    meterlnf2.setColour(foleys::LevelMeter::lmMeterGradientLowColour,
                        juce::Colours::ivory);
    meterIn.setLookAndFeel(&meterlnf2);
    meterOut.setLookAndFeel(&meterlnf2);

    int meterHeight = 255;
    int meterWidth = 20;
    meterIn.setBounds(20, (getHeight() / 2) - (meterHeight / 2) + 10,
                      meterWidth, meterHeight);
    meterOut.setBounds(getWidth() - 30,
                       (getHeight() / 2) - (meterHeight / 2) + 10, meterWidth,
                       meterHeight);
  }
}

void NamEditor::updateAfterPresetLoad() {

  setToneStackEnabled(
      bool(*audioProcessor.apvts.getRawParameterValue("TONE_STACK_ON_ID")));

  normalizeButton->setLedState(
      *audioProcessor.apvts.getRawParameterValue("NORMALIZE_ID"));
  toneStackButton->setLedState(
      *audioProcessor.apvts.getRawParameterValue("TONE_STACK_ON_ID"));
}