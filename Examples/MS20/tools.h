#pragma once
#include "IPlug_include_in_plug_hdr.h"
#include <vector>

#define PLUGIN_TANH fast_tanh

#define sample double
#define PI 3.14159265358979323846

inline sample fast_tanh(sample x)
{
  return 2.0 / (1.0 + exp(-2.0 * x)) - 1.0;
};

inline sample f_trafo_hz(sample freq)
{
  return exp((1.0 - freq) * log(20.0 / 22050.0));
};

sample sign(sample val)
{
  if (val > 0.0)
    return 1;
  else
    return -1;
};

class DC_Fix
{
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

/* Modified Bessel of the 0th kind */
inline sample Bessel(sample x) {
  sample d = 0.0;
  sample ds = 1.0;
  sample s = 1.0;
  for (int i = 1; i < 10; i++)
  {
    d = d + 2.0;
    ds *= x * x / (d * d);
    s += ds;
  };

  return s;
};

inline sample sinc(sample x)
{
  return (x != 0) ? sin(x) / x : 1.0;
};

inline std::vector<sample> KaiserSinc(sample fc, sample Att, sample transitionBand)
{
  sample total{ 0.0 };

  sample beta;
  if (Att > 50.0) {
    beta = (0.1102 * (Att - 8.7));
  } else if (Att < 21.0) {
    beta = 0;
  } else {
    beta = (0.5842 * pow(Att - 21.0, 0.4) + 0.07866 * (Att - 21.0));
  };
  double N = ceil( (Att - 7.95) / (14.357 * transitionBand) ) + 1;
  if ((floor(N / 2) - N / 2) == 0) N = N + 1;

  std::vector<sample> window;
  window.reserve(N);

  for (double n = -floor(N/2); n < ceil(N/2); n += 1.0)
  {
    // Compute sinc contribution /*(n - (N - 1) / 2.0)*/
    sample s = 2.0 * fc * sinc(2.0 * PI * fc * n);

    // Compute Kaiser contribution
    sample t = n/(N/2); /*2.0 * n / (N - 1.0) - 1.0;*/
    sample K = Bessel(beta * sqrt(1 - t * t)) / Bessel(beta);

    sample tap = K * s;
    window.push_back(tap);
    total = total + tap;
  }

  /* Normalize the kernel */
  //for (auto it : window)
  //  it /= total;

  return std::move(window);
};


class OverSampling {
  std::vector<sample> m_taps;
  std::vector<std::vector<sample>> m_filterbank;
  std::vector<sample> m_upsampleHist;
  std::vector<sample> m_downsampleHist;
  size_t m_upsampleIdx = 0;
  size_t m_downsampleIdx = 0;
  int m_oversampling{ -1 };

  public:
    OverSampling() {};

    void setOversampling(int oversampling)
    {
      if (m_oversampling != oversampling)
      {
        m_oversampling = oversampling;
        m_taps = KaiserSinc(.5 / oversampling, 96, .2 / oversampling);

        m_filterbank.clear();
        for (int i = 0; i < oversampling; i++)
          m_filterbank.push_back(std::vector<sample>());

        // Divide the filter over the polyphase bank (for upsampling)
        int current_filter = 0;
        for (auto it : m_taps) {
          m_filterbank[current_filter].push_back(it);
          current_filter += 1;
          if (current_filter == oversampling) {
            current_filter = 0;
          }
        }

        m_upsampleHist.resize(m_taps.size());
        m_downsampleHist.resize(m_taps.size());
        m_upsampleIdx = 0;
        m_downsampleIdx = 0;
      };
    };

    /* Called every sample */
    void updateUpsampleMemory(sample x)
    {
      m_upsampleHist[m_upsampleIdx] = x;
      m_upsampleIdx += 1;
      if (m_upsampleIdx == m_upsampleHist.size())
      {
        m_upsampleIdx = 0;
      }
    };

    /* Returns upsampled sample */
    sample upSample(int current_filter)
    {
      sample out{ 0.0 };
      signed int position = m_upsampleIdx - 1;
      for (auto tap : m_filterbank[current_filter])
      {
        if (position < 0)
          position = m_upsampleHist.size() - 1;

        out += tap * m_upsampleHist[position];
        position -= 1;
      }

      return out;
    };

    /* Push sample in here after processing */
    void updateDownsampleMemory(sample x)
    {
      m_downsampleHist[m_downsampleIdx] = x;
      m_downsampleIdx += 1;
      if (m_downsampleIdx == m_downsampleHist.size())
        m_downsampleIdx = 0;
    };

    /* Called on first sample to get AA'd result */
    sample downSample()
    {
      sample out{ 0.0 };
      signed int position = m_downsampleIdx - 1;
      for (auto tap : m_taps)
      {
        if (position < 0)
          position += m_downsampleHist.size();

        out += tap * m_downsampleHist[position];
        position -= 1;
      }

      return out;
    };

    //sample process(sample x, sample(&process_sample)(sample))
    template <typename Proc>
    sample process(sample x, Proc process_sample)
    {
      sample out;

      if (m_oversampling > 1)
      {
        int currentSample = 0;
        updateUpsampleMemory(x);
        for (int i = 0; i < m_oversampling; i++)
        {
          sample input = m_oversampling * upSample(currentSample);
          sample output = process_sample(input);
          updateDownsampleMemory(output);
          currentSample += 1;
          if (currentSample == 1)
            out = downSample();// / m_oversampling;
        }
        return out;
      } else return process_sample(x);
    }
};

/*class InertiaSmoother
{
  private:
    sample prev{ 0.0 };
    sample val{ 0.0 };
    sample boost{ 0.0 };

    static sample inertia;

  public:
    sample operator(sample newval)
    {
      sample error = val - newval;
      sample diff = val - prev;
      prev = val;
      boost = min(1, max(0, 15 * abs(diff) - .1));
      val = val - (.3 + .7 * boost) * error * blockFactor;
      return val;
    }
};*/

class Inertia
{
  private:
    sample value{ 0.0 };
    static sample inertia;

  public:
    void setInertia(sample milliseconds, sample samplerate)
    {
      if (milliseconds > 0.01)
      {
        const sample seconds = milliseconds * .001;
        inertia = std::log(2.0) / (seconds * samplerate);
      } else inertia = -1.0;
    }

    sample operator() (sample newValue)
    {
      if (inertia < 0)
        return newValue;
      else
        return value += inertia * (newValue - value);
    }
};
sample Inertia::inertia = -1.0;

template<typename T>
class Multi
{
  public:
    Multi() { set_channels(2); };

    void set_channels(int channels)
    {
      if (channels != effect.size())
      {
        effect.clear();
        for (auto a = 0; a < channels; a++)
        {
          effect.push_back(T());
        }
      }
    }

    T& get(int N)
    {
      return effect[N];
    }

  private:
    std::vector<T> effect;
};
