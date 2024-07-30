// mlvg test application
// Copyright (C) 2019-2022 Madrona Labs LLC
// This software is provided 'as-is', without any express or implied warranty.
// See LICENSE.txt for details.

#include "madronalib.h"


constexpr int kInputChannels = 0;
constexpr int kOutputChannels = 2;
constexpr int kSampleRate = 48000;
constexpr float kDefaultGain = 0.1f;
constexpr float kMaxGain = 0.5f;
constexpr float kFreqLo = 40, kFreqHi = 4000;



inline void readParameterDescriptions(ParameterDescriptionList& params)
{
    // Processor parameters
    params.push_back(std::make_unique< ParameterDescription >(WithValues{
      { "name", "freq1" },
      { "range", { kFreqLo, kFreqHi } },
      { "log", true },
      { "units", "Hz" },
      { "default", 0.75 }
        }));

    params.push_back(std::make_unique< ParameterDescription >(WithValues{
      { "name", "freq2" },
      { "range", { kFreqLo, kFreqHi } },
      { "log", true },
      { "units", "Hz" }
      // no default here means the normalized default will be 0.5 (400 Hz)
        }));

    params.push_back(std::make_unique< ParameterDescription >(WithValues{
      { "name", "gain" },
      { "range", {0, kMaxGain} },
      { "plaindefault", kDefaultGain }
        }));

    // Controller parameters
    params.push_back(std::make_unique< ParameterDescription >(WithValues{
      { "name", "view_size" },
      { "save_in_controller", true }
        }));
}