#ifndef __RESAMPLING_NAM_H__
#define __RESAMPLING_NAM_H__

#define DEFAULT_BLOCK_SIZE 256

namespace iplug { static const double PI = 3.1415926535897931; };

#include <algorithm> // std::clamp, std::min
#include <cmath> // pow
#include <filesystem>
#include <iostream>
#include <utility>
#include "../Modules/NeuralAmpModelerCore/NAM/dsp.h"
#include "../Modules/AudioDSPTools/dsp/NoiseGate.h"
#include "../Modules/AudioDSPTools/dsp/dsp.h"
#include "../Modules/AudioDSPTools/dsp/ResamplingContainer/ResamplingContainer.h"
#include "../Modules/AudioDSPTools/dsp/ImpulseResponse.h"
#include "../Modules/AudioDSPTools/dsp/wav.h"

// Get the sample rate of a NAM model.
// Sometimes, the model doesn't know its own sample rate; this wrapper guesses 48k based on the way that most
// people have used NAM in the past.
static inline double GetNAMSampleRate(const std::unique_ptr<nam::DSP>& model)
{
  // Some models are from when we didn't have sample rate in the model.
  // For those, this wraps with the assumption that they're 48k models, which is probably true.
  const double assumedSampleRate = 48000.0;
  const double reportedEncapsulatedSampleRate = model->GetExpectedSampleRate();
  const double encapsulatedSampleRate =
    reportedEncapsulatedSampleRate <= 0.0 ? assumedSampleRate : reportedEncapsulatedSampleRate;
  return encapsulatedSampleRate;
};

class ResamplingNAM : public nam::DSP
{
public:
  // Resampling wrapper around the NAM models
  ResamplingNAM(std::unique_ptr<nam::DSP> encapsulated, const double expected_sample_rate)
  : nam::DSP(expected_sample_rate)
  , mEncapsulated(std::move(encapsulated))
  , mResampler(GetNAMSampleRate(mEncapsulated))
  {
    // Assign the encapsulated object's processing function  to this object's member so that the resampler can use it:
    auto ProcessBlockFunc = [&](NAM_SAMPLE** input, NAM_SAMPLE** output, int numFrames) {
      mEncapsulated->process(input[0], output[0], numFrames);
      mEncapsulated->finalize_(numFrames);
    };
    mBlockProcessFunc = ProcessBlockFunc;

    // Get the other information from the encapsulated NAM so that we can tell the outside world about what we're
    // holding.
    if (mEncapsulated->HasLoudness())
      SetLoudness(mEncapsulated->GetLoudness());

    // NOTE: prewarm samples doesn't mean anything--we can prewarm the encapsulated model as it likes and be good to
    // go.
    // _prewarm_samples = 0;

    // And be ready
    int maxBlockSize = 2048; // Conservative
    Reset(expected_sample_rate, maxBlockSize);
  };

  ~ResamplingNAM() = default;

  void prewarm() override { mEncapsulated->prewarm(); };

  void process(NAM_SAMPLE* input, NAM_SAMPLE* output, const int num_frames) override
  {
    if (!mFinalized)
      throw std::runtime_error("Processing was called before the last block was finalized!");
    if (num_frames > mMaxExternalBlockSize)
      // We can afford to be careful
      throw std::runtime_error("More frames were provided than the max expected!");

    if (!NeedToResample())
    {
      mEncapsulated->process(input, output, num_frames);
      mEncapsulated->finalize_(num_frames);
    }
    else
    {
      mResampler.ProcessBlock(&input, &output, num_frames, mBlockProcessFunc);
    }

    // Prepare for external call to .finalize_()
    lastNumExternalFramesProcessed = num_frames;
    mFinalized = false;
  };

  void finalize_(const int num_frames) override
  {
    if (mFinalized)
      throw std::runtime_error("Call to ResamplingNAM.finalize_() when the object is already in a finalized state!");
    if (num_frames != lastNumExternalFramesProcessed)
      throw std::runtime_error(
        "finalize_() called on ResamplingNAM with a different number of frames from what was just processed. Something "
        "is probably going wrong.");

    // We don't actually do anything--it was taken care of during BlockProcessFunc()!

    // prepare for next call to `.process()`
    mFinalized = true;
  };

  int GetLatency() const { return NeedToResample() ? mResampler.GetLatency() : 0; };

  void Reset(const double sampleRate, const int maxBlockSize)
  {
    mExpectedSampleRate = sampleRate;
    mMaxExternalBlockSize = maxBlockSize;
    mResampler.Reset(sampleRate, maxBlockSize);

    // Allocations in the encapsulated model (HACK)
    // Stolen some code from the resampler; it'd be nice to have these exposed as methods? :)
    const double mUpRatio = sampleRate / GetEncapsulatedSampleRate();
    const auto maxEncapsulatedBlockSize = static_cast<int>(std::ceil(static_cast<double>(maxBlockSize) / mUpRatio));
    std::vector<NAM_SAMPLE> input, output;
    for (int i = 0; i < maxEncapsulatedBlockSize; i++)
      input.push_back((NAM_SAMPLE)0.0);
    output.resize(maxEncapsulatedBlockSize); // Doesn't matter what's in here
    mEncapsulated->process(input.data(), output.data(), maxEncapsulatedBlockSize);
    mEncapsulated->finalize_(maxEncapsulatedBlockSize);

    mFinalized = true; // prepare for `.process()`
  };

  // So that we can let the world know if we're resampling (useful for debugging)
  double GetEncapsulatedSampleRate() const { return GetNAMSampleRate(mEncapsulated); };

private:
  bool NeedToResample() const { return GetExpectedSampleRate() != GetEncapsulatedSampleRate(); };
  // The encapsulated NAM
  std::unique_ptr<nam::DSP> mEncapsulated;
  // The processing for NAM is a little weird--there's a call to .finalize_() that's expected.
  // This flag makes sure that the NAM sees alternating instances of .process() and .finalize_()
  // A value of `true` means that we expect the ResamplingNAM object to see .process() next;
  // `false` means we expect .finalize_() next.
  bool mFinalized = true;

  // The resampling wrapper
  dsp::ResamplingContainer<NAM_SAMPLE, 1, 12> mResampler;

  // Used to check that we don't get too large a block to process.
  int mMaxExternalBlockSize = 0;
  // Keep track of how many frames were processed so that we can be sure that finalize_() is being used correctly.
  // This is kind of hacky, but I'm not sure I want to rethink the core right now.
  int lastNumExternalFramesProcessed = -1;

  // This function is defined to conform to the interface expected by the iPlug2 resampler.
  std::function<void(NAM_SAMPLE**, NAM_SAMPLE**, int)> mBlockProcessFunc;
};

#endif