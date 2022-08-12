
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
  std::cout << "_controllerName = " << _controllerName << "\n";

  // register ourself
  auto myName = TextFragment(appName, "view", ml::textUtils::naturalNumberToText(instanceNum));
  registerActor(myName, this);
  std::cout << "REGISTER " << myName << "\n";

}

AppView::~AppView()
{
  _ioTimer.stop();
  Actor::stop();
  removeActor(this);
}

void AppView::startup(const ParameterDescriptionList& pdl)
{
  makeWidgets();
  _initializeParams(pdl);

  // start timers
  _previousFrameTime = system_clock::now();
  _ioTimer.start([=](){ _handleGUIEvents(); }, milliseconds(1000/60));
  _debugTimer.start([=]() { _debug(); }, milliseconds(1000));
  Actor::start();
}

// called when native view size changes.
// width and height are the size of the view in system coordinates.
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
    float contentWidth = systemGridSize*gridUnitsX;
    float contentHeight = systemGridSize*gridUnitsY;
    float borderX = (systemWidth - contentWidth)/2;
    float borderY = (systemHeight - contentHeight)/2;
    _borderRect = ml::Rect{borderX, borderY, contentWidth, contentHeight};
  }
  else
  {
    // not a fixed ratio - fit a whole number of grid units into the current window size.
    // TODO user-adjustable grid size? only for subviews?
    systemGridSize = _defaultGridSize;
    
    gridUnitsX = systemWidth/systemGridSize;
    gridUnitsY = systemHeight/systemGridSize;
    _sizeInGridUnits = Vec2(gridUnitsX, gridUnitsY);
    
    float contentWidth = systemGridSize*gridUnitsX;
    float contentHeight = systemGridSize*gridUnitsY;
    float borderX = (systemWidth - contentWidth)/2;
    float borderY = (systemHeight - contentHeight)/2;
    _borderRect = ml::Rect{borderX, borderY, contentWidth, contentHeight};
  }
  
  //std::cout << "new grid size: " << gridUnitsX << " x " << gridUnitsY << " @ " << systemGridSize << "\n";
  _view->setProperty("grid_units_x", gridUnitsX);
  _view->setProperty("grid_units_y", gridUnitsY);

  // set new coordinate transform values for GUI renderer
  _GUICoordinates.gridSizeInPixels = systemGridSize*displayScale;
  _GUICoordinates.origin = getTopLeft(_borderRect)*displayScale;
  
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
      systemAnchor = systemAnchor * newSize;
      
      // fixed widget bounds are in system coords (for same apparent size)
      Vec4 systemBounds = w.getRectProperty("fixed_bounds");
      
      //Vec4 viewBounds = _GUICoordinates.systemToPixel(systemBounds);
      systemBounds = translate(systemBounds, systemAnchor);
      ml::Rect gridBounds = _GUICoordinates.systemToGrid(systemBounds);
      w.setBounds(gridBounds);
    }
  }
   );
  layoutView();
  _view->setDirty(true);
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
  
  Vec2 systemPosition = _GUICoordinates.pixelToSystem(e.position);
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

size_t AppView::_getElapsedTime()
{
  // return time elapsed since last render
  time_point<system_clock> now = system_clock::now();
  auto elapsedTime = duration_cast<milliseconds>(now - _previousFrameTime).count();
  _previousFrameTime = now;
  return elapsedTime;
}

void AppView::renderView(NativeDrawContext* nvg, Layer* backingLayer)
{
  if(!backingLayer) return;
  int w = backingLayer->width;
  int h = backingLayer->height;
  
  // TODO move resource types into Renderer, DrawContext points to Renderer
  DrawContext dc{nvg, &_resources, &_drawingProperties, &_vectorImages, _GUICoordinates};
  
  // first, allow Widgets to draw any needed animations outside of main nvgBeginFrame().
  // Do animations and handle any resulting messages immediately.
  MessageList ml = _view->animate(_getElapsedTime(), dc);
  enqueueMessageList(ml);
  handleMessagesInQueue();
  
  // begin the frame on the backing layer
  drawToLayer(backingLayer);
  nvgBeginFrame(nvg, w, h, 1.0f);
  
  // if top level view is dirty, clear entire window
  if(_view->isDirty())
  {
    nvgBeginPath(nvg);
    nvgRect(nvg, ml::Rect{-10000, -10000, 20000, 20000});
    auto bgColor = getColorWithDefault(dc, "background", colors::black);
    nvgFillColor(nvg, bgColor);
    nvgFill(nvg);
  }
  
  ml::Rect topViewBounds = dc.coords.gridToPixel(_view->getBounds());
  nvgIntersectScissor(nvg, topViewBounds);
  auto topLeft = getTopLeft(topViewBounds);
  nvgTranslate(nvg, topLeft);
  
  _view->draw(translate(dc, -topLeft));
  
  // end the frame.
  nvgEndFrame(nvg);
  _view->setDirty(false);
}

void AppView::doAttached(void* pParent, int flags)
{
  Vec2 dims;
  auto initialSize = _params["view_size"].getMatrixValue();
  if(initialSize.getWidth() == 2)
  {
    dims = Vec2(initialSize[0], initialSize[1]);
  }
  else
  {
    dims = getDefaultDims();
  }

  Vec2 cDims = _constrainSize(dims);
  doResize(cDims);

  if(pParent != _parent)
  {
    _parent = pParent;
    _platformView = ml::make_unique< PlatformView >(pParent, Vec4(0, 0, cDims.x(), cDims.y()), this, _platformHandle, flags);
  }
}

// set new editor size in system coordinates.
void AppView::doResize(Vec2 newSize)
{
  int width = newSize[0];
  int height = newSize[1];
  float scale = _GUICoordinates.displayScale;
  
  Vec2 newPixelSize = Vec2(width, height)*scale;
  
//  if(_GUICoordinates.viewSizeInPixels != newPixelSize)
  {
    _GUICoordinates.viewSizeInPixels = newPixelSize;
    onResize(newSize);
    // resize our canvas, in system coordinates
    if(_platformView)
      _platformView->resizeView(width, height);
  }
}

void AppView::pushEvent(GUIEvent g)
{
  _inputQueue.push(g);
}

}
