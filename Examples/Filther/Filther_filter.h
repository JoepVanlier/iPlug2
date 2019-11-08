#pragma once
#include "IPlug_include_in_plug_hdr.h"

#include <variant>
#include <tuple>
#include <algorithm>
#include "tools.h"

enum Filther_Modes
{
  Filther_Lowpass = 0,
  Filther_Bandpass = 1,
  Filther_Highpass = 2,
  Filther_Bandreject = 3,
  kFilther_OperationModes
};

enum ClipModes
{
  Filther_Clip = 0,
  Filther_Soft = 1,
  Filther_Asym = 2,
  Filther_Rezz = 3,
  kClipModes
};

enum NonLinearityModes
{
  Filther_Expensive = 0,
  Filther_Cheap = 1,
  Filther_Crossover = 2,
  Filther_Broken = 3,
  Filther_Asymmetric = 4,
  kNonLinearityModes
};

namespace Filther_Nonlinearities
{
  static inline std::tuple<sample, sample> diode_clip(sample s)
  {
    return std::tuple<sample, sample>(std::max(-1.0, std::min(1.0, s)), 1.0 - 1.0 * (abs(s) > 1.0));
  };

  static inline std::tuple<sample, sample> diode_slow(sample s)
  {
    if (abs(s) > 1)
    {
      sample g = s - .75 * sign(s) * (abs(s) - 1.0);
      sample dg = .25;
      return std::tuple<sample, sample>(g, dg);
    } else {
      sample g = s;
      sample dg = 1.0;
      return std::tuple<sample, sample>(g, dg);
    }
  };

  static inline std::tuple<sample, sample> diode_asym(sample s)
  {
    s = s - .2;
    return std::tuple<sample, sample>(.2 + std::max(-1.0, std::min(1.0, s)), 1.0 - 1.0 * (abs(s) > 1.0));
  };

  static inline sample rapidTanh(sample s)
  {
    sample s2 = s * s;
    sample a = s * (135135.0 + s2 * (17325.0 + s2 * (378.0 + s2)));
    sample b = 135135.0 + s2 * (62370.0 + s2 * (3150.0 + s2 * 28.0));
    sample c = a / b;
    if (c < -1) c = -1;
    if (c > 1) c = 1;

    return c;
  }

  static inline std::tuple<sample, sample> diode_rezzy(sample s)
  {
    const sample tanhterm = rapidTanh(s-s*s+s*s*s);

    return std::tuple<sample, sample>(tanhterm, (1.0-2.0*s+3.0*s*s)*(1.0 - tanhterm * tanhterm));
  }

  static inline std::tuple<sample, sample> nonlinear_tanh(sample s)
  {
    sample c = tanh(s);
    return std::tuple<sample, sample>(c, 1.0 - c * c);
  };


  static inline std::tuple<sample, sample> nonlinear_fasttanh(sample s)
  {
    sample c = rapidTanh(s);

    return std::tuple<sample, sample>(c, 1.0 - c * c);
  };

  static inline std::tuple<sample, sample> nonlinear_broken(sample s)
  {
    sample s2_in = s * s;
    s = s2_in * s;
    sample s2 = s * s;
    sample a = s * (135135.0 + s2 * (17325.0 + s2 * (378.0 + s2)));
    sample b = 135135.0 + s2 * (62370.0 + s2 * (3150.0 + s2 * 28.0));
    sample c = a / b;
    if (c < -1) c = -1;
    if (c > 1) c = 1;

    return std::tuple<sample, sample>(c, 3.0*(1.0 - c * c)*s2_in);
  };

  static inline std::tuple<sample, sample> nonlinear_crossover(sample s)
  {
    const sample crossover = .02;

    if (abs(s) < crossover)
      return std::tuple<sample, sample>(.1*s, .1);
    else
      s = s - .9 * crossover * sign(s);

    sample s2 = s * s;
    sample a = s * (135135.0 + s2 * (17325.0 + s2 * (378.0 + s2)));
    sample b = 135135.0 + s2 * (62370.0 + s2 * (3150.0 + s2 * 28.0));
    sample c = a / b;
    if (c < -1) c = -1;
    if (c > 1) c = 1;

    return std::tuple<sample, sample>(c, 1.0 - c * c);
  }

