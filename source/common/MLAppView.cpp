
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
// newSize is in system coordinates.
void AppView::viewResized(NativeDrawContext* nvg, Vec2 newSize)
{
  // get dims in system coordinates
  float displayScale = _GUICoordinates.displayScale;
  int systemWidth = newSize.x();
  int systemHeight = newSize.y();
  
  int systemGridSize;
  Vec2 sizeInGridUnits(getSizeInGridUnits());
  float gridUnitsX(sizeInGridUnits.x());
  float gridUnitsY(sizeInGridUnits.y());
  
  if(_fixedRatioSize)
  {
    // fixed ratio:
    // scale both the app and the grid,
    // leave a border around the content area so that it is always
    // an even multiple of the grid size in system coordinates.
    int ux = systemWidth/gridUnitsX;
    int uy = systemHeight/gridUnitsY;
    systemGridSize = std::min(ux, uy);
  }
  else
  {
    // not a fixed ratio - fit a whole number of grid units into the current window size.
    // TODO user-adjustable grid size?
    systemGridSize = _defaultGridSize;
    
    gridUnitsX = systemWidth/systemGridSize;
    gridUnitsY = systemHeight/systemGridSize;
    _sizeInGridUnits = Vec2(gridUnitsX, gridUnitsY);
  }
  
  float contentWidth = systemGridSize*gridUnitsX;
  float contentHeight = systemGridSize*gridUnitsY;

  float borderX = (systemWidth - contentWidth)/2;
  float borderY = (systemHeight - contentHeight)/2;
  _borderRect = ml::Rect{borderX, borderY, contentWidth, contentHeight};
  
  _view->setProperty("grid_units_x", gridUnitsX);
  _view->setProperty("grid_units_y", gridUnitsY);
  
  // get origin of grid system, relative to view top left,
  // in pixel coordinate system.
  Vec2 origin = getTopLeft(_borderRect)*displayScale;
  
  // set new coordinate transform values for GUI renderer
  _GUICoordinates.gridSizeInPixels = systemGridSize*displayScale;
  _GUICoordinates.origin = getTopLeft(_borderRect)*displayScale;
  
  GUICoordinates newCoords{int(systemGridSize*displayScale), newSize*displayScale, displayScale, origin};
  setCoords(newCoords);
  
  // set bounds for top-level View in grid coordinates
  _view->setBounds({0, 0, gridUnitsX, gridUnitsY});
  
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
      Vec2 systemAnchor = matrixToVec2(w.getProperty("anchor").getMatrixValue());
      systemAnchor = systemAnchor * getDims(_borderRect) + getTopLeft(_borderRect);
      
      // fixed widget bounds are in system coords (for same apparent size)
      Vec4 systemBounds = w.getRectProperty("fixed_bounds");
      systemBounds = translate(systemBounds, systemAnchor);
      ml::Rect gridBounds = _GUICoordinates.systemToGrid(systemBounds);
      w.setBounds(gridBounds);
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
  /*
  guiToResizeCounter++;
  if(guiToResizeCounter > 2)
  {
    guiToResizeCounter = 0;
    doResizeIfNeeded();
  }*/
  
  doResizeIfNeeded();

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

void AppView::_debug()
{
  //std::cout << "UtuViewView: " << getMessagesAvailable() << " messages in queue. max: "
  //  << _maxQueueSize << " handled: " << _msgCounter << " \n";
  //_msgCounter = 0;
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

// given a size in system UI coordinates, tweak it to be a valid window size.
Vec2 AppView::constrainSize(Vec2 size) const
{
  Vec2 newSize = size;
  
    Vec2 minDims = getMinDims();
    Vec2 maxDims = getMaxDims();
    newSize = vmax(newSize, minDims);
    newSize = vmin(newSize, maxDims);

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
    MessageList ml = _view->animate(_getElapsedTime(), dc);
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
  nvgBeginFrame(nvg, layerSize.x(), layerSize.y(), 1.0f);

  // if top level view is dirty, clear entire window
  if(_view->isDirty())
  {
    nvgBeginPath(nvg);
    
    // we don't have the window dims. so add sufficient margin around the top view bounds to cover the window.
    nvgRect(nvg, grow(topViewBounds, 1000));
    
    auto bgColor = getColorWithDefault(dc, "background", colors::black);
    nvgFillColor(nvg, bgColor);
    nvgFill(nvg);
  }
  
  // translate the draw context to top level view bounds and draw.
  nvgIntersectScissor(nvg, topViewBounds);
  auto topLeft = getTopLeft(topViewBounds);
  nvgTranslate(nvg, topLeft);
  _view->draw(translate(dc, -topLeft));
  
  // end the frame.
  nvgEndFrame(nvg);
  _view->setDirty(false);
}

void AppView::createPlatformView(void* pParent, int flags, int targetFPS)
{
  // create the native platform view
  if(pParent != _parent)
  {
    _parent = pParent;
    Rect wSize = PlatformView::getWindowRect(pParent, flags);
    _platformView = std::make_unique< PlatformView >(pParent, wSize, this, _platformHandle, flags, targetFPS);
  }
}

void AppView::startTimersAndActor()
{
  _previousFrameTime = system_clock::now();
  _ioTimer.start([=](){ _handleGUIEvents(); }, milliseconds(1000/30));
  _debugTimer.start([=]() { _debug(); }, milliseconds(1000));
  Actor::start();
}

void AppView::stopTimersAndActor()
{
  Actor::stop();
  _ioTimer.stop();
  _debugTimer.stop();
}

// set new editor size in system coordinates.
void AppView::doResizeIfNeeded()
{
    if (_needsResize)
    {
        Vec2 newPixelSize = newDisplaySize * newDisplayScale;

        _GUICoordinates.viewSizeInPixels = newPixelSize;
        _GUICoordinates.displayScale = newDisplayScale;

        onResize(newDisplaySize);

        // resize our canvas, in system coordinates
        if (_platformView)
            _platformView->resizePlatformView(newDisplaySize[0], newDisplaySize[1]);

        _needsResize = false;
    }
}

void AppView::setDisplayScale(float newScale)
{
  if(newDisplayScale != newScale)
  {
    newDisplayScale = newScale;
    _needsResize = true;
  }
}


void AppView::setDisplaySize(Vec2 newSize)
{
  if(newDisplaySize != newSize)
  {
    newDisplaySize = newSize;
    _needsResize = true;
  }
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


