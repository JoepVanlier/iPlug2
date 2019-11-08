#pragma once
#include "IPlug_include_in_plug_hdr.h"
#include "IControl.h"
#include "IControls.h"

#include "IGraphicsStructs.h"
#include "IColorPickerControl.h"
#include "IVKeyboardControl.h"
#include "IVMeterControl.h"
#include "IVScopeControl.h"
#include "IVMultiSliderControl.h"
#include "IRTTextControl.h"
#include "IVDisplayControl.h"

using namespace iplug;
using namespace igraphics;

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

inline IRECT RectXYWH(float x, float y, float w, float h)
{
  return IRECT(x, y, x + w, y + h);
}

class IVDualSlider : public IVectorBase, public IControl
{
private:
  std::string mText;
  bool over{ false }, overHandle{ false };
  int capture{ 0 };
  float lastX{ 0.0 }, lastY{ 0.0 };

public:
  IVDualSlider::IVDualSlider(const IRECT& bounds, int paramIdx1, int paramIdx2, const char* label = "", const IVStyle& style = DEFAULT_STYLE, EDirection dir = EDirection::Horizontal) :
    IVectorBase(style, false, false), IControl(bounds, { paramIdx1, paramIdx2 }, nullptr)
  {
    DisablePrompt(true);
    AttachIControl(this, label);
  };

  void IVDualSlider::Draw(IGraphics& g)
  {
    const IRECT& rect = this->GetRECT();
    const float x = rect.L;
    const float w = rect.R - rect.L;
    const float ycenter = (rect.T + rect.B) * .5f;
    const float h = rect.B - rect.T;
    IColor color = GetColor(kFG);
    
    g.DrawLine(GetColor(kFG), x, ycenter, x + w, ycenter, 0, 1.0f);
    g.DrawRect(GetColor(kFR), rect, nullptr, mStyle.frameThickness);
    g.DrawRect(GetColor(kFR), rect, nullptr, mStyle.frameThickness);

    DrawHandle(g, GetColor(kFG), x, w, h, ycenter, 0, over);
    DrawRange(g, GetColor(kFG), x, w, h, ycenter, 0, 1);
  }

  void IVDualSlider::DrawRange(IGraphics& g, IColor color, float x, float w, float h, float ycenter, int paramIdx1, int paramIdx2)
  {
    double pValue = GetParam(paramIdx1)->GetNormalized();
    double pValue2 = GetParam(paramIdx2)->GetNormalized() * 2.0 - 1.0;

    if (pValue2 > 0)
    {
      if ((pValue + pValue2) > 1.0) pValue2 = 1.0 - pValue;
      g.DrawRect(color, RectXYWH(x + pValue * w, ycenter - .1 * h, pValue2 * w + 1, h * .3));
    } else {
      if ((pValue - pValue2) < 0.0) pValue2 = - pValue;
      g.DrawRect(color, RectXYWH(x + (pValue + pValue2) * w, ycenter - .1 * h, abs(pValue2) * w, h * .3));
    }
  }

  void IVDualSlider::DrawHandle(IGraphics& g, IColor color, float x, float w, float h, float ycenter, double paramIdx, bool over)
  {
    double pValue = GetParam(paramIdx)->GetNormalized();
    g.FillRect(color, RectXYWH(x - 2.0f + pValue * w, ycenter - .2f * h, 4.0f, h * .4f));
    color.A = .4 * color.A;
    g.FillRect(color, RectXYWH(x - 1.0f + pValue * w, ycenter - .2f * h + 1.0f, 2.0f, h * .4f - 2.0f));

    if (overHandle) g.FillRect(color, RectXYWH(x - 3.0f + pValue * w, ycenter - .2f * h - 1.0f, 4.0f + 2.0f, h * .4f + 2.0f));

    color.A = .5 * color.A;
    g.FillRect(color, RectXYWH(x - 2.0f + pValue * w + 5.0f, ycenter - .3f * h, 1.0, h * .6f + 1));
    g.FillRect(color, RectXYWH(x - 2.0f + pValue * w - 2.0f, ycenter - .3f * h, 1.0, h * .6f + 1));
    g.FillRect(color, RectXYWH(x - 2.0f + pValue * w - 1.0f, ycenter - .3f * h, 6.0, 1));
    g.FillRect(color, RectXYWH(x - 2.0f + pValue * w - 1.0f, ycenter + .3f * h - 1.0, 6, 1));
  }

