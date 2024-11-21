
// mlvg: GUI library for madronalib apps and plugins
// Copyright (C) 2019-2022 Madrona Labs LLC
// This software is provided 'as-is', without any express or implied warranty.
// See LICENSE.txt for details.


#include "MLAppView.h"

namespace ml {

AppView::AppView(TextFragment appName, size_t instanceNum)
{
  // build View, pointing at root Widgets
  _view = std::make_unique< View > (_rootWidgets, WithValues{});
  
  // get names of other Actors we might communicate with
  _controllerName = TextFragment(appName, "controller", ml::textUtils::naturalNumberToText(instanceNum));
  
  // register ourself
  auto myName = TextFragment(appName, "view", ml::textUtils::naturalNumberToText(instanceNum));
  registerActor(myName, this);

  // stash in object
  appName_ = appName;
}

AppView::~AppView()
{
  removeActor(this);
}

// called when native view size changes in the PlatformView callback.
// newSize is in pixel coordinates. displayScale is pixels per system size unit.
void AppView::viewResized(NativeDrawContext* nvg, Vec2 newSize, float displayScale)
{
  float gridSizeInPixels{0};
  
  if(aspectRatioIsFixed_)
  {
    // fixed ratio:
    // the app aspect ratio is fixed. grid size may be fractional.
    // Normally we will only use this mode if the size of the window is also constrained.

    float ux = newSize.x()/_sizeInGridUnits.x();
    float uy = newSize.y()/_sizeInGridUnits.y();
    gridSizeInPixels = std::min(ux, uy);
  }
  else
  {
    // not a fixed ratio - fit a whole number of grid units into the current window size.
    // TODO user-adjustable grid size?
    gridSizeInPixels = _defaultGridSize * displayScale;
    
    _sizeInGridUnits.x() = newSize.x()/gridSizeInPixels;
    _sizeInGridUnits.y() = newSize.y()/gridSizeInPixels;
  }
  
  Vec2 origin (0, 0);
  GUICoordinates newCoords{gridSizeInPixels, newSize, displayScale, origin};
  setCoords(newCoords);
  
  // set bounds for top-level View in grid coordinates
  Vec4 newGridSize = newCoords.pixelToGrid(newCoords.viewSizeInPixels);
  _view->setBounds({0, 0, newGridSize.x(), newGridSize.y()});
  
  layoutFixedSizeWidgets_();
  
  DrawContext dc{nvg, &_resources, &_drawingProperties, _GUICoordinates};
  layoutView(dc);
  
  _view->setDirty(true);
}

void AppView::layoutFixedSizeWidgets_()
{
  // fixed size widgets are measured in pixels, so they need their grid-based sizes recalculated
  // when the view dims change.
  forEach< Widget >
  (_view->_widgets, [&](Widget& w)
   {
    if(w.getProperty("fixed_size"))
    {
      // get anchor point for widget in system coords from anchor param on (0, 1)
      Vec2 systemViewSize = _GUICoordinates.pixelToSystem(_GUICoordinates.viewSizeInPixels);
      Vec2 systemAnchor = matrixToVec2(w.getProperty("anchor").getMatrixValue());
      systemAnchor = systemAnchor * systemViewSize;
      
      // fixed widget bounds are in system coords (for same apparent size)
      Vec4 systemWidgetBounds = w.getRectProperty("fixed_bounds");
      systemWidgetBounds = translate(systemWidgetBounds, systemAnchor);
      ml::Rect gridWidgetBounds = _GUICoordinates.systemToGrid(systemWidgetBounds);
      w.setBounds(gridWidgetBounds);
    }
  }
   );
}

void AppView::_deleteWidgets()
{
  _rootWidgets.clear();
}

void AppView::_updateParameterDescription(const ParameterDescriptionList& pdl, Path pname)
{
  for(auto& paramDesc : pdl)
  {
    Path paramName = paramDesc->getTextProperty("name");
    if(paramName == pname)
    {
      forEach< Widget >
      (_view->_widgets, [&](Widget& w)
       {
        if(w.knowsParam(paramName))
        {
          w.setParameterDescription(paramName, *paramDesc);
          w.setupParams();
        }
      }
       );
    }
  }
}

void AppView::_setupWidgets(const ParameterDescriptionList& pdl)
{
  _widgetsByParameter.clear();
  _widgetsByCollection.clear();
  _widgetsBySignal.clear();

  // build index of widgets by parameter.
  // for each parameter, collect Widgets responding to it
  // and add the parameter to the Widget's param tree.
  for(auto& paramDesc : pdl)
  {
    Path paramName = paramDesc->getTextProperty("name");
    
    forEach< Widget >
    (_view->_widgets, [&](Widget& w)
     {
      if(w.knowsParam(paramName))
      {
        _widgetsByParameter[paramName].push_back(&w);
        w.setParameterDescription(paramName, *paramDesc);
      }
    }
     );
    
    //std::cout << paramName << ": " << _widgetsByParameter[paramName].size() << " widgets.\n";
  }
  
  // build index of any widgets that refer to collections.
  forEach< Widget >
  (_view->_widgets, [&](Widget& w)
   {
    if(w.hasProperty("collection"))
    {
      const Path collName(w.getTextProperty("collection"));
      _widgetsByCollection[collName].push_back(&w);
    }
  });
  
  // build index of any widgets that need signals, and subscribe to those signals.
  for(auto& w : _view->_widgets)
  {
    if(w->hasProperty("signal_name"))
    {
      const Path sigName(w->getTextProperty("signal_name"));
      _widgetsBySignal[sigName].push_back(w.get());
      
      // message the controller to subscribe to the signal.
      sendMessageToActor(_controllerName, Message{"do/subscribe_to_signal", pathToText(sigName)});
    }
  }
  
  // give each Widget a chance to do setup now: after it has its
  // parameter description(s) and before it is animated or drawn.
  for(auto& w : _view->_widgets)
  {
    w->setupParams();
  }
}

// Handle the queue of GUIEvents from the View by routing events to Widgets
// and handling any returned Messages.
void AppView::_handleGUIEvents()
{
  // doResizeIfNeeded();

  while (_inputQueue.elementsAvailable())
  {
    auto e = _inputQueue.pop();
    GUIEvent nativeEvent = _detectDoubleClicks(e);
    GUIEvent gridEvent(nativeEvent);
    gridEvent.position = _GUICoordinates.pixelToGrid(gridEvent.position);
    
    // let subclasses do any additional processing
    onGUIEvent(gridEvent);
    
    // The top-level processGUIEvent call.
    // Send input events to all Widgets in our View and handle any resulting messages.
    enqueueMessageList(_view->processGUIEvent(_GUICoordinates, gridEvent));
    handleMessagesInQueue();
  }
}

void AppView::clearWidgets()
{
  _rootWidgets.clear();
}


void AppView::_sendParameterMessageToWidgets(const Message& msg)
{
  if(msg.value)
  {
    // get param name
    Path pname = tail(msg.address);
    
    MessageList replies;

    // send to Widgets that care about it
    for(auto pw : _widgetsByParameter[pname])
    {
      // if Widget is not engaged, send it the new value.
      if(!pw->engaged)
      {
        sendMessageExpectingReply(*pw, msg, &replies);
      }
    }
    
    // send to any modal Widgets that care about it right now,
    // substituting wild card for head to match param name in widget
    if(_currentModalParam)
    {
      if(pname.beginsWith(_currentModalParam))
      {
        for(auto pw : _modalWidgetsByParameter[pname])
        {
          if(!pw->engaged)
          {
            Path wildCardMessageAddress("set_param", "*",
                                        lastN(pname, pname.getSize() - _currentModalParam.getSize()));
            sendMessageExpectingReply(*pw, {wildCardMessageAddress, msg.value}, &replies);

          }
        }
      }
    }
    
    // handle any replies
    enqueueMessageList(replies);
  }
}


// maintain states for click-and-hold timer
// param: event in native coordinates
GUIEvent AppView::_detectDoubleClicks(GUIEvent e)
{
  constexpr int kDoubleClickRadius = 8;
  constexpr int kDoubleClickMs = 500;
  
  Vec2 systemPosition = _GUICoordinates.pixelToSystem(e.position);
  GUIEvent r = e;
  
  if(e.type == "up")
  {
    if(_doubleClickTimer.isActive())
    {
      if(magnitude(Vec2(systemPosition - _doubleClickStartPosition)) < kDoubleClickRadius)
      {
        // fake command modifier?
        r.keyFlags |= commandModifier;
        _doubleClickTimer.stop();
      }
    }
    else
    {
      _doubleClickStartPosition = systemPosition;
      
      // timer doesn't need to execute anything, just expire.
      _doubleClickTimer.callOnce( [](){}, milliseconds(kDoubleClickMs) );
    }
  }
  return r;
}

// given a window size in system coordinates, tweak it to be a valid window size.
Vec2 AppView::constrainSize(Vec2 size) const
{
  Vec2 newSize = size;


  // TODO fix cross-platform stuff. is this really system size input? Doesn't make sense as currently commented.
#if ML_WINDOWS
  newSize = _GUICoordinates.pixelToSystem(newSize);
#endif


    Vec2 minDims = getMinDims();
    Vec2 maxDims = getMaxDims();
    newSize = vmax(newSize, minDims);
    newSize = vmin(newSize, maxDims);

#if ML_WINDOWS
   newSize = _GUICoordinates.systemToPixel(newSize);
#endif
  return newSize;
}

size_t AppView::_getElapsedTime()
{
  // return time elapsed since last render
  time_point<system_clock> now = system_clock::now();
  auto elapsedTime = duration_cast<milliseconds>(now - _previousFrameTime).count();
  _previousFrameTime = now;
  return elapsedTime;
}

void AppView::animate(NativeDrawContext* nvg)
{
    // Allow Widgets to draw any needed animations outside of main nvgBeginFrame().
    // Do animations and handle any resulting messages immediately.
    DrawContext dc{nvg, &_resources, &_drawingProperties, _GUICoordinates };
    MessageList ml = _view->animate((int)_getElapsedTime(), dc);
    enqueueMessageList(ml);
    handleMessagesInQueue();
}

void AppView::render(NativeDrawContext* nvg)
{
  // TODO move resource types into Renderer, DrawContext points to Renderer
  DrawContext dc{nvg, &_resources, &_drawingProperties, _GUICoordinates};

  auto layerSize = _GUICoordinates.viewSizeInPixels;
  ml::Rect topViewBounds = dc.coords.gridToPixel(_view->getBounds());
  
  // begin the frame on the backing layer
  int w = layerSize.x();
  int h = layerSize.y();
  nvgBeginFrame(nvg, w, h, 1.0);
  
  // translate the draw context to top level view bounds and draw.
  nvgIntersectScissor(nvg, topViewBounds);
  auto topLeft = getTopLeft(topViewBounds);
  
  // translate to the view's location and draw the view in its local coordinates
  nvgTranslate(nvg, topLeft);
  _view->draw(translate(dc, -topLeft));

  // end the frame.
  nvgEndFrame(nvg);
  _view->setDirty(false);
}


void AppView::startTimersAndActor()
{
  _previousFrameTime = system_clock::now();
  _ioTimer.start([=](){ _handleGUIEvents(); }, milliseconds(1000/60));
  _debugTimer.start([=]() { debugAppView(); }, milliseconds(1000));
  Actor::start();
}

void AppView::stopTimersAndActor()
{
  Actor::stop();
  _ioTimer.stop();
  _debugTimer.stop();
}

bool AppView::willHandleEvent(GUIEvent g)
{
  // Just check against types of events we return. These checks are stateless.
  // Future checks that look for example at "are we editing text?" may require
  // all events in queue to be handled before deciding.
  if(g.type == "keydown")
  {
    if(g.keyCode == KeyCodes::deleteKey)
    {
      return true;
    }
    else
    {
      return false;
    }
  }
  return true;
}

bool AppView::pushEvent(GUIEvent g)
{
  bool willHandle = willHandleEvent(g);
  if(willHandle)
  {
    _inputQueue.push(g);
  }
  return willHandle;
}

} // namespace ml


