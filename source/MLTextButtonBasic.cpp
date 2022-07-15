
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
  int gridSize = dc.coords.gridSize;
  
  NativeFontHandle fontHandle = getImageHandleResource(dc, "madronasans");
  if(!isValid(fontHandle)) return;
  
  float buttonDownShift = _down ? gridSize/32.f : 0.f;
  nvgTranslate(nvg, Vec2(0, buttonDownShift));
  
  // TEMP TODO add opacity at the View level
  
  float opacity = getFloatPropertyWithDefault("opacity", 1.0f);
  opacity *= getBoolPropertyWithDefault("enabled", true) ? 1.f : 0.25f;

  auto markColor = multiplyAlpha(getColor(dc, "mark"), opacity);
  
  float textSize = gridSize*getFloatPropertyWithDefault("text_size", 0.5f);
  
  auto hAlign = NVG_ALIGN_LEFT;
  auto vAlign = NVG_ALIGN_TOP;
  float textX = bounds.left();
  float textY = bounds.top();
  
  hAlign = NVG_ALIGN_CENTER;
  textX = bounds.center().x();
  vAlign = NVG_ALIGN_MIDDLE;
  textY = bounds.center().y();

  nvgFontFaceId(nvg, fontHandle);
  nvgFontSize(nvg, textSize);
  nvgTextLetterSpacing(nvg, textSize*0.00f);
  nvgFillColor(nvg, markColor);
  
  drawText(nvg, {textX, textY}, getTextProperty("text"), hAlign | vAlign);
}
