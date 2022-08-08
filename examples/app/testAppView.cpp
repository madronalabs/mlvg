// mlvg test application
// Copyright (C) 2019-2022 Madrona Labs LLC
// This software is provided 'as-is', without any express or implied warranty.
// See LICENSE.txt for details.

#include "testAppView.h"

#include "madronalib.h"

#include "MLDialBasic.h"
#include "MLResizer.h"
#include "MLTextLabelBasic.h"
#include "MLSVGImage.h"
#include "MLSVGButton.h"

#include "MLParameters.h"
#include "MLSerialization.h"

#include "../build/resources/testapp/resources.c"

constexpr float textSizeFix{0.85f};
ml::Rect largeDialRect{0, 0, 1.5, 1.5};
ml::Rect labelRect(0, 0, 2, 0.125);

TestAppView::TestAppView(TextFragment appName, size_t instanceNum) :
  AppView(appName, instanceNum)
{
}

void TestAppView::layoutView()
{
  Vec2 gridDims = getSizeInGridUnits();
  int gx = gridDims.x();
  int gy = gridDims.y();

  // set grid size of entire view, for background and other drawing
  _view->setProperty("grid_units_x", gx);
  _view->setProperty("grid_units_y", gy);
    
  Rect viewBounds(0, 0, gx, gy);
  
  // layout dials
  _view->_widgets["freq1"]->setBounds(alignCenterToPoint(largeDialRect, {1, 1}));
  _view->_widgets["freq2"]->setBounds(alignCenterToPoint(largeDialRect, {gx - 1.f, 1}));
  _view->_widgets["gain"]->setBounds(alignCenterToPoint(largeDialRect, {gx - 1.f, gy - 1.f}));
  
  // layout test image
  _view->_widgets["tess"]->setBounds(alignCenterToPoint(largeDialRect, {1, gy - 1.f}));
  
  // layout labels
  ml::Rect labelRect(0, 0, 2, 0.5);
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
}

TestAppView::~TestAppView ()
{

}

int TestAppView::getElapsedTime()
{
  // return time elapsed since last render
  time_point<system_clock> now = system_clock::now();
  auto elapsedTime = duration_cast<milliseconds>(now - _previousFrameTime).count();
  _previousFrameTime = now;
  return elapsedTime;
}

#pragma mark from ml::AppView

void TestAppView::initializeResources(NativeDrawContext* nvg)
{
  // initialize drawing properties before controls are made
  _drawingProperties.setProperty("mark", colorToMatrix({0.01, 0.01, 0.01, 1.0}));
  _drawingProperties.setProperty("background", colorToMatrix({0.8, 0.8, 0.8, 1.0}));
  _drawingProperties.setProperty("draw_background_grid", true);

  // fonts
  int font1 = nvgCreateFontMem(nvg, "MLVG_sans", (unsigned char*)resources::D_DIN_otf, resources::D_DIN_otf_size, 0);
  const unsigned char* pFont1 = reinterpret_cast<const unsigned char *>(&font1);
  _resources["d_din"] = ml::make_unique< Resource >(pFont1, pFont1 + sizeof(int));

  int font2 = nvgCreateFontMem(nvg, "MLVG_italic", (unsigned char *)resources::D_DIN_Italic_otf, resources::D_DIN_Italic_otf_size, 0);
  const unsigned char* pFont2 = reinterpret_cast<const unsigned char *>(&font2);
  _resources["d_din_italic"] = ml::make_unique< Resource >(pFont2, pFont2 + sizeof(int));
  
  // raster images
  int flags = 0;
  int img1 = nvgCreateImageMem(nvg, flags, (unsigned char *)resources::vignette_jpg, resources::vignette_jpg_size);
  const unsigned char* pImg1 = reinterpret_cast<const unsigned char *>(&img1);
  //_resources["background"] = ml::make_unique< Resource >(pImg1, pImg1 + sizeof(int));
  
  // SVG images
  ml::AppView::createVectorImage("tesseract", resources::Tesseract_Mark_svg, resources::Tesseract_Mark_svg_size);
}

