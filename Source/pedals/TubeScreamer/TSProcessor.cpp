#include "TSProcessor.h"
#include "dsp/ClippingStage.h"
#include "dsp/ToneStage.h"

TSProcessor::TSProcessor()
    : oversampling(2, 1,
                   juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR) {
  // Initialize DSP instances
  for (int ch = 0; ch < 2; ++ch) {
    clippingStage[ch] = std::make_unique<ClippingStage>();
    toneStage[ch] = std::make_unique<ToneStage>();
  }
}

TSProcessor::~TSProcessor() = default;

void TSProcessor::prepare(const juce::dsp::ProcessSpec &spec) {
  sampleRate = spec.sampleRate;
  maxBlockSize = spec.maximumBlockSize;
  numChannels = spec.numChannels;

  // Initialize oversampling (2x oversampling, order 1)
  oversampling.initProcessing(maxBlockSize);

  // Oversample factor for clipping stage
  const float oversampledRate =
      (float)sampleRate * std::pow(2.0f, 1.0f); // 2^1 = 2x

  // Prepare clipping and tone stages for each channel
  for (int ch = 0; ch < 2; ++ch) {
    clippingStage[ch]->prepare(oversampledRate);
    clippingStage[ch]->setDrive(currentDrive);

    toneStage[ch]->prepare((float)sampleRate);
    toneStage[ch]->setTone(currentTone);
  }
}

void TSProcessor::reset() {
  oversampling.reset();

  for (int ch = 0; ch < 2; ++ch) {
    clippingStage[ch]->reset();
    toneStage[ch]->reset();
  }
}

void TSProcessor::setDrive(float drive) {
  currentDrive = juce::jlimit(0.0f, 10.0f, drive);
}

void TSProcessor::setTone(float tone) {
  currentTone = juce::jlimit(0.0f, 10.0f, tone);
}

void TSProcessor::setLevel(float level) {
  currentLevel = juce::jlimit(0.0f, 10.0f, level);
}

void TSProcessor::process(juce::AudioBuffer<float> &buffer) {
  const auto numSamples = buffer.getNumSamples();
  juce::dsp::AudioBlock<float> block(buffer);

  // ========== CLIPPING STAGE (with 2x oversampling) ==========
  auto osBlock = oversampling.processSamplesUp(block);

  for (int ch = 0; ch < osBlock.getNumChannels(); ++ch) {
    clippingStage[ch]->setDrive(currentDrive);
    auto *x = osBlock.getChannelPointer(ch);

    for (int n = 0; n < osBlock.getNumSamples(); ++n) {
      x[n] = clippingStage[ch]->processSample(x[n]);
    }
  }

  oversampling.processSamplesDown(block);

  // ========== TONE STAGE ==========
  for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
    auto *x = buffer.getWritePointer(ch);

    toneStage[ch]->setTone(currentTone);
    toneStage[ch]->processBlock(x, numSamples);
  }

  // ========== LEVEL (Output Volume) ==========
  // Simple linear gain, just like the real TS-808 Level pot
  const float levelGain = currentLevel / 10.0f;
  buffer.applyGain(levelGain);
}

float TSProcessor::getCurrentDrive() const { return currentDrive; }
float TSProcessor::getCurrentTone() const { return currentTone; }
float TSProcessor::getCurrentLevel() const { return currentLevel; }
