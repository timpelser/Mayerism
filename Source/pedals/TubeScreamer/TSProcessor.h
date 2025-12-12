#pragma once
#include <JuceHeader.h>
#include <memory>

// Forward declarations to hide implementation details
class ClippingStage;
class ToneStage;

/**
 * TubeScreamer TS808 Processor
 * Handles the drive/clipping stage and tone control
 * Based on circuit-accurate WDF (Wave Digital Filter) implementation
 */
class TSProcessor {
public:
  TSProcessor();
  ~TSProcessor();

  void prepare(const juce::dsp::ProcessSpec &spec);
  void reset();

  /**
   * Set the drive amount (0.0 to 10.0)
   * Controls the gain into the clipping stage
   */
  void setDrive(float drive);

  /**
   * Set the tone control (0.0 to 10.0)
   * 0 = dark/bass, 10 = bright/treble
   */
  void setTone(float tone);

  /**
   * Set the level/output volume (0.0 to 10.0)
   * Controls the output volume after tone shaping
   */
  void setLevel(float level);

  /**
   * Process audio buffer through TS808 clipping + tone stages
   */
  void process(juce::AudioBuffer<float> &buffer);

  float getCurrentDrive() const;
  float getCurrentTone() const;
  float getCurrentLevel() const;

private:
  // Audio processing specs
  double sampleRate = 44100.0;
  int maxBlockSize = 512;
  int numChannels = 2;

  // Implementation details hidden behind unique_ptrs
  std::unique_ptr<ClippingStage> clippingStage[2];
  std::unique_ptr<ToneStage> toneStage[2];

  // Oversampling for clipping stage
  juce::dsp::Oversampling<float> oversampling;

  // Current parameter values
  float currentDrive = 2.0f;
  float currentTone = 5.0f;
  float currentLevel = 7.0f;
};
