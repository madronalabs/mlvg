// mlvg test application
// Copyright (C) 2019-2022 Madrona Labs LLC
// This software is provided 'as-is', without any express or implied warranty.
// See LICENSE.txt for details.

#include "testAppView.h"
#include "testAppParameters.h"
#include "build/resources/resources.c"

TestAppView::TestAppView(TextFragment appName, size_t instanceNum) :
  AppView(appName, instanceNum)
{
}

TestAppView::~TestAppView ()
{
}

#pragma mark AppView

void TestAppView::layoutView(DrawContext dc)
{
  ml::Rect largeDialRect{0, 0, 1.5, 1.5};
  ml::Rect labelRect(0, 0, 2, 0.5);
  
  Vec2 pixelSize = dc.coords.viewSizeInPixels;
  float gridSize = dc.coords.gridSizeInPixels;
  
  int gx = pixelSize.x / gridSize;
  int gy = pixelSize.y / gridSize;

  
  // layout dials
  _view->_widgets["freq1"]->setBounds(alignCenterToPoint(largeDialRect, {1, 1}));
  _view->_widgets["freq2"]->setBounds(alignCenterToPoint(largeDialRect, {gx - 1.f, 1}));
  _view->_widgets["freq2b"]->setBounds(alignCenterToPoint(largeDialRect, {gx - 3.f, 1}));
  _view->_widgets["gain"]->setBounds(alignCenterToPoint(largeDialRect, {gx - 1.f, gy - 1.f}));
  
  // layout test images
  _view->_widgets["tess"]->setBounds(alignCenterToPoint(largeDialRect, { 1, gy - 1.f }));
  _view->_widgets["view1"]->setBounds(alignCenterToPoint({0, 0, 1.5, 1.5}, { 3, gy - 1.f }));

  // layout labels
  auto positionLabelUnderDial = [&](Path dialName)
  {
    Path labelName (TextFragment(pathToText(dialName), "_label"));
    ml::Rect dialRect = _view->_widgets[dialName]->getRectProperty("bounds");
    _view->_backgroundWidgets[labelName]->setRectProperty
    ("bounds", alignCenterToPoint(labelRect, getBottomCenter(dialRect) - Vec2(0, 0.125)));
  };
  for(auto dialName : {"freq1", "freq2", "gain"})
  {
    positionLabelUnderDial(dialName);
  }
  
  // layout buttons
  float centerX = gx/2.f;
  int bottomY = gy - 2;
  float buttonWidth = 3;
  float buttonHeight = 0.75;
  ml::Rect textButtonRect(0, 0, buttonWidth, buttonHeight);
  float halfButtonWidth = buttonWidth/2.f;
  float buttonsY1 = bottomY + 0.5;
  
  Rect openButtonRect = alignCenterToPoint(textButtonRect, { centerX, buttonsY1 });
  _view->_widgets["open"]->setRectProperty("bounds", openButtonRect);
  
  // resize widgets
  forEach< Widget >
  (_view->_widgets, [&](Widget& w)
   {
    w.resize(dc);
  });
}

void TestAppView::initializeResources(NativeDrawContext* nvg)
{
    // initialize drawing properties before controls are made
  _drawingProperties.setColorProperty("mark", { 0.01, 0.01, 0.01, 1.0 });
  _drawingProperties.setColorProperty("mark_bright", { 0.9, 0.9, 0.9, 1.0 });
    _drawingProperties.setColorProperty("background", { 0.8, 0.8, 0.8, 1.0 });
    _drawingProperties.setProperty("draw_background_grid", true);
    _drawingProperties.setProperty("common_stroke_width", 1 / 32.f);

    // _drawingProperties.setProperty("draw_widget_bounds", true);
    _drawingProperties.setProperty("draw_dirty_widgets", true);

    // fonts
    _resources.fonts["d_din"] = std::make_unique< FontResource >(nvg, "MLVG_sans", resources::D_DIN_otf, resources::D_DIN_otf_size);
    _resources.fonts["d_din_italic"] = std::make_unique< FontResource >(nvg, "MLVG_italic", resources::D_DIN_Italic_otf, resources::D_DIN_Italic_otf_size);

    // raster images
    _resources.rasterImages["vignette"] = std::make_unique< RasterImage >(nvg, resources::vignette_jpg, resources::vignette_jpg_size);

    // SVG images
    _resources.vectorImages["tesseract"] = std::make_unique< VectorImage >(nvg, resources::Tesseract_Mark_svg, resources::Tesseract_Mark_svg_size);

    // drawable images
    _resources.drawableImages["screen1"] = std::make_unique< DrawableImage >(nvg, 320, 240);
}

void TestAppView::clearResources()
{
  _resources.fonts.clear();
  _resources.rasterImages.clear();
  _resources.vectorImages.clear();
  _resources.drawableImages.clear();
}

void TestAppView::stop()
{
    stopTimersAndActor();
    clearResources();
    clearWidgets();

    _platformView = nullptr;
}

void TestAppView::attachToWindow(SDL_Window* window)
{
    // read parameter descriptions into a list
    ParameterDescriptionList pdl;
    readParameterDescriptions(pdl);

    ParentWindowInfo windowInfo = ml::getParentWindowInfo(window);

    _platformView = std::make_unique< PlatformView >(windowInfo.windowPtr, nullptr, windowInfo.flags, 60);

    // set initial size. When this is not a fixed-ratio app, the window sizes
    // freely and the grid unit size remains constant.
    setGridSizeDefault(kDefaultGridUnitSize);

#if TEST_FIXED_RATIO
    setFixedAspectRatio(kDefaultGridUnits);
#endif

   // connect PlatformView to the AppView before making widgets, to initialize resources
   _platformView->setAppView(this);

   // make UI and startup
   makeWidgets(pdl);
   startTimersAndActor();

   // resize will trigger layout of widgets, so wait until after making widgets to resize for the first time
   int w, h;
   SDL_GetWindowSize(window, &w, &h);
  _platformView->resizePlatformView(w, h);

   // connect window to the PlatformView: watch for window resize events during drag
   watcherData_ = ResizingEventWatcherData{ window, _platformView.get() };
   SDL_AddEventWatch(resizingEventWatcher, &watcherData_);
}

void TestAppView::makeWidgets(const ParameterDescriptionList& pdl)
{
  // add labels to background
  auto addControlLabel = [&](Path name, TextFragment t)
  {
    _view->_backgroundWidgets.add_unique< TextLabelBasic >(name, WithValues{
      { "h_align", "center" },
      { "v_align", "middle" },
      { "text", t },
      { "font", "d_din_italic" },
      { "text_size", 0.30 },
      { "text_spacing", 0.0f }
    } );
  };
  addControlLabel("freq1_label", "frequency L");
  addControlLabel("freq2_label", "frequency R");
  addControlLabel("gain_label", "gain");
  
  // add Dials to view
  float mediumDialSize{0.4f};
  float largeDialSize{0.55f};
  _view->_widgets.add_unique< DialBasic >("freq1", WithValues{
    {"size", largeDialSize },
    {"param", "freq1" }
  } );
  
  _view->_widgets.add_unique< DialBasic >("freq2", WithValues{
    {"size", largeDialSize },
    {"param", "freq2" }
  } );
  
  _view->_widgets.add_unique< DialBasic >("freq2b", WithValues{
    {"size", mediumDialSize },
    {"param", "freq2" }
  } );
  
  _view->_widgets.add_unique< DialBasic >("gain", WithValues{
    {"size", largeDialSize },
    {"param", "gain" }
  } );

  _view->_widgets.add_unique< SVGImage >("tess", WithValues{
    {"image_name", "tesseract" }
      });

  _view->_widgets.add_unique< DrawableImageView >("view1", WithValues{
    {"image_name", "screen1" }
      });

  // buttons
  _view->_widgets.add_unique< TextButtonBasic >("open", WithValues{
    {"text", "open" },
    {"text_size", 0.4f },
    {"action", "open" }
  } );

#if TEST_RESIZER
  _view->_widgets.add_unique< Resizer >("resizer", WithValues{
    {"fix_ratio", (kDefaultGridUnits.x) / (kDefaultGridUnits.y)},
    {"z", -3}, // stay on top of popups
    {"fixed_size", true},
    {"fixed_bounds", { -16, -16, 16, 16 }}, // fixed size rect in system coords
    {"anchor", {1, 1}} // for fixed-size widgets, anchor is a point on the view from top left {0, 0} to bottom right {1, 1}.
      });
#endif
  
  // make all the above Widgets visible
  forEach< Widget >
  (_view->_widgets, [&](Widget& w)
   {
    w.setProperty("visible", true);
  }
   );
  
  _setupWidgets(pdl);
}

void TestAppView::animate(NativeDrawContext* nvg)
{
  
  AppView::animate(nvg);
}

void TestAppView::render(NativeDrawContext * nvg)
{
  AppView::render(nvg);
}



// Actor implementation

void TestAppView::onMessage(Message msg)
{
  if(head(msg.address) == "editor")
  {
    // we are the editor, so remove "editor" and handle message
    msg.address = tail(msg.address);
  }
  
  switch(hash(head(msg.address)))
  {
    case(hash("set_param")):
    {
      switch(hash(head(tail(msg.address))))
      {
          case(hash("view_size")):
          {
              // TODO better API for all this, no matrixes
              Value v = msg.value;

              if(v && (v.getType() == Value::kBlob))
              {
                // resize platform view
                Point c = valueToPoint(v);
                Vec2 cs = constrainSize(c);
                _platformView->resizePlatformView(cs.x, cs.y);
                onResize(cs);

                  // set constrained value and send it back to Controller
                  Path paramName = tail(msg.address);
                  _params.setFromRealValue(paramName, msg.value);

                  // if size change is not from the controller, send it there so it will be saved with controller params.
                  if (!(msg.flags & kMsgFromController))
                  {
                      sendMessageToActor(_controllerName, Message{ "set_param/view_size", pointToValue(cs) });
                  }
              }
              break;
          }
        default:
        {
          // no local parameter was found, set a plugin parameter
          
          // store param value in local tree.
          Path paramName = tail(msg.address);
          _params.setFromNormalizedValue(paramName, msg.value);
          
          // if the parameter change message is not from the controller,
          // forward it to the controller.
          if(!(msg.flags & kMsgFromController))
          {
            sendMessageToActor(_controllerName, msg);
          }
          
          // if the message comes from a Widget, we do send the parameter back
          // to other Widgets so they can synchronize. It's up to individual
          // Widgets to filter out duplicate values.
          _sendParameterMessageToWidgets(msg);
          break;
        }
      }
      break;
    }
    case(hash("do")):
    {
      switch(hash(second(msg.address)))
      {
        default:
        {
          // if the message is not from the controller,
          // forward it to the controller.
          if(!(msg.flags & kMsgFromController))
          {
            sendMessageToActor(_controllerName, msg);
          }
          break;
        }
      }
      break;
    }
    default:
    {
      // try to forward the message to another receiver
      switch(hash(head(msg.address)))
      {
        case(hash("controller")):
        {
          msg.address = tail(msg.address);
          sendMessageToActor(_controllerName, msg);
          break;
        }
        default:
        {
          // uncaught
          break;
        }
      }
      break;
    }
  }
}

