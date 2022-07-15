// mlvg: GUI library for madronalib apps and plugins
// Copyright (C) 2019-2022 Madrona Labs LLC
// This software is provided 'as-is', without any express or implied warranty.
// See LICENSE.txt for details.

#include "MLPanel.h"

using namespace ml;

void Panel::draw(ml::DrawContext dc)
{
  NativeDrawContext* nvg = getNativeContext(dc);
  Rect bounds = getLocalBounds(dc, *this);
  
  bool enabled = getBoolPropertyWithDefault("enabled", true);
  if(enabled)
  {
    // draw panel
    auto color = getColorPropertyWithDefault("color", getColor(dc, "panel_bg"));

    nvgFillColor(nvg, color);
    nvgBeginPath(nvg);
    nvgRect(nvg, bounds);
    nvgFill(nvg);
  }
}


