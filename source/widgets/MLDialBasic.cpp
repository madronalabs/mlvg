// mlvg: GUI library for madronalib apps and plugins
// Copyright (C) 2019-2022 Madrona Labs LLC
// This software is provided 'as-is', without any express or implied warranty.
// See LICENSE.txt for details.

#include "MLDialBasic.h"
#include "MLDSPProjections.h"

using namespace ml;

// internals

constexpr int kScrollEngageMs{250};

float DialBasic::_trackPositionToNormalValue(Vec2 p)
{
  float r = getFloatPropertyWithDefault("size", 1.0f);
  float a0 = getFloatPropertyWithDefault("a0", kTwoPi*0.375f);
  float a1 = getFloatPropertyWithDefault("a1", kTwoPi);
  float centerDist = magnitude(p);

  // if far enough from center
  if (within(magnitude(p), 0.05f, r))
  {
    float angle = atan2 ((float) p.y(), (float) p.x());
    while (angle < a0)
      angle += kTwoPi;

    // if between start and end
    if (angle < a1)
    {
      const float proportion = (angle - a0) / (a1 - a0);
      return (ml::clamp (proportion, 0.0f, 1.0f));
    }
  }
  return -1.f;
}

float DialBasic::_quantizeNormalizedValue(float v)
{
  if(!hasProperty("detents"))
  {
    return v;
  }
  else if(_normDetents.size() == 1)
  {
    return _normDetents[0];
  }
  else
  {
    size_t minIdx{0};
    float minDist{std::numeric_limits<float>::max()};
    for (int i=0; i<_normDetents.size(); ++i)
    {
      float d = fabs(v - _normDetents[i]);
      if (d < minDist)
      {
        minIdx = i;
        minDist = d;
      }
    }
    
    return _normDetents[minIdx];
  }
}

// Widget implementation

void DialBasic::setupParams()
{
  // get normalized detents
  if(hasProperty("detents"))
  {
    Path pname{getTextProperty("param")};
    Matrix detents = getMatrixProperty("detents");
    
    // NOT getSize()! TODO look at removing Matrix property.
    // replace with float array maybe.
    size_t nDetents = detents.getWidth();
    
    _normDetents.resize(nDetents);
    for(int i=0; i<nDetents; ++i)
    {
      _normDetents[i] = _params.projections[pname].realToNormalized(detents[i]);
      
    }
  }
  
  Widget::setupParams();
}

MessageList DialBasic::processGUIEvent(const GUICoordinates& gc, GUIEvent e)
{
  constexpr float kComponentDragScale{-0.005f};
  constexpr float kScrollScale{-0.04f};
  const float kFineDragScale = getFloatPropertyWithDefault("fine_drag_scale", 0.1f);

  Path pname{getTextProperty("param")};
  Path paramRequestPath = Path("editor/set_param", pname);

  MessageList r{};

  if(!getBoolPropertyWithDefault("enabled", true)) return r;
  bool hasDetents = hasProperty("detents");
  
  auto type = e.type;

  // use coords centered on our bounding box for events
  Rect bounds = getBounds();
  Vec2 centeredPos = e.position - getCenter(bounds);

  Vec2 componentPosition = centeredPos*gc.gridSizeInPixels;
  bool doFineDrag = e.keyFlags & shiftModifier;
  
  float rawNormValue = getParamValue(pname).getFloatValue();
  
  if(type == "down")
  {
    float valueToSend{_params.getNormalizedFloatValue(pname)};
    if (!(e.keyFlags & commandModifier))
    {
      _dragY1 = componentPosition.y();

      // on click: if in track, jump
      float trackVal = _trackPositionToNormalValue(centeredPos);
      if(within(trackVal, 0.0f, 1.f))
      {
        float cookedNormValue = hasDetents ? _quantizeNormalizedValue(trackVal) : trackVal;
        setParamValue(pname, cookedNormValue);
        valueToSend = cookedNormValue;
      }
    }
    
    // cancel any engage timer from scrolling
    _scrollTimer.stop();
    
    // end any gesture in progress
    if(engaged)
    {
      _doEndScroll = false;
      r.push_back(Message{paramRequestPath, valueToSend, kMsgSequenceEnd});
      engaged = false;
    }

    // always push a sequence start message
    r.push_back(Message{paramRequestPath, valueToSend, kMsgSequenceStart});
    engaged = true;
  }
  else if(type == "drag")
  {
    // change value
    float dragY0 = componentPosition.y();
    float delta = dragY0 - _dragY1;
    _dragY1 = dragY0;

    if(delta != 0.f)
    {
      delta *= kComponentDragScale;
      if(doFineDrag) delta *= kFineDragScale;
      
      // keep track of raw value before quantize
      rawNormValue = clamp(rawNormValue + delta, 0.f, 1.f);
      
      float cookedNormValue = (hasDetents && !doFineDrag) ? _quantizeNormalizedValue(rawNormValue) : rawNormValue;
      float currentParamValue = _params.getNormalizedFloatValue(pname);
      
      if(cookedNormValue != currentParamValue)
      {
        setParamValue(pname, cookedNormValue);
        r.push_back(Message{paramRequestPath, cookedNormValue});
      }
    }
  }
  else if(type == "up")
  {
    Value valueToSend;
    if(e.keyFlags & commandModifier)
    {
      auto defaultVal = getNormalizedDefaultValue(_params, pname);
      setParamValue(pname, defaultVal);
      valueToSend = defaultVal;
    }
    else
    {
      valueToSend = _params.getNormalizedFloatValue(pname);
    };
      
    // if engaged, disengage and send a sequence end message
    if(engaged)
    {
      r.push_back(Message{paramRequestPath, valueToSend, kMsgSequenceEnd});
      engaged = false;
    }
  }
  else if(type == "scroll")
  {
    if(e.delta.y() != 0.f)
    {
      float scaledDelta = e.delta.y() * kScrollScale;
      if(doFineDrag) scaledDelta *= kFineDragScale;

      // keep track of raw value before quantize
      rawNormValue = clamp(rawNormValue + scaledDelta, 0.f, 1.f);
      float cookedNormValue = (hasDetents && !doFineDrag) ? _quantizeNormalizedValue(rawNormValue) : rawNormValue;
      float currentParamValue = _params.getNormalizedFloatValue(pname);
      
      if(cookedNormValue != currentParamValue)
      {
        setParamValue(pname, cookedNormValue);
        if(engaged)
        {
          _scrollTimer.postpone(milliseconds(kScrollEngageMs));
          r.push_back(Message{paramRequestPath, cookedNormValue});
        }
        else
        {
          // start timer to turn off the engage flag.
          // TODO find a good way to send kMsgSequenceEnd back to the editor when the timer expires-
          // even if we are not drawing.
          _scrollTimer.callOnce([&]()
                                { _doEndScroll = true; },
                                milliseconds(kScrollEngageMs)
                                );

          r.push_back(Message{paramRequestPath, cookedNormValue, kMsgSequenceStart});
          engaged = true;
        }
      }
    }
  }

  return r;
}

