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
  viewPos = vmax(viewPos, Vec2());

  if(type == "down")
  {
    _sizeStart = gc.pixelToSystem(gc.viewSizeInPixels);
    _dragStart = viewPos;
    _dragDelta = Vec2(0, 0);
    
    // this ValueChange does nothing but indicate that we got the event
    reqList.push_back({"ack"});

  }
  else if(type == "drag")
  {
    constexpr int kMinHeight = 64;
    constexpr int kDragQuantum = 4;
    Vec2 newDragDelta = viewPos - _dragStart;

    newDragDelta.quantize(kDragQuantum);

    if (newDragDelta != _dragDelta)
    {
      Vec2 newSize = _sizeStart + newDragDelta;

      if (getProperty("fix_ratio"))
      {
        // constrain to ratio
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
  
  return reqList;
}

void Resizer::draw(ml::DrawContext dc)
{
  NativeDrawContext* nvg = getNativeContext(dc);
  Rect bounds = getLocalBounds(dc, *this);
  
  auto bgColorA = getColor(dc, "mark");

  //  paint triangle
  nvgBeginPath(nvg);
  nvgMoveTo(nvg, bounds.right(), bounds.top());
  nvgLineTo(nvg, bounds.right(), bounds.bottom());
  nvgLineTo(nvg, bounds.left(), bounds.bottom());
  nvgClosePath(nvg);
  nvgFillColor(nvg, bgColorA);
  nvgFill(nvg);
}

