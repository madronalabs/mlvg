
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
  // pure virtual methods that subclasses must implement:
  //
  // render the view into the draw context and backing layer.
  // TODO not virtual!
  
  virtual void renderView(NativeDrawContext* nvg, Layer* backingLayer) = 0;
  
  
  //
  // initialize resources such as images, needed to draw the View.
  virtual void initializeResources(NativeDrawContext* nvg) = 0;
  //
  // create any widgets needed to draw the View.
  virtual void makeWidgets() = 0;
  //
  // set the bounds of all the Widgets.
  virtual void layoutView() = 0;
  //
  // push a GUI event to the queue of input events.
  virtual void pushEvent(GUIEvent g) = 0;


  
  AppView(TextFragment appName, size_t instanceNum);
  virtual ~AppView()
  {
    _ioTimer.stop();
    Actor::stop();
    removeActor(this);
  }
  
  // build widgets and params, start Actor
  void startup(const ParameterDescriptionList& pdl);
  
  // set the size of the view in system coordinates.
  void viewResized(NativeDrawContext* nvg, int w, int h);

  void createVectorImage(Path newImageName, const unsigned char* dataStart, size_t dataSize);
  void setDisplayScale(float scale) {_GUICoordinates.displayScale = scale;}
  float getDisplayScale() {return _GUICoordinates.displayScale;}
  
  // for a fixed ratio layout, get (width, height) of window in grid units.
  Vec2 getSizeInGridUnits(){ return _sizeInGridUnits; }
  void setSizeInGridUnits(Vec2 size){ _sizeInGridUnits = size; }

  bool getFixedRatioSize(){ return _fixedRatioSize; }
  
  Vec2 getMinDims(){ return _sizeInGridUnits*_minGridSize; }
  Vec2 getDefaultDims(){ return _sizeInGridUnits*_defaultGridSize; }
  Vec2 getMaxDims(){ return _sizeInGridUnits*_maxGridSize; }

protected:
  
  
  // param tree
  ParameterTreeNormalized _params;

  
  Path _controllerName;

  
  Vec2 _sizeInGridUnits;

  // should we constrain window to a fixed ratio layout? (like most plugins)
  bool _fixedRatioSize {false};
  
  // min, default and max values of the grid size stored in GUICoordinates.
  size_t _minGridSize{40};
  size_t _defaultGridSize{120};
  size_t _maxGridSize{240};
  void setGridSizeLimits(size_t min, size_t max, size_t d)
  {
    _minGridSize = min;
    _maxGridSize = max;
    _defaultGridSize = d;
  }
  
  ml::Rect _borderRect;

  
  // the top level View.
  std::unique_ptr< View > _view;
  
  GUICoordinates _GUICoordinates;
  
  VectorImageTree _vectorImages;
  
  // images and tables used for drawing.
  ResourceTree _resources;
  
  // properties used for drawing.
  PropertyTree _drawingProperties;
  
  
  void* _platformHandle{ nullptr };
  
  // Widgets
  Tree< std::vector< Widget* > > _widgetsByParameter;
  Tree< std::vector< Widget* > > _widgetsByProperty;
  Tree< std::vector< Widget* > > _widgetsByCollection;
  Tree< std::vector< Widget* > > _widgetsBySignal;
  
  // timing
  time_point< system_clock > _previousFrameTime;
  Timer _ioTimer;
  Timer _doubleClickTimer;
  Timer _animationTimer;
  Timer _debugTimer;
  
  // GUI Events
  Queue< GUIEvent > _inputQueue{ 1024 };
  Vec2 _clickAndHoldStartPosition;
  Vec2 _doubleClickStartPosition;
  


  
  void _deleteWidgets();
  void _initializeParams(const ParameterDescriptionList& pdl);
  void _handleGUIEvents();
  void _debug();
  void _sendParameterToWidgets(const Message& msg);
  GUIEvent _detectDoubleClicks(GUIEvent e);
  Vec2 _constrainSize(Vec2 size);

private:
  // here is where all the Widgets are stored. Other instances of Collection < Widget >
  // may reference this.
  CollectionRoot< Widget > _rootWidgets;

};

}
