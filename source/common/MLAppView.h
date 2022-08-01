
// mlvg: GUI library for madronalib apps and plugins
// Copyright (C) 2019-2022 Madrona Labs LLC
// This software is provided 'as-is', without any express or implied warranty.
// See LICENSE.txt for details.


#pragma once

#include "MLActor.h"
#include "MLDrawContext.h"
#include "MLGUIEvent.h"
#include "MLView.h"
#include "MLWidget.h"

namespace ml {

class AppView : public Actor
{
public:
  AppView()
  {
    _view = ml::make_unique< View > (_rootWidgets, WithValues{});
  }
  
  virtual ~AppView() {}
  
  virtual void renderView(NativeDrawContext* nvg, Layer* backingLayer) = 0;

  // set the size of the view in system coordinates.
  virtual void viewResized(NativeDrawContext* nvg, int w, int h) = 0;

  // push a GUI event to the queue of input events.
  virtual void pushEvent(GUIEvent g) = 0;

  // initialize resources such as images, needed to draw the View.
  virtual void initializeResources(NativeDrawContext* nvg) = 0;
  
  void createVectorImage(Path newImageName, const unsigned char* dataStart, size_t dataSize);

  void setDisplayScale(float scale) {_GUICoordinates.displayScale = scale;}
  float getDisplayScale() {return _GUICoordinates.displayScale;}
  
private:
  // here is where all the Widgets are stored. Other instances of Collection < Widget >
  // may reference this.
  CollectionRoot< Widget > _rootWidgets;
  
protected:
  void deleteWidgets();
  
  // the top level View.
  std::unique_ptr< View > _view;
  
  GUICoordinates _GUICoordinates;
  
  VectorImageTree _vectorImages;
  
  // images and tables used for drawing.
  ResourceTree _resources;
  
  // properties used for drawing.
  PropertyTree _drawingProperties;
};

}
