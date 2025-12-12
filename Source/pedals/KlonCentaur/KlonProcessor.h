#pragma once
#include <JuceHeader.h>
#include <memory>

// Forward declarations to hide implementation details
class GainStageProc;
class InputBufferProcessor;
class ToneFilterProcessor;
class OutputStageProc;

/**
 * Klon Centaur Processor
 * Wrapper for the Klon Centaur circuit model
 */
class KlonProcessor {
public:
  KlonProcessor();
  ~KlonProcessor();

  void prepare(const juce::dsp::ProcessSpec &spec);
  void reset();

  /**
   * Set the gain control (0.0 to 1.0)
   * Maps to the Klon's gain pot
   */
  void setGain(float gain);

  /**
   * Set the treble control (0.0 to 1.0)
   * Maps to the Klon's treble pot
   */
  void setTreble(float treble);

  /**
   * Set the level control (0.0 to 1.0)
   * Maps to the Klon's output level pot
   */
  void setLevel(float level);

  /**
   * Process audio buffer through Klon circuit
   */
  void process(juce::AudioBuffer<float> &buffer);

  // Getters for current parameter values (for UI)
  float getCurrentGain() const;
  float getCurrentTreble() const;
  float getCurrentLevel() const;

private:
  // Audio processing specs
  double sampleRate = 44100.0;
  int maxBlockSize = 512;
  int numChannels = 2;

  // Implementation details hidden behind unique_ptrs
  // This prevents header leakage of chowdsp types
  std::unique_ptr<InputBufferProcessor> inProc[2];
  std::unique_ptr<ToneFilterProcessor> tone[2];
  std::unique_ptr<OutputStageProc> outProc[2];
  std::unique_ptr<GainStageProc> gainStageProc;

  // Current parameter values
  float currentGain = 0.5f;
  float currentTreble = 0.5f;
  float currentLevel = 0.5f;
};
