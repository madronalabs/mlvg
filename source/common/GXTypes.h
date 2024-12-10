
// mlvg: GUI library for madronalib apps and plugins
// Copyright (C) 2019-2022 Madrona Labs LLC
// This software is provided 'as-is', without any express or implied warranty.
// See LICENSE.txt for details.

#pragma once

#include "MLMath2D.h"

using namespace ml;

namespace gx {

// Point

using Point = Vec2;

inline Value pointToValue(Point p){ return valueFromPODType<Point>(p); }

// Rect

using Rect = ml::Rect;

// Color

using Color = NVGcolor;

constexpr inline Color rgba(float pr, float pg, float pb, float pa)
{
  return NVGcolor{ {{pr, pg, pb, pa}} };
}

constexpr inline Color rgba(const uint32_t hexRGB)
{
  int r = (hexRGB & 0xFF0000) >> 16;
  int g = (hexRGB & 0xFF00) >> 8;
  int b = (hexRGB & 0xFF);
  return NVGcolor{ {{r / 255.f, g / 255.f, b / 255.f, 1.0f}} };
}


} // namespace ml

