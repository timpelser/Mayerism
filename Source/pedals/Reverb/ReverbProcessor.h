#pragma once
#include <JuceHeader.h>

/**
 * Reverb Pedal Processor
 * Stereo reverb effect with Mix, Tone, and Size controls
 * Uses JUCE's built-in juce::Reverb with hardcoded width and freeze mode
 */
class ReverbProcessor {
public:
  ReverbProcessor() {}
  ~ReverbProcessor() {}

  void prepare(const juce::dsp::ProcessSpec &spec) {
    sampleRate = spec.sampleRate;
    numChannels = spec.numChannels;

    // Initialize JUCE Reverb
    reverb.setSampleRate(sampleRate);

    // Set initial parameters
    updateReverbParameters();
  }

  void reset() { reverb.reset(); }

  /**
   * Set the mix/wet-dry balance (0.0 to 10.0)
   * 0 = 100% dry (no reverb)
   * 5 = 50% wet / 50% dry (balanced)
   * 10 = 100% wet (full reverb)
   */
  void setMix(float mix) {
    currentMix = juce::jlimit(0.0f, 10.0f, mix);
  }

  /**
   * Set the tone/damping (0.0 to 10.0)
   * 0 = no damping (bright, ringing reverb)
   * 5 = medium damping (balanced)
   * 10 = full damping (dark, warm reverb)
   */
  void setTone(float tone) {
    currentTone = juce::jlimit(0.0f, 10.0f, tone);
  }

  /**
   * Set the size/room size (0.0 to 10.0)
   * 0 = tiny room (short decay)
   * 5 = medium room (balanced decay)
   * 10 = large hall (long decay)
   */
  void setSize(float size) {
    currentSize = juce::jlimit(0.0f, 10.0f, size);
  }

  /**
   * Process audio buffer through reverb
   */
  void process(juce::AudioBuffer<float> &buffer) {
    // Rebuild parameters every block from current setter values
    updateReverbParameters();

    // Process stereo (plugin architecture guarantees 2 channels)
    reverb.processStereo(buffer.getWritePointer(0), buffer.getWritePointer(1),
                         buffer.getNumSamples());
  }

  // Getters for current parameter values (for UI)
  float getCurrentMix() const { return currentMix; }
  float getCurrentTone() const { return currentTone; }
  float getCurrentSize() const { return currentSize; }

private:
  // JUCE Reverb instance
  juce::Reverb reverb;
  juce::Reverb::Parameters currentParams;

  // Audio processing specs
  double sampleRate = 44100.0;
  int numChannels = 2;

  // User-adjustable parameters
  // Defaults optimized for subtle, natural reverb
  float currentMix = 5.0f;   // Default: 50% wet/dry balance
  float currentTone = 5.0f;  // Default: medium damping (0.5)
  float currentSize = 5.0f;  // Default: medium room (0.5 size)

  // Hardcoded parameters (not exposed to user)
  // Optimized for natural guitar reverb
  static constexpr float hardcodedWidth = 1.0f;       // Full stereo width
  static constexpr float hardcodedFreezeMode = 0.0f;  // Normal mode

  /**
   * Helper function to update reverb parameters from current user values
   */
  void updateReverbParameters() {
    // Mix: Normalize 0-10 to 0-1 for wet/dry balance
    // Linear crossfade: at 5.0, both wet and dry are 0.5 (balanced)
    float normalizedMix = currentMix / 10.0f;
    currentParams.wetLevel = normalizedMix;
    currentParams.dryLevel = (1.0f - normalizedMix);

    // Tone: Normalize 0-10 to 0-1 for damping
    // Direct 1:1 mapping
    currentParams.damping = currentTone / 10.0f;

    // Size: Normalize 0-10 to 0-1 for room size
    // Direct 1:1 mapping
    currentParams.roomSize = currentSize / 10.0f;

    // Hardcoded parameters
    currentParams.width = hardcodedWidth;
    currentParams.freezeMode = hardcodedFreezeMode;

    // Apply parameters to reverb
    reverb.setParameters(currentParams);
  }
};
