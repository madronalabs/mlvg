
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
#include "MLPlatformView.h"

namespace ml {

class AppView : public Actor
{
public:
  
  // pure virtual methods that subclasses must implement:
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
  // do any additional processing for events.
  virtual void onGUIEvent(const GUIEvent& event) = 0;
  //
  // do any additional processing for resizing.
  virtual void onResize(Vec2 newSize) = 0;

  AppView(TextFragment appName, size_t instanceNum);
  virtual ~AppView();

  // build widgets and params, start Actor
  void startup(const ParameterDescriptionList& pdl);
  
  // set the size of the view in system coordinates.
  void viewResized(NativeDrawContext* nvg, Vec2 newSize);

  void createVectorImage(Path newImageName, const unsigned char* dataStart, size_t dataSize);
  
  void setCoords(const GUICoordinates& c) {_GUICoordinates = c;}
  const GUICoordinates& getCoords() { return _GUICoordinates; }
  
  void setDisplayScale(float scale) {_GUICoordinates.displayScale = scale;}
//  float getDisplayScale() {return _GUICoordinates.displayScale;}
  
  // for a fixed ratio layout, get (width, height) of window in grid units.
  Vec2 getSizeInGridUnits() const { return _sizeInGridUnits; }
  void setSizeInGridUnits(Vec2 size){ _sizeInGridUnits = size; }

  bool getFixedRatioSize() const { return _fixedRatioSize; }
  void setFixedRatioSize(bool b) { _fixedRatioSize = b; }

  Vec2 getMinDims() const { return _sizeInGridUnits*_minGridSize; }
  Vec2 getDefaultDims() const { return _sizeInGridUnits*_defaultGridSize; }
  Vec2 getMaxDims() const { return _sizeInGridUnits*_maxGridSize; }
  void setGridSizeDefault(int b){ _defaultGridSize = b; }
  void setGridSizeLimits(int a, int c){ _minGridSize = a; _maxGridSize = c; }

  void renderView(NativeDrawContext* nvg, Layer* backingLayer);
  void doAttached (void* pParent, int flags);
  void doResize(Vec2 newSize);
  void pushEvent(GUIEvent g);
  
  
protected:
  
  // the top level View.
  std::unique_ptr< View > _view;

  // all parameters we are viewing
  ParameterTreeNormalized _params;
  
  // Actors
  Path _controllerName;

  // dimensions

  GUICoordinates _GUICoordinates;
  Vec2 _sizeInGridUnits;
  ml::Rect _borderRect;
  bool _fixedRatioSize {false};
  size_t _minGridSize{30};
  size_t _defaultGridSize{60};
  size_t _maxGridSize{240};

  
  // drawing resources
  VectorImageTree _vectorImages;
  ResourceTree _resources;
  PropertyTree _drawingProperties;
  
  // windowing
  void* _platformHandle{ nullptr };
  void* _parent{nullptr};
  std::unique_ptr< PlatformView > _platformView;

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
  size_t _getElapsedTime();

private:
  // here is where all the Widgets are stored. Other instances of Collection < Widget >
  // may reference this.
  CollectionRoot< Widget > _rootWidgets;
};

}
