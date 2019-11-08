#include "Saturator.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"
#include "fft.h"
#include "saturation.h"

Saturator::Saturator(const InstanceInfo& info)
  : Plugin(info, MakeConfig(kNumParams, kNumPrograms))
{
  GetParam(kGain)->InitDouble("Gain", 0., -6.0, 24.0, 0.001, "dB");
  GetParam(kCeiling)->InitDouble("Ceiling", 0., -24.0, 24.0, 0.001, "dB");
  GetParam(kMode)->InitEnum("Interpolation", 1, kOperationModes, "", 0, "", "None", "Constant", "Linear");
  GetParam(kFixDC)->InitBool("DC fix?", false);

  #if IPLUG_EDITOR // All UI methods and member variables should be within an IPLUG_EDITOR guard, should you want distributed UI
    mMakeGraphicsFunc = [&]() {
      return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, 1.);
    };

    mLayoutFunc = [&](IGraphics* pGraphics) {
      pGraphics->AttachCornerResizer(EUIResizerMode::Scale, false);
      pGraphics->AttachPanelBackground(COLOR_GRAY);
      pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
      const IRECT b = pGraphics->GetBounds();
      const IRECT controls = b.GetGridCell(1, 1, 0);
      //pGraphics->AttachControl(new ITextControl(b.GetMidVPadded(50), "Gain (dB)", IText(20)));
      pGraphics->AttachControl(new IVKnobControl(controls.GetGridCell(0, 1, 4).GetCentredInside(100), kGain));
      pGraphics->AttachControl(new IVKnobControl(controls.GetGridCell(1, 1, 4).GetCentredInside(100), kCeiling));
      pGraphics->AttachControl(new IVRadioButtonControl(controls.GetGridCell(2, 1, 4).GetCentredInside(100), kMode));
      pGraphics->AttachControl(new IVRadioButtonControl(controls.GetGridCell(3, 1, 4).GetCentredInside(100), kFixDC));
    };
  #endif
};

auto tanh_const = Effect<Antialiased_Tanh_Const>();
auto tanh_linear = Effect<Antialiased_Tanh_Linear>();
auto dc_fixer = Effect<DC_Fix>();

#if IPLUG_DSP
void Saturator::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  const double gain = pow(10.0, GetParam(kGain)->Value()/20.0);
  const double ceil = pow(10.0, -GetParam(kCeiling)->Value()/20.0);
  const int mode = GetParam(kMode)->Value();
  const bool dcfix = GetParam(kFixDC)->Value();

  const int nChans = NOutChansConnected();
  tanh_const.set_channels(nChans);
  tanh_linear.set_channels(nChans);

  for (int c = 0; c < nChans; c++) {
    for (int s = 0; s < nFrames; s++) {
      sample smp = inputs[c][s];
      smp = smp * gain * ceil;

      switch (mode)
      {
      case 0:
        smp = tanh(smp);
        break;
      case 1:
        smp = tanh_const.process(c, smp);
        break;
      case 2:
        smp = tanh_linear.process(c, smp);
        break;
      }

      if ( dcfix )
        smp = dc_fixer.process(c, smp);

      smp = smp / ceil;
      outputs[c][s] = smp;
    }
  }
}
#endif
