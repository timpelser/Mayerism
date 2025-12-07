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

  tsProcessor.prepare(spec);

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

  // Hook Compressor parameters
  compVolume = apvts.getRawParameterValue("COMP_VOLUME_ID");
  compAttack = apvts.getRawParameterValue("COMP_ATTACK_ID");
  compSustain = apvts.getRawParameterValue("COMP_SUSTAIN_ID");
  compEnabled = apvts.getRawParameterValue("COMP_ENABLED_ID");

  doubler.prepare(spec);

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

  // Apply independent input gain BEFORE input metering
  buffer.applyGain(std::powf(10.0f, pluginInputGain->load() / 20.0f));

  meterInSource.measureBlock(buffer);

  // Compressor (at beginning of chain, before TS and amp)
  if (compEnabled->load() > 0.5f) {
    compressorProcessor.setVolume(compVolume->load());
    compressorProcessor.setAttack(compAttack->load());
    compressorProcessor.setSustain(compSustain->load());
    compressorProcessor.process(buffer);
  }

  // TubeScreamer TS808 (before amp) - only process if enabled
  if (tsEnabled->load() > 0.5f) {
    tsProcessor.setDrive(tsDrive->load());
    tsProcessor.setTone(tsTone->load());
    tsProcessor.setLevel(tsLevel->load());
    tsProcessor.process(buffer);
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

  // Apply independent output gain AFTER doubler, BEFORE output metering
  buffer.applyGain(std::powf(10.0f, pluginOutputGain->load() / 20.0f));

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

  // Compressor parameters
  parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
      "COMP_VOLUME_ID", "COMP_VOLUME", 0.0f, 10.0f, 5.0f));
  parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
      "COMP_ATTACK_ID", "COMP_ATTACK", 5.0f, 50.0f, 26.38f));
  parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
      "COMP_SUSTAIN_ID", "COMP_SUSTAIN", 50.0f, 500.0f, 263.75f));
  parameters.push_back(std::make_unique<juce::AudioParameterBool>(
      "COMP_ENABLED_ID", "COMP_ENABLED", false));

  auto normRange = NormalisableRange<float>(0.0, 20.0, 0.1f);
  parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
      "DOUBLER_SPREAD_ID", "DOUBLER_SPREAD", normRange, 0.0));
  parameters.push_back(std::make_unique<juce::AudioParameterBool>(
      "SMALL_WINDOW_ID", "SMALL_WINDOW", false, "SMALL_WINDOW"));

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
