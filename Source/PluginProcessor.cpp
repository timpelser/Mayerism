/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
NamJUCEAudioProcessor::NamJUCEAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(
          BusesProperties()
#if !JucePlugin_IsMidiEffect
#if !JucePlugin_IsSynth
              .withInput("Input", juce::AudioChannelSet::mono(), true)
#endif
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
              ),
      apvts(*this, nullptr, "Params", createParameters()), presetManager(apvts)
#endif
{
}

NamJUCEAudioProcessor::~NamJUCEAudioProcessor() {}

//==============================================================================
const juce::String NamJUCEAudioProcessor::getName() const {
  return JucePlugin_Name;
}

bool NamJUCEAudioProcessor::acceptsMidi() const {
#if JucePlugin_WantsMidiInput
  return true;
#else
  return false;
#endif
}

bool NamJUCEAudioProcessor::producesMidi() const {
#if JucePlugin_ProducesMidiOutput
  return true;
#else
  return false;
#endif
}

bool NamJUCEAudioProcessor::isMidiEffect() const {
#if JucePlugin_IsMidiEffect
  return true;
#else
  return false;
#endif
}

double NamJUCEAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int NamJUCEAudioProcessor::getNumPrograms() {
  return 1; // NB: some hosts don't cope very well if you tell them there are 0
            // programs, so this should be at least 1, even if you're not really
            // implementing programs.
}

int NamJUCEAudioProcessor::getCurrentProgram() { return 0; }

void NamJUCEAudioProcessor::setCurrentProgram(int index) {}

const juce::String NamJUCEAudioProcessor::getProgramName(int index) {
  return {};
}

void NamJUCEAudioProcessor::changeProgramName(int index,
                                              const juce::String &newName) {}

//==============================================================================
void NamJUCEAudioProcessor::prepareToPlay(double sampleRate,
                                          int samplesPerBlock) {
  juce::dsp::ProcessSpec spec;

  spec.sampleRate = sampleRate;
  spec.numChannels = getNumOutputChannels();
  spec.maximumBlockSize = samplesPerBlock;

  compressorProcessor.prepare(spec);

  cleanBoostProcessor.prepare(spec);

  tsProcessor.prepare(spec);
  klonProcessor.prepare(spec);

  myNAM.prepare(spec);
  myNAM.hookParameters(apvts);

  // Hook independent input/output gain parameters
  pluginInputGain = apvts.getRawParameterValue("PLUGIN_INPUT_ID");
  pluginOutputGain = apvts.getRawParameterValue("PLUGIN_OUTPUT_ID");

  // Hook Tube Screamer parameters
  tsDrive = apvts.getRawParameterValue("TS_DRIVE_ID");
  tsTone = apvts.getRawParameterValue("TS_TONE_ID");
  tsLevel = apvts.getRawParameterValue("TS_LEVEL_ID");
  tsEnabled = apvts.getRawParameterValue("TS_ENABLED_ID");

  // Hook Klon Centaur parameters
  klonGain = apvts.getRawParameterValue("KLON_GAIN_ID");
  klonTreble = apvts.getRawParameterValue("KLON_TREBLE_ID");
  klonLevel = apvts.getRawParameterValue("KLON_LEVEL_ID");
  klonEnabled = apvts.getRawParameterValue("KLON_ENABLED_ID");

  // Hook Compressor parameters
  compVolume = apvts.getRawParameterValue("COMP_VOLUME_ID");
  compAttack = apvts.getRawParameterValue("COMP_ATTACK_ID");
  compSustain = apvts.getRawParameterValue("COMP_SUSTAIN_ID");
  compEnabled = apvts.getRawParameterValue("COMP_ENABLED_ID");

  // Hook Clean Boost parameters
  boostVolume = apvts.getRawParameterValue("BOOST_VOLUME_ID");
  boostEnabled = apvts.getRawParameterValue("BOOST_ENABLED_ID");

  // Hook Chorus parameters
  chorusRate = apvts.getRawParameterValue("CHORUS_RATE_ID");
  chorusDepth = apvts.getRawParameterValue("CHORUS_DEPTH_ID");
  chorusMix = apvts.getRawParameterValue("CHORUS_MIX_ID");
  chorusEnabled = apvts.getRawParameterValue("CHORUS_ENABLED_ID");

  // Hook Reverb parameters
  reverbMix = apvts.getRawParameterValue("REVERB_MIX_ID");
  reverbTone = apvts.getRawParameterValue("REVERB_TONE_ID");
  reverbSize = apvts.getRawParameterValue("REVERB_SIZE_ID");
  reverbEnabled = apvts.getRawParameterValue("REVERB_ENABLED_ID");

  // Hook Delay parameters
  delayTime = apvts.getRawParameterValue("DELAY_TIME_ID");
  delayFeedback = apvts.getRawParameterValue("DELAY_FEEDBACK_ID");
  delayMix = apvts.getRawParameterValue("DELAY_MIX_ID");
  delayEnabled = apvts.getRawParameterValue("DELAY_ENABLED_ID");

  doubler.prepare(spec);

  chorusProcessor.prepare(spec);

  reverbProcessor.prepare(spec);

  delayProcessor.prepare(spec);

  meterInSource.resize(getTotalNumOutputChannels(),
                       sampleRate * 0.1 / samplesPerBlock);
  meterOutSource.resize(getTotalNumOutputChannels(),
                        sampleRate * 0.1 / samplesPerBlock);

  // Load the baked-in model via a temporary file
  auto tempDir = juce::File::getSpecialLocation(juce::File::tempDirectory);
  auto modelTempFile = tempDir.getChildFile("tworock_baked.nam");

  // Write binary data to temp file if it doesn't exist or has different size
  // (Simple check to avoid rewriting every time, though rewriting is also
  // fast)
  if (!modelTempFile.existsAsFile() ||
      modelTempFile.getSize() != BinaryData::tworock_namSize) {
    modelTempFile.replaceWithData(BinaryData::tworock_nam,
                                  BinaryData::tworock_namSize);
  }

  if (modelTempFile.existsAsFile()) {
    namModelLoaded =
        myNAM.loadModel(modelTempFile.getFullPathName().toStdString());
  } else {
    DBG("Failed to create temp model file at: " +
        modelTempFile.getFullPathName());
  }

  // namModelLoaded =
  //     myNAM.loadModel("/Users/timpelser/Documents/SideProjects/Mayerism/"
  //                     "codebase/nam-juce/Assets/AmpModels/tworock.nam");
}

