
// mlvg: GUI library for madronalib apps and plugins
// Copyright (C) 2019-2022 Madrona Labs LLC
// This software is provided 'as-is', without any express or implied warranty.
// See LICENSE.txt for details.


#include "MLAppView.h"

namespace ml {

AppView::AppView(TextFragment appName, size_t instanceNum)
{
  // build View, pointing at root Widgets
  _view = ml::make_unique< View > (_rootWidgets, WithValues{});
  
  // get names of other Actors we might communicate with
  _controllerName = TextFragment(appName, "controller", ml::textUtils::naturalNumberToText(instanceNum));
  
  // register ourself
  auto myName = TextFragment(appName, "view", ml::textUtils::naturalNumberToText(instanceNum));
  registerActor(myName, this);
}

void AppView::startup(const ParameterDescriptionList& pdl)
{
  makeWidgets();
  
  _initializeParams(pdl);
  
  _previousFrameTime = system_clock::now();

  // start timers
  _ioTimer.start([=](){ _handleGUIEvents(); }, milliseconds(1000/60));
  _debugTimer.start([=]() { _debug(); }, milliseconds(1000));
  Actor::start();
}

// called when native view size changes.
// width and height are the size of the view in system coordinates.
void AppView::viewResized(NativeDrawContext* nvg, int width, int height)
{
  _view->setDirty(true);
  
  // get dims in UI coordinates (pixels)
  float displayScale = _GUICoordinates.displayScale;
  int nativeWidth = width*displayScale;
  int nativeHeight = height*displayScale;
  
  int gridSize;
  Vec2 sizeInGridUnits(getSizeInGridUnits());
  int gridUnitsX(sizeInGridUnits.x());
  int gridUnitsY(sizeInGridUnits.y());
  
  if(_fixedRatioSize)
  {
    // fixed ratio:
    // scale both the app and the grid,
    // leave a border around the content area so that it is always
    // an even multiple of the grid size in native pixels.
    int ux = nativeWidth/gridUnitsX;
    int uy = nativeHeight/gridUnitsY;
    gridSize = std::min(ux, uy);
    float contentWidth = gridSize*gridUnitsX;
    float contentHeight = gridSize*gridUnitsY;
    float borderX = (nativeWidth - contentWidth)/2;
    float borderY = (nativeHeight - contentHeight)/2;
    _borderRect = ml::Rect{borderX, borderY, contentWidth, contentHeight};
  }
  else
  {
    // not a fixed ratio - fit a whole number of grid units into the current window size.
    // TODO user-adjustable grid size? only for subviews?
    gridSize = _defaultGridSize;
    
    gridUnitsX = nativeWidth/gridSize;
    gridUnitsY = nativeHeight/gridSize;
    _sizeInGridUnits = Vec2(gridUnitsX, gridUnitsY);
    
    _view->setProperty("grid_units_x", gridUnitsX);
    _view->setProperty("grid_units_y", gridUnitsY);
    
    float contentWidth = gridSize*gridUnitsX;
    float contentHeight = gridSize*gridUnitsY;
    float borderX = (nativeWidth - contentWidth)/2;
    float borderY = (nativeHeight - contentHeight)/2;
    _borderRect = ml::Rect{borderX, borderY, contentWidth, contentHeight};
    //   _borderRect = ml::Rect(0, 0, nativeWidth, nativeHeight);
  }
  
  // set new coordinate transform values for GUI renderer - displayScale remains the same
  // width and height are in pixel coordinates
  // MLTEST _GUICoordinates.pixelSize = Vec2(nativeWidth, nativeHeight);
  _GUICoordinates.gridSize = gridSize;
  _GUICoordinates.origin = getTopLeft(_borderRect);
  
  // set bounds for top-level View in grid coordinates
  _view->setBounds({0, 0, float(gridUnitsX), float(gridUnitsY)});
  
  // fixed size widgets are measured in pixels, so they need their grid-based sizes recalculated
  // when the view dims change.
  forEach< Widget >
  (_view->_widgets, [&](Widget& w)
   {
    if(w.getProperty("fixed_size"))
    {
      // get anchor point for widget in system coords from anchor param on (0, 1)
      Vec2 systemAnchor = matrixToVec2(w.getProperty("anchor").getMatrixValue());
      systemAnchor = systemAnchor * Vec2(width, height);
      
      // fixed widget bounds are in system coords (for same apparent size)
      Vec4 systemBounds = w.getRectProperty("fixed_bounds");
      
      //Vec4 viewBounds = _GUICoordinates.systemToNative(systemBounds);
      systemBounds = translate(systemBounds, systemAnchor);
      ml::Rect gridBounds = _GUICoordinates.systemToGrid(systemBounds);
      w.setProperty("bounds", rectToMatrix(gridBounds));
    }
  }
   );
  layoutView();
}

void AppView::createVectorImage(Path newImageName, const unsigned char* dataStart, size_t dataSize)
{
  // horribly, nsvgParse will clobber the data it reads! So let's copy that data before parsing.
  auto tempData = malloc(dataSize);
  
  if(tempData)
  {
    memcpy(tempData, dataStart, dataSize);
    _vectorImages[newImageName] = ml::make_unique< VectorImage >(static_cast< char* >(tempData));
  }
  else
  {
    std::cout << "createVectorImage: alloc failed! " << newImageName << ", " << dataSize << " bytes \n";
    return;
  }

  free(tempData);
}


void AppView::_deleteWidgets()
{
  _rootWidgets.clear();
}



void AppView::_initializeParams(const ParameterDescriptionList& pdl)
{
  buildParameterTree(pdl, _params);
  
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
  
  // now that widgets are set up, send default param values to widgets.
  for(auto& paramDesc : _params.descriptions)
  {
    Path pname = paramDesc->getTextProperty("name");
    auto pval = _params.getNormalizedValue(pname);
    Message msg(Path("set_param", pname), pval);
    _sendParameterToWidgets(msg);
  }
}


// Handle the queue of GUIEvents from the View by routing events to Widgets
// and handling any returned Messages.
void AppView::_handleGUIEvents()
{
  while (_inputQueue.elementsAvailable())
  {
    auto e = _inputQueue.pop();
    GUIEvent nativeEvent = _detectDoubleClicks(e);
    GUIEvent gridEvent(nativeEvent);
    gridEvent.position = _GUICoordinates.nativeToGrid(gridEvent.position);
    
    // The top-level processGUIEvent call.
    // Send input events to all Widgets in our View and handle any resulting messages.
    enqueueMessageList(_view->processGUIEvent(_GUICoordinates, gridEvent));
    handleMessagesInQueue();
  }
}


void AppView::_debug()
{
  //std::cout << "TestAppView: " << getMessagesAvailable() << " messages in queue. max: "
  //  << _maxQueueSize << " handled: " << _msgCounter << " \n";
  //_msgCounter = 0;
}


void AppView::_sendParameterToWidgets(const Message& msg)
{
  if(msg.value)
  {
    // get param name
    Path pname = tail(msg.address);
    
    // send to Widgets that care about it
    for(auto pw : _widgetsByParameter[pname])
    {
      // if Widget is not engaged, send it the new value.
      if(!pw->engaged)
      {
        sendMessage(*pw, msg);
      }
    }
  }
}

// maintain states for click-and-hold timer
// param: event in native coordinates
GUIEvent AppView::_detectDoubleClicks(GUIEvent e)
{
  constexpr int kDoubleClickRadius = 8;
  constexpr int kDoubleClickMs = 500;
  
  Vec2 systemPosition = _GUICoordinates.nativeToSystem(e.position);
  GUIEvent r = e;
  
  if(e.type == "up")
  {
    if(_doubleClickTimer.isActive())
    {
      if(magnitude(Vec2(systemPosition - _doubleClickStartPosition)) < kDoubleClickRadius)
      {
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

// given a pointer to a size in system UI coordinates, tweak the size to be a valid window size.
Vec2 AppView::_constrainSize(Vec2 size)
{
  Vec2 newSize = size;
  
  if(getFixedRatioSize())
  {
    Vec2 minDims = getMinDims();
    Vec2 maxDims = getMaxDims();
    newSize = vmax(newSize, minDims);
    newSize = vmin(newSize, maxDims);
  }
  return newSize;
}

}
