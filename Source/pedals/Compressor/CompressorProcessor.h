#pragma once
#include <JuceHeader.h>

/**
 * Compressor Pedal Processor
 * Simple guitar compressor with Volume, Attack, and Sustain controls
 * Uses JUCE's built-in dsp::Compressor with hardcoded threshold and ratio
 */
class CompressorProcessor {
public:
  CompressorProcessor() {}
  ~CompressorProcessor() {}

  void prepare(const juce::dsp::ProcessSpec &spec) {
    sampleRate = spec.sampleRate;
    numChannels = spec.numChannels;

    // Prepare compressors for each channel
    for (int ch = 0; ch < 2; ++ch) {
      compressor[ch].prepare(spec);
      compressor[ch].reset();

      // Set hardcoded parameters
      compressor[ch].setThreshold(hardcodedThreshold);
      compressor[ch].setRatio(hardcodedRatio);

      // Set initial user parameters
      compressor[ch].setAttack(currentAttack);
      compressor[ch].setRelease(currentRelease);
    }
  }

  void reset() {
    for (int ch = 0; ch < 2; ++ch) {
      compressor[ch].reset();
    }
  }

  /**
   * Set the attack time (5.0 to 50.0 ms)
   * Slower attack = preserves pick attack (Mayer style)
   */
  void setAttack(float attackMs) {
    currentAttack = juce::jlimit(5.0f, 50.0f, attackMs);
  }

  /**
   * Set the sustain/release time (50 to 500 ms)
   * Faster release = more transparency, less pumping
   */
  void setSustain(float sustainMs) {
    currentRelease = juce::jlimit(50.0f, 500.0f, sustainMs);
  }

  /**
   * Set the output volume/makeup gain (0.0 to 10.0)
   * Compensates for gain reduction from compression
   */
  void setVolume(float volume) {
    currentVolume = juce::jlimit(0.0f, 10.0f, volume);
  }

  /**
   * Process audio buffer through compressor + makeup gain
   */
  void process(juce::AudioBuffer<float> &buffer) {
    // ========== COMPRESSION STAGE ==========
    for (int ch = 0; ch < buffer.getNumChannels() && ch < 2; ++ch) {
      // Update dynamic parameters
      compressor[ch].setAttack(currentAttack);
      compressor[ch].setRelease(currentRelease);

      // Create a mono audio block for this channel
      juce::dsp::AudioBlock<float> block(buffer);
      auto monoBlock = block.getSingleChannelBlock(ch);

      // Process the channel through the compressor
      juce::dsp::ProcessContextReplacing<float> context(monoBlock);
      compressor[ch].process(context);
    }

    // ========== MAKEUP GAIN / VOLUME STAGE ==========
    // Convert 0-10 range to linear gain
    // 0 = silence, 5 = unity (~0dB), 10 = +6dB
    const float volumeGain = currentVolume / 5.0f; // 0-2x gain range
    buffer.applyGain(volumeGain);
  }

  // Getters for current parameter values (for UI)
  float getCurrentAttack() const { return currentAttack; }
  float getCurrentRelease() const { return currentRelease; }
  float getCurrentVolume() const { return currentVolume; }

private:
  // Audio processing specs
  double sampleRate = 44100.0;
  int numChannels = 2;

  // Compressor instances (stereo)
  juce::dsp::Compressor<float> compressor[2];

  // Hardcoded parameters (not exposed to user)
  // Optimized for John Mayer / SRV style per research
  // Threshold lowered to -25dB to ensure it engages with standard pick strength
  static constexpr float hardcodedThreshold = -25.0f;
  static constexpr float hardcodedRatio =
      3.0f; // 3:1 ratio (smooth, transparent)

  // User-adjustable parameters
  // Defaults optimized for vintage blues/rock guitar tone
  float currentAttack = 26.38f;   // Default: 15 ms (lets pick attack through)
  float currentRelease = 263.75f; // Default: 200 ms (musical sustain)
  float currentVolume = 5.0f;     // Default: unity gain (5/5)
};
