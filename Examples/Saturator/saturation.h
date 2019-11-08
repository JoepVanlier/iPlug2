#pragma once
#include "IPlug_include_in_plug_hdr.h"

inline sample F0(sample x)
{
  return x - log(2.0 / (1.0 + exp(-2.0 * x)));
};

#define PLUGIN_TANH fast_tanh
inline sample fast_tanh(sample x)
{
  return 2.0 / (1.0 + exp(-2.0 * x)) - 1.0;
};

static inline sample rapid_tanh(sample s)
{
  const sample s2 = s * s;
  const sample a = s * (135135.0 + s2 * (17325.0 + s2 * (378.0 + s2)));
  const sample b = 135135.0 + s2 * (62370.0 + s2 * (3150.0 + s2 * 28.0));
  const sample c = a / b;
  if (c < -1)
    return -1;
  else if (c > 1)
    return 1;
  else return c;
}

static inline sample int_rapid_tanh(sample s)
{
  if (-1 < s < 1)
  {
    const sample s2 = s * s;
    return 0.0178571 * s2 + 1. * log(s2 + 2.4674) + 1.02273 * log(s2 + 22.2934) + 2.71834 * log(s2 + 87.7392);
  } else return 0;
}

static inline sample int_rapid_xtanh(sample s)
{
  if (-1 < s < 1)
  {
    const sample s3 = s * s * s;
    return 9.48214 * s + 0.0119048 * s3 - 50.9249 * atan(0.106759 * s) - 9.65786 * atan(0.211793 * s) - 3.14159 * atan(0.63662 * s);
  }
  else return 0;
}

inline sample Li2(sample x)
{
  sample A, ALFA, B0, B1, B2, H, S, T, Y, Q;
  const sample HF = 0.5;
  const sample PI3 = PI * PI * PI / 3.0;
  const sample PI6 = PI * PI / 6.0;
  const sample PI12 = PI * PI / 12.0;

  if (x == 1) {
    H = PI6;
  }
  else if (x == -1) {
    H = -PI12;
  }
  else {
    T = -x;
  };


  if (T <= -2.0) {
    Y = -1.0 / (1.0 + T);
    S = 1.0;
    A = log(-T);
    Q = log(1.0 + 1.0 / T);
    A = -PI3 + HF * (A * A - Q * Q);
  }
  else if (T < -1.0) {
    Y = -1.0 - T;
    S = -1.0;
    A = log(-T);
    Q = log(1.0 + 1.0 / T);
    A = -PI6 + A * (A + Q);
  }
  else if (T <= -0.5) {
    Y = -(1 + T) / T;
    S = 1.0;
    A = log(-T);
    Q = log(1.0 + T);
    A = -PI6 + A * (-HF * A + Q);
  }
  else if (T < 0.0) {
    Y = -T / (1.0 + T);
    S = -1.0;
    Q = log(1.0 + T);
    A = HF * Q * Q;
  }
  else if (T <= 1.0) {
    Y = T;
    S = 1.0;
    A = 0.0;
  }
  else {
    Y = 1.0 / T;
    S = -1.0;
    Q = log(T);
    A = PI6 + HF * Q * Q;
  };

  H = Y + Y - 1.0;
  ALFA = H + H;
  B1 = 0.0;
  B2 = 0.0;
  B0 = 0.00000000000000002 + ALFA * B1 - B2; B2 = B1; B1 = B0;
  B0 = -0.00000000000000014 + ALFA * B1 - B2; B2 = B1; B1 = B0;
  B0 = 0.00000000000000093 + ALFA * B1 - B2; B2 = B1; B1 = B0;
  B0 = -0.00000000000000610 + ALFA * B1 - B2; B2 = B1; B1 = B0;
  B0 = 0.00000000000004042 + ALFA * B1 - B2; B2 = B1; B1 = B0;
  B0 = -0.00000000000027007 + ALFA * B1 - B2; B2 = B1; B1 = B0;
  B0 = 0.00000000000182256 + ALFA * B1 - B2; B2 = B1; B1 = B0;
  B0 = -0.00000000001244332 + ALFA * B1 - B2; B2 = B1; B1 = B0;
  B0 = 0.00000000008612098 + ALFA * B1 - B2; B2 = B1; B1 = B0;
  B0 = -0.00000000060578480 + ALFA * B1 - B2; B2 = B1; B1 = B0;
  B0 = 0.00000000434545063 + ALFA * B1 - B2; B2 = B1; B1 = B0;
  B0 = -0.00000003193341274 + ALFA * B1 - B2; B2 = B1; B1 = B0;
  B0 = 0.00000024195180854 + ALFA * B1 - B2; B2 = B1; B1 = B0;
  B0 = -0.00000190784959387 + ALFA * B1 - B2; B2 = B1; B1 = B0;
  B0 = 0.00001588415541880 + ALFA * B1 - B2; B2 = B1; B1 = B0;
  B0 = -0.00014304184442340 + ALFA * B1 - B2; B2 = B1; B1 = B0;
  B0 = 0.00145751084062268 + ALFA * B1 - B2; B2 = B1; B1 = B0;
  B0 = -0.01858843665014592 + ALFA * B1 - B2; B2 = B1; B1 = B0;
  B0 = 0.40975987533077105 + ALFA * B1 - B2; B2 = B1; B1 = B0;
  B0 = 0.42996693560813697 + ALFA * B1 - B2; B2 = B1; B1 = B0;

  return -(S * (B0 - H * B2) + A);
};