  //default, minval, maxval, value, value2, label, lastleft, lastright, lastclick, yslidercenter, cap, twoval, thisUI, htime, hint, onmarker, lastrightclick, disabled


  inline float IVDualSlider::relativeToSlider(float mouse_x, float x, float w)
  {
    return (mouse_x - x) / w;
  }

  /* sliderWidget_processMouse */
  void IVDualSlider::OnMouseDown(float x, float y, const IMouseMod& mod)
  {
    const IRECT& rect = this->GetRECT();
    const float this_x = rect.L;
    const float w = rect.R - rect.L;

    if (mod.L) {
      capture = 1;
      if (!mod.C && !mod.S && !mod.A)
      {
        float par = relativeToSlider(x, this_x, w);
        SetValue(par, 0);
        SetDirty(1, 0);
      }
    } else if (mod.R)
    {
      capture = 2;

      if (!mod.C && !mod.S && !mod.A)
      {
        double pValue = GetParam(0)->GetNormalized();
        double pValue2 = GetParam(1)->ToNormalized(relativeToSlider(x, this_x, w) - pValue);

        SetValue(pValue2, 1);
        SetDirty(1, 1);
      }
    }
  }

  void IVDualSlider::OnMouseUp(float x, float y, const IMouseMod& mod)
  {
    if (!mod.L && !mod.R)
      capture = 0;
  }

  inline float clamp(float value, float min, float max)
  {
    if (value > max)
      return max;
    else if (value < min)
      return min;
    else return value;
  }

  void IVDualSlider::OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod)
  {
    const IRECT& rect = this->GetRECT();
    const float this_x = rect.L;
    const float w = rect.R - rect.L;

    float mul = 1.0f / capture;
    if (mod.S) mul = mul * .125f;
    if (mod.C) mul = mul * .16666667f;
    SetValue(GetParam(capture - 1)->GetNormalized() + mul * dX / w, capture-1);

    SetDirty(1, capture - 1);
  }

  /*void IVDualSlider::OnMouseDblClick()
  {

  }*/

  void IVDualSlider::OnMouseOver(float x, float y, const IMouseMod& mod)
  {
    double pValue = GetParam()->Value();
    const IRECT& rect = this->GetRECT();
    const float this_x = rect.L;
    const float w = rect.R - rect.L;
    const float ycenter = (rect.T + rect.B) * .5f;
    const float h = rect.B - rect.T;

    over = rect.Contains(x, y);
    overHandle = RectXYWH(this_x - 3.0f + pValue * w, ycenter - .2f * h - 1.0f, 4.0f + 2.0f, h * .4f + 2.0f).Contains(x, y);
  }

  void IVDualSlider::DrawTrack(IGraphics& g, const IRECT& filledArea)
  {
    //g.FillRect(GetColor(kSH), mTrack);
    //g.FillRect(GetColor(kX1), filledArea);

    //if (mStyle.drawFrame)
    //  g.DrawRect(GetColor(kFR), mTrack, nullptr, mStyle.frameThickness);
  }

  void IVDualSlider :: SetDirty(bool push, int validx)
  {
    IControl::SetDirty(push, validx);
    
    if (validx == 0)
    {
      const IParam* pParam = GetParam();
      if ( pParam ) pParam->GetDisplayForHostWithLabel(mValueStr);
    }
  }
};






