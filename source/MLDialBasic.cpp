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
      _normDetents[i] = _params.projections[pname].plainToNormalized(detents[i]);
    }
  }
  
  Widget::setupParams();
}

void DialBasic::sendPopupRequestDetails(MessageList& r, GUIEvent e)
{
  constexpr float kClickAndHoldMs{1000};
  
  Path pname{getTextProperty("param")};
  
  // tell controller which param to use for midi learn
  r.push_back(Message("controller/set_prop/learn_param", pathToText(pname)));
  
  // first, tell editor which modal widget we want
  r.push_back(Message("editor/do/set_popup_widget", "popup"));

  // tell editor which param we want to control with the modal widget
  r.push_back(Message("editor/set_popup_prop/modal_param", pathToText(pname)));
  
  // set target bounds, used by the view to calculate popup position
  r.push_back(Message("editor/set_popup_prop/target_bounds", rectToMatrix(getBounds(*this))));
  
  // send click center to editor for UI use
  r.push_back(Message("editor/set_popup_prop/target_click_position", vec2ToMatrix(e.position)));

  // set color for pre-open animation
  auto indicatorColor = getColorPropertyWithDefault("indicator", rgba(0.8, 0.8, 0.8, 1.0));
  r.push_back(Message("editor/set_popup_prop/indicator_color", colorToMatrix(indicatorColor)));
  
  // send the click and hold interval to the popup so it can animate correctly
  r.push_back(Message("editor/set_popup_prop/click_and_hold_time", kClickAndHoldMs));
}

void DialBasic::startDelayedPopupOpen(MessageList& r, GUIEvent e)
{
  sendPopupRequestDetails(r, e);
  r.push_back(Message("editor/do/request_popup", false));
}

void DialBasic::cancelDelayedPopupOpen(MessageList& r)
{
  r.push_back(Message("editor/do/cancel_popup"));
}

void DialBasic::doImmediatePopupOpen(MessageList& r, GUIEvent e)
{
  sendPopupRequestDetails(r, e);
  r.push_back(Message("editor/do/request_popup", true));
}

