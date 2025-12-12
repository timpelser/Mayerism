#pragma once

// clang-format off
#include "PluginProcessor.h"
#include "MyLookAndFeel.h"
#include "TopBarComponent.h"
#include "PresetManager/PresetManagerComponent.h"
// clang-format on

#define NUM_SLIDERS 28

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
    KlonGain,      // Klon Centaur Gain
    KlonTreble,    // Klon Centaur Treble
    KlonLevel,     // Klon Centaur Level
    CompVolume,    // Compressor volume/makeup gain
    CompAttack,    // Compressor attack time
    CompSustain,   // Compressor sustain/release time
    BoostVolume,   // Clean Boost volume
    ChorusRate,    // Chorus rate/speed in Hz
    ChorusDepth,   // Chorus depth/modulation amount
    ChorusMix,     // Chorus wet/dry mix
    ReverbMix,     // Reverb wet/dry mix
    ReverbTone,    // Reverb damping/tone
    ReverbSize,    // Reverb room size
    DelayTime,     // Delay time in milliseconds
    DelayFeedback, // Delay feedback amount
    DelayMix       // Delay wet/dry mix
  };

  enum PageIndex { PRE_EFFECTS = 0, AMP = 1, POST_EFFECTS = 2 };

private:
  std::unique_ptr<CustomSlider> sliders[NUM_SLIDERS];
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      sliderAttachments[NUM_SLIDERS];

  juce::String sliderIDs[NUM_SLIDERS]{
      "PLUGIN_INPUT_ID", "INPUT_ID",         "NGATE_ID",
      "BASS_ID",         "MIDDLE_ID",        "TREBLE_ID",
      "OUTPUT_ID",       "PLUGIN_OUTPUT_ID", "DOUBLER_ID",
      "TS_DRIVE_ID",     "TS_TONE_ID",       "TS_LEVEL_ID",
      "KLON_GAIN_ID",    "KLON_TREBLE_ID",   "KLON_LEVEL_ID",
      "COMP_VOLUME_ID",  "COMP_ATTACK_ID",   "COMP_SUSTAIN_ID",
      "BOOST_VOLUME_ID", "CHORUS_RATE_ID",   "CHORUS_DEPTH_ID",
      "CHORUS_MIX_ID",   "REVERB_MIX_ID",    "REVERB_TONE_ID",
      "REVERB_SIZE_ID",  "DELAY_TIME_ID",    "DELAY_FEEDBACK_ID",
      "DELAY_MIX_ID"};

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

  // Pedal button images for post-effects pedals
  juce::Image pedalButtonOnPostEffects;
  juce::Image pedalButtonOffPostEffects;

  // juce::TooltipWindow tooltipWindow{ this, 200 };

  knobLookAndFeel lnf{knobLookAndFeel::KnobTypes::Main};
  knobLookAndFeel lnfMinimal{knobLookAndFeel::KnobTypes::Minimal};
  knobLookAndFeel lnfPreEffects{knobLookAndFeel::KnobTypes::PreEffects};
  knobLookAndFeel lnfPostEffects{knobLookAndFeel::KnobTypes::PostEffects};

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

  // Klon enable/bypass toggle
  std::unique_ptr<juce::ImageButton> klonEnabledToggle;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>
      klonEnabledAttachment;

  // Compressor enable/bypass toggle
  std::unique_ptr<juce::ImageButton> compEnabledToggle;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>
      compEnabledAttachment;

  // Clean Boost enable/bypass toggle
  std::unique_ptr<juce::ImageButton> boostEnabledToggle;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>
      boostEnabledAttachment;

  // Reverb enable/bypass toggle
  std::unique_ptr<juce::ImageButton> reverbEnabledToggle;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>
      reverbEnabledAttachment;

  // Delay enable/bypass toggle
  std::unique_ptr<juce::ImageButton> delayEnabledToggle;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>
      delayEnabledAttachment;

  // Chorus enable/bypass toggle
  std::unique_ptr<juce::ImageButton> chorusEnabledToggle;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>
      chorusEnabledAttachment;

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
  void updateKlonToggleAppearance();
  void updateCompToggleAppearance();
  void updateBoostToggleAppearance();
  void updateChorusToggleAppearance();
  void updateReverbToggleAppearance();
  void updateDelayToggleAppearance();

  // Slider initialization functions
  void initializeTopRow();
  void initializeAmpSliders();
  void initializeTSSliders();
  void initializeKlonSliders();
  void initializeCompSliders();
  void initializeBoostSliders();
  void initializeChorusSliders();
  void initializeReverbSliders();
  void initializeDelaySliders();
  void initializeSliderAttachments();
};