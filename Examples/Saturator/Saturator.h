#pragma once

#include "IPlug_include_in_plug_hdr.h"

const int kNumPrograms = 1;

enum OperationModes
{
  None = 0,
  Constant = 1,
  Linear = 2,
  kOperationModes
};

enum EParams
{
  kGain = 0,
  kCeiling = 1,
  kMode = 2,
  kFixDC = 3,
  kNumParams
};

using namespace iplug;
using namespace igraphics;

class Saturator : public Plugin
{
public:
  Saturator(const InstanceInfo& info);

#if IPLUG_DSP // All DSP methods and member variables should be within an IPLUG_DSP guard, should you want distributed UI
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
#endif
};
