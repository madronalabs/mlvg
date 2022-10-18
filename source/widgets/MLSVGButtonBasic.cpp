// mlvg: GUI library for madronalib apps and plugins
// Copyright (C) 2019-2022 Madrona Labs LLC
// This software is provided 'as-is', without any express or implied warranty.
// See LICENSE.txt for details.

#include "MLSVGButtonBasic.h"

using namespace ml;

MessageList SVGButton::processGUIEvent(const GUICoordinates& gc, GUIEvent e)
{
  MessageList r{};
  if(getBoolPropertyWithDefault("enabled", true))
  {
    Path actionRequestPath = Path("do", Path(getTextProperty("action")));

    auto type = e.type;
    auto bounds = getBounds();
    bool hit = within(e.position, bounds);

    bool wasDown = _down;

    if(type == "down")
    {
      _down = true;
      if(hit)
      {
        // push acknowledge so that we get the _stilldown set in the parent view
        r.push_back(Message{"ack"});
      }
    }
    else if(type == "up")
    {
      _down = false;
      if(hit)
      {
        // request an action
        r.push_back(Message{actionRequestPath});
      }
    }
    else if(type == "drag")
    {
      _down = hit;
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

void SVGButton::draw(ml::DrawContext dc)
{
  NativeDrawContext* nvg = getNativeContext(dc);
  const int gridSizeInPixels = dc.coords.gridSizeInPixels;
  Rect bounds = getLocalBounds(dc, *this);

  float buttonDownShiftAmount = gridSizeInPixels/32.f;
  float buttonDownShift = _down ? buttonDownShiftAmount : 0.f;
  
  nvgTranslate(nvg, Vec2(0, buttonDownShift));
  
  float opacity = getFloatPropertyWithDefault("opacity", 1.0f);
  opacity *= getBoolPropertyWithDefault("enabled", true) ? 1.f : 0.25f;
  
  nvgSave(nvg);
  if(opacity < 1.0f) { nvgGlobalAlpha(nvg, opacity); }
  
  auto image = getVectorImage(dc, getTextProperty("image"));
  
  if(image)
  {
    // get max rectangle for SVG image
    float imageAspect = image->width/image->height;
    float boundsAspect = bounds.width()/bounds.height();
    
    float imgScale;
    Vec2 imageSize{image->width, image->height};
    
    if(imageAspect >= boundsAspect)
    {
      imgScale = bounds.width()/imageSize.x();
    }
    else
    {
      imgScale = bounds.height()/imageSize.y();
    }

    nvgTranslate(nvg, getCenter(bounds) - imageSize*imgScale/2);
    nvgScale(nvg, imgScale, imgScale);
    nvgDrawSVG(nvg, image->_pImage);
  }
  else
  {
    NativeFontHandle fontHandle = getImageHandleResource(dc, "d_din");
    if(!isValid(fontHandle)) return;
    float textSize = gridSizeInPixels*0.5f;
    nvgFontFaceId(nvg, fontHandle);
    nvgFontSize(nvg, textSize);
    nvgFillColor(nvg, getColor(dc, "mark"));
    drawText(nvg, getCenter(bounds), "?", NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
  }
   nvgRestore(nvg);
}
