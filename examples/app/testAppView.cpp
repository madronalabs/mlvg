// mlvg test application
// Copyright (C) 2019-2022 Madrona Labs LLC
// This software is provided 'as-is', without any express or implied warranty.
// See LICENSE.txt for details.

#include "testAppView.h"
#include "../build/resources/testapp/resources.c"

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
  
  Vec2 gridDims = getSizeInGridUnits();
  int gx = gridDims.x();
  int gy = gridDims.y();
  
  // set grid size of entire view, for background and other drawing
  _view->setProperty("grid_units_x", gx);
  _view->setProperty("grid_units_y", gy);
  
  // layout dials
  _view->_widgets["freq1"]->setBounds(alignCenterToPoint(largeDialRect, {1, 1}));
  _view->_widgets["freq2"]->setBounds(alignCenterToPoint(largeDialRect, {gx - 1.f, 1}));
  _view->_widgets["freq2b"]->setBounds(alignCenterToPoint(largeDialRect, {gx - 3.f, 1}));
  _view->_widgets["gain"]->setBounds(alignCenterToPoint(largeDialRect, {gx - 1.f, gy - 1.f}));
  
  // layout test image
  _view->_widgets["tess"]->setBounds(alignCenterToPoint(largeDialRect, {1, gy - 1.f}));
  
  // layout labels
  auto positionLabelUnderDial = [&](Path dialName)
  {
    Path labelName (TextFragment(pathToText(dialName), "_label"));
    ml::Rect dialRect = _view->_widgets[dialName]->getRectProperty("bounds");
    _view->_backgroundWidgets[labelName]->setRectProperty
    ("bounds", alignCenterToPoint(labelRect, dialRect.bottomCenter() - Vec2(0, 0.125)));
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
  
  _view->_widgets["open"]->setRectProperty("bounds", alignCenterToPoint(textButtonRect, {centerX, buttonsY1}));
  
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
  _drawingProperties.setProperty("mark", colorToMatrix({0.01, 0.01, 0.01, 1.0}));
  _drawingProperties.setProperty("background", colorToMatrix({0.8, 0.8, 0.8, 1.0}));
  _drawingProperties.setProperty("draw_background_grid", true);
  _drawingProperties.setProperty("common_stroke_width", 1/32.f);
  
  // fonts
  int font1 = nvgCreateFontMem(nvg, "MLVG_sans", (unsigned char*)resources::D_DIN_otf, resources::D_DIN_otf_size, 0);
  const unsigned char* pFont1 = reinterpret_cast<const unsigned char *>(&font1);
  _resources["d_din"] = std::make_unique< Resource >(pFont1, pFont1 + sizeof(int));

  int font2 = nvgCreateFontMem(nvg, "MLVG_italic", (unsigned char *)resources::D_DIN_Italic_otf, resources::D_DIN_Italic_otf_size, 0);
  const unsigned char* pFont2 = reinterpret_cast<const unsigned char *>(&font2);
  _resources["d_din_italic"] = std::make_unique< Resource >(pFont2, pFont2 + sizeof(int));
  
  // raster images
  int flags = 0;
  int img1 = nvgCreateImageMem(nvg, flags, (unsigned char *)resources::vignette_jpg, resources::vignette_jpg_size);
  const unsigned char* pImg1 = reinterpret_cast<const unsigned char *>(&img1);
  //_resources["background"] = std::make_unique< Resource >(pImg1, pImg1 + sizeof(int));
  
  // SVG images
  ml::AppView::createVectorImage("tesseract", resources::Tesseract_Mark_svg, resources::Tesseract_Mark_svg_size);
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
  } );
  
  // buttons
  _view->_widgets.add_unique< TextButtonBasic >("open", WithValues{
    {"text", "open" },
    {"text_size", 0.4f },
    {"action", "open" }
  } );
  
  // make all the above Widgets visible
  forEach< Widget >
  (_view->_widgets, [&](Widget& w)
   {
    w.setProperty("visible", true);
  }
   );
  
  _setupWidgets(pdl);
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
        case(hash("set_audio_data")):
        {
          // get Sample pointer
          //Sample* pSample = *reinterpret_cast<Sample**>(msg.value.getBlobValue());
          //_view->_widgets["sample"]->receiveNamedRawPointer("sample", pSample);
          
          break;
        }
          
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

