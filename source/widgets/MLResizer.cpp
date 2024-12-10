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
  
  // resizer needs actual screen position otherwise it gets chaotic when we resize the window
  Vec2 eventScreenPos = e.screenPos;
  bool wasEngaged = engaged;
  
  // std::cout << "screen pos: " << eventScreenPos << "\n";

  if(type == "down")
  {
    engaged = true;

// TODO fix cross-platform coords and remove this
#if ML_MAC
    _sizeStart = gc.pixelToSystem(gc.viewSizeInPixels);
#elif ML_WINDOWS
    _sizeStart = (gc.viewSizeInPixels); 
#endif


    _dragStart = eventScreenPos;
    _dragDelta = Vec2(0, 0);

    //std::cout << "DOWN: viewSizeInPixels" << gc.viewSizeInPixels << "\n";
   // std::cout << "DOWN: _sizeStart" << _sizeStart << "\n\n";

    
    // this ValueChange does nothing but indicate that we got the event
    reqList.push_back({"ack"});
  }
  else if(type == "drag")
  {
    if(!engaged) return reqList;

    constexpr int kDragQuantum = 1;
    Vec2 newDragDelta = gc.pixelToSystem(eventScreenPos - _dragStart);

    newDragDelta.quantize(kDragQuantum);

    if (newDragDelta != _dragDelta)
    {
      _dragDelta = newDragDelta;

     // generate a view size change request,
     // and constrain to the fixed ratio if one exists
      Point newSize = _sizeStart + newDragDelta;
      if (getProperty("fix_ratio"))
      {
        float ratio = getFloatProperty("fix_ratio");
        float freeRatio = newSize.x()/ newSize.y();
        if (freeRatio < ratio)
        {
          float newY = newSize[1];
          newSize[0] = newY * ratio;       
        }
        else
        {
          float newX = newSize[0];
          newSize[1] = newX / ratio;
        }      
      }

      // send new size in system coordinates to the editor
      reqList.push_back(Message("set_param/view_size", pointToValue(newSize)));
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
  auto fillColor = engaged ? getColor(dc, "mark_bright") : getColor(dc, "mark");

  //  paint triangle
  nvgBeginPath(nvg);
  nvgMoveTo(nvg, bounds.right(), bounds.top());
  nvgLineTo(nvg, bounds.right(), bounds.bottom());
  nvgLineTo(nvg, bounds.left(), bounds.bottom());
  nvgClosePath(nvg);
  nvgFillColor(nvg, fillColor);
  nvgFill(nvg);
}