  static inline std::tuple<sample, sample> nonlinear_asym(sample s)
  {
    /*const sample crossover = .02;
    const sample Vpos = 5;
    const sample Vneg = -5;
    const sample VdropPos = 3;
    const sample VdropNeg = 1.5;
    const sample Vt = 4;
    return tanh(Vin / (2 * Vt)) - Is * (exp((Vout + VdropPos - Vpos) / Vt) - exp((Vneg + VdropNeg - Vout) / Vt))*/

    sample f;
    if (s > 0)
    {
      sample c = rapidTanh(s);
      return std::tuple<sample, sample>(c, 1.0 - c * c);
    } else {
      sample c = rapidTanh(2*s);
      return std::tuple<sample, sample>(c, 2.0 * (1.0 - c * c));
    }
  };
};

/**
 * Filther Filter
 *
 * @tparam diode Nonlinearity for the diode
 * @tparam nonlinearity Nonlinearity for the capacitor
 */
class Filther_Filter
{
  public:
    Filther_Filter()
    {
      setParams(0.5, 0.5, 2);
    };

    void setParams(sample cutoff, sample resonance, sample oversampling)
    {
      sample h = f_trafo_hz(cutoff) * PI / oversampling;
      hh = 0.5 * h;
      k = 2.0 * resonance;
    };

    /*template <std::tuple<sample, sample>(*Diode)(sample), std::tuple<sample, sample>(*Nonlinearity)(sample)>*/
    inline sample process(const int mode, const int nonlinearity, const int clippingMode, sample x)
    {
      std::tuple<sample, sample>(*Nonlinearity)(sample);
      switch (nonlinearity)
      {
        case Filther_Expensive: { Nonlinearity = Filther_Nonlinearities::nonlinear_tanh; break; }
        case Filther_Cheap: { Nonlinearity = Filther_Nonlinearities::nonlinear_fasttanh; break; }
        case Filther_Crossover: { Nonlinearity = Filther_Nonlinearities::nonlinear_crossover; break; }
        case Filther_Broken: { Nonlinearity = Filther_Nonlinearities::nonlinear_broken; break; }
        case Filther_Asymmetric: { Nonlinearity = Filther_Nonlinearities::nonlinear_asym; break; }
      }

      std::tuple<sample, sample>(*Diode)(sample);
      switch (clippingMode)
      {
        case Filther_Soft: { Diode = Filther_Nonlinearities::diode_slow; break; }
        case Filther_Clip: { Diode = Filther_Nonlinearities::diode_clip; break; }
        case Filther_Asym: { Diode = Filther_Nonlinearities::diode_asym; break; }
        case Filther_Rezz: { Diode = Filther_Nonlinearities::diode_rezzy; break; }
      }

      switch (mode)
      {
        case Filther_Lowpass: return lowpass(x, Diode, Nonlinearity);
        case Filther_Bandpass: return bandpass(x, Diode, Nonlinearity);
        case Filther_Highpass: return highpass(x, Diode, Nonlinearity);
        case Filther_Bandreject: return bandreject(x, Diode, Nonlinearity);
        default:
          return 0.0;
      };
    }

    /*template <std::tuple<sample, sample>(*Diode)(sample), std::tuple<sample, sample>(*Nonlinearity)(sample)>*/
    inline sample lowpass(sample x, std::tuple<sample, sample>(*Diode)(sample), std::tuple<sample, sample>(*Nonlinearity)(sample))
    {
      const sample dfb = k * d2;                                        /* Filter specific */
      const sample g_dfb = std::get<0>(Diode(dfb));
      const sample term1 = std::get<0>(Nonlinearity(-d1 + x - g_dfb));  /* Filter specific */
      const sample term2 = std::get<0>(Nonlinearity(d1 - d2 + g_dfb));  /* Filter specific */

      int iter = 0;
      sample lastError = 1000.0;
      sample step = 1.0;
      sample y1Last = y1;
      sample y2Last = y2;
      while ((lastError > eps) && (iter < 10))
      {
        sample fb = k * y2;                             /* Filter specific */
        sample g_fb, dg_fb;
        std::tie(g_fb, dg_fb) = Diode(fb);

        sample sig1 = x - y1 - g_fb;                    /* Filter specific */
        sample sig2 = y1 - y2 + g_fb;                   /* Filter specific */

        sample fsig1, fsig2, dfsig1, dfsig2;
        std::tie(fsig1, dfsig1) = Nonlinearity(sig1);
        std::tie(fsig2, dfsig2) = Nonlinearity(sig2);

        sample f1 = y1 - d1 - hh * (term1 + fsig1);
        sample f2 = y2 - d2 - hh * (term2 + fsig2);
        sample fError = abs(f1) + abs(f2);

        if (fError > lastError)
        {
          y1 = y1Last;
          y2 = y2Last;
          step *= 0.5;
        };
        lastError = fError;
        y1Last = y1;
        y2Last = y2;

        sample hhdfsig1 = hh * dfsig1;
        sample hhdfsig2 = hh * dfsig2;

        sample a = hhdfsig1 + 1.0;
        sample b = k * hhdfsig1 * dg_fb;
        sample c = -hhdfsig2;
        sample d = -hhdfsig2 * (k * dg_fb - 1.0) + 1.0;

        sample det = (a * d - b * c);
        if (abs(det) < 0.01) det = sign(det) * 0.01;
        sample norm = step / det;

        y1 = y1 - (d * f1 - b * f2) * norm;
        y2 = y2 - (a * f2 - c * f1) * norm;
        iter += 1;
      }

      d1 = y1;
      d2 = y2;

      return y2;
    };

