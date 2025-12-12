#ifndef GAINSTAGEPROC_H_INCLUDED
#define GAINSTAGEPROC_H_INCLUDED

#include "AmpStage.h"
#include "ClippingStage.h"
#include "FeedForward2.h"
#include "PreAmpStage.h"
#include "SummingAmp.h"

class GainStageProc {
public:
  GainStageProc(double sampleRate);

  void reset(double sampleRate, int samplesPerBlock);
  void processBlock(AudioBuffer<float> &buffer);

  // Direct gain control (0.0 to 1.0)
  void setGain(float gain) { gainValue = gain; }
  float getGain() const { return gainValue; }

private:
  float gainValue = 0.5f; // Direct storage instead of pointer

  AudioBuffer<float> ff1Buff;
  AudioBuffer<float> ff2Buff;
  dsp::Oversampling<float> os{
      2, 1, dsp::Oversampling<float>::FilterType::filterHalfBandPolyphaseIIR};

  GainStageSpace::PreAmpWDF preAmpL, preAmpR;
  GainStageSpace::PreAmpWDF *preAmp[2]{&preAmpL, &preAmpR};

  GainStageSpace::ClippingWDF clipL, clipR;
  GainStageSpace::ClippingWDF *clip[2]{&clipL, &clipR};

  GainStageSpace::FeedForward2WDF ff2L, ff2R;
  GainStageSpace::FeedForward2WDF *ff2[2]{&ff2L, &ff2R};

  GainStageSpace::AmpStage amp[2];
  GainStageSpace::SummingAmp sumAmp[2];
};

#endif // GAINSTAGEPROC_H_INCLUDED
