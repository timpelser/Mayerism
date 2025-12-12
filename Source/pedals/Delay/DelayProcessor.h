#pragma once
#include <JuceHeader.h>
#include <algorithm>
#include <cmath>

/**
 * Delay Pedal Processor
 * Stereo delay effect with Time, Feedback, and Mix controls
 * DSP algorithm adapted from FAUST-generated code
 * Features: variable delay time with smooth crossfading, feedback with filtering
 */
class DelayProcessor {
public:
  DelayProcessor() {}
  ~DelayProcessor() {}

  void prepare(const juce::dsp::ProcessSpec &spec) {
    sampleRate = spec.sampleRate;
    numChannels = spec.numChannels;

    // Initialize constants based on sample rate
    instanceConstants();

    // Clear internal state
    reset();
  }

  void reset() {
    // Clear filter state arrays
    for (int i = 0; i < 2; ++i) {
      fRec3[i] = 0.0;
      fRec4[i] = 0.0;
      fRec5[i] = 0.0;
      fRec6[i] = 0.0;
      fRec0[i] = 0.0;
    }

    // Clear delay buffer
    IOTA = 0;
    for (int i = 0; i < 262144; ++i) {
      fVec0[i] = 0.0;
    }

    // Clear recursive filter states
    for (int i = 0; i < 3; ++i) {
      fRec2[i] = 0.0;
      fRec1[i] = 0.0;
    }
  }

  /**
   * Set the delay time in milliseconds (1.0 to 1000.0)
   * 1 = very short slapback
   * 250 = typical quarter note at 120 BPM
   * 1000 = very long ambient delay
   */
  void setTime(float timeMs) { currentTime = juce::jlimit(1.0f, 1000.0f, timeMs); }

  /**
   * Set the feedback amount (0.0 to 1.0)
   * 0 = no repeats (single echo)
   * 0.5 = moderate repeats (balanced)
   * 0.95 = very long decay (nearly infinite)
   */
  void setFeedback(float fb) { currentFeedback = juce::jlimit(0.0f, 1.0f, fb); }

  /**
   * Set the mix/wet-dry balance (0.0 to 1.2)
   * 0 = 100% dry (no delay)
   * 0.5 = 50% wet / 50% dry (balanced)
   * 1.0 = 100% wet (full delay)
   * 1.2 = boosted wet (slightly over unity)
   */
  void setMix(float mix) { currentMix = juce::jlimit(0.0f, 1.2f, mix); }

  /**
   * Process audio buffer through delay
   */
  void process(juce::AudioBuffer<float> &buffer) {
    // For stereo processing, we'll process both channels through the same delay line
    // This creates a mono delay effect sent to both channels

    auto *channelL = buffer.getWritePointer(0);
    auto *channelR = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;
    int numSamples = buffer.getNumSamples();

    // Process left channel (and right channel will get same delay)
    processDelayLine(channelL, channelR, numSamples);
  }

  // Getters for current parameter values (for UI)
  float getCurrentTime() const { return currentTime; }
  float getCurrentFeedback() const { return currentFeedback; }
  float getCurrentMix() const { return currentMix; }

private:
  // Sample rate and channel info
  double sampleRate = 44100.0;
  int numChannels = 2;

  // User-adjustable parameters
  float currentTime = 250.0f;      // Default: 250ms delay
  float currentFeedback = 0.5f;    // Default: moderate feedback
  float currentMix = 0.5f;         // Default: 50% wet/dry balance

  // Hardcoded tone shaping parameters
  static constexpr float hardcodedWarmth = 0.5f;  // Amount of filtering in decay
  static constexpr float hardcodedHiLo = 0.5f;    // Balance of filtering (lo=0, hi=1)

  // DSP state - Filter and delay buffer
  double fConst0;  // Sample rate clamped
  double fConst1;  // Pi / sampleRate
  double fConst2;  // 0.001 * sampleRate (ms to samples conversion)
  double fConst3;  // 10.0 / sampleRate
  double fConst4;  // -(10.0 / sampleRate)

  // Recursive filter states
  double fRec3[2];   // Delay time ramping state
  double fRec4[2];   // Delay time ramping progress
  double fRec5[2];   // Target delay time (new)
  double fRec6[2];   // Current delay time (old)
  int IOTA;          // Circular buffer index
  double fVec0[262144];  // 262144 samples = ~6 seconds at 44.1kHz
  double fRec2[3];   // Highpass filter state
  double fRec1[3];   // Lowpass filter state
  double fRec0[2];   // Output filter state

  /**
   * Initialize constants that depend on sample rate
   */
  void instanceConstants() {
    fConst0 = std::min<double>(192000.0, std::max<double>(1.0, sampleRate));
    fConst1 = (3.1415926535897931 / fConst0);
    fConst2 = (0.001 * fConst0);
    fConst3 = (10.0 / fConst0);
    fConst4 = (0.0 - fConst3);
  }

