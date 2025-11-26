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

  myNAM.prepare(spec);
  myNAM.hookParameters(apvts);

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
  meterInSource.measureBlock(buffer);
  juce::ScopedNoDenormals noDenormals;
  auto totalNumInputChannels = getTotalNumInputChannels();
  auto totalNumOutputChannels = getTotalNumOutputChannels();

  for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
    buffer.clear(i, 0, buffer.getNumSamples());

  juce::dsp::AudioBlock<float> block(buffer);

  auto *channelDataLeft = buffer.getWritePointer(0);
  auto *channelDataRight = buffer.getWritePointer(1);

  myNAM.processBlock(buffer);

  // Do Dual Mono
  for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    channelDataRight[sample] = channelDataLeft[sample];

  // Doubler
  if (*apvts.getRawParameterValue("DOUBLER_SPREAD_ID") > 0.0) {
    doubler.setDelayMs(*apvts.getRawParameterValue("DOUBLER_SPREAD_ID"));
    doubler.process(buffer);
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
