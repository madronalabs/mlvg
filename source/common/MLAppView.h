
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
  // clear all resources that could depend on the draw context.
  virtual void clearResources() = 0;
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
  
  virtual void animate(NativeDrawContext* nvg);
  virtual void render(NativeDrawContext* nvg);
  
  // called by the PlatformView to set our size in pixel coordinates.
  void viewResized(NativeDrawContext* nvg, Vec2 newSize, float displayScale);
  
  void setCoords(const GUICoordinates& c) { _GUICoordinates = c; }
  const GUICoordinates& getCoords() { return _GUICoordinates; }

  // for a fixed ratio layout, get (width, height) of window in grid units.
  Vec2 getFixedAspectRatio() const { return _sizeInGridUnits; }
  void setFixedAspectRatio(Vec2 size)
  {
    _sizeInGridUnits = size;
    aspectRatioIsFixed_ = true;
  }

  // Rect getBorderRect() const { return _borderRect; }
  bool getStretchToScreenMode() const { return _stretchToScreenMode; }
  
  Vec2 getMinDims() const {
    if (aspectRatioIsFixed_)
    {
      // for a fixed ratio size window, _sizeInGridUnits is fixed.
      return _sizeInGridUnits * _minGridSize;
    }
    else
    {
      // for a variable ratio window, _sizeInGridUnits may vary, grid size does not.
      return _minSizeInGridUnits * _defaultGridSize;
    }
  }
  Vec2 getDefaultDims() const {
    return _sizeInGridUnits * _defaultGridSize;
  }
  Vec2 getMaxDims() const { return _sizeInGridUnits * _maxGridSize; }
  void setGridSizeDefault(int b) { _defaultGridSize = b; }
  void setGridSizeLimits(int a, int c) { _minGridSize = a; _maxGridSize = c; }
  
  void setMinSizeInGridUnits(Vec2 g) { _minSizeInGridUnits = g; }
  
  void startTimersAndActor();
  void stopTimersAndActor();
  
  // return true if the event will be handled by the View.
  bool willHandleEvent(GUIEvent g);
  
  // push event to the input queue and return true if the event will be handled by the View.
  bool pushEvent(GUIEvent g);
  
  Vec2 constrainSize(Vec2 size) const;
  
  void clearWidgets();
  
protected:
  
  // main view and top-level things.
  // order is important for default destructor!
  std::unique_ptr< ml::View > _view;
  DrawingResources _resources;
  PropertyTree _drawingProperties;
  ParameterTree _params;
  
  // Actors
  TextFragment appName_;
  Path _controllerName;
  
  // dimensions
  GUICoordinates _GUICoordinates;
  Vec2 _sizeInGridUnits;
  Vec2 _minSizeInGridUnits{ 12, 9 };
  
  bool aspectRatioIsFixed_{ false };
  size_t _minGridSize{ 48 };
  size_t _defaultGridSize{ 96 };
  size_t _maxGridSize{ 240 };
  bool _stretchToScreenMode{false};

  // windowing
  void* _platformHandle{ nullptr };
  
  // Widgets
  Tree< std::vector< Widget* > > _widgetsByParameter;
  Tree< std::vector< Widget* > > _widgetsByProperty;
  Tree< std::vector< Widget* > > _widgetsByCollection;
  Tree< std::vector< Widget* > > _widgetsBySignal;
  Tree< std::vector< Widget* > > _modalWidgetsByParameter;
  Path _currentModalParam;
  
  // timing
  time_point< system_clock > _previousFrameTime;
  Timer _ioTimer;
  Timer _doubleClickTimer;
  Timer _animationTimer;
  Timer _debugTimer;
  int guiToResizeCounter{0};
  
  // GUI Events
  Queue< GUIEvent > _inputQueue{ 1024 };
  Vec2 _clickAndHoldStartPosition;
  Vec2 _doubleClickStartPosition;
  
  // why underscores?! TODO clean up.
  void _deleteWidgets();
  void _setupWidgets(const ParameterDescriptionList& pdl);
  void _updateParameterDescription(const ParameterDescriptionList& pdl, Path pname);
  void _handleGUIEvents();

  
  void _debug();
  void _sendParameterMessageToWidgets(const Message& msg);
  GUIEvent _detectDoubleClicks(GUIEvent e);
  
  size_t _getElapsedTime();
  void layoutFixedSizeWidgets_();
  
private:
  // here is where all the Widgets are stored. Other instances of Collection < Widget >
  // may reference this.
  CollectionRoot< Widget > _rootWidgets;
  
  // temp
  int queueSize{ 0 };
};

}