bool NamJUCEAudioProcessor::getTriggerStatus() {
  auto t_state = myNAM.getTrigger();
  return t_state->isGating();
}

void NamJUCEAudioProcessor::releaseResources() {}

#ifndef JucePlugin_PreferredChannelConfigurations
bool NamJUCEAudioProcessor::isBusesLayoutSupported(
    const BusesLayout &layouts) const {
#if JucePlugin_IsMidiEffect
  juce::ignoreUnused(layouts);
  return true;
#else
  // This is the place where you check if the layout is supported.
  // In this template code we only support mono or stereo.
  // Some plugin hosts, such as certain GarageBand versions, will only
  // load plugins that support stereo bus layouts.
  if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
      layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
    return false;

    // This checks if the input layout matches the output layout
#if !JucePlugin_IsSynth
  if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
    return false;
#endif

  return true;
#endif
}
#endif

void NamJUCEAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer,
                                         juce::MidiBuffer &midiMessages) {
  juce::ScopedNoDenormals noDenormals;
  auto totalNumInputChannels = getTotalNumInputChannels();
  auto totalNumOutputChannels = getTotalNumOutputChannels();

  for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
    buffer.clear(i, 0, buffer.getNumSamples());

  juce::dsp::AudioBlock<float> block(buffer);

  auto *channelDataLeft = buffer.getWritePointer(0);
  auto *channelDataRight = buffer.getWritePointer(1);

  buffer.applyGain(std::powf(10.0f, pluginInputGain->load() / 20.0f));

  meterInSource.measureBlock(buffer);

  // Apply -10dB Safety Pad
  buffer.applyGain(juce::Decibels::decibelsToGain(-10.0f));

  // Compressor (at beginning of chain, before TS and amp)
  if (compEnabled->load() > 0.5f) {
    compressorProcessor.setVolume(compVolume->load());
    compressorProcessor.setAttack(compAttack->load());
    compressorProcessor.setSustain(compSustain->load());
    compressorProcessor.process(buffer);
  }

  // Clean Boost (after compressor, before TS)
  if (boostEnabled->load() > 0.5f) {
    cleanBoostProcessor.setBoost(boostVolume->load());
    cleanBoostProcessor.process(buffer);
  }

  // TubeScreamer TS808 (before amp) - only process if enabled
  if (tsEnabled->load() > 0.5f) {
    tsProcessor.setDrive(tsDrive->load());
    tsProcessor.setTone(tsTone->load());
    tsProcessor.setLevel(tsLevel->load());
    tsProcessor.process(buffer);
  }

  // Klon Centaur (after TS, before amp) - only process if enabled
  if (klonEnabled->load() > 0.5f) {
    klonProcessor.setGain(klonGain->load() / 10.0f);
    klonProcessor.setTreble(klonTreble->load() / 10.0f);
    klonProcessor.setLevel(klonLevel->load() / 10.0f);
    klonProcessor.process(buffer);
  }

  myNAM.processBlock(buffer);

  // Do Dual Mono
  for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    channelDataRight[sample] = channelDataLeft[sample];

  // Doubler
  if (*apvts.getRawParameterValue("DOUBLER_SPREAD_ID") > 0.0) {
    doubler.setDelayMs(*apvts.getRawParameterValue("DOUBLER_SPREAD_ID"));
    doubler.process(buffer);
  }

  // Chorus
  if (chorusEnabled->load() > 0.5f) {
    chorusProcessor.setRate(chorusRate->load());
    chorusProcessor.setDepth(chorusDepth->load());
    chorusProcessor.setMix(chorusMix->load());
    chorusProcessor.process(buffer);
  }

  // Delay
  if (delayEnabled->load() > 0.5f) {
    delayProcessor.setTime(delayTime->load());
    delayProcessor.setFeedback(delayFeedback->load());
    delayProcessor.setMix(delayMix->load());
    delayProcessor.process(buffer);
  }

  // Reverb (at end of chain, post-effects)
  if (reverbEnabled->load() > 0.5f) {
    reverbProcessor.setMix(reverbMix->load());
    reverbProcessor.setTone(reverbTone->load());
    reverbProcessor.setSize(reverbSize->load());
    reverbProcessor.process(buffer);
  }

  // Apply independent output gain AFTER all post-effects
  buffer.applyGain(std::powf(10.0f, pluginOutputGain->load() / 20.0f));

  // --- SAFETY OUTPUT CLIPPER ---
  // Soft clip the final output to prevent harsh digital clipping.
  // Limiting slightly below 0dBfs (-0.1dB = ~0.988)
  const float clipThreshold = 0.988f;

  for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
    auto *data = buffer.getWritePointer(channel);
    for (int i = 0; i < buffer.getNumSamples(); ++i) {
      // std::tanh creates a smooth "analog-like" curve as it approaches limit
      data[i] = clipThreshold * std::tanh(data[i] / clipThreshold);
    }
  }

  meterOutSource.measureBlock(buffer);
}

