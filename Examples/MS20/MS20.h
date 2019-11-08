#pragma once
#include "IPlug_include_in_plug_hdr.h"
#include "MS20_filter.h"

const int kNumPrograms = 1;

enum EParams
{
  kDrive = 0,
  kBoost = 1,
  kType = 2,
  kInertia = 3,
  kNonLinearity = 4,
  kClipper = 5,
  kCutoff = 6,
  kResonance = 7,
  kOversampling = 8,
  kNumParams
};

using namespace iplug;
using namespace igraphics;

class MS20 : public Plugin
{
public:
  MS20(const InstanceInfo& info);

  Inertia iDrive;
  Inertia iBoost;
  Inertia iCutoff;
  Inertia iResonance;

#if IPLUG_DSP // All DSP methods and member variables should be within an IPLUG_DSP guard, should you want distributed UI
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
#endif

};
