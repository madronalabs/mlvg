// mlvg: GUI library for madronalib apps and plugins
// Copyright (C) 2019-2022 Madrona Labs LLC
// This software is provided 'as-is', without any express or implied warranty.
// See LICENSE.txt for details.

#include "MLTextLabelBasic.h"
#include "MLDSPProjections.h"

using namespace ml;

void TextLabelBasic::draw(ml::DrawContext dc)
{
  NativeDrawContext* nvg = getNativeContext(dc);
  Rect bounds = getLocalBounds(dc, *this);
  int gridSizeInPixels = dc.coords.gridSizeInPixels;
  
  auto fontFace = getTextPropertyWithDefault("font", "d_din");
  auto text = getTextProperty("text");
  
  auto font = getFontResource(dc, Path(fontFace));
  if(!font) return;
  
  float textSize = gridSizeInPixels*getFloatPropertyWithDefault("text_size", 0.25f);
  float spacing = getFloatPropertyWithDefault("text_spacing", 0.f);
  bool multiLine = getBoolProperty("multi_line");

  float opacity = getFloatPropertyWithDefault("opacity", 1.f);
  auto tc = getColorPropertyWithDefault("text_color", getColor(dc, "mark"));
  auto textColor = multiplyAlpha(tc, opacity);

  auto hAlign = NVG_ALIGN_LEFT;
  auto vAlign = NVG_ALIGN_TOP;
  float textX = bounds.left();
  float textY = bounds.top();

  auto hProp = getTextProperty("h_align");
  if(hProp == "center")
  {
    hAlign = NVG_ALIGN_CENTER;
    textX = bounds.center().x();
  }
  else if(hProp == "right")
  {
    hAlign = NVG_ALIGN_RIGHT;
    textX = bounds.right();
  }

  auto vProp = getTextProperty("v_align");
  if(vProp == "middle")
  {
    vAlign = NVG_ALIGN_MIDDLE;
    textY = bounds.center().y();
  }
  else if(vProp == "bottom")
  {
    vAlign = NVG_ALIGN_BOTTOM;
    textY = bounds.bottom();
  }

  nvgFontFaceId(nvg, font->handle);
  nvgFontSize(nvg, textSize);
  nvgTextLetterSpacing(nvg, textSize*spacing);
  nvgFillColor(nvg, textColor);
  
  if(multiLine)
  {
    drawTextBox(nvg, {textX, textY}, bounds.width(), text, hAlign | vAlign);
  }
  else
  {
    drawText(nvg, {textX, textY}, text, hAlign | vAlign);
  }
  
  constexpr bool kShowBounds{false};
  if(kShowBounds)
  {
    // TEST
    nvgBeginPath(nvg);
    nvgStrokeColor(nvg, rgba(1, 1, 1, 1.0));
    nvgStrokeWidth(nvg, 6);
    nvgRect(nvg, bounds);
    nvgStroke(nvg);
  }
  
}
