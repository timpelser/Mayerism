#pragma once

// clang-format off
#include "PluginProcessor.h"
#include "MyLookAndFeel.h"
#include "AssetManager.h"
#include "TopBarComponent.h"
#include "PresetManager/PresetManagerComponent.h"
// clang-format on

#define NUM_SLIDERS 9

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
    Doubler
  };

private:
  std::unique_ptr<CustomSlider> sliders[NUM_SLIDERS];
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      sliderAttachments[NUM_SLIDERS];

  juce::String sliderIDs[NUM_SLIDERS]{
      "PLUGIN_INPUT_ID", "INPUT_ID",         "NGATE_ID",
      "BASS_ID",         "MIDDLE_ID",        "TREBLE_ID",
      "OUTPUT_ID",       "PLUGIN_OUTPUT_ID", "DOUBLER_ID"};

  std::unique_ptr<AssetManager> assetManager;

  // juce::TooltipWindow tooltipWindow{ this, 200 };

  knobLookAndFeel lnf{knobLookAndFeel::KnobTypes::Main};
  knobLookAndFeel lnfMinimal{knobLookAndFeel::KnobTypes::Minimal};

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
};