//==============================================================================
bool NamJUCEAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor *NamJUCEAudioProcessor::createEditor() {
  return new NamJUCEAudioProcessorEditor(*this);
}

//==============================================================================
void NamJUCEAudioProcessor::getStateInformation(juce::MemoryBlock &destData) {}

void NamJUCEAudioProcessor::setStateInformation(const void *data,
                                                int sizeInBytes) {}

juce::AudioProcessorValueTreeState::ParameterLayout
NamJUCEAudioProcessor::createParameters() {
  std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;

  myNAM.createParameters(parameters);

  // Independent input/output gain parameters
  parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
      "PLUGIN_INPUT_ID", "PLUGIN_INPUT", -20.0f, 20.0f, 0.0f));
  parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
      "PLUGIN_OUTPUT_ID", "PLUGIN_OUTPUT", -20.0f, 20.0f, 0.0f));

  // Tube Screamer parameters (0-10 range, matching real TS-808)
  parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
      "TS_DRIVE_ID", "TS_DRIVE", 0.0f, 10.0f, 5.0f));
  parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
      "TS_TONE_ID", "TS_TONE", 0.0f, 10.0f, 5.0f));
  parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
      "TS_LEVEL_ID", "TS_LEVEL", 0.0f, 10.0f, 5.0f));
  parameters.push_back(std::make_unique<juce::AudioParameterBool>(
      "TS_ENABLED_ID", "TS_ENABLED", false));

  // Klon Centaur parameters (0-10 range)
  parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
      "KLON_GAIN_ID", "KLON_GAIN", 0.0f, 10.0f, 5.0f));
  parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
      "KLON_TREBLE_ID", "KLON_TREBLE", 0.0f, 10.0f, 5.0f));
  parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
      "KLON_LEVEL_ID", "KLON_LEVEL", 0.0f, 10.0f, 5.0f));
  parameters.push_back(std::make_unique<juce::AudioParameterBool>(
      "KLON_ENABLED_ID", "KLON_ENABLED", false));

  // Compressor parameters
  parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
      "COMP_VOLUME_ID", "COMP_VOLUME", 0.0f, 10.0f, 5.0f));
  parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
      "COMP_ATTACK_ID", "COMP_ATTACK", 5.0f, 50.0f, 26.38f));
  parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
      "COMP_SUSTAIN_ID", "COMP_SUSTAIN", 50.0f, 500.0f, 263.75f));
  parameters.push_back(std::make_unique<juce::AudioParameterBool>(
      "COMP_ENABLED_ID", "COMP_ENABLED", false));

  // Clean Boost parameters
  parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
      "BOOST_VOLUME_ID", "BOOST_VOLUME", 0.0f, 10.0f, 5.0f));
  parameters.push_back(std::make_unique<juce::AudioParameterBool>(
      "BOOST_ENABLED_ID", "BOOST_ENABLED", false));

  // Chorus parameters (Boss CE-1 authentic ranges)
  parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
      "CHORUS_RATE_ID", "CHORUS_RATE", 0.3f, 3.0f, 1.6f));
  parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
      "CHORUS_DEPTH_ID", "CHORUS_DEPTH", 0.01f, 0.05f, 0.03f));
  parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
      "CHORUS_MIX_ID", "CHORUS_MIX", 0.0f, 100.0f, 50.0f));
  parameters.push_back(std::make_unique<juce::AudioParameterBool>(
      "CHORUS_ENABLED_ID", "CHORUS_ENABLED", false));

  auto normRange = NormalisableRange<float>(0.0, 20.0, 0.1f);
  parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
      "DOUBLER_SPREAD_ID", "DOUBLER_SPREAD", normRange, 0.0));
  parameters.push_back(std::make_unique<juce::AudioParameterBool>(
      "SMALL_WINDOW_ID", "SMALL_WINDOW", false, "SMALL_WINDOW"));

  // Reverb parameters
  parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
      "REVERB_MIX_ID", "REVERB_MIX", 0.0f, 10.0f, 5.0f));
  parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
      "REVERB_TONE_ID", "REVERB_TONE", 0.0f, 10.0f, 5.0f));
  parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
      "REVERB_SIZE_ID", "REVERB_SIZE", 0.0f, 10.0f, 5.0f));
  parameters.push_back(std::make_unique<juce::AudioParameterBool>(
      "REVERB_ENABLED_ID", "REVERB_ENABLED", false));

  // Delay parameters
  parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
      "DELAY_TIME_ID", "DELAY_TIME", 1.0f, 997.0f, 499.0f));
  parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
      "DELAY_FEEDBACK_ID", "DELAY_FEEDBACK", 0.0f, 1.0f, 0.5f));
  parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
      "DELAY_MIX_ID", "DELAY_MIX", 0.0f, 1.0f, 0.5f));
  parameters.push_back(std::make_unique<juce::AudioParameterBool>(
      "DELAY_ENABLED_ID", "DELAY_ENABLED", false));

  return {parameters.begin(), parameters.end()};
}

bool NamJUCEAudioProcessor::supportsDoublePrecisionProcessing() const {
  return supportsDouble;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
  return new NamJUCEAudioProcessor();
}
