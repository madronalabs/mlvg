// mlvg: GUI library for madronalib apps and plugins
// Copyright (C) 2019-2022 Madrona Labs LLC
// This software is provided 'as-is', without any express or implied warranty.
// See LICENSE.txt for details.

#include "MLToggleButtonBasic.h"

using namespace ml;



// TODO full events handling - drag


MessageList ToggleButtonBasic::processGUIEvent(const GUICoordinates& gc, GUIEvent e)
{
  MessageList r{};
  
  Path paramName{getTextProperty("param")};
  auto currentNormalizedValue = getNormalizedValue(_params, paramName);
  bool currentValue = currentNormalizedValue > 0.5f;
  
  if(getBoolPropertyWithDefault("enabled", true))
  {
    Path paramPath{getTextProperty("param")};
    
    auto type = e.type;
    auto bounds = getBounds(*this);
    
    bool wasDown = _down;

    if(type == "down")
    {
      _down = true;
      if(within(e.position, bounds))
      {
        // push acknowledge so that we get the _stilldown set in the parent view
        r.push_back(Message{"ack"});
      }
    }
    else if(type == "up")
    {
      _down = false;
      if(within(e.position, bounds))
      {
        bool newValue = !currentValue;
        setParamValue(paramPath, newValue);
        r.push_back(Message{paramPath, newValue});
      }
    }
    else if(type == "drag")
    {
      _down = within(e.position, bounds);
    }

    if(wasDown != _down)
    {
      _dirty = true;
    }
  }
  else
  {
    // push ack to eat the event if not enabled
    r.push_back(Message{"ack"});
  }
  return r;
}


void ToggleButtonBasic::draw(ml::DrawContext dc)
{
  Path paramName{getTextProperty("param")};
  auto currentNormalizedValue = getNormalizedValue(_params, paramName);
  bool currentValue = currentNormalizedValue > 0.5f;

  NativeDrawContext* nvg = getNativeContext(dc);
  const int gridSize = dc.coords.gridSize;
  Rect bounds = getLocalBounds(dc, *this);

  bool enabled = getBoolPropertyWithDefault("enabled", true);
  float opacity = getFloatPropertyWithDefault("opacity", 1.0f);
  opacity *= enabled ? 1.f : 0.25f;
  float buttonSize = getFloatPropertyWithDefault("size", 0.125f);

  // colors
  auto markColor = multiplyAlpha(getColor(dc, "mark"), opacity);
  auto fillColor = multiplyAlpha(getColorProperty("color"), 0.75f*opacity);
  auto indicatorColor = multiplyAlpha(getColorProperty("indicator"), opacity);
  auto fillColor2 = multiplyAlpha(getColorPropertyWithDefault("color2", nvgLerpRGBA(fillColor, indicatorColor, 0.75f)), opacity);

  // dimensions
  float strokeWidth = gridSize/64.f;
  float localSize = gridSize*buttonSize;
  Rect sizeRect {0, 0, localSize, localSize};
  Rect buttonRect = alignCenterToRect(sizeRect, bounds);

  {
    // nudge upwards from center for alignment
    nvgTranslate(nvg, {0, -gridSize/32.f});

    // fill
    nvgBeginPath(nvg);
    nvgRect(nvg, buttonRect);
    if(!currentValue)
    {
      auto trackColor = getColor(dc, "track");
      nvgFillColor(nvg, trackColor);
      nvgFill(nvg);
    }
    else
    {
      auto fillGradient = nvgLinearGradient(nvg, 0, buttonRect.top(), 0, buttonRect.bottom(), fillColor2, fillColor );
      nvgFillPaint(nvg, fillGradient);
      nvgFill(nvg);
    }
    
    // outline
    if(enabled)
    {
      nvgStrokeColor(nvg, markColor);
      nvgStrokeWidth(nvg, strokeWidth);
      nvgBeginPath(nvg);
      nvgRect(nvg, buttonRect);
      nvgStroke(nvg);
    }
  }
}