MessageList DialBasic::processGUIEvent(const GUICoordinates& gc, GUIEvent e)
{
  constexpr float kComponentDragScale{-0.005f};
  constexpr float kScrollScale{-0.04f};
  constexpr float kFineDragScale{0.1f}; // TODO different fine drag behavior based on parameter values

  Path pname{getTextProperty("param")};
  Path paramRequestPath = Path("editor/set_param", pname);

  MessageList r{};

  if(!getBoolPropertyWithDefault("enabled", true)) return r;
  bool hasDetents = hasProperty("detents");
  
  auto type = e.type;

  // use coords centered on our bounding box for events
  Rect bounds = getBounds(*this);
  Vec2 centeredPos = e.position - getCenter(bounds);

  Vec2 componentPosition = centeredPos*gc.gridSize;
  bool doFineDrag = e.keyFlags & shiftModifier;
  
  if(type == "down")
  {
    float valueToSend{getNormalizedValue(_params, pname)};
    if (!(e.keyFlags & commandModifier))
    {
      _dragY1 = componentPosition.y();

      // on click: if in track, jump
      float trackVal = _trackPositionToNormalValue(centeredPos);
      if(within(trackVal, 0.0f, 1.f))
      {
        //_rawNormValue = trackVal;
        //float cookedNormValue = hasDetents ? _quantizeNormalizedValue(_rawNormValue) : _rawNormValue;
        float cookedNormValue = hasDetents ? _quantizeNormalizedValue(trackVal) : trackVal;
        setParamValue(pname, cookedNormValue);
        _rawNormValue = cookedNormValue;
        valueToSend = cookedNormValue;
      }
      
      // trigger editor popup sequence
      
      // TODO add a way to share functional packages of messages in any Widget.
      // if we have the trigger_popup package, we call several different functions based on
      // incoming UI messages.
      
      if(getBoolProperty("trigger_popup"))
      {
        if (!(e.keyFlags & controlModifier))
        {
          // open popup after a delay
          // see animate() for countdown timer code
          startDelayedPopupOpen(r, e);
          
          // store click position in order to cancel on a large move during pre-open
          _clickAndHoldStartPosition = componentPosition;
        }
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
    
  
    // TEMP
    // std::cout << "DialBasic drag pos: " << dragY0 << "  delta: " << delta << "\n";

    if(delta != 0.f)
    {
      delta *= kComponentDragScale;
      if(doFineDrag) delta *= kFineDragScale;
      
      // keep track of raw value before quantize
      _rawNormValue = clamp(_rawNormValue + delta, 0.f, 1.f);
      
      float cookedNormValue = (hasDetents && !doFineDrag) ? _quantizeNormalizedValue(_rawNormValue) : _rawNormValue;
      float currentParamValue = getNormalizedValue(_params, pname);
      
      if(cookedNormValue != currentParamValue)
      {
        setParamValue(pname, cookedNormValue);
        r.push_back(Message{paramRequestPath, cookedNormValue});
      }
    }
    
    if(getBoolProperty("trigger_popup"))
    {
      // cancel popup on sufficiently large drag
      Vec2 dragVec = componentPosition - _clickAndHoldStartPosition;
      float dv = magnitude(dragVec);
      
      constexpr float kPopupCancelDistance{5.0f};
      if(dv > kPopupCancelDistance)
      {
        cancelDelayedPopupOpen(r);
      }
    }
  }
  else if(type == "up")
  {
    float valueToSend{getNormalizedValue(_params, pname)};
    if(e.keyFlags & commandModifier)
    {
      // set parameter to default
      float defaultValue = _params.descriptions[pname]->getFloatPropertyWithDefault("default", 0.5f);
      setParamValue(pname, defaultValue);
      _rawNormValue = defaultValue;
      valueToSend = defaultValue;
    }
      
    // if engaged, disengage and send a sequence end message
    if(engaged)
    {
      r.push_back(Message{paramRequestPath, valueToSend, kMsgSequenceEnd});
      engaged = false;
    }
        
    if(getBoolProperty("trigger_popup"))
    {
      if (e.keyFlags & controlModifier)
      {
        doImmediatePopupOpen(r, e);
      }
      else
      {
        cancelDelayedPopupOpen(r);
      }
    }
  }
  else if(type == "scroll")
  {
    if(e.delta.y() != 0.f)
    {
      float scaledDelta = e.delta.y() * kScrollScale;
      if(doFineDrag) scaledDelta *= kFineDragScale;

      // keep track of raw value before quantize
      _rawNormValue = clamp(_rawNormValue + scaledDelta, 0.f, 1.f);
      float cookedNormValue = (hasDetents && !doFineDrag) ? _quantizeNormalizedValue(_rawNormValue) : _rawNormValue;
      float currentParamValue = getNormalizedValue(_params, pname);
      
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

void DialBasic::processSignal(DSPVector sig, size_t channel)
{
  constexpr float eps{0.0001f};
  
  // this dial does not draw a full waveform, just one modulated value

  Path paramName{getTextProperty("param")};
  float newValue = _params.projections[paramName].plainToNormalized(sig[0]);
  
  if(fabs(_indicatorNormalizedValue - newValue) > eps)
  {
    _indicatorNormalizedValue = newValue;
    _dirty = true;
  }
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
      r.push_back(Message{paramRequestPath, _params[pname], kMsgSequenceEnd});
      engaged = false;
    }
  }
  return r;
}

void DialBasic::draw(ml::DrawContext dc)
{
  Path paramName{getTextProperty("param")};
  auto currentNormalizedValue = getNormalizedValue(_params, paramName);
  auto currentPlainValue = getPlainValue(_params, paramName);
  
  if(isNaN( currentPlainValue))
  {
    std::cout << "nan\v";
  }
  
  NativeDrawContext* nvg = getNativeContext(dc);
  int gridSize = dc.coords.gridSize;
  Rect bounds = getLocalBounds(dc, *this);
  
  //auto& resources = getResources(dc);
  const float kMinAngle = 0.01f;

  // properties
  float opacity = getFloatPropertyWithDefault("opacity", 1.0f);

  bool opaqueBg = getBoolPropertyWithDefault("opaque_bg", false);
  
  bool bipolar = getBoolPropertyWithDefault("bipolar", false);
  bool enabled = getBoolPropertyWithDefault("enabled", true);
  if(!enabled) opacity *= 0.25f;
  bool hasSignal = hasProperty("signal_name");
  bool outline = getBoolPropertyWithDefault("outline", true);
  float dialSize = getFloatPropertyWithDefault("size", 1.0f);
  float featureScale = getFloatPropertyWithDefault("feature_scale", 1.0f);
  
  float textScale = getFloatPropertyWithDefault("text_size", getFloat(dc, "dial_text_size"));

  // colors
  auto markColor = multiplyAlpha(getColor(dc, "mark"), opacity);

  
  float normalizedValue = enabled ? currentNormalizedValue : 0.f;
  
  float a0, a1, a2, a3, a4; // angles for track start, track end, fill start, fill end, indicator
  if(bipolar)
  {
    a0 = getFloatPropertyWithDefault("a0", kTwoPi*0.375f);
    a1 = getFloatPropertyWithDefault("a1", kTwoPi);
    if(normalizedValue > 0.5f)
    {
      a2 = lerp(a0, a1, 0.5f);
      a3 = lerp(a0, a1, normalizedValue);
      a4 = hasSignal ? lerp(a0, a1, _indicatorNormalizedValue) : a3;
    }
    else
    {
      a2 = lerp(a0, a1, normalizedValue);
      a3 = lerp(a0, a1, 0.5f);
      a4 = hasSignal ? lerp(a0, a1, _indicatorNormalizedValue) : a2;
    }
  }
  else
  {
    a0 = getFloatPropertyWithDefault("a0", kTwoPi*0.375f);
    a1 = getFloatPropertyWithDefault("a1", kTwoPi);
    a2 = a0;
    a3 = lerp(a0, a1, normalizedValue);
    a4 = hasSignal ? lerp(a0, a1, _indicatorNormalizedValue) : a3;
  }
  
  //auto& testMatrix = *resources["test"];
  //  std::cout << "test: " <<  testMatrix << "\n";

//  float strokeWidth = outline ? gridSize/48.f*featureScale : 0;
  float strokeWidth = gridSize/64.f*featureScale;
  float tickWidth = gridSize/256.f*featureScale;
  float textSize = gridSize*dialSize*textScale;
  
  // indicator does not scale with dial size, just grid.
  // like a typographical stroke.
  // Aaltoverb: const float kIndicatorWidth = gridSize/12.f;
  const float kIndicatorWidth = strokeWidth*1.5f;//gridSize/48.f*featureScale;
  float shadowDepth = gridSize/48.f*featureScale;

  float r0 = gridSize*dialSize; // master size / radius
  float r1 = r0*0.85f; // outline radius
  float r4 = r0*1.00f; // ticks start
  float r5 = r0*1.06f; // ticks end
  
  //float r6 = r0*0.125f; // inner shadow line thickness
  
  float numWidth = textSize*0.45f; // approximate width of a number
  float numHeight = textSize; // approximate height of a number

  {
    // translate to center for easy drawing calcs
    nvgSave(nvg);
    nvgTranslate(nvg, getCenter(bounds).getIntPart());
    
    auto indicatorColor = markColor;
    //auto trackColor = multiplyAlpha(markColor, opacity);
    auto fillColor = multiplyAlpha(markColor, 0.25f);
    

    // draw background fill. (fill color paints over this)
    bool wholeCircle = fabs(fmod(a0, kTwoPi) - fmod(a1, kTwoPi)) < kMinAngle;

          
    // fill color
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
    if(enabled)
    {
      nvgSave(nvg);
      float ixy = kIndicatorWidth/2.f;
      float cw = r1;
      float ch = r1*sinf(kPi/4) + cw;
      if(a4 < a0 + kPi/4)
      {
        // allow everything after angle a0
        nvgRotate(nvg, a0);
        nvgIntersectScissor(nvg, 0, 0, cw, ch);
        // rotate to indicator angle
        nvgRotate(nvg, a4 - a0);
      }
      else if(a4 > a1 - kPi/4)
      {
        // allow everything before angle a1
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
      // show indicator scissor rect
      if(0)
      {
       nvgBeginPath(nvg);
       nvgRect(nvg, -10000, -10000, 20000, 20000);
       nvgFillColor(nvg, nvgRGBA(255, 64, 64, 64));
       nvgFill(nvg);
      }
      nvgRestore(nvg);
    }
 
    // outline arc
    if(outline)
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
        float detentValueNorm = _params.projections[paramName].plainToNormalized(detentValuePlain);
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
      constexpr float kParamTransformSlop = 0.00001f;
      int digits(2);
      int precision(2);
      bool doSign{false};
      TextFragment numText;
      
      bool maxInf = _params.descriptions[paramName]->getProperty("maxinfinity").getBoolValueWithDefault(false);
      bool atInfinity{false};
      auto paramName = getTextProperty("param");

      if(maxInf)
      {
        auto range = _params.descriptions[paramName]->getProperty("range").getMatrixValue();
        if(currentPlainValue >= range[1] - kParamTransformSlop)
        {
          atInfinity = true;
        }
      }
      
      if(!atInfinity)
      {
        numText = textUtils::formatNumber(currentPlainValue, digits, precision, doSign);
        nvgFontFaceId(nvg, 0);
        nvgFontSize(nvg, textSize);
        nvgTextLetterSpacing(nvg, 1.0f);
        nvgTextAlign(nvg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
        nvgFillColor(nvg, markColor);
        nvgText(nvg, -numWidth/4.f, r1*0.125f, numText.getText(), nullptr);
      }
      else
      {
        auto image = getVectorImage(dc, "infinity");
        if(image)
        {
          float imgSrcWidth = image->width;
          float imageDestWidth = numWidth*3.5;
          float imgScale = imageDestWidth/imgSrcWidth;
          nvgTranslate(nvg, -numWidth/4.f, r1*0.1875f  );
          nvgScale(nvg, imgScale, imgScale);
          nvgDrawSVG(nvg, image->_pImage);
        }
      }
    }
    
    // restore center offset
    nvgRestore(nvg);
  }
}



// TODO we can probably get rid of this

void DialBasic::handleMessage(Message msg, Message*)
{
  switch(hash(head(msg.address)))
  {
    case(hash("set_param")):
    {
      // handle normalized parameter value
      setParamValue(tail(msg.address), msg.value);
      _rawNormValue = msg.value.getFloatValue();
      _dirty = true;
      break;
    }
    case(hash("set_prop")):
    {
      setProperty(tail(msg.address), msg.value);
      _dirty = true;
      break;
    }
    case(hash("do")):
    {
      // default widget: nothing to do
      break;
    }
    default:
    {
      // default widget: no forwarding
      break;
    }
  }
}
