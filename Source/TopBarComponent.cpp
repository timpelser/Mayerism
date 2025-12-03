#include "TopBarComponent.h"

TopBarComponent::TopBarComponent(NamJUCEAudioProcessor &p)
    : AudioProcessorEditor(&p), audioProcessor(p) {
  lnf.setColour(juce::PopupMenu::backgroundColourId,
                Colours::grey.withAlpha(0.6f));

  settingsButton.reset(new juce::ImageButton("SettingsButton"));
  addAndMakeVisible(settingsButton.get());
  settingsButton->setImages(false, true, true, settingsUnpushed, 1.0f,
                            juce::Colours::transparentBlack, settingsUnpushed,
                            1.0f, juce::Colours::transparentBlack,
                            settingsPushed, 1.0f,
                            juce::Colours::transparentBlack, 0);
  settingsButton->setMouseCursor(juce::MouseCursor::PointingHandCursor);
  settingsButton->setTooltip("Settings");
  settingsButton->onClick = [this] {
    if (settingsDropdown->isPopupActive())
      settingsDropdown->hidePopup();
    else
      settingsDropdown->showPopup();
  };

  settingsDropdown.reset(new juce::ComboBox("Settings"));
  addAndMakeVisible(settingsDropdown.get());
  settingsDropdown->setVisible(false);
  if (JUCEApplication::isStandaloneApp())
    settingsDropdown->addItem(TRANS("Audio/Midi Settings..."), 1);
  settingsDropdown->addItem(TRANS("Info"), 2);
  settingsDropdown->addListener(this);
  settingsDropdown->setLookAndFeel(&lnf);
}

TopBarComponent::~TopBarComponent() {}

void TopBarComponent::paint(juce::Graphics &g) { g.fillAll(backgroundColour); }

void TopBarComponent::resized() {
  // Position settings button on the right side of topbar
  settingsButton->setBounds(getWidth() - 35, 7, 25, 25);
  settingsDropdown->setBounds(settingsButton->getBounds());
}

void TopBarComponent::setBackgroundColour(juce::Colour colour) {
  backgroundColour = colour;
  repaint();
}

void TopBarComponent::comboBoxChanged(ComboBox *comboBoxThatHasChanged) {
  if (comboBoxThatHasChanged == settingsDropdown.get()) {
    int selection = JUCEApplication::isStandaloneApp()
                        ? comboBoxThatHasChanged->getSelectedItemIndex()
                        : comboBoxThatHasChanged->getSelectedItemIndex() + 1;

    switch (selection) {
    case DropdownOptions::AudioSettings:
      if (JUCEApplication::isStandaloneApp())
        juce::StandalonePluginHolder::getInstance()->showAudioSettingsDialog();
      break;
    case DropdownOptions::Info:
      openInfoWindow("CraftLabs Mayerism \n\nVersion " +
                     juce::String(PLUG_VERSION) +
                     "\n\nAll Mayer tones in one plugin.");
      break;
    default:
      break;
    }

    // std::cout<<std::to_string(comboBoxThatHasChanged->getSelectedItemIndex())<<std::endl;
    settingsDropdown->setSelectedItemIndex(
        -1, juce::NotificationType::dontSendNotification);
  }
}

void TopBarComponent::openInfoWindow(juce::String m) {
  juce::DialogWindow::LaunchOptions options;
  auto *label = new Label();
  label->setText(m, dontSendNotification);
  label->setColour(Label::textColourId, Colours::whitesmoke);
  label->setJustificationType(juce::Justification::centred);
  options.content.setOwned(label);

  juce::Rectangle<int> area(0, 0, 300, 200);

  options.content->setSize(area.getWidth(), area.getHeight());

  options.dialogTitle = "Info";
  options.dialogBackgroundColour = juce::Colours::darkgrey;
  options.escapeKeyTriggersCloseButton = true;
  options.useNativeTitleBar = true;
  options.resizable = true;

  dialogWindow = options.launchAsync();
  // dialogWindow->setResizable(true, false);
  dialogWindow->setResizeLimits(300, 200, 300, 200);

  if (dialogWindow != nullptr)
    dialogWindow->centreWithSize(300, 200);
}