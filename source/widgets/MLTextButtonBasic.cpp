
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
  
  float buttonDownShift = _down ? gridSizeInPixels/32.f : 0.f;
  nvgTranslate(nvg, Vec2(0, buttonDownShift));
  
  float opacity = getFloatPropertyWithDefault("opacity", 1.0f);
  opacity *= getBoolPropertyWithDefault("enabled", true) ? 1.f : 0.25f;
  auto markColor = multiplyAlpha(getColor(dc, "mark"), opacity);
  
  float strokeWidth = gridSizeInPixels/64.f;
  float margin = gridSizeInPixels/16.f;
  float textSize = gridSizeInPixels*getFloatPropertyWithDefault("text_size", 0.5f);
  
  auto hAlign = NVG_ALIGN_CENTER;
  auto vAlign = NVG_ALIGN_MIDDLE;
  float textX = bounds.center().x();
  float textY = bounds.center().y();

  nvgFontFaceId(nvg, fontHandle);
  nvgFontSize(nvg, textSize);
  nvgTextLetterSpacing(nvg, textSize*0.00f);
  nvgFillColor(nvg, markColor);
  drawText(nvg, {textX, textY}, getTextProperty("text"), hAlign | vAlign);
  
  nvgBeginPath(nvg);
  nvgRect(nvg, shrink(bounds, margin));
  nvgStrokeWidth(nvg, strokeWidth);
  nvgStrokeColor(nvg, markColor);
  nvgStroke(nvg);
  return;
}
