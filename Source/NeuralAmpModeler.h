#ifndef __NEURAL_AMP_MODELER_H__
#define __NEURAL_AMP_MODELER_H__

// #define NAM_SAMPLE_FLOAT
// #define DSP_SAMPLE_FLOAT

#include "ResamplingNAM.h"
#include "StatusedTrigger.h"
#include "ToneStack.h"
#include "architecture.hpp"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

class NeuralAmpModeler {
public:
  NeuralAmpModeler();
  ~NeuralAmpModeler();

  void prepare(juce::dsp::ProcessSpec &spec);
  void processBlock(juce::AudioBuffer<float> &buffer);

  // Returns true if model staged successfully
  bool loadModel(const std::string modelPath);
  bool loadModelFromMemory(const void *data, const int size);

  bool isModelLoaded();
  void clearModel();

  void createParameters(
      std::vector<std::unique_ptr<juce::RangedAudioParameter>> &parameters);
  void hookParameters(juce::AudioProcessorValueTreeState &);

  enum Parameters {
    kInputLevel = 0,
    kNoiseGateThreshold,
    kToneBass,
    kToneMid,
    kToneTreble,
    kOutputLevel,
  };

  StatusedTrigger *getTrigger() { return &mNoiseGateTrigger; };

private:
  double sampleRate;
  int samplesPerBlock;
  juce::AudioBuffer<float> outputBuffer;

  // Parameter Pointers
  std::atomic<float> *params[6];
  bool noiseGateActive{false};

  bool modelLoaded{false};
  bool shouldRemoveModel{false};

  std::unique_ptr<ResamplingNAM> mModel, mStagedModel;
  std::unique_ptr<dsp::tone_stack::AbstractToneStack> mToneStack;

  // Noise gate
  StatusedTrigger mNoiseGateTrigger;
  dsp::noise_gate::Gain mNoiseGateGain;

  // Noise gate Params
  const double ns_time = 0.01;
  const double ns_ratio = 0.1; // Quadratic...
  const double ns_openTime = 0.005;
  const double ns_holdTime = 0.01;
  const double ns_closeTime = 0.05;

private:
  // Moves DSP modules from staging area to the main area.
  // Also deletes DSP modules that are flagged for removal.
  // Exists so that we don't try to use a DSP module that's only
  // partially-instantiated.
  void applyDSPStaging();

  void resetModel();

  void updateParameters();
  double dB_to_linear(double db_value);
  void doDualMono(juce::AudioBuffer<float> &mainBuffer, float **input);
};

#endif