void TestAppView::makeWidgets()
{
  // add labels to background
  auto addControlLabel = [&](Path name, TextFragment t)
  {
    _view->_backgroundWidgets.add_unique< TextLabelBasic >(name, WithValues{
      { "bounds", rectToMatrix(labelRect) },
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
  float largeDialSize{0.55f};
  _view->_widgets.add_unique< DialBasic >("freq1", WithValues{
    {"size", largeDialSize },
    {"param", "freq1" }
  } );
  
  _view->_widgets.add_unique< DialBasic >("freq2", WithValues{
    {"size", largeDialSize },
    {"param", "freq2" }
  } );
  
  _view->_widgets.add_unique< DialBasic >("gain", WithValues{
    {"size", largeDialSize },
    {"param", "gain" }
  } );
  
  _view->_widgets.add_unique< SVGImage >("tess", WithValues{
    {"image_name", "tesseract" }
  } );
  
  
  // make all the above Widgets visible
  forEach< Widget >
  (_view->_widgets, [&](Widget& w)
   {
    w.setProperty("visible", true);
  }
   );
}

void TestAppView::renderView(NativeDrawContext* nvg, Layer* backingLayer)
{
  if(!backingLayer) return;
  int w = backingLayer->width;
  int h = backingLayer->height;
  
  // TODO move resource types into Renderer, DrawContext points to Renderer
  DrawContext dc{nvg, &_resources, &_drawingProperties, &_vectorImages, _GUICoordinates};
  
  // first, allow Widgets to draw any needed animations outside of main nvgBeginFrame().
  // Do animations and handle any resulting messages immediately.
  MessageList ml = _view->animate(getElapsedTime(), dc);
  enqueueMessageList(ml);
  handleMessagesInQueue();
  
  // begin the frame on the backing layer
  drawToLayer(backingLayer);
  nvgBeginFrame(nvg, w, h, 1.0f);
  
  // TODO lower level clear?
  
  // if top level view is dirty, clear entire window
  if(_view->isDirty())
  {
    nvgBeginPath(nvg);
    nvgRect(nvg, ml::Rect{-10000, -10000, 20000, 20000});
    auto bgColor = getColor(dc, "background");

    nvgFillColor(nvg, bgColor);
    nvgFill(nvg);
  }
  
  ml::Rect topViewBounds = dc.coords.gridToNative(_view->getBounds());
  nvgIntersectScissor(nvg, topViewBounds);
  auto topLeft = getTopLeft(topViewBounds);
  nvgTranslate(nvg, topLeft);
  
  _view->draw(translate(dc, -topLeft));
  
  // end the frame.
  nvgEndFrame(nvg);
  _view->setDirty(false);
}

void TestAppView::doAttached (void* pParent, int flags)
{
  Vec2 dims = getDefaultDims();
  Vec2 cDims = _constrainSize(dims);
  if(pParent != _parent)
  {
    _platformView = ml::make_unique< PlatformView >(pParent, Vec4(0, 0, cDims.x(), cDims.y()), this, _platformHandle, flags);
    _parent = pParent;
  }
  
  doResize(cDims);
}

void TestAppView::pushEvent(GUIEvent g)
{
  _inputQueue.push(g);
}


// set new editor size in system coordinates.
void TestAppView::doResize(Vec2 newSize)
{
  int width = newSize[0];
  int height = newSize[1];
  float scale = _GUICoordinates.displayScale;
    
  // std::cout << "do resize: " << width << " x " << height << " (" << scale << ")\n";
  Vec2 newViewSize = Vec2(width, height)*scale;
  
  if(_GUICoordinates.pixelSize != newViewSize)
  {
    _GUICoordinates.pixelSize = newViewSize;
    
    // resize our canvas, in system coordinates
    if(_platformView)
      _platformView->resizeView(width, height);
  }
}

// Actor implementation

void TestAppView::onMessage(Message msg)
{
  std::cout << "TestAppView: onMessage: " << msg.address << " : " << msg.value << "\n";
  
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
          _params.setParamFromNormalizedValue(paramName, msg.value);
          
          // if the parameter change message is not from the controller,
          // forward it to the controller.
          if(!(msg.flags & kMsgFromController))
          {
            sendMessageToActor(_controllerName, msg);
          }
          
          // if the message comes from a Widget, we do send the parameter back
          // to other Widgets so they can synchronize. It's up to individual
          // Widgets to filter out duplicate values.
          _sendParameterToWidgets(msg);
          break;
        }
      }
      break;
    }
      
    case(hash("do")):
    {
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