MessageList DialBasic::animate(int elapsedTimeInMs, ml::DrawContext dc)
{
  MessageList r;
  
  if(_doEndScroll)
  {
    _doEndScroll = false;
    if(engaged)
    {
      Path pname{getTextProperty("param")};
      Path paramRequestPath = Path("editor/set_param", pname);
      float val = _params.getNormalizedFloatValue(pname);
      r.push_back(Message{paramRequestPath, val, kMsgSequenceEnd});
      engaged = false;
    }
  }
  return r;
}

void DialBasic::draw(ml::DrawContext dc)
{
  // get parameter value
  Path paramName{getTextProperty("param")};
  auto currentNormalizedValue = _params.getNormalizedFloatValue(paramName);
  auto currentPlainValue = _params.getRealFloatValue(paramName);
  
  // get context and dimensions
  NativeDrawContext* nvg = getNativeContext(dc);
  int gridSizeInPixels = dc.coords.gridSizeInPixels;
  Rect bounds = getLocalBounds(dc, *this);
  
  // properties
  bool opaqueBg = getBoolPropertyWithDefault("opaque_bg", false);  
  bool bipolar = getBoolPropertyWithDefault("bipolar", false);
  bool enabled = getBoolPropertyWithDefault("enabled", true);
  float dialSize = getFloatPropertyWithDefault("size", 1.0f);
  float textScale = getFloatPropertyWithDefault("text_size",0.625);
  float normalizedValue = enabled ? currentNormalizedValue : 0.f;
  float opacity = enabled ? 1.0f : 0.25f;
  float strokeWidthMul = getFloatPropertyWithDefault("stroke_width", getFloat(dc, "common_stroke_width"));

  // colors
  auto markColor = multiplyAlpha(getColor(dc, "mark"), opacity);

  // angles
  const float kMinAngle = 0.01f;
  float a0, a1, a2, a3, a4; // angles for track start, track end, fill start, fill end, indicator
  if(bipolar)
  {
    a0 = getFloatPropertyWithDefault("a0", kTwoPi*0.375f);
    a1 = getFloatPropertyWithDefault("a1", kTwoPi);
    if(normalizedValue > 0.5f)
    {
      a2 = lerp(a0, a1, 0.5f);
      a3 = lerp(a0, a1, normalizedValue);
      a4 = a3;
    }
    else
    {
      a2 = lerp(a0, a1, normalizedValue);
      a3 = lerp(a0, a1, 0.5f);
      a4 = a2;
    }
  }
  else
  {
    a0 = getFloatPropertyWithDefault("a0", kTwoPi*0.375f);
    a1 = getFloatPropertyWithDefault("a1", kTwoPi);
    a2 = a0;
    a3 = lerp(a0, a1, normalizedValue);
    a4 = a3;
  }

  // radii
  float r0 = gridSizeInPixels*dialSize; // master size / radius
  float r1 = r0*0.85f; // outline radius
  float r4 = r0*1.00f; // ticks start
  float r5 = r0*1.06f; // ticks end

  // other sizes
  float strokeWidth = gridSizeInPixels*strokeWidthMul;
  float tickWidth = gridSizeInPixels/128.f;
  float textSize = gridSizeInPixels*dialSize*textScale;
  
  // indicator does not scale with dial size, just grid.
  // like a typographical stroke.
  const float kIndicatorWidth = strokeWidth;

  {
    // translate to center of bounding box for easy drawing calcs
    nvgSave(nvg);
    nvgTranslate(nvg, getCenter(bounds).getIntPart());
    
    auto indicatorColor = markColor;
    auto fillColor = multiplyAlpha(markColor, 0.33f);
    
    // fill
    if(enabled && (a3 - a2 > kMinAngle))
    {
      nvgBeginPath(nvg);
      nvgMoveTo(nvg, 0, 0);
      nvgLineTo(nvg, nvgAngle2Vec(a2)*r1);
      nvgArc(nvg, 0, 0, r1, a2, a3, NVG_CW);
      nvgClosePath(nvg);
      nvgFillColor(nvg, fillColor);
      nvgFill(nvg);
    }
    
    // indicator
    {
      nvgSave(nvg);
      float ixy = kIndicatorWidth/2.f;
      float cw = r1;
      float ch = r1*sinf(kPi/4) + cw;
      if(a4 < a0 + kPi/4)
      {
        // set scissor region to everything after angle a0
        nvgRotate(nvg, a0);
        nvgIntersectScissor(nvg, 0, 0, cw, ch);
        // rotate to indicator angle
        nvgRotate(nvg, a4 - a0);
      }
      else if(a4 > a1 - kPi/4)
      {
        // set scissor region to everything before angle a1
        nvgRotate(nvg, a1);
        nvgIntersectScissor(nvg, 0, -ch, cw, ch);
        // rotate to indicator angle
        nvgRotate(nvg, a4 - a1);
      }
      else
      {
        nvgRotate(nvg, a4);
      }
      nvgBeginPath(nvg);
      nvgMoveTo(nvg, 0, 0);
      nvgLineTo(nvg, ixy, -ixy);
      nvgLineTo(nvg, r1, -ixy);
      nvgLineTo(nvg, r1, ixy);
      nvgLineTo(nvg, ixy, ixy);
      nvgClosePath(nvg);
      nvgStrokeWidth(nvg, 0);
      nvgFillColor(nvg, indicatorColor);
      nvgFill(nvg);
      nvgRestore(nvg);
    }
 
    // outline arc
    {
      nvgStrokeColor(nvg, markColor);
      nvgStrokeWidth(nvg, strokeWidth);
      nvgBeginPath(nvg);
      nvgArc(nvg, 0, 0, r1 + strokeWidth*0.5f, a0, a1, NVG_CW);
      nvgStroke(nvg);
    }
    
    // ticks
    if(int nTicks = getIntPropertyWithDefault("ticks", 0))
    {
      auto tick2Angle = projections::linear( {0, nTicks - 1.f}, {a0, a1} );
      nvgStrokeColor(nvg, markColor);
      nvgStrokeWidth(nvg, tickWidth);
      nvgBeginPath(nvg);
      for(int t=0; t<nTicks; ++t)
      {
        auto a = tick2Angle(t);
        nvgMoveTo(nvg, nvgAngle2Vec(a)*r4);
        nvgLineTo(nvg, nvgAngle2Vec(a)*r5);
      }
      nvgStroke(nvg);
    }
    
    // detents
    if(hasProperty("detents"))
    {
      Matrix detents = getMatrixProperty("detents");
      size_t nDetents = detents.getSize();
      for(int i=0; i<nDetents; ++i)
      {
        float detentValuePlain = detents[i];
        float detentValueNorm = _params.projections[paramName].realToNormalized(detentValuePlain);
        auto a = lerp(a0, a1, detentValueNorm);
        nvgStrokeColor(nvg, markColor);
        nvgStrokeWidth(nvg, tickWidth);
        nvgBeginPath(nvg);
        nvgMoveTo(nvg, nvgAngle2Vec(a)*r4);
        nvgLineTo(nvg, nvgAngle2Vec(a)*r5);
        nvgStroke(nvg);
      }
    }
    
    // number
    if(enabled && getBoolPropertyWithDefault("draw_number", true))
    {
      float numWidth = textSize*0.45f; // approximate width of a number
      constexpr float kParamTransformSlop = 0.00001f;
      int digits(2);
      int precision(2);
      bool doSign{false};
      TextFragment numText;
      numText = textUtils::formatNumber(currentPlainValue, digits, precision, doSign);
      
      auto fontFace = getTextPropertyWithDefault("font", "d_din");
      NativeFontHandle fontHandle = getImageHandleResource(dc, Path(fontFace));
      if(isValid(fontHandle))
      {
        nvgFontFaceId(nvg, fontHandle);
        nvgFontSize(nvg, textSize);
        nvgTextLetterSpacing(nvg, 1.0f);
        nvgTextAlign(nvg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
        nvgFillColor(nvg, markColor);
        nvgText(nvg, -numWidth/4.f, r1*0.125f, numText.getText(), nullptr);
      }
    }
    
    // restore center offset
    nvgRestore(nvg);
  }
}