inline sample F1(sample x)
{
  sample em2x = exp(-2.0 * x);
  return .5 * (x * (x + 2.0 * log(em2x + 1.0)) - Li2(-em2x));
};

inline sample sign(sample x)
{
  return (0 < x) - (x < 0);
};

class Antialiased_Tanh_Linear
{
  sample xnm1{ 0.0 }, xnm2{ 0.0 };
  sample F0_xnm1{ 0.0 }, F0_xnm2{ 0.0 };
  sample F1_xnm1{ 0.0 }, F1_xnm2{ 0.0 };
  sample F0_xn{ 0.0 }, F1_xn{ 0.0 };
  sample term1{ 0.0 }, term2{ 0.0 };

  const sample eps = .2;

  public:
    inline sample process(sample xn)
    {
      sample absxn = abs(xn);
      F0_xn = F0(absxn);

      const sample hpi12 = .5 * PI * PI / 12.0;
      F1_xn = (F1(absxn) - hpi12) * sign(xn) + hpi12;

      sample diff1 = (xn - xnm1);
      sample diff2 = (xnm2 - xnm1);

      if (abs(diff1) > eps) {
        term1 = (xn * (F0_xn - F0_xnm1) - (F1_xn - F1_xnm1)) / (diff1 * diff1);
      }
      else {
        term1 = .5 * PLUGIN_TANH((xn + 2.0 * xnm1) * .33333333333333333333333333333);
      };

      if (abs(diff2) > eps) {
        term2 = (xnm2 * (F0_xnm2 - F0_xnm1) - (F1_xnm2 - F1_xnm1)) / (diff2 * diff2);
      }
      else {
        term2 = .5 * PLUGIN_TANH((xnm2 + 2.0 * xnm1) * .33333333333333333333333333333);
      };

      F1_xnm2 = F1_xnm1;
      F1_xnm1 = F1_xn;
      F0_xnm2 = F0_xnm1;
      F0_xnm1 = F0_xn;
      xnm2 = xnm1;
      xnm1 = xn;

      return term1 + term2;
    };
};

class Antialiased_Tanh_Const {
  const sample eps = 0.0000000001;
  sample F0_xnm1{ 0.0 }, xnm1{ 0.0 };

  public:
    inline sample process(sample x)
    {
      sample F0_xn = F0(abs(x));
      sample diff = (x - xnm1);
      sample antialias = (abs(diff) > eps) ? (F0_xn - F0_xnm1) / diff : PLUGIN_TANH(.5 * (x + xnm1));
  
      F0_xnm1 = F0_xn;
      xnm1 = x;
      
      return antialias;
    };
};

class Antialiased_Approx_Tanh_Const {
  const sample eps = 0.0000000001;
  sample F0_xnm1{ 0.0 }, xnm1{ 0.0 };

public:
  inline sample process(sample x)
  {
    sample F0_xn = F0(abs(x));
    sample diff = (x - xnm1);
    sample antialias = (abs(diff) > eps) ? (F0_xn - F0_xnm1) / diff : PLUGIN_TANH(.5 * (x + xnm1));

    F0_xnm1 = F0_xn;
    xnm1 = x;

    return antialias;
  };
};

class DC_Fix {
  sample prev{ 0.0 };
  sample DC_fixed{ 0.0 };

  public:
    inline sample process(sample x)
    {
      DC_fixed = 0.999 * DC_fixed + x - prev;
      prev = x;

      return DC_fixed;
    };
};

//antialiased_tanh_const
template<typename T>
class Effect
{
  public:
    Effect() { set_channels(2); };

    void set_channels(int channels)
    {
      if (channels != antialiased_tanh.size())
      {
        antialiased_tanh.clear();
        for (auto a = 0; a < channels; a++)
        {
          antialiased_tanh.push_back(T());
        }
      }
    }

    inline sample process(int channel, sample x)
    {
      return antialiased_tanh[channel].process(x);
    }

  private:
    std::vector<T> antialiased_tanh;
};
