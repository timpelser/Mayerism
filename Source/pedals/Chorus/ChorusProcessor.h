#pragma once
#include <JuceHeader.h>

/**
 * Chorus Pedal Processor
 * Stereo chorus effect with Rate, Depth, and Mix controls
 * Uses JUCE's built-in juce::dsp::Chorus
 */
class ChorusProcessor {
public:
  ChorusProcessor() {
    // Explicit initialization of chorus object
    jassert(sampleRate > 0);
    initialized = false;
  }
  ~ChorusProcessor() {}

  void prepare(const juce::dsp::ProcessSpec &spec) {
    jassert(spec.sampleRate > 0);
    jassert(spec.numChannels > 0);

    sampleRate = spec.sampleRate;
    numChannels = spec.numChannels;

    // Initialize JUCE Chorus with ProcessSpec
    chorus.prepare(spec);

    // Set initial parameters immediately after prepare
    updateChorusParameters();

    // Mark as initialized
    initialized = true;
  }

  void reset() {
    if (initialized) {
      chorus.reset();
    }
  }

  /**
   * Set the chorus rate in Hz (0.5 to 2.5)
   * 0.5 = slow, subtle modulation
   * 1.5 = typical vintage chorus rate
   * 2.5 = fast, pronounced modulation
   */
  void setRate(float rateHz) { currentRate = juce::jlimit(0.5f, 2.5f, rateHz); }

  /**
   * Set the chorus depth (0.0 to 0.2)
   * 0.05 = subtle effect
   * 0.1 = moderate effect
   * 0.2 = pronounced detuning
   */
  void setDepth(float depth) { currentDepth = juce::jlimit(0.0f, 0.2f, depth); }

  /**
   * Set the mix/wet-dry balance (0.0 to 100.0)
   * 0 = 100% dry (no chorus)
   * 50 = 50% wet / 50% dry (balanced)
   * 100 = 100% wet (full chorus)
   */
  void setMix(float mix) { currentMix = juce::jlimit(0.0f, 100.0f, mix); }

  /**
   * Process audio buffer through chorus
   */
  void process(juce::AudioBuffer<float> &buffer) {
    // Only process if properly initialized
    if (!initialized) {
      return;
    }

    // Rebuild parameters every block from current setter values
    updateChorusParameters();

    // Create AudioBlock from buffer
    juce::dsp::AudioBlock<float> block(buffer);

    // Use a subset block to ensure channel count matches what we prepared for
    // (e.g. 2 channels) This prevents mismatches if the host buffer has extra
    // channels
    if ((size_t)numChannels <= block.getNumChannels()) {
      auto processBlock = block.getSubsetChannelBlock(0, (size_t)numChannels);
      juce::dsp::ProcessContextReplacing<float> context(processBlock);
      chorus.process(context);
    }
  }

  // Getters for current parameter values (for UI)
  float getCurrentRate() const { return currentRate; }
  float getCurrentDepth() const { return currentDepth; }
  float getCurrentMix() const { return currentMix; }

private:
  // JUCE Chorus instance
  juce::dsp::Chorus<float> chorus;

  // Audio processing specs
  double sampleRate = 44100.0;
  int numChannels = 2;
  bool initialized = false;

  // User-adjustable parameters
  float currentRate =
      0.8f; // Default: 0.8 Hz rate (classic Boss CE-1 slow sweep)
  float currentDepth = 0.03f; // Default: subtle depth (vintage analog chorus)
  float currentMix = 50.0f;   // Default: 50% wet/dry (balanced chorus effect)

  // Hardcoded parameters (not exposed to user)
  // Optimized for vintage 80s chorus (Boss CE-1, EHX Small Clone)
  static constexpr float hardcodedCentreDelay =
      7.0f; // ms (JUCE docs recommend 7-8ms for classic chorus)
  static constexpr float hardcodedFeedback =
      0.5f; // Moderate feedback for richness

  /**
   * Helper function to update chorus parameters from current user values
   */
  void updateChorusParameters() {
    if (!initialized) {
      return;
    }

    // JUCE Chorus uses: rate (Hz), depth (0-1), centerDelayMs, feedback (-1 to
    // 1)
    chorus.setRate(currentRate);
    chorus.setDepth(currentDepth);
    chorus.setCentreDelay(hardcodedCentreDelay);
    chorus.setFeedback(hardcodedFeedback);

    // Handle wet/dry mix separately
    // Normalize mix from 0-100 to 0-1
    float normalizedMix = juce::jlimit(0.0f, 1.0f, currentMix / 100.0f);
    chorus.setMix(normalizedMix);
  }
};
