#pragma once

// clang-format off
#include "PluginProcessor.h"
#include "MyLookAndFeel.h"
#include "TopBarComponent.h"
#include "PresetManager/PresetManagerComponent.h"
// clang-format on

#define NUM_SLIDERS 15

class NamEditor : public juce::AudioProcessorEditor,
                  public juce::Timer,
                  public juce::Slider::Listener {
public:
  NamEditor(NamJUCEAudioProcessor &);
  ~NamEditor() override;

  void paint(juce::Graphics &) override;
  void resized() override;

  void timerCallback();
  void sliderValueChanged(juce::Slider *slider);

  void setMeterPosition();

  enum PluginKnobs {
    PluginInput = 0, // Independent input gain
    Input,           // NAM amp input
    NoiseGate,
    Bass,
    Middle,
    Treble,
    Output,       // NAM amp output
    PluginOutput, // Independent output gain
    Doubler,
    TSDrive,
    TSTone,
    TSLevel,
    CompVolume, // Compressor volume/makeup gain
    CompAttack, // Compressor attack time
    CompSustain // Compressor sustain/release time
  };

  enum PageIndex { PRE_EFFECTS = 0, AMP = 1, POST_EFFECTS = 2 };

private:
  std::unique_ptr<CustomSlider> sliders[NUM_SLIDERS];
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      sliderAttachments[NUM_SLIDERS];

  juce::String sliderIDs[NUM_SLIDERS]{
      "PLUGIN_INPUT_ID", "INPUT_ID",       "NGATE_ID",       "BASS_ID",
      "MIDDLE_ID",       "TREBLE_ID",      "OUTPUT_ID",      "PLUGIN_OUTPUT_ID",
      "DOUBLER_ID",      "TS_DRIVE_ID",    "TS_TONE_ID",     "TS_LEVEL_ID",
      "COMP_VOLUME_ID",  "COMP_ATTACK_ID", "COMP_SUSTAIN_ID"};

  // Page background images
  juce::Image backgroundPreEffects;
  juce::Image backgroundPreEffectsNoKnobs; // Debug version without knobs
  juce::Image backgroundAmp;
  juce::Image backgroundPostEffects;

  // Debug flag: set to true to show background without knobs for positioning
  bool useDebugPreBackground = false;

  // Pedal button images for TS toggle
  juce::Image pedalButtonOn;
  juce::Image pedalButtonOff;

  // juce::TooltipWindow tooltipWindow{ this, 200 };

  knobLookAndFeel lnf{knobLookAndFeel::KnobTypes::Main};
  knobLookAndFeel lnfMinimal{knobLookAndFeel::KnobTypes::Minimal};
  knobLookAndFeel lnfPreEffects{knobLookAndFeel::KnobTypes::PreEffects};

  juce::String ngThreshold{"Null"};

  int screensOffset = 46;

  foleys::LevelMeter meterIn{foleys::LevelMeter::SingleChannel},
      meterOut{foleys::LevelMeter::SingleChannel};
  MeterLookAndFeel meterlnf, meterlnf2;

  TopBarComponent topBar;
  PresetManagerComponent pmc;
  juce::Label knobLabels[4]; // Labels for Input, Gate, Doubler, Output
  juce::Label
      knobValueLabels[4]; // Value labels for Input, Gate, Doubler, Output

  // Page navigation tabs
  std::unique_ptr<juce::ImageButton> preEffectsPage;
  std::unique_ptr<juce::ImageButton> ampPage;
  std::unique_ptr<juce::ImageButton> postEffectsPage;
  int currentPage = AMP; // Default to AMP page

  // Tube Screamer enable/bypass toggle
  std::unique_ptr<juce::ImageButton> tsEnabledToggle;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>
      tsEnabledAttachment;

  // Compressor enable/bypass toggle
  std::unique_ptr<juce::ImageButton> compEnabledToggle;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>
      compEnabledAttachment;

  NamJUCEAudioProcessor &audioProcessor;
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NamEditor)

  // Private Functions
private:
  // Pass this to the Preset Manager for updating the gui after loading a new
  // preset. Maybe not the best way of doing it...
  void updateAfterPresetLoad();
  void drawKnobLabels();
  void drawKnobValueLabels();
  void updateKnobValueLabels();
  void setupPageTabs();
  void switchToPage(int pageIndex);
  void updatePageTabHighlight();
  void setKnobVisibility();
  void updateTSToggleAppearance();
  void updateCompToggleAppearance();

  // Slider initialization functions
  void initializeTopRow();
  void initializeAmpSliders();
  void initializeTSSliders();
  void initializeCompSliders();
  void initializeSliderAttachments();
};