#pragma once

// clang-format off
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "NeuralAmpModeler.h"
#include <ff_meters/ff_meters.h>
#include "DoublerProcessor.h"
#include "pedals/TubeScreamer/TSProcessor.h"
#include "pedals/KlonCentaur/KlonProcessor.h"
#include "pedals/Compressor/CompressorProcessor.h"
#include "pedals/CleanBoost/CleanBoostProcessor.h"
#include "pedals/Chorus/ChorusProcessor.h"
#include "pedals/Reverb/ReverbProcessor.h"
#include "pedals/Delay/DelayProcessor.h"
#include "PresetManager/PresetManager.h"
// clang-format on

//==============================================================================
/**
 */
class NamJUCEAudioProcessor : public juce::AudioProcessor {
public:
  //==============================================================================
  NamJUCEAudioProcessor();
  ~NamJUCEAudioProcessor() override;

  //==============================================================================
  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
  bool isBusesLayoutSupported(const BusesLayout &layouts) const override;
#endif

  void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;

  //==============================================================================
  juce::AudioProcessorEditor *createEditor() override;
  bool hasEditor() const override;

  //==============================================================================
  const juce::String getName() const override;

  bool acceptsMidi() const override;
  bool producesMidi() const override;
  bool isMidiEffect() const override;
  double getTailLengthSeconds() const override;

  //==============================================================================
  int getNumPrograms() override;
  int getCurrentProgram() override;
  void setCurrentProgram(int index) override;
  const juce::String getProgramName(int index) override;
  void changeProgramName(int index, const juce::String &newName) override;

  //==============================================================================
  void getStateInformation(juce::MemoryBlock &destData) override;
  void setStateInformation(const void *data, int sizeInBytes) override;

  bool getTriggerStatus();

  bool supportsDoublePrecisionProcessing() const override;

  foleys::LevelMeterSource &getMeterInSource() { return meterInSource; }
  foleys::LevelMeterSource &getMeterOutSource() { return meterOutSource; }

  juce::AudioProcessorValueTreeState apvts;
  juce::AudioProcessorValueTreeState::ParameterLayout createParameters();

  PresetManager &getPresetManager() { return presetManager; };

  void loadFromPreset(juce::String modelPath, juce::String irPath);

  bool isNamModelLoaded() const { return namModelLoaded; }

private:
  //==============================================================================

  NeuralAmpModeler myNAM;

  bool namModelLoaded{false};

  Doubler doubler;
  TSProcessor tsProcessor;
  KlonProcessor klonProcessor;
  CompressorProcessor compressorProcessor;
  CleanBoostProcessor cleanBoostProcessor;
  ChorusProcessor chorusProcessor;
  ReverbProcessor reverbProcessor;
  DelayProcessor delayProcessor;

  bool supportsDouble{false};

  foleys::LevelMeterSource meterInSource;
  foleys::LevelMeterSource meterOutSource;

  PresetManager presetManager;

  // Independent input/output gain parameters
  std::atomic<float> *pluginInputGain;
  std::atomic<float> *pluginOutputGain;

  // Tube Screamer parameters
  std::atomic<float> *tsDrive;
  std::atomic<float> *tsTone;
  std::atomic<float> *tsLevel;
  std::atomic<float> *tsEnabled;

  // Klon Centaur parameters
  std::atomic<float> *klonGain;
  std::atomic<float> *klonTreble;
  std::atomic<float> *klonLevel;
  std::atomic<float> *klonEnabled;

  // Compressor parameters
  std::atomic<float> *compVolume;
  std::atomic<float> *compAttack;
  std::atomic<float> *compSustain;
  std::atomic<float> *compEnabled;

  // Clean Boost parameters
  std::atomic<float> *boostVolume;
  std::atomic<float> *boostEnabled;

  // Chorus parameters
  std::atomic<float> *chorusRate;
  std::atomic<float> *chorusDepth;
  std::atomic<float> *chorusMix;
  std::atomic<float> *chorusEnabled;

  // Reverb parameters
  std::atomic<float> *reverbMix;
  std::atomic<float> *reverbTone;
  std::atomic<float> *reverbSize;
  std::atomic<float> *reverbEnabled;

  // Delay parameters
  std::atomic<float> *delayTime;
  std::atomic<float> *delayFeedback;
  std::atomic<float> *delayMix;
  std::atomic<float> *delayEnabled;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NamJUCEAudioProcessor)
};
