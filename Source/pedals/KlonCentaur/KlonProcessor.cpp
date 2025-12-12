#include "KlonProcessor.h"
#include "dsp/GainStageProc.h"
#include "dsp/InputBufferProcessor.h"
#include "dsp/OutputStageProcessor.h"
#include "dsp/ToneFilterProcessor.h"
#include "klon_pch.h"

KlonProcessor::KlonProcessor() {
  // Initialize stereo processors
  for (int ch = 0; ch < 2; ++ch) {
    inProc[ch] = std::make_unique<InputBufferProcessor>();
    tone[ch] = std::make_unique<ToneFilterProcessor>();
    outProc[ch] = std::make_unique<OutputStageProc>();
  }

  // Initialize gain stage (does stereo internally)
  gainStageProc = std::make_unique<GainStageProc>(44100.0);
}

KlonProcessor::~KlonProcessor() = default;

void KlonProcessor::prepare(const juce::dsp::ProcessSpec &spec) {
  sampleRate = spec.sampleRate;
  maxBlockSize = spec.maximumBlockSize;
  numChannels = spec.numChannels;

  // Reset gain stage
  gainStageProc->reset(sampleRate, maxBlockSize);
  gainStageProc->setGain(currentGain);

  // Prepare stereo processors
  for (int ch = 0; ch < 2; ++ch) {
    inProc[ch]->prepare((float)sampleRate);
    tone[ch]->prepare((float)sampleRate);
    outProc[ch]->prepare((float)sampleRate);
  }
}

void KlonProcessor::reset() {
  if (gainStageProc) {
    gainStageProc->reset(sampleRate, maxBlockSize);
  }
  for (int ch = 0; ch < 2; ++ch) {
    inProc[ch]->prepare((float)sampleRate);
    tone[ch]->prepare((float)sampleRate);
    outProc[ch]->prepare((float)sampleRate);
  }
}

void KlonProcessor::setGain(float gain) {
  currentGain = juce::jlimit(0.0f, 1.0f, gain);
  if (gainStageProc) {
    gainStageProc->setGain(currentGain);
  }
}

void KlonProcessor::setTreble(float treble) {
  currentTreble = juce::jlimit(0.0f, 1.0f, treble);
}

void KlonProcessor::setLevel(float level) {
  currentLevel = juce::jlimit(0.0f, 1.0f, level);
}

void KlonProcessor::process(juce::AudioBuffer<float> &buffer) {
  const auto numSamples = buffer.getNumSamples();

  // Process each channel
  for (int ch = 0; ch < buffer.getNumChannels() && ch < 2; ++ch) {
    auto *x = buffer.getWritePointer(ch);

    // ========== INPUT BUFFER STAGE ==========
    juce::FloatVectorOperations::multiply(x, 0.5f, numSamples);
    inProc[ch]->processBlock(x, numSamples);
    juce::FloatVectorOperations::clip(x, x, -4.5f, 4.5f,
                                      numSamples); // op amp clipping
  }

  // ========== GAIN STAGE (WDF-based) ==========
  if (gainStageProc) {
    gainStageProc->processBlock(buffer);
  }

  // ========== TONE STAGE ==========
  for (int ch = 0; ch < buffer.getNumChannels() && ch < 2; ++ch) {
    auto *x = buffer.getWritePointer(ch);

    tone[ch]->setTreble(currentTreble);
    tone[ch]->processBlock(x, numSamples);

    // Inverting amplifier + op-amp clipping (from original circuit)
    juce::FloatVectorOperations::multiply(x, -1.0f, numSamples);
    juce::FloatVectorOperations::clip(x, x, -13.1f, 11.7f, numSamples);

    // ========== OUTPUT STAGE ==========
    outProc[ch]->setLevel(currentLevel);
    outProc[ch]->processBlock(x, numSamples);
  }
}

float KlonProcessor::getCurrentGain() const { return currentGain; }
float KlonProcessor::getCurrentTreble() const { return currentTreble; }
float KlonProcessor::getCurrentLevel() const { return currentLevel; }
