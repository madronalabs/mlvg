
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
  // set the bounds of all the Widgets.
  virtual void layoutView(DrawContext dc) = 0;
  //
  // do any additional processing for events.
  virtual void onGUIEvent(const GUIEvent& event) = 0;
  //
  // do any additional processing for resizing.
  virtual void onResize(Vec2 newSize) = 0;

  AppView(TextFragment appName, size_t instanceNum);
  virtual ~AppView();

  void render(NativeDrawContext* nvg, Layer* backingLayer);
  
  // called by the PlatformView to set our size in system coordinates.
  void viewResized(NativeDrawContext* nvg, Vec2 newSize);

  void createVectorImage(Path newImageName, const unsigned char* dataStart, size_t dataSize);
  
  void setCoords(const GUICoordinates& c) {_GUICoordinates = c;}
  const GUICoordinates& getCoords() { return _GUICoordinates; }
  void setDisplayScale(float scale) {_GUICoordinates.displayScale = scale;}
  
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

  void createPlatformView(void* pParent, int flags);
  
  void startTimersAndActor();
  void stopTimersAndActor();
  
  void doResize(Vec2 newSize);
  
  // return true if the event will be handled by the View.
  bool willHandleEvent(GUIEvent g);
  
  // push event to the input queue and return true if the event will be handled by the View.
  bool pushEvent(GUIEvent g);
  
protected:
  
  // the top level View.
  std::unique_ptr< View > _view;

  // all parameters we are viewing
  ParameterTree _params;
  
  // Actors
  Path _controllerName;

  // dimensions
  Vec2 viewSize_;
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
  
  // TODO ------------------------
    
  Tree< std::vector< Widget* > > _modalWidgetsByParameter;
  Path _currentModalParam;

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
  void _setupWidgets(const ParameterDescriptionList& pdl);
  void _updateParameterDescription(const ParameterDescriptionList& pdl, Path pname);
  void _handleGUIEvents();
  void _debug();
  void _sendParameterMessageToWidgets(const Message& msg);
  GUIEvent _detectDoubleClicks(GUIEvent e);
  Vec2 _constrainSize(Vec2 size);
  size_t _getElapsedTime();
  void layoutFixedSizeWidgets_(Vec2 newSize);

private:
  // here is where all the Widgets are stored. Other instances of Collection < Widget >
  // may reference this.
  CollectionRoot< Widget > _rootWidgets;
};

}