/*
IVSliderControl::IVSliderControl(const IRECT& bounds, IActionFunction actionFunc, const char* label, const IVStyle& style, bool valueIsEditable, EDirection dir, bool onlyHandle, float handleSize, float trackSize)
  : ISliderControlBase(bounds, actionFunc, dir, onlyHandle, handleSize)
  , IVectorBase(style)
  , mTrackSize(trackSize)
{
  DisablePrompt(!valueIsEditable);
  mText = style.valueText;
  AttachIControl(this, label);
}

void IVSliderControl::Draw(IGraphics& g)
{
  DrawBackGround(g, mRECT);
  DrawWidget(g);
  DrawLabel(g);
  DrawValue(g, mValueMouseOver);
}

void IVSliderControl::DrawTrack(IGraphics& g, const IRECT& filledArea)
{
  g.FillRect(GetColor(kSH), mTrack);
  g.FillRect(GetColor(kX1), filledArea);

  if (mStyle.drawFrame)
    g.DrawRect(GetColor(kFR), mTrack, nullptr, mStyle.frameThickness);
}

void IVSliderControl::DrawWidget(IGraphics& g)
{
  IRECT filledTrack = mTrack.FracRect(mDirection, (float)GetValue());

  if (!mOnlyHandle)
    DrawTrack(g, filledTrack);

  float cx, cy;

  const float offset = (mStyle.drawShadows && mShape != EVShape::Ellipse) ? mStyle.shadowOffset * 0.5f : 0.;

  if (mDirection == EDirection::Vertical)
  {
    cx = filledTrack.MW() + offset;
    cy = filledTrack.T;
  }
  else
  {
    cx = filledTrack.R;
    cy = filledTrack.MH() + offset;
  }

  IRECT handleBounds = IRECT(cx - mHandleSize, cy - mHandleSize, cx + mHandleSize, cy + mHandleSize);

  DrawHandle(g, mShape, handleBounds, mMouseDown, mMouseIsOver);
}

void IVSliderControl::OnMouseDown(float x, float y, const IMouseMod& mod)
{
  if (mStyle.showValue && mValueBounds.Contains(x, y))
  {
    PromptUserInput(mValueBounds);
  }
  else
  {
    if (mStyle.hideCursor)
      GetUI()->HideMouseCursor(true, false);

    ISliderControlBase::OnMouseDown(x, y, mod);
  }
}

void IVSliderControl::OnMouseUp(float x, float y, const IMouseMod& mod)
{
  if (mStyle.hideCursor)
    GetUI()->HideMouseCursor(false);

  ISliderControlBase::OnMouseUp(x, y, mod);

  SetDirty(true);
}

void IVSliderControl::OnMouseOver(float x, float y, const IMouseMod& mod)
{
  if (mStyle.showValue && !mDisablePrompt)
    mValueMouseOver = mValueBounds.Contains(x, y);

  ISliderControlBase::OnMouseOver(x, y, mod);
}

void IVSliderControl::OnResize()
{
  SetTargetRECT(MakeRects(mRECT));

  if (mDirection == EDirection::Vertical)
    mTrack = mWidgetBounds.GetPadded(-mHandleSize).GetMidHPadded(mTrackSize);
  else
    mTrack = mWidgetBounds.GetPadded(-mHandleSize).GetMidVPadded(mTrackSize);

  SetDirty(false);
}

bool IVSliderControl::IsHit(float x, float y) const
{
  if (!mDisablePrompt)
  {
    if (mValueBounds.Contains(x, y))
    {
      return true;
    }
  }

  return mWidgetBounds.Contains(x, y);
}

void IVSliderControl::SetDirty(bool push, int valIdx)
{
  ISliderControlBase::SetDirty(push);

  const IParam* pParam = GetParam();

  if (pParam)
    pParam->GetDisplayForHostWithLabel(mValueStr);
}

void IVSliderControl::OnInit()
{
  const IParam* pParam = GetParam();

  if (pParam)
  {
    if (!mLabelStr.GetLength())
      mLabelStr.Set(pParam->GetNameForHost());

    pParam->GetDisplayForHostWithLabel(mValueStr);
  }
}*/

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE