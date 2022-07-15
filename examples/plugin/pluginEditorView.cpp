// mlvg: GUI library for madronalib
// (c) 2020, Madrona Labs LLC, all rights reserved
// see LICENSE.txt for details

#include "pluginEditorView.h"

#include "MLVGUtils.h"
#include "mldebug.h"

#include "madronalib.h"

#include "MLDial.h"
#include "MLResizer.h"
#include "MLTextLabel.h"
#include "MLSVGImage.h"
#include "MLRegLabel.h"
#include "MLMeter.h"
#include "MLScope.h"
#include "MLFPSMeter.h"
#include "MLPopup.h"

#include "MLSerialization.h"

#include "../build/resources/resources.c"

constexpr int kScreenBuffers{1};

namespace Steinberg {
namespace Vst {
namespace llllpluginnamellll {

//------------------------------------------------------------------------
PluginEditorView::PluginEditorView(PluginController& controller, int w, int h, void* platformHandle) :
View(WithValues{}),
_controller(controller),
_platformHandle(platformHandle)
{
  CPluginView::setRect(ViewRect(0, 0, w, h));
  
  _widgets["masthead"] = ml::make_unique<SVGImage>(WithValues{
    { "bounds",{ 0, 0, 3.5, 1 } },
    {"imageName", "masthead" }
  } );
  
  _widgets["byline"] = ml::make_unique<TextLabel>(WithValues{
    {"bounds", {3.5, 0.5, 3, 0.5} },
    {"text", "by Madrona Labs" }
  } );
  
  _widgets["gain"] = ml::make_unique<Dial>(WithValues{
    {"bounds", {2, 1.25, 3, 2.5} },
    {"color", {241, 127, 32, 255} },
    {"indicator", {255, 185, 127, 255} },
    {"label", "gain" },
    {"param", "gain" },
    {"ticks", 11 }
  } );
  
  _widgets["freq_l"] = ml::make_unique<Dial>(WithValues{
    {"bounds", {5, 1.25, 3, 2.5} },
    {"color", {230, 200, 37, 255} },
    {"indicator", {250, 228, 116, 255} },
    {"label", "freq left" },
    {"param", "freq_l" },
    {"signalName", "freq_l_mod"} // TEST
  } );
  
  _widgets["freq_r"] = ml::make_unique<Dial>(WithValues{
    {"bounds", {8, 1.25, 3, 2.5} },
    {"color", {160, 209, 73, 255} },
    {"indicator", {208, 249, 128, 255} },
    {"label", "freq right" },
    {"param", "freq_r" },
    {"use_popup", true }
  } );
  
  _widgets["reglabel"] = ml::make_unique<RegLabel>(WithValues{
    {"bounds", {9, 0, 6.75, 1} },
    {"line1", Text(getPluginName(), " ", getPluginAPI())},
    {"line2", "test2"}
  } );
  
  _widgets["fps"] = ml::make_unique<FPSMeter>(WithValues{
    {"bounds", {0.25, 3, 1, 0.5} }
  } );
  
  _widgets["popup"] = ml::make_unique<Popup>(WithValues{
    {"bounds", {0, 0, 0, 0} },
    {"z", -1 } // stay on top, except for resizer
  } );
  
  
  /*
   _widgets["input_meter"] = ml::make_unique<Meter>(WithValues{
   {"bounds", {0.625, 1.5, 1, 0.75} },
   {"signalName", "input_meter"}
   } );
   
   _widgets["output_meter"] = ml::make_unique<Meter>(WithValues{
   {"bounds", {14.375, 1.5, 1, 0.75} },
   {"signalName", "output_meter"}
   } );
   */
  
  _widgets["resizer"] = ml::make_unique<Resizer>(WithValues{
    {"fix_ratio", kGridUnitsX/kGridUnitsY},
    {"z", -2}, // stay on top
    {"fixed_size", true},
    {"fixed_bounds", { -16, -16, 16, 16 }},
    {"anchor", {1, 1}} // for fixed-size widgets, anchor is a point on the view from top left {0, 0} to bottom right {1, 1}.
  } );
  
  _widgets["scope"] = ml::make_unique<Scope>(WithValues{
    {"bounds", {11, 2, 3, 1}},
    {"signalName", "scope"}
  } );
  
  // for each parameter, send description to Widgets and collect a list of Widgets that respond to it
  for(int i=0, nParams = _controller.getParameterCount(); i<nParams; ++i)
  {
    View::broadcastParameterDescription(_controller.getParamDescriptionByIndex(i));

    auto paramName = _controller.getParamNameByID(i);
    std::vector< Widget* > vw;
    View::broadcastCollectionPointer(paramName, vw);
   _widgetsByParameter[paramName] = vw;
  }
    
  // store partial parameter names for popup
  _popupParamNames = { "lfo/rate", "lfo/amount", "learn/amount" };

  // update widgets once before any drawing to get the initial parameter values
  updateWidgets();
  
  markEverythingAsDirty();
  
  _previousFrameTime = system_clock::now();
  
  _ioTimer.start([=](){ handleGUIEvents(); updateWidgets(); }, milliseconds(1000/60));
}

PluginEditorView::~PluginEditorView ()
{
  _ioTimer.stop();
  /*
   #if SMTG_OS_MACOS
   ReleaseVSTGUIBundleRef ();
   #endif
   */
}

int PluginEditorView::getElapsedTime()
{
  // return time elapsed since last render
  time_point<system_clock> now = system_clock::now();
  auto elapsedTime = duration_cast<milliseconds>(now - _previousFrameTime).count();
  _previousFrameTime = now;
  return elapsedTime;
}

#pragma mark from ml::Renderer

void PluginEditorView::initializeResources(NativeDrawContext* nvg)
{
  std::cout << "initializeResources() \n";
  
  _drawContext = nvg;
  
  // initialize fonts
  // TODO stash handles?
  //  NativeDrawContext* nvg = getNativeContext(dc);
  if (nvg)
  {
    int font1 = nvgCreateFontMem(nvg, "MadronaSans", (unsigned char*)resources::MadronaSans_Regular_ttf, resources::MadronaSans_Regular_ttf_size, 0);
    const unsigned char* pFont1 = reinterpret_cast<const unsigned char *>(&font1);
    _resources["madronasans"] = ml::make_unique< Resource >(pFont1, pFont1 + sizeof(int));
    
    int font2 = nvgCreateFontMem(nvg, "MadronaSans", (unsigned char *)resources::MadronaSans_CondensedOblique_ttf, resources::MadronaSans_CondensedOblique_ttf_size, 0);
    const unsigned char* pFont2 = reinterpret_cast<const unsigned char *>(&font2);
    _resources["madronasansoblique"] = ml::make_unique< Resource >(pFont2, pFont2 + sizeof(int));
    
    int flags = 0;
    
    int img1 = nvgCreateImageMem(nvg, flags, (unsigned char *)resources::background1_jpg, resources::background1_jpg_size);
    const unsigned char* pImg1 = reinterpret_cast<const unsigned char *>(&img1);
    _resources["background"] = ml::make_unique< Resource >(pImg1, pImg1 + sizeof(int));
    
    // SVG images
    _resources["masthead"] = ml::make_unique< Resource >(resources::masthead_svg, resources::masthead_svg + resources::masthead_svg_size);
  }
}

// called when native view size changes.
// width and height are the size of the view in system coordinates.
void PluginEditorView::viewResized(int width, int height)
{
  markEverythingAsDirty();
  
  // get dims in UI coordinates (pixels)
  float displayScale = _GUICoordinates.displayScale;
  int nativeWidth = width*displayScale;
  int nativeHeight = height*displayScale;
  
  // leave a border around the content area so that it is always an even multiple of the grid size in native pixels.
  int ux = nativeWidth/kGridUnitsX;
  int uy = nativeHeight/kGridUnitsY;
  int gridSize = std::min(ux, uy);
  float contentWidth = gridSize*kGridUnitsX;
  float contentHeight = gridSize*kGridUnitsY;
  float borderX = (nativeWidth - contentWidth)/2;
  float borderY = (nativeHeight - contentHeight)/2;
  _borderRect = ml::Rect{borderX, borderY, contentWidth, contentHeight};
  
  // set new coordinate transform values for GUI renderer - displayScale remains the same
  // width and height are in native pixel coordinates
  _GUICoordinates.nativeWidth = nativeWidth;
  _GUICoordinates.nativeHeight = nativeHeight;
  _GUICoordinates.gridSize = gridSize;
  _GUICoordinates.origin = {borderX, borderY};
  
  // fixed size widgets are measured in pixels, so they need their grid-based sizes recalculated
  // when the view dims change.
  for(auto& w : _widgets)
  {
    if(w->getProperty("fixed_size"))
    {
      // get anchor point for widget in system coords from anchor param on (0, 1)
      Vec2 systemAnchor = matrixToVec2(w->getProperty("anchor").getMatrixValue());
      systemAnchor = systemAnchor * Vec2(width, height);
      
      // fixed widget bounds are in system coords (for same apparent size)
      Vec4 systemBounds = w->getRectProperty("fixed_bounds");
      
      //Vec4 viewBounds = _GUICoordinates.systemToNative(systemBounds);
      systemBounds = translate(systemBounds, systemAnchor);
      ml::Rect gridBounds = _GUICoordinates.systemToGrid(systemBounds);
      w->setProperty("bounds", rectToMatrix(gridBounds));
    }
  }
}

Vec2 PluginEditorView::getViewSize()
{
  float scale = _GUICoordinates.displayScale;
  return Vec2(_GUICoordinates.nativeWidth/scale, _GUICoordinates.nativeHeight/scale);
}

void PluginEditorView::doRender(NativeDrawContext* nvg)
{
  constexpr bool drawGrid{false};
  DrawContext dc{nvg, &_resources, _GUICoordinates};
  
  // update any Widget animations in render thread to get the elapsed time
  // since the last frame and find out the new bounds before redrawing.
  animate(getElapsedTime());
  
  if(_frameCounter <= _redrawAllUntilFrame)
  {
    drawBackground(dc, drawGrid);
    drawAllWidgets(dc, _widgets);
  }
  else
  {
    // only draw dirty widgets
    drawDirtyWidgets(dc, _widgets);
  }
  
  _frameCounter++;
}

void PluginEditorView::pushEvent(GUIEvent g)
{
  _inputQueue.push(g);
}

#pragma mark from VST3 SDK / IPlugView

//------------------------------------------------------------------------
tresult PLUGIN_API PluginEditorView::isPlatformTypeSupported(FIDString type)
{
#if SMTG_OS_WINDOWS
  if (strcmp(type, kPlatformTypeHWND) == 0)
    return kResultTrue;
  
#elif SMTG_OS_MACOS
#if TARGET_OS_IPHONE
  if (strcmp(type, kPlatformTypeUIView) == 0)
    return kResultTrue;
#else
#if MAC_CARBON
  if (strcmp(type, kPlatformTypeHIView) == 0)
    return kResultTrue;
#endif
  
  //#if MAC_COCOA
  if (strcmp(type, kPlatformTypeNSView) == 0)
    return kResultTrue;
  //#endif
  
#endif
#elif SMTG_OS_LINUX
  if (strcmp(type, kPlatformTypeX11EmbedWindowID) == 0)
    return kResultTrue;
#endif
  
  return kInvalidArgument;
}


//------------------------------------------------------------------------
tresult PLUGIN_API PluginEditorView::attached (void* pParent, FIDString type)
{
  tresult ret{Steinberg::kResultFalse};
  
  // TODO fix: controller sends default size msg on creation
  auto defaultSize = _controller.getMatrixPropertyWithDefault("view_size", { kGridUnitsX*kDefaultGridSize, kGridUnitsY*kDefaultGridSize });
  
  float w = defaultSize[0];
  float h = defaultSize[1];
  
  if(pParent != _parent)
  {
    _mlvgView = ml::make_unique<VGView>(pParent, Vec4(0, 0, w, h), this, _platformHandle);
    _parent = pParent;
    
    // TODO error
    ret = Steinberg::kResultOk;
  }
  
  ViewRect newSize(0, 0, w, h); // TODO converters
  CPluginView::setRect(newSize);
  plugFrame->resizeView(this, &newSize);
  _mlvgView->resizeCanvas(w, h);
  
  return ret;
}

//------------------------------------------------------------------------
tresult PLUGIN_API PluginEditorView::removed ()
{
  return Steinberg::kResultOk;
}

/*
 tresult PLUGIN_API PluginEditorView::onSize(ViewRect* pSize)
 {
 if (pSize)
 rect = *pSize;
 return kResultTrue;
 }
 
 
 tresult PLUGIN_API PluginEditorView::getSize(Steinberg::ViewRect* pSize)
 {
 //*pSize = Steinberg::ViewRect(0, 0, mOwner.GetEditorWidth(), mOwner.GetEditorHeight());
 return CPluginView::getSize(pSize);
 }*/

//------------------------------------------------------------------------

tresult PLUGIN_API PluginEditorView::setContentScaleFactor (ScaleFactor factor)
{
  // _contentScaleFactor = factor;
  // std::cout << "SCALE FACTOR: " << factor << "\n";
  return Steinberg::kResultOk;
}

/*
 //------------------------------------------------------------------------
 CMessageResult PluginEditorView::notify (CBaseObject* sender, const char* message)
 {
 if (message == CVSTGUITimer::kMsgTimer)
 {
 if (frame)
 frame->idle ();
 
 return kMessageNotified;
 }
 
 return kMessageUnknown;
 }
 */

//------------------------------------------------------------------------
tresult PLUGIN_API PluginEditorView::onKeyDown (char16 key, int16 keyMsg, int16 modifiers)
{
  return kResultFalse;
}

//------------------------------------------------------------------------
tresult PLUGIN_API PluginEditorView::onKeyUp (char16 key, int16 keyMsg, int16 modifiers)
{
  return kResultFalse;
}

//------------------------------------------------------------------------
tresult PLUGIN_API PluginEditorView::onWheel (float distance)
{
  return kResultFalse;
}

/*
 // given a pointer to a size in system UI coordinates, tweak the size to be a valid window size.
 tresult PLUGIN_API PluginEditorView::checkSizeConstraint (Steinberg::ViewRect* rect)
 {
 Vec2 newSize(rect->getWidth(), rect->getHeight());
 newSize = constrainSize(newSize);
 rect->right = rect->left + newSize[0];
 rect->bottom = rect->top + newSize[1];
 return kResultTrue;
 }
 */

// given a pointer to a size in system UI coordinates, tweak the size to be a valid window size.
Vec2 PluginEditorView::constrainSize(Vec2 size)
{
  Vec2 newSize = vclamp(size, Vec2{kMinGridSize*kGridUnitsX, kMinGridSize*kGridUnitsY}, Vec2{kMaxGridSize*kGridUnitsX, kMaxGridSize*kGridUnitsY});
  return newSize;
}

/*
ml::Path PluginEditorView::findWidgetsForParameter(ml::Path paramName)
{
  return paramName;
  _widgetsByParameter[paramName]
  //return _paramToWidgetMap[paramName]; // TEMP param name = widget names.
}
*/

// set new editor size in system coordinates.
void PluginEditorView::resizeEditor(Vec2 newSize)
{
  float width = newSize[0];
  float height = newSize[1];
  float scale = _GUICoordinates.displayScale;
  
  //std::cout << "resizeEditor: " << width << " x " << height << " @ " << scale << "\n";
  
  if(getViewSize() != Vec2(width*scale, height*scale))
  {
    // prevent animated resizing
    VGView::AnimationGroup g;
    
    // VST API call to resize window, in system coordinates
    Steinberg::ViewRect newRect = Steinberg::ViewRect(0, 0, width, height);
    CPluginView::setRect(newRect);
    plugFrame->resizeView(this, &newRect);
    
    // resize our canvas, in system coordinates
    _mlvgView->resizeCanvas(width, height);
    
    // send new size to the contoller so it will be saved
    
    // TODO not a reference but a message pipe
    _controller.handleChangeFromEditor({"view_size", { width, height }});
  }
}


void PluginEditorView::cancelModalWidget()
{
  if(_activeModalWidget)
  {
    Symbol modalWidgetState {_widgets[_activeModalWidget]->getTextProperty("state")};
    if(modalWidgetState == "pre_open")
    {
      _widgets[_activeModalWidget]->processValueChange({"state", "cancel"});
      _activeModalWidget = {};
    }
  }
}

// maintain states for click-and-hold timer
// param: event in native coordinates
void PluginEditorView::updateClickAndHold(GUIEvent e)
{
  constexpr int kClickAndHoldRadius = 8;
  constexpr int kClickAndHoldMs = 1000;
  
  Vec2 systemPosition = _GUICoordinates.nativeToSystem(e.position);
  Vec2 gridPosition = _GUICoordinates.nativeToGrid(e.position);
  if(e.type == "down")
  {
    _clickAndHoldStartPosition = systemPosition;
  
    // start the click-and-hold timer to send "hold" to the _stillDownWidget soon if not cancelled
    if(_stillDownWidget)
    {
      if(_stillDownWidget != _activeModalWidget)
      {
        std::cout << "updateClickAndHold: stilldown = " << _stillDownWidget << "\n";
        _clickAndHoldTimer.callOnce( [=](){
          processValueChangeList(sendEventToWidget(_GUICoordinates, GUIEvent {"hold", _GUICoordinates.systemToGrid(_clickAndHoldStartPosition)}, _stillDownWidget));
        }, milliseconds(kClickAndHoldMs) );
      }
    }
  }
  else if(e.type == "up")
  {
    _clickAndHoldTimer.stop();
    cancelModalWidget();
  }
  else if(e.type == "drag")
  {
    // if we dragged more than a small distance since the mouse down, cancel
    // any modal widget startup in progress
    if(magnitude(Vec2(systemPosition - _clickAndHoldStartPosition)) > kClickAndHoldRadius) // TODO operator- types
    {
      _clickAndHoldTimer.stop();
      cancelModalWidget();
    }
  }
}

void PluginEditorView::dismissModalWidgetOnClick(GUIEvent e)
{
  if(e.type == "down")
  {
    // dismiss modal widget on a click outside its bounds
    if(_activeModalWidget)
    {
      if(_widgets[_activeModalWidget]->getTextProperty("state") == "open")
      {
        if(!within(e.position, getBoundsRect(*_widgets[_activeModalWidget])))
        {
          _widgets[_activeModalWidget]->processValueChange({"state", "closing"});
          _activeModalWidget = {} ;
        }
      }
    }
  }
}

// Handle the queue of GUIEvents from the View by routing events to Widgets
// and handling any returned ValueChanges.

void PluginEditorView::handleGUIEvents()
{
  while (_inputQueue.elementsAvailable())
  {
    GUIEvent nativeEvent = _inputQueue.pop();
    GUIEvent gridEvent(nativeEvent);
    gridEvent.position = _GUICoordinates.nativeToGrid(gridEvent.position);
    
    // use View subclass to send input to Widgets
    processValueChangeList(View::processInput(_GUICoordinates, gridEvent));
    
    updateClickAndHold(nativeEvent);
    dismissModalWidgetOnClick(gridEvent);
  }
}

// get changed values from Processor and update Widgets
// this also pops editor / environment parameters from the Processor. So, maybe rename
void PluginEditorView::updateWidgets()
{

// debug
  auto t = _controller._changesToReport.elementsAvailable();
  if(t > 0)
  {
  std::cout << "***** PluginEditorView: changes: " << t << " *****\n";
  }
  
  
  while(auto vc = _controller.getChange()) //
    //    while(auto vc = getMessage(_controller)) //       // TODO not a reference but a message pipe
  {
    switch(hash(head(vc.name)))
    {
      case hash("param"):
        {
          for(auto pw : _widgetsByParameter[tail(vc.name)])
          {
            if(!pw->engaged)
            {
              pw->processValueChange(vc);
            }
          }
        }
        break;
      case hash("editor_bounds"):
        {
          Matrix m = vc.newValue.getMatrixValue();
          if(m != Matrix()) // hack, fix
          {
            //setBounds(juce::Rectangle<int> (0, 0, m[2], m[3]));
          }
          else
          {
            //std::cout << "NULL Bounds\n";
          }
        }
        break;
      case hash("license_status"):
        {
          //std::cout << "FLASH: " << vc.newValue.getTextValue() << "\n";
          //startLicensePopup(vc.newValue.getTextValue());
        }
        break;
      default:
        break;
    }
  }
  
  // send published signals to widgets
  
  for(auto& w : _widgets)
  {
    if(w->getProperty("signalName"))
    {
      Text sigName = w->getTextProperty("signalName");
      auto buf = _controller.getSignalFromProcessor(sigName); // again remove API TODO controller sends changed signals via messages
      if(buf)
      {
        if(buf->getReadAvailable())
        {
          // the widget can use DSPBuffer.peekMostRecent() to view the signal without clearing it
          w->processSignal(*buf);
          
          // discard the oldest signal, more slowly than it will be used by the widget.
          buf->discard(kFloatsPerDSPVector*4);
        }
      }
    }
  }
  
  // DEBUG
  //_widgets["last_click"]->properties["text"] = _clickText;
}

// -> view?
void PluginEditorView::markEverythingAsDirty()
{
  _redrawAllUntilFrame = _frameCounter + kScreenBuffers;
}

// Widget implementation

void PluginEditorView::processValueChange(ValueChange v)
{
  Path destination = v.name;
  if(head(destination) == "param")
  {
  
  std::cout << " param: " << destination << "\n";
    // destination is just a parameter.
    // change a parameter using the controller.
    
    // TODO not a reference but a message pipe
    // sendMessage(_controller, v);
    _controller.handleChangeFromEditor(v);
  }
  else if(head(destination) == "processor")
  {
    // request special changes from the processor, not parameters but other states
    //_processor.handleChange(v);
  }
  else if(head(destination) == "editor")
  {
    // handle editor changes here in the editor itself
    destination = tail(destination);
    if (head(destination) == "size")
    {
      resizeEditor(constrainSize(matrixToVec2(v.newValue.getMatrixValue())));
    }
    
    else if (head(destination) == "popup")
    {
      // tail of destination is now the parameter path
      destination = tail(destination);
      
      // handle popup requests from Widgets
      // to generalize, sender could request a different modal widget beforehand
      Path modalWidgetRequested {"popup"};
      Path senderParam = tail(destination);
      switch(hash(head(destination)))
      {
        case(hash("pre_open")):
        {
          if(!_activeModalWidget)
          {
            // the widget is made inactive before the close animation, so we also need to
            // check its state to make sure the animation is not still happening
            if(_widgets[modalWidgetRequested]->getTextProperty("state") == "closed")
            {
              // click-and-hold before popup commits to opening
              //_activeModalWidgetTargetParam = senderParam;
              _activeModalWidget = modalWidgetRequested;
              auto& modalWidget = _widgets[_activeModalWidget];
              
              // send stem start position and state change to popup
              modalWidget->processValueChange({"position", v.newValue.getMatrixValue()});
              modalWidget->processValueChange({"state", "pre_open"});
              
              // set target param for modal widget
              modalWidget->processValueChange(ValueChange{concat("target", senderParam)});
  
              // send currently targeted popup parameter descriptions and values to Popup widget
              for(auto& partialParamName : _popupParamNames)
              {
                Path fullParamName = concat(senderParam, partialParamName);
                
                ParameterDescription* targetParamDescPtr = _controller.getParamDescriptionByPath(fullParamName);
               if(targetParamDescPtr)
               {
                // store a copy of the parameter description we are targeting
                 ParameterDescription paramDesc = *targetParamDescPtr;
                 _popupParams[partialParamName] = make_unique< ParameterDescription >(paramDesc);
                 
                 // change the name of the description to the partial name for the popup parameter,
                 // and broadcast a pointer to it to the popup
                  _popupParams[partialParamName]->setProperty("name", pathToText(partialParamName));
                  _widgets[_activeModalWidget]->broadcastParameterDescription(_popupParams[partialParamName].get());
                
                  auto paramID = _controller.getParamIDByName(fullParamName);
                  auto paramValue = _controller.getParamNormalized(paramID);
                  
                  std::cout << paramID << " -> " << paramValue << "\n";
                  modalWidget->processValueChange({concat("enable", partialParamName), true});
                  modalWidget->processValueChange({concat("param", partialParamName), paramValue});
                }
                else
                {
                  modalWidget->processValueChange({concat("enable", partialParamName), false});
                }
              }
            }
          }
          break;
        }
        case(hash("content_size")):
        {
          // used by widgets to request a popup size
          if(_activeModalWidget)
          {
            auto& modalWidget = _widgets[_activeModalWidget];
            auto sizeMatrix = v.newValue.getMatrixValue();
            modalWidget->processValueChange({"content_size", sizeMatrix});
          }
          break;
        }
        case(hash("opening")):
        {
          if(_activeModalWidget)
          {
            auto& popupWidget = _widgets[_activeModalWidget];
            
            // get bounds where we'll put popup.
            // try to float on the side of the widget closest to the click
            
            auto widgetPtrs = _widgetsByParameter[senderParam];
            if(widgetPtrs.size() > 0)
            {
              // NOTE widgets with more than one parameter may cause problems
              Widget* pw = widgetPtrs[0];
              
              Rect senderBounds = getBoundsRect(*pw);
              auto senderCenter = getCenter(senderBounds);
              
              Vec2 clickPosition = matrixToVec2(v.newValue.getMatrixValue());
              Vec2 clickPositionOnGrid = translate(clickPosition, senderCenter);
              Rect popupRect = popupWidget->getRectPropertyWithDefault("content_size", kDefaultPopupSize);
              Symbol side = (clickPositionOnGrid.x() > senderCenter.x()) ? "right" : "left";
              constexpr float kMargin = 0.25f;
              auto popupRectFloated = floatToSide(senderBounds, popupRect, kMargin, kGridUnitsX, kGridUnitsY, side);
              
              // send content bounds to widget and start opening animation
              popupWidget->processValueChange({"content_bounds", rectToMatrix(popupRectFloated)});
              popupWidget->processValueChange({"state", "opening"});
            }
          }
          break;
        }
          
        case(hash("closing")):
          // handle request from the popup itself telling us it's closing soon. 
          _activeModalWidget = {};
          break;
      }
    }
  }
}


void PluginEditorView::drawBackground(DrawContext dc, bool drawGrid)
{
  NativeDrawContext* nvg = getNativeContext(dc);
  auto white = nvgRGBAf(1, 1, 1, 0.25);
  auto bgColorA = nvgRGBA(131, 162, 199, 255);
  auto bgColorB = nvgRGBA(115, 149, 185, 255);
  int u = dc.coords.gridSize;
  
  NVGpaint paintPattern;
  Resource* pr = getResource(dc, "background");
  
  if (pr)
  {
    /// MLTEST TODO better cache image handles
    const int* pImgHandle = reinterpret_cast<const int *>(pr->data());
    int bgh = pImgHandle[0];
    int backgroundHandle = bgh;
    
    //  int imgw, imgh;
    //  nvgImageSize(nvg, backgroundHandle, &imgw, &imgh);
    paintPattern = nvgImagePattern(nvg, -u, -u, dc.coords.nativeWidth + u, dc.coords.nativeHeight + u, 0, backgroundHandle, 1.0f);
  }
  else
  {
    paintPattern = nvgLinearGradient(nvg, 0, -u, 0, dc.coords.nativeHeight + u, bgColorA, bgColorB);
  }
  
  // draw background image or gradient
  nvgBeginPath(nvg);
  nvgRect(nvg, 0, 0, dc.coords.nativeWidth, dc.coords.nativeHeight);
  nvgFillPaint(nvg, paintPattern);
  
  nvgFill(nvg);
  
  int gridLevels = 1;
  static auto levelProj = ml::projections::linear({0.f, gridLevels + 0.f}, {1., 0.5});
  
  /*
   if(drawGrid)
   {
   nvgStrokeColor(nvg, white);
   nvgStrokeWidth(nvg, 1.0f*dc.coords.displayScale); // clean single-pixel or 2-pixel lines
   float fx = 0.5f;
   float fy = 0.5f;
   int u = dc.coords.gridSize;
   
   for(int k=0; k<gridLevels; ++k)
   {
   float ratio = levelProj(k);
   nvgScale(nvg, ratio, ratio);
   
   for(int i=0; i <= kGridUnitsX; ++i)
   {
   nvgBeginPath(nvg);
   nvgMoveTo(nvg, _borderRect.x() + i*u + fx, _borderRect.top());
   nvgLineTo(nvg, _borderRect.x() + i*u + fx, _borderRect.bottom());
   nvgStroke(nvg);
   }
   for(int i=0; i <= kGridUnitsY; ++i)
   {
   nvgBeginPath(nvg);
   nvgMoveTo(nvg, _borderRect.left(), _borderRect.y() + i*u + fy);
   nvgLineTo(nvg, _borderRect.right(), _borderRect.y() + i*u + fy);
   nvgStroke(nvg);
   }
   nvgResetTransform(nvg);
   }
   }*/
}


}}} // namespaces



