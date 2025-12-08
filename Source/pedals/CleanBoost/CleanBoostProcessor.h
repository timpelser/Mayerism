#pragma once
#include <JuceHeader.h>
#include <juce_dsp/juce_dsp.h>

/**
 * Clean Boost Processor
 * Transparent boost pedal with tone shaping and soft clipping capabilities.
 *
 * Algorithm:
 * 1. High-pass filter (30Hz, 2nd order)
 * 2. Gain stage (0dB to +20dB)
 * 3. Soft clipping (tanh)
 * 4. Presence boost (2kHz, +2dB, Q=1.5)
 * 5. High-frequency roll-off (10kHz, 1st order)
 * 6. Output limiter (+-0.95)
 */
class CleanBoostProcessor {
public:
  CleanBoostProcessor() {}
  ~CleanBoostProcessor() {}

  void prepare(const juce::dsp::ProcessSpec &spec) {
    sampleRate = spec.sampleRate;

    // 1. High-pass filter coefficients (2nd order, 30 Hz)
    // Removes DC offset and sub-bass rumble
    auto hpfCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(
        sampleRate, 30.0f, 0.707f); // 0.707Q = Butterworth

    // 4. Presence boost coefficients (Peak, 2 kHz, +2 dB, Q=1.5)
    // Adds "sparkle" and clarity
    auto presenceCoeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        sampleRate, 2000.0f, 1.5f, juce::Decibels::decibelsToGain(2.0f));

    // 5. High-frequency roll-off coefficients (Low-pass, 1st order, 10 kHz)
    // Tames digital harshness
    auto lpfCoeffs = juce::dsp::IIR::Coefficients<float>::makeFirstOrderLowPass(
        sampleRate, 10000.0f);

    for (int ch = 0; ch < 2; ++ch) {
      // Initialize filters
      highPassFilter[ch].prepare(spec);
      highPassFilter[ch].coefficients = hpfCoeffs;

      presenceFilter[ch].prepare(spec);
      presenceFilter[ch].coefficients = presenceCoeffs;

      lowPassFilter[ch].prepare(spec);
      lowPassFilter[ch].coefficients = lpfCoeffs;

      // Reset state
      highPassFilter[ch].reset();
      presenceFilter[ch].reset();
      lowPassFilter[ch].reset();
    }
  }

  void reset() {
    for (int ch = 0; ch < 2; ++ch) {
      highPassFilter[ch].reset();
      presenceFilter[ch].reset();
      lowPassFilter[ch].reset();
    }
  }

  /**
   * Set the boost amount (0.0 to 10.0)
   * Maps to 0 dB to +8 dB of gain
   */
  void setBoost(float knobValue) {
    // Map 0-10 knob to 0-8 dB
    float dbGain = (knobValue / 10.0f) * 8.0f;
    currentGain = juce::Decibels::decibelsToGain(dbGain);
  }

  void process(juce::AudioBuffer<float> &buffer) {
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
      auto *channelData = buffer.getWritePointer(ch);
      // Use channel 0 state for mono, otherwise match channel index (up to 2)
      int idx = (ch < 2) ? ch : 0;

      for (int i = 0; i < buffer.getNumSamples(); ++i) {
        float sample = channelData[i];

        // 1. Pre-Filtering: High-pass (30 Hz)
        sample = highPassFilter[idx].processSample(sample);

        // 2. Gain Stage
        sample *= currentGain;

        // 3. Soft Clipping (Waveshaper)
        // tanh provides tube-like saturation curve
        sample = std::tanh(sample);

        // 4. Post-Filtering: Presence Boost (2 kHz)
        sample = presenceFilter[idx].processSample(sample);

        // 5. Post-Filtering: HF Roll-off (10 kHz)
        sample = lowPassFilter[idx].processSample(sample);

        // 6. Safety Limiter
        // Hard clip at +/- 0.95 to prevent digital overs
        sample = juce::jlimit(-0.95f, 0.95f, sample);

        channelData[i] = sample;
      }
    }
  }

private:
  double sampleRate = 44100.0;
  float currentGain = 1.0f;

  // Stereo filters
  juce::dsp::IIR::Filter<float> highPassFilter[2];
  juce::dsp::IIR::Filter<float> presenceFilter[2];
  juce::dsp::IIR::Filter<float> lowPassFilter[2];
};