    /*template <std::tuple<sample, sample>(*Diode)(sample), std::tuple<sample, sample>(*Nonlinearity)(sample)>*/
    inline sample bandpass(sample x, std::tuple<sample, sample>(*Diode)(sample), std::tuple<sample, sample>(*Nonlinearity)(sample))
    {
      const sample dfb = k * d2;                          /* Filter specific */
      const sample g_dfb = std::get<0>(Diode(dfb));
      const sample term1 = std::get<0>(Nonlinearity(-d1 - x - g_dfb));      /* Filter specific */
      const sample term2 = std::get<0>(Nonlinearity(d1 - d2 + x + g_dfb));  /* Filter specific */

      int iter = 0;
      sample lastError = 1000.0;
      sample step = 1.0;
      sample y1Last = y1;
      sample y2Last = y2;
      while ((lastError > eps) && (iter < 10))
      {
        sample fb = k * y2;                         /* Filter specific */
        sample g_fb, dg_fb;
        std::tie(g_fb, dg_fb) = Diode(fb);

        sample sig1 = -x - y1 - g_fb;               /* Filter specific */
        sample sig2 = x + y1 - y2 + g_fb;           /* Filter specific */

        sample fsig1, fsig2, dfsig1, dfsig2;
        std::tie(fsig1, dfsig1) = Nonlinearity(sig1);
        std::tie(fsig2, dfsig2) = Nonlinearity(sig2);

        sample f1 = y1 - d1 - hh * (term1 + fsig1);
        sample f2 = y2 - d2 - hh * (term2 + fsig2);

        sample fError = abs(f1) + abs(f2);
        if (fError > lastError)
        {
          y1 = y1Last;
          y2 = y2Last;
          step *= 0.5;
        };
        lastError = fError;
        y1Last = y1;
        y2Last = y2;

        sample hhdfsig1 = hh * dfsig1;
        sample hhdfsig2 = hh * dfsig2;

        sample a = hhdfsig1 + 1.0;
        sample b = k * hhdfsig1 * dg_fb;
        sample c = -hhdfsig2;
        sample d = -hhdfsig2 * (k * dg_fb - 1.0) + 1.0;

        sample det = (a * d - b * c);
        if (abs(det) < 0.01) det = sign(det) * 0.01;
        sample norm = step / det;

        y1 = y1 - (d * f1 - b * f2) * norm;
        y2 = y2 - (a * f2 - c * f1) * norm;
        iter += 1;
      }

      d1 = y1;
      d2 = y2;

      return y2;
    };

