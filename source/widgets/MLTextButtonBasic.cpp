
// mlvg: GUI library for madronalib apps and plugins
// Copyright (C) 2019-2022 Madrona Labs LLC
// This software is provided 'as-is', without any express or implied warranty.
// See LICENSE.txt for details.

#include "MLTextButtonBasic.h"

using namespace ml;

MessageList TextButtonBasic::processGUIEvent(const GUICoordinates& gc, GUIEvent e)
{
  MessageList r{};
  if(getBoolPropertyWithDefault("enabled", true))
  {
    Path actionRequestPath = Path("do", Path(getTextProperty("action")));
    
    auto type = e.type;
    auto bounds = getBounds();
    
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
        r.push_back(Message{actionRequestPath});
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

void TextButtonBasic::draw(ml::DrawContext dc)
{
  NativeDrawContext* nvg = getNativeContext(dc);
  Rect bounds = getLocalBounds(dc, *this);
  int gridSizeInPixels = dc.coords.gridSizeInPixels;
  
  NativeFontHandle fontHandle = getImageHandleResource(dc, "d_din");
  if(!isValid(fontHandle)) return;

  float opacity = getFloatPropertyWithDefault("opacity", 1.0f);
  auto markColor = multiplyAlpha(getColor(dc, "mark"), opacity);
  auto backgroundColor = multiplyAlpha(getColor(dc, "background"), opacity);
  
  constexpr float kDisabledAlpha{0.25f};
  if(!getBoolPropertyWithDefault("enabled", true))
  {
    markColor = multiplyAlpha(markColor, kDisabledAlpha);
    backgroundColor = multiplyAlpha(backgroundColor, kDisabledAlpha);
  }
  
  
  float strokeWidthMul = getFloatPropertyWithDefault("stroke_width", getFloat(dc, "common_stroke_width"));
  float strokeWidth = gridSizeInPixels*strokeWidthMul;
  float margin = gridSizeInPixels/8.f;
  float textSizeGrid = getFloatPropertyWithDefault("text_size", 0.6f);
  float textSize = gridSizeInPixels*textSizeGrid;

  auto textColor = _down ? backgroundColor : markColor;
  
  nvgBeginPath(nvg);
  nvgRoundedRect(nvg, shrink(bounds, margin), strokeWidth*2);
  if(_down)
  {
    // draw fill
    nvgFillColor(nvg, markColor);
    nvgFill(nvg);
  }
  else
  {
    // draw outline
    nvgStrokeWidth(nvg, strokeWidth);
    nvgStrokeColor(nvg, markColor);
    nvgStroke(nvg);
  }
  
  nvgFontFaceId(nvg, fontHandle);
  nvgFontSize(nvg, textSize);
  nvgFillColor(nvg, textColor);
  drawText(nvg, bounds.center() - Vec2(0, gridSizeInPixels/64.f), getTextProperty("text"), NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);

  return;
}
