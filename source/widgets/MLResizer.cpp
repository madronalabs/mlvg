// mlvg: GUI library for madronalib apps and plugins
// Copyright (C) 2019-2022 Madrona Labs LLC
// This software is provided 'as-is', without any express or implied warranty.
// See LICENSE.txt for details.

#include "MLResizer.h"
#include "MLDSPProjections.h"

using namespace ml;

MessageList Resizer::processGUIEvent(const GUICoordinates& gc, GUIEvent e)
{
  MessageList reqList;
  auto type = e.type;
  Vec2 viewPos = gc.gridToSystem(e.position);
  
  viewPos = e.screenPos;
  
  std::cout << " Resizer: screenPos " << e.screenPos << "\n";
  
  bool wasEngaged = engaged;

  if(type == "down")
  {
    engaged = true;
    _sizeStart = gc.pixelToSystem(gc.viewSizeInPixels);
    _dragStart = viewPos;
    _dragDelta = Vec2(0, 0);
    
    // this ValueChange does nothing but indicate that we got the event
    reqList.push_back({"ack"});
  }
  else if(type == "drag")
  {
    if(!engaged) return reqList;
    constexpr int kMinHeight = 64;
    constexpr int kDragQuantum = 1;
    Vec2 newDragDelta = viewPos - _dragStart;

    newDragDelta.quantize(kDragQuantum);

    if (newDragDelta != _dragDelta)
    {
      Vec2 newSize = _sizeStart + newDragDelta;

      if (getProperty("fix_ratio"))
      {
        // generate a view size change request,
        // constrained to the fixed ratio if one exists
        float ratio = getFloatProperty("fix_ratio");
        float freeRatio = newSize.x()/ newSize.y();
        Vec2 minSize = { kMinHeight*ratio, kMinHeight + 0.f };
        if (freeRatio < ratio)
        {
          float newY = newSize[1];
          if (newY > kMinHeight)
          {
            newSize[0] = newY * ratio;
          }
          else
          {
            newSize = minSize;
          }         
        }
        else
        {
          float newX = newSize[0];
          if (newX > kMinHeight*ratio)
          {
            newSize[1] = newX / ratio;
          }
          else
          {
            newSize = minSize;
          }
        }      
      }
      
      // send new size in system coordinates to the editor
      reqList.push_back(Message("set_param/view_size", vec2ToMatrix(newSize)));
      _dragDelta = newDragDelta;
    }
  }
  else if(type == "up")
  {
    engaged = false;
    // this ValueChange does nothing but indicate that we got the event
    reqList.push_back({"ack"});
  }
  
  if(wasEngaged != engaged) _dirty = true;

  return reqList;
}

void Resizer::draw(ml::DrawContext dc)
{
  NativeDrawContext* nvg = getNativeContext(dc);
  Rect bounds = getLocalBounds(dc, *this);

  // TODO color property
  auto fillColor = engaged ? colors::gray8 : getColor(dc, "mark");

  //  paint triangle
  nvgBeginPath(nvg);
  nvgMoveTo(nvg, bounds.right(), bounds.top());
  nvgLineTo(nvg, bounds.right(), bounds.bottom());
  nvgLineTo(nvg, bounds.left(), bounds.bottom());
  nvgClosePath(nvg);
  nvgFillColor(nvg, fillColor);
  nvgFill(nvg);
}