    /*template <std::tuple<sample, sample>(*Diode)(sample), std::tuple<sample, sample>(*Nonlinearity)(sample)>*/
    inline sample highpass(sample x, std::tuple<sample, sample>(*Diode)(sample), std::tuple<sample, sample>(*Nonlinearity)(sample))
    {
      const sample dfb = k * (x + d2);              /* Filter specific */
      const sample g_dfb = std::get<0>(Diode(dfb));
      const sample term1 = std::get<0>(Nonlinearity(-d1 - g_dfb));      /* Filter specific */
      const sample term2 = std::get<0>(Nonlinearity(d1 - d2 - x + g_dfb));  /* Filter specific */

      int iter = 0;
      sample lastError = 1000.0;
      sample step = 1.0;
      sample y1Last = y1;
      sample y2Last = y2;
      while ((lastError > eps) && (iter < 10))
      {
        sample fb = k * (x + y2);             /* Filter specific */
        sample g_fb, dg_fb;
        std::tie(g_fb, dg_fb) = Diode(fb);

        sample sig1 = -y1 - g_fb;            /* Filter specific */
        sample sig2 = y1 - y2 - x + g_fb;    /* Filter specific */

        sample fsig1, fsig2, dfsig1, dfsig2;
        std::tie(fsig1, dfsig1) = Nonlinearity(sig1);
        std::tie(fsig2, dfsig2) = Nonlinearity(sig2);

        sample f1 = y1 - d1 - hh * (term1 + fsig1);
        sample f2 = y2 - d2 - hh * (term2 + fsig2);

        sample fError = abs(f1) + abs(f2);
        if (fError > lastError)
        {
          y1 = y1Last;
          y2 = y2Last;
          step *= 0.5;
        };
        lastError = fError;
        y1Last = y1;
        y2Last = y2;

        sample hhdfsig1 = hh * dfsig1;
        sample hhdfsig2 = hh * dfsig2;

        sample a = hhdfsig1 + 1.0;
        sample b = k * hhdfsig1 * dg_fb;
        sample c = -hhdfsig2;
        sample d = -hhdfsig2 * (k * dg_fb - 1.0) + 1.0;

        sample det = (a * d - b * c);
        if (abs(det) < 0.01) det = sign(det) * 0.01;
        sample norm = step / det;

        y1 = y1 - (d * f1 - b * f2) * norm;
        y2 = y2 - (a * f2 - c * f1) * norm;
        iter += 1;
      }

      d1 = y1;
      d2 = y2;

      return y2 + x;
    };

    /*template <std::tuple<sample, sample>(*Diode)(sample), std::tuple<sample, sample>(*Nonlinearity)(sample)>*/
    inline sample bandreject(sample x, std::tuple<sample, sample>(*Diode)(sample), std::tuple<sample, sample>(*Nonlinearity)(sample))
    {
      const sample dfb = k * d2;                          /* Filter specific */
      const sample g_dfb = std::get<0>(Diode(dfb));
      const sample term1 = std::get<0>(Nonlinearity(-d1 - x - g_dfb));      /* Filter specific */
      const sample term2 = std::get<0>(Nonlinearity(d1 - d2 + x + g_dfb));  /* Filter specific */

      int iter = 0;
      sample lastError = 1000.0;
      sample step = 1.0;
      sample y1Last = y1;
      sample y2Last = y2;
      while ((lastError > eps) && (iter < 10))
      {
        sample fb = k * y2;                         /* Filter specific */
        sample g_fb, dg_fb;
        std::tie(g_fb, dg_fb) = Diode(fb);

        sample sig1 = -x - y1 - g_fb;               /* Filter specific */
        sample sig2 = x + y1 - y2 + g_fb;           /* Filter specific */

        sample fsig1, fsig2, dfsig1, dfsig2;
        std::tie(fsig1, dfsig1) = Nonlinearity(sig1);
        std::tie(fsig2, dfsig2) = Nonlinearity(sig2);

        sample f1 = y1 - d1 - hh * (term1 + fsig1);
        sample f2 = y2 - d2 - hh * (term2 + fsig2);

        sample fError = abs(f1) + abs(f2);
        if (fError > lastError)
        {
          y1 = y1Last;
          y2 = y2Last;
          step *= 0.5;
        };
        lastError = fError;
        y1Last = y1;
        y2Last = y2;

        sample hhdfsig1 = hh * dfsig1;
        sample hhdfsig2 = hh * dfsig2;

        sample a = hhdfsig1 + 1.0;
        sample b = k * hhdfsig1 * dg_fb;
        sample c = -hhdfsig2;
        sample d = -hhdfsig2 * (k * dg_fb - 1.0) + 1.0;

        sample det = (a * d - b * c);
        if (abs(det) < 0.01) det = sign(det) * 0.01;
        sample norm = step / det;

        y1 = y1 - (d * f1 - b * f2) * norm;
        y2 = y2 - (a * f2 - c * f1) * norm;
        iter += 1;
      }

      d1 = y1;
      d2 = y2;

      return x - y2;
    };

  private:
    int mode = 0;
    sample hh{ 1.0 };
    sample k{ 1.0 };
    sample d1{ 0.0 }, d2{ 0.0 };
    sample y1{ 0.0 }, y2{ 0.0 };
    sample eps{ 0.001* 0.00000001 };
};
