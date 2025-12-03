#include "PresetManagerComponent.h"

PresetManagerComponent::PresetManagerComponent(
    PresetManager &pm, std::function<void()> &&updateFunction)
    : presetManager(pm), parentUpdater(std::move(updateFunction)) {
  constructUI();
}

void PresetManagerComponent::constructUI() {
  // Light theme styling for ComboBox
  lnf.setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xFFEBEBEB));
  lnf.setColour(juce::PopupMenu::textColourId, juce::Colour(0xFF2C2C2C));
  lnf.setColour(juce::PopupMenu::highlightedBackgroundColourId,
                juce::Colour(0xFFD0D0D0));
  lnf.setColour(juce::TextEditor::textColourId, juce::Colour(0xFF2C2C2C));
  lnf.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xFFEBEBEB));

  // addAndMakeVisible(&presetName);
  presetName.setLookAndFeel(&lnf);
  presetName.setReadOnly(true);

  presetComboBox.clear(juce::dontSendNotification);
  addAndMakeVisible(&presetComboBox);
  presetComboBox.setEditableText(false);
  presetComboBox.setJustificationType(juce::Justification::centredLeft);
  presetComboBox.addListener(this);

  presetComboBox.setLookAndFeel(&lnf);
  // Light gray background with dark text
  presetComboBox.setColour(juce::ComboBox::backgroundColourId,
                           juce::Colour(0xFFE4E3E3));
  presetComboBox.setColour(juce::ComboBox::textColourId,
                           juce::Colour(0xFF2C2C2C));
  presetComboBox.setColour(juce::ComboBox::outlineColourId,
                           juce::Colours::transparentBlack);
  presetComboBox.setColour(juce::ComboBox::arrowColourId,
                           juce::Colour(0xFF717171));
  presetComboBox.setColour(juce::ComboBox::buttonColourId,
                           juce::Colour(0xFFEBEBEB));
  // Remove transparency for clean, solid appearance
  presetComboBox.setAlpha(1.0f);

  loadComboBox();

  addAndMakeVisible(&previousButton);
  addAndMakeVisible(&nextButton);

  nextButton.setImages(false, true, true, forwardUnpushed, 1.0f,
                       juce::Colours::transparentBlack, forwardUnpushed, 1.0f,
                       juce::Colours::transparentBlack, forwardPushed, 1.0f,
                       juce::Colours::transparentBlack, 0);
  previousButton.setImages(false, true, true, backUnushed, 1.0f,
                           juce::Colours::transparentBlack, backUnushed, 1.0f,
                           juce::Colours::transparentBlack, backPushed, 1.0f,
                           juce::Colours::transparentBlack, 0);

  nextButton.setMouseCursor(juce::MouseCursor::PointingHandCursor);
  previousButton.setMouseCursor(juce::MouseCursor::PointingHandCursor);

  nextButton.onClick = [this] {
    const auto index = presetManager.loadNextPreset();
    presetComboBox.setSelectedItemIndex(index, juce::sendNotification);
  };

  previousButton.onClick = [this] {
    const auto index = presetManager.loadPreviousPreset();
    presetComboBox.setSelectedItemIndex(index, juce::sendNotification);
  };

  addAndMakeVisible(&saveButton);
  saveButton.setButtonText("SAVE");
  saveButton.setColour(juce::TextButton::buttonColourId,
                       juce::Colours::transparentBlack);
  saveButton.setColour(juce::TextButton::buttonOnColourId,
                       juce::Colours::transparentBlack);
  saveButton.setColour(juce::TextButton::textColourOffId, juce::Colours::black);
  saveButton.setColour(juce::TextButton::textColourOnId, juce::Colours::black);
  saveButton.setColour(juce::ComboBox::outlineColourId,
                       juce::Colours::transparentBlack);
  saveButton.setMouseCursor(juce::MouseCursor::PointingHandCursor);
  saveButton.setTooltip("Save Preset");
  saveButton.onClick = [this] {
    fileChooser = std::make_unique<juce::FileChooser>(
        "Enter Preset Name", PresetManager::defaultPresetDirectory,
        "*." + PresetManager::presetExtension);
    fileChooser->launchAsync(juce::FileBrowserComponent::saveMode,
                             [&](const juce::FileChooser &chooser) {
                               const auto resultFile = chooser.getResult();
                               presetManager.savePreset(
                                   resultFile.getFileNameWithoutExtension());
                               loadComboBox();
                             });
  };
}

void PresetManagerComponent::loadComboBox() {
  presetComboBox.clear(juce::dontSendNotification);

  const auto allPresets = presetManager.getAllPresets();
  const auto currentPreset = presetManager.getCurrentPreset();
  presetComboBox.addItemList(presetManager.getAllPresets(), 1);
  presetComboBox.setSelectedItemIndex(allPresets.indexOf(currentPreset),
                                      juce::dontSendNotification);
}

void PresetManagerComponent::setColour(juce::Colour colourToUse, float alpha) {
  barColour = colourToUse;
  barAlpha = alpha;
  repaint();
}

void PresetManagerComponent::setColour(juce::Colour colourToUse) {
  barColour = colourToUse;
  barAlpha = 0.4f;
  repaint();
}

void PresetManagerComponent::paint(juce::Graphics &g) {
  g.fillAll(barColour.withAlpha(barAlpha));
  g.setColour(juce::Colours::white.withAlpha(0.3f));
}

void PresetManagerComponent::resized() {
  presetName.setBounds(20, 35, getWidth() * 0.58, 44);
  presetComboBox.setBounds(0, 20, getWidth(), getHeight() - 20);
  previousButton.setBounds(2, 0, 16, 16);

  nextButton.setBounds(25, 0, 16, 16);
  saveButton.setBounds(presetName.getX() + 198, -2, 50, 20);
}

void PresetManagerComponent::parameterChanged() {}

void PresetManagerComponent::comboBoxChanged(
    juce::ComboBox *comboBoxThatHasChanged) {
  presetName.setText(comboBoxThatHasChanged->getItemText(
      comboBoxThatHasChanged->getSelectedId() - 1));
  presetManager.loadPreset(comboBoxThatHasChanged->getItemText(
      comboBoxThatHasChanged->getSelectedItemIndex()));

  parentUpdater();
}
