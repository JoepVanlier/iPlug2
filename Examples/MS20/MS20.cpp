#include "MS20.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"
#include "MS20_filter.h"
#include "fft.h"

MS20::MS20(const InstanceInfo& info)
  : Plugin(info, MakeConfig(kNumParams, kNumPrograms))
{
  GetParam(kDrive)->InitDouble("Drive", 0.0, -32.0, 32.0, 0.001, "dB");
  GetParam(kBoost)->InitDouble("Output Gain", 0.0, -32.0, 32.0, 0.001, "dB");
  GetParam(kType)->InitEnum("Mode", 0, kMS20_OperationModes, "", 0, "", "LP", "BP", "HP", "BR");
  GetParam(kInertia)->InitDouble("Inertia", 0.0, 0, 25.0, 0.001, "ms");
  GetParam(kNonLinearity)->InitEnum("Nonlinearity", 1, kNonLinearityModes, "", 0, "", "Tanh", "Fast", "Cross", "Broken", "Asym");
  GetParam(kClipper)->InitEnum("Diode", 1, kClipModes, "", 0, "", "Hard", "Soft", "Asym", "Rezz");
  GetParam(kCutoff)->InitDouble("Cutoff", 0.5, 0.0, 1.0, 0.001, "-");
  GetParam(kResonance)->InitDouble("Resonance", 0.0, 0.0, 1.0, 0.001, "-");
  GetParam(kOversampling)->InitInt("Oversampling", 1, 1, 4, "x");

  GetParam(kDrive)->SetInterpolationMethod();
  GetParam(kBoost)->SetInterpolationMethod();
  GetParam(kCutoff)->SetInterpolationMethod();
  GetParam(kResonance)->SetInterpolationMethod();

  #if IPLUG_EDITOR // All UI methods and member variables should be within an IPLUG_EDITOR guard, should you want distributed UI
    mMakeGraphicsFunc = [&]() {
      return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, 1.);
    };

    mLayoutFunc = [&](IGraphics* pGraphics) {
      pGraphics->AttachCornerResizer(EUIResizerMode::Scale, false);
      pGraphics->AttachPanelBackground(COLOR_GRAY);
      pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
      const IRECT b = pGraphics->GetBounds();
      const IRECT controls = b.GetGridCell(0, 1, 1);
      //pGraphics->AttachControl(new ITextControl(b.GetMidVPadded(50), "Gain (dB)", IText(20)));
      pGraphics->AttachControl(new IVKnobControl(controls.GetGridCell(0, 1, 7).GetCentredInside(100), kDrive));
      pGraphics->AttachControl(new IVKnobControl(controls.GetGridCell(1, 1, 7).GetCentredInside(100), kCutoff));
      pGraphics->AttachControl(new IVKnobControl(controls.GetGridCell(2, 1, 7).GetCentredInside(100), kResonance));
      pGraphics->AttachControl(new IVKnobControl(controls.GetGridCell(3, 1, 7).GetCentredInside(100), kBoost, "Gain"));
      pGraphics->AttachControl(new IVKnobControl(controls.GetGridCell(4, 1, 7).GetCentredInside(100), kOversampling, "Over"));
      pGraphics->AttachControl(new IVKnobControl(controls.GetGridCell(5, 1, 7).GetCentredInside(100), kInertia));
      const IRECT controls2 = controls.GetGridCell(6, 1, 7);
      IVStyle nolabel = IVStyle(false);
      pGraphics->AttachControl(new IVSwitchControl(controls2.GetGridCell(0, 3, 1).GetCentredInside(60, 40), kType, "", nolabel));
      pGraphics->AttachControl(new IVSwitchControl(controls2.GetGridCell(1, 3, 1).GetCentredInside(60, 40), kNonLinearity, "", nolabel));
      pGraphics->AttachControl(new IVSwitchControl(controls2.GetGridCell(2, 3, 1).GetCentredInside(60, 40), kClipper, "", nolabel));
    };
  #endif
};

auto oversamplers = Multi<OverSampling>();
auto filters = Multi<MS20_Filter>();

#if IPLUG_DSP
void MS20::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  const int mode = GetParam(kType)->Value();
  const int oversampling = GetParam(kOversampling)->Value();
  const double inertia = GetParam(kInertia)->Value();
  iDrive.setInertia(inertia, this->GetSampleRate());

  const int nonlinearity = GetParam(kNonLinearity)->Value();
  const int clippingMode = GetParam(kClipper)->Value();

  const int nChans = NOutChansConnected();
  oversamplers.set_channels(nChans);

  for (int s = 0; s < nFrames; s++) {
    const double cutoff = iCutoff(GetParam(kCutoff)->Value(s));
    const double resonance = iResonance(GetParam(kResonance)->Value(s));
    const double drive = iDrive(pow(10.0, GetParam(kDrive)->Value(s) / 20.0));
    const double boost = iResonance(pow(10.0, GetParam(kBoost)->Value(s) / 20.0));

    for (int c = 0; c < nChans; c++) {
      OverSampling& oversampler = oversamplers.get(c);
      oversampler.setOversampling(oversampling);
      MS20_Filter& filter = filters.get(c);

      filter.setParams(cutoff, resonance, oversampling);

      auto process = [&](sample s) -> sample { return filters.get(c).process(mode, nonlinearity, clippingMode, s); };
      outputs[c][s] = boost * oversampler.process(drive * inputs[c][s], process);
    }
  }
}
#endif