  /**
   * Helper function for power calculations
   */
  static double mydsp_faustpower2_f(double value) { return (value * value); }

  /**
   * Process the audio through the delay line
   * Adapted from the original Delay.cpp compute function
   */
  void processDelayLine(float *inputL, float *inputR, int count) {
    // Calculate filter frequencies based on hardcoded tone parameters
    double hipass =
        20.0 + (500.0 * mydsp_faustpower2_f(hardcodedWarmth) * hardcodedHiLo);
    double lowpass = 10500.0 - (10000.0 * mydsp_faustpower2_f(hardcodedWarmth) *
                                 (1.0 - hardcodedHiLo));

    // Calculate filter coefficients
    double fSlow0 = std::tan((fConst1 * lowpass));
    double fSlow1 = (1.0 / fSlow0);
    double fSlow2 = (1.0 / (((fSlow1 + 1.4142135623730949) / fSlow0) + 1.0));
    double fSlow3 = std::tan((fConst1 * hipass));
    double fSlow4 = (1.0 / fSlow3);
    double fSlow5 = (1.0 / (((fSlow4 + 1.4142135623730949) / fSlow3) + 1.0));
    double fSlow6 = mydsp_faustpower2_f(fSlow3);
    double fSlow7 = (1.0 / fSlow6);
    double fSlow8 = (fConst2 * currentTime);
    double fSlow9 = (((fSlow4 + -1.4142135623730949) / fSlow3) + 1.0);
    double fSlow10 = (2.0 * (1.0 - fSlow7));
    double fSlow11 = (0.0 - (2.0 / fSlow6));
    double fSlow12 =
        (2.0 * (1.0 - (1.0 / mydsp_faustpower2_f(fSlow0))));
    double fSlow13 = (((fSlow1 + -1.4142135623730949) / fSlow0) + 1.0);

    // Process each sample
    for (int i = 0; i < count; ++i) {
      double input = static_cast<double>(inputL[i]);

      // Delay time ramping logic (smooth crossfade between old and new delay times)
      double fTemp1 = ((fRec3[1] != 0.0)
                           ? (((fRec4[1] > 0.0) & (fRec4[1] < 1.0)) ? fRec3[1]
                              : 0.0)
                           : (((fRec4[1] == 0.0) & (fSlow8 != fRec5[1]))
                                  ? fConst3
                                  : (((fRec4[1] == 1.0) & (fSlow8 != fRec6[1]))
                                         ? fConst4
                                         : 0.0)));
      fRec3[0] = fTemp1;
      fRec4[0] =
          std::max<double>(0.0, std::min<double>(1.0, (fRec4[1] + fTemp1)));
      fRec5[0] = (((fRec4[1] >= 1.0) & (fRec6[1] != fSlow8)) ? fSlow8
                  : fRec5[1]);
      fRec6[0] = (((fRec4[1] <= 0.0) & (fRec5[1] != fSlow8)) ? fSlow8
                  : fRec6[1]);

      // Delay line with feedback
      double fTemp2 = (input + (currentFeedback * fRec0[1]));
      fVec0[(IOTA & 262143)] = fTemp2;

      // Read from delay buffer with interpolation between old and new delay times
      fRec2[0] =
          ((1.0 *
            ((fRec4[0] *
              fVec0[((IOTA - int(std::min<double>(
                           192000.0, std::max<double>(0.0, fRec6[0]))))
                     & 262143)]) +
             ((1.0 - fRec4[0]) *
              fVec0[((IOTA - int(std::min<double>(
                           192000.0, std::max<double>(0.0, fRec5[0]))))
                     & 262143)]))) -
           (fSlow5 * ((fSlow9 * fRec2[2]) + (fSlow10 * fRec2[1]))));

      // Highpass filter
      fRec1[0] =
          ((fSlow5 * (((fSlow7 * fRec2[0]) + (fSlow11 * fRec2[1])) +
                      (fSlow7 * fRec2[2]))) -
           (fSlow2 * ((fSlow12 * fRec1[1]) + (fSlow13 * fRec1[2]))));

      // Lowpass filter
      fRec0[0] = (fSlow2 * (fRec1[2] + (fRec1[0] + (2.0 * fRec1[1]))));

      // Output: dry signal + filtered delay signal
      double output = input + (currentMix * fRec0[0]);
      inputL[i] = static_cast<float>(output);

      // Copy to right channel if stereo
      if (inputR != nullptr) {
        inputR[i] = static_cast<float>(output);
      }

      // Update circular indices
      fRec3[1] = fRec3[0];
      fRec4[1] = fRec4[0];
      fRec5[1] = fRec5[0];
      fRec6[1] = fRec6[0];
      IOTA = (IOTA + 1);
      fRec2[2] = fRec2[1];
      fRec2[1] = fRec2[0];
      fRec1[2] = fRec1[1];
      fRec1[1] = fRec1[0];
      fRec0[1] = fRec0[0];
    }
  }
};
