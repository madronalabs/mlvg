
// mlvg: GUI library for madronalib apps and plugins
// Copyright (C) 2019-2022 Madrona Labs LLC
// This software is provided 'as-is', without any express or implied warranty.
// See LICENSE.txt for details.


#include "nanovg.h"
#include "nanovg_mtl.h"

#include <MetalKit/MetalKit.h>

#include "MLAppView.h"
#include "MLPlatformView.h"
#include "MLGUICoordinates.h"
#include "MLGUIEvent.h"
#include "MLDrawContext.h"

// MyMTKView

@interface MyMTKView : MTKView

- (void)setAppView:(AppView*) pView;
- (AppView*)getAppView;
- (NSPoint) convertPointToScreen:(NSPoint)point;
- (void)convertEventPositions:(NSEvent *)appKitEvent toGUIEvent:(GUIEvent*)vgEvent;
- (void)convertEventPositionsWithOffset:(NSEvent *)appKitEvent withOffset:(NSPoint)offset toGUIEvent:(GUIEvent*)vgEvent;
- (void)convertEventFlags:(NSEvent *)appKitEvent toGUIEvent:(GUIEvent*)vgEvent;

@end

@implementation MyMTKView
{
@private
  AppView* _appView;
  NSPoint _totalDrag;
}

- (void)setAppView:(AppView*) pView
{
  _appView = pView;
}

- (AppView*)getAppView
{
  return _appView;
}

- (NSPoint) convertPointToScreen:(NSPoint)point
{
  NSRect convertRect = [self.window convertRectToScreen:NSMakeRect(point.x, point.y, 0.0, 0.0)];
  return NSMakePoint(convertRect.origin.x, convertRect.origin.y);
}

- (void)convertEventPositions:(NSEvent *)appKitEvent toGUIEvent:(GUIEvent*)vgEvent
{
  NSPoint pt = [self convertPoint:[appKitEvent locationInWindow] fromView:nil];
  NSPoint fromBottom = NSMakePoint(pt.x, self.frame.size.height - pt.y);
  NSPoint fromBottomBacking = [self convertPointToBacking:fromBottom ];
  vgEvent->position = Vec2(fromBottomBacking.x, fromBottomBacking.y);
}

- (void)convertEventPositionsWithOffset:(NSEvent *)appKitEvent withOffset:(NSPoint)offset toGUIEvent:(GUIEvent*)vgEvent;
{
  NSPoint eventLocation = [appKitEvent locationInWindow];
  NSPoint eventLocationWithOffset = NSMakePoint(eventLocation.x + offset.x, eventLocation.y + offset.y);
  NSPoint pt = [self convertPoint:eventLocationWithOffset fromView:nil];
  NSPoint fromBottom = NSMakePoint(pt.x, self.frame.size.height - pt.y);
  NSPoint fromBottomBacking = [self convertPointToBacking:fromBottom ];
  vgEvent->position = Vec2(fromBottomBacking.x, fromBottomBacking.y);
}

- (void)convertEventFlags:(NSEvent *)appKitEvent toGUIEvent:(GUIEvent*)vgEvent
{
  NSEventModifierFlags flags = [appKitEvent modifierFlags];

  uint32_t myFlags{0};
  if(flags&NSEventModifierFlagShift)
  {
    myFlags |= shiftModifier;
  }
  if(flags&NSEventModifierFlagCommand)
  {
    myFlags |= commandModifier;
  }
  if(flags&NSEventModifierFlagControl)
  {
    myFlags |= controlModifier;
  }
  vgEvent->keyFlags = myFlags;
}

- (void)viewDidChangeBackingProperties
{
  if(_appView)
  {
    _appView->setDisplayScale([[self window] backingScaleFactor]);
  }
}

- (void)setMousePosition:(NSPoint)newPos
{
  NSRect screen0Frame = NSScreen.screens[0].frame;
  float screenMaxY = NSMaxY(screen0Frame);
  NSPoint newPosFlipped = NSMakePoint(newPos.x, screenMaxY - newPos.y);
  
  CGAssociateMouseAndMouseCursorPosition(false);
  CGWarpMouseCursorPosition(newPosFlipped);
  CGAssociateMouseAndMouseCursorPosition(true);
}

// from NSView

- (BOOL)isOpaque
{
  return YES;
}

- (BOOL)acceptsFirstMouse:(nullable NSEvent *)theEvent
{
  return YES;
}

- (void)mouseDown:(NSEvent*)pEvent
{
  GUIEvent e{"down"};
  
  [self convertEventPositions:pEvent toGUIEvent:&e];
  [self convertEventFlags:pEvent toGUIEvent:&e];
  
  NSPoint pt = [self convertPointToScreen:[pEvent locationInWindow]];
  _totalDrag = NSMakePoint(0, 0);
  
  if (_appView)
  {
    _appView->pushEvent(e);
  }
}

- (void)rightMouseDown:(NSEvent*)pEvent
{
  GUIEvent e{"down"};
  
  [self convertEventPositions:pEvent toGUIEvent:&e];
  [self convertEventFlags:pEvent toGUIEvent:&e];
  
  // set control down for right click. TODO different event!
  e.keyFlags |= controlModifier;
  
  NSPoint pt = [self convertPointToScreen:[pEvent locationInWindow]];
  _totalDrag = NSMakePoint(0, 0);
  
  // rename to delegate? event handler? UIView?
  if (_appView)
  {
    _appView->pushEvent(e);
  }
}

- (void) mouseDragged:(NSEvent*)pEvent
{
  NSPoint eventPos = [self convertPointToScreen:[pEvent locationInWindow]];
  
  // get union of screens.
  // TODO move!
  int screenCount = NSScreen.screens.count;
  NSRect unionRect = NSZeroRect;
  for(int i=0; i< screenCount; ++i)
  {
    unionRect = NSUnionRect(unionRect, NSScreen.screens[i].frame);
  }
  float screenMinY = NSMinY(unionRect);
  float screenMaxY = NSMaxY(unionRect);

  // allow dragging past top and bottom of screen by repositioning the mouse.
  const CGFloat dragMargin = 50;
  NSPoint moveDelta;
  bool repositioned = false;
  if(eventPos.y < screenMinY + 1)
  {
    moveDelta = NSMakePoint(0, dragMargin);
    repositioned = true;
  }
  else if(eventPos.y > screenMaxY - 1)
  {
    moveDelta = NSMakePoint(0, -dragMargin);
    repositioned = true;
  }
  
  if(repositioned)
  {
    // don't send an event when repositioning
    NSPoint moveToPos = NSMakePoint(eventPos.x + moveDelta.x, eventPos.y + moveDelta.y);
    [self setMousePosition:moveToPos];
    _totalDrag = NSMakePoint(_totalDrag.x - moveDelta.x, _totalDrag.y - moveDelta.y);
  }
  else if (_appView)
  {
    GUIEvent e{"drag"};
    
    // add total drag offset to event and send
    [self convertEventPositionsWithOffset:pEvent withOffset:_totalDrag toGUIEvent:&e];
    [self convertEventFlags:pEvent toGUIEvent:&e];
    _appView->pushEvent(e);
  }
}

- (void)mouseUp:(NSEvent*) pEvent
{
  GUIEvent e{"up"};
  
  [self convertEventPositions:pEvent toGUIEvent:&e];
  [self convertEventFlags:pEvent toGUIEvent:&e];
  
  if (_appView)
  {
    _appView->pushEvent(e);
  }
}

- (void)rightMouseUp:(NSEvent*) pEvent
{
  GUIEvent e{"up"};
  
  [self convertEventPositions:pEvent toGUIEvent:&e];
  [self convertEventFlags:pEvent toGUIEvent:&e];
  
  // set control down for right click. TODO different event!
  e.keyFlags |= controlModifier;

  if (_appView)
  {
    _appView->pushEvent(e);
  }
}

Vec2 makeDelta(CGFloat x, CGFloat y)
{
  return Vec2(x, y);
}

- (void)scrollWheel:(NSEvent *)pEvent
{
  GUIEvent e{"scroll"};

  Vec2 eventDelta = makeDelta(pEvent.deltaX, pEvent.deltaY);
  if(eventDelta != Vec2())
  {
    [self convertEventPositions:pEvent toGUIEvent:&e];
    [self convertEventFlags:pEvent toGUIEvent:&e];
    e.delta = eventDelta;
    
    // restore device values if user has "natural scrolling" off in Prefs
    bool scrollDirectionNatural = pEvent.directionInvertedFromDevice;
    if(!scrollDirectionNatural) e.delta *= Vec2(-1, -1);
    
    if (_appView)
    {
      _appView->pushEvent(e);
    }
  }
}

- (void)keyDown:(NSEvent*)pEvent
{
  GUIEvent e{"keydown"};
  
  [self convertEventPositions:pEvent toGUIEvent:&e];
  [self convertEventFlags:pEvent toGUIEvent:&e];
  
  auto chars = [pEvent charactersIgnoringModifiers];
  auto utf8 = [chars UTF8String];
  TextFragment t(utf8);
  auto it = t.begin();
  e.keyCode = *it;
  
  NSPoint pt = [self convertPointToScreen:[pEvent locationInWindow]];
  _totalDrag = NSMakePoint(0, 0);
   
  if (_appView->willHandleEvent(e))
  {
    _appView->pushEvent(e);
  }
  else
  {
    [super keyDown:pEvent];
  }
}


- (BOOL)acceptsFirstResponder
{
  return YES;
}

@end


// MetalNanoVGRenderer

@interface MetalNanoVGRenderer : NSObject < MTKViewDelegate >
- (nonnull instancetype)initWithMetalKitView:(nonnull MyMTKView*)mtkView withBounds:(NSRect)b withScale:(float)scale;
- (void) resize:(CGSize)size ;
@end

@implementation MetalNanoVGRenderer
{
@private
  id<MTLDevice> _device;
  NVGcontext* _nvg;
  AppView* _appView;
  std::unique_ptr< DrawableImage > _backingLayer;
  Vec2 _nativeSize;
}

- (nonnull instancetype)initWithMetalKitView:(nonnull MyMTKView*)mtkView withBounds:(NSRect)bounds withScale:(float)scale
{
  if (self = [super init])
  {
    _device = mtkView.device;
    int width = bounds.size.width;
    int height = bounds.size.height;

    // NVG_STENCIL_STROKES looks bad
    // NVG_TRIPLE_BUFFER has been recommended, NVG_DOUBLE_BUFFER seems to work as well
    _nvg = nvgCreateMTL((__bridge void *)mtkView.layer, NVG_ANTIALIAS | NVG_TRIPLE_BUFFER);
    if(!_nvg)
    {
      NSLog(@"Could not create nanovg.");
      return self;
    }
    
    _appView = [mtkView getAppView];
    _appView->setDisplayScale(scale);
    _appView->initializeResources(_nvg);
  }
  return self;
}

-(void)dealloc
{
  if(_nvg)
  {
    _backingLayer = nullptr;
    nvgDeleteMTL(_nvg);
  }
}


// MTKViewDelegate protocol

// resize callback, called by the MTKView in pixel coordinates
- (void) mtkView: (nonnull MTKView*)view drawableSizeWillChange:(CGSize)newSize
{
  if(_appView)
  {
    float scale = _appView->getCoords().displayScale;
    CGSize scaledSize = CGSizeMake(newSize.width/scale, newSize.height/scale);
    [self resize: scaledSize];
  }
}

// draw callback, called by the MTKView to draw itself
- (void)drawInMTKView:(nonnull MTKView*)view
{
  if(_appView && _nvg && _backingLayer)
  {
    int w = _backingLayer->width;
    int h = _backingLayer->height;

    // give the view a chance to animate
    _appView->animate(_nvg);
    
    // draw the AppView to the backing layer
    drawToImage(_backingLayer.get());
    _appView->render(_nvg);
    
    // get border rect that the AppView is drawn into
    ml::Rect b = _appView->getBorderRect();
    float scale = _appView->getCoords().displayScale;
    b *= scale;
    
    // aspect ratio
    float ax = w/b.width();
    float ay = h/b.height();

    // blit backing layer to main layer
    drawToImage(nullptr);
    nvgBeginFrame(_nvg, w, h, 1.0f);
    
    // get image pattern, either stretching border rect to whole screen or blitting to the same size, leaving a border
    NVGpaint img;
    if(_appView->getStretchToScreenMode())
    {
      img = nvgImagePattern(_nvg, 0 - b.left()*ax, 0 - b.top()*ay, w*ax, h*ay, 0, _backingLayer->_buf->image, 1.0f);
    }
    else
    {
      img = nvgImagePattern(_nvg, 0, 0, w, h, 0, _backingLayer->_buf->image, 1.0f);
    }
    // blit the image
    nvgSave(_nvg);
    nvgResetTransform(_nvg);
    nvgBeginPath(_nvg);
    nvgRect(_nvg, 0, 0, w, h);
    nvgFillPaint(_nvg, img);
    nvgFill(_nvg);
    nvgRestore(_nvg);
    
    // end main update
    nvgEndFrame(_nvg);
  }
}


// TEMP
float drawImage(NVGcontext* vg, int image, float alpha,
                float sx, float sy, float sw, float sh, // sprite location on texture
                float x, float y, float w, float h) // position and size of the sprite rectangle on screen
{
  float ax, ay;
  int iw,ih;
  NVGpaint img;
  
  nvgImageSize(vg, image, &iw, &ih);
  
  // Aspect ration of pixel in x an y dimensions. This allows us to scale
  // the sprite to fill the whole rectangle.
  ax = w / sw;
  ay = h / sh;
  
  img = nvgImagePattern(vg, x - sx*ax, y - sy*ay, (float)iw*ax, (float)ih*ay,
                        0, image, alpha);
  nvgBeginPath(vg);
  nvgRect(vg, x,y, w,h);
  nvgFillPaint(vg, img);
  nvgFill(vg);
}



// internal resize, in system coordinates
- (void) resize:(CGSize)size
{
  int width = size.width;
  int height = size.height;
  
  if(!_appView) return;
  if(!_nvg) return;
  
  float displayScale = _appView->getCoords().displayScale;
  Vec2 systemSize(width, height);
  Vec2 viewSizeInPixels = systemSize*displayScale;

  if((viewSizeInPixels != _nativeSize) || (!_backingLayer.get()))
  {
    _nativeSize = viewSizeInPixels;
    _backingLayer = std::make_unique< DrawableImage >(_nvg, viewSizeInPixels.x(), viewSizeInPixels.y());
    _appView->viewResized(_nvg, systemSize);
  }
}

@end


// PlatformView static helpers

// get default point to put the center of a new window
Vec2 PlatformView::getPrimaryMonitorCenter()
{
  NSScreen* screen =[NSScreen mainScreen];
  NSRect frm = [screen frame];
  float x = frm.origin.x + frm.size.width/2;
  float y = frm.origin.y + frm.size.height/2;
  return Vec2{x, y};
}

// get the scale at the device covering the given point
float PlatformView::getDeviceScaleAtPoint(Vec2 p)
{
  return 1.0f;
}

// get the scale the OS considers the window's device to be at, compared to "usual" DPI
float PlatformView::getDeviceScaleForWindow(void* pParent, int platformFlags)
{
  if(!pParent) return 1.0f;
  
  // get parent view, from either NSWindow or NSView according to flag
  NSView* parentView{nullptr};
  if(platformFlags & PlatformView::kParentIsNSWindow)
  {
    auto parentWindow = (__bridge NSWindow *)(pParent);
    parentView = [parentWindow contentView];
  }
  else
  {
    parentView = (__bridge NSView *)(pParent);
  }
  
  NSRect frm = [parentView frame];
  NSRect backingFrm = [parentView convertRectToBacking:frm];
  

  
  float ratio = backingFrm.size.width / frm.size.width;
  
  
  return ratio;
}



ml::Rect PlatformView::getWindowRect(void* pParent, int platformFlags)
{
  ml::Rect r{0, 0, 0, 0};
  if(!pParent) return r;
  
  
  // get parent view, from either NSWindow or NSView according to flag
  NSView* parentView{nullptr};
  if(platformFlags & PlatformView::kParentIsNSWindow)
  {
    auto parentWindow = (__bridge NSWindow *)(pParent);
    parentView = [parentWindow contentView];
  }
  else
  {
    parentView = (__bridge NSView *)(pParent);
  }

  NSRect frm = [parentView frame];
  
  return ml::Rect{(float)frm.origin.x, (float)frm.origin.y,
    (float)frm.size.width, (float)frm.size.height};
}


// PlatformView class

struct PlatformView::Impl
{
  MyMTKView* _mtkView{nullptr};
  MetalNanoVGRenderer* _renderer{nullptr};
};

PlatformView::PlatformView(void* pParent, ml::Rect bounds, AppView* pView, void* platformHandle, int platformFlags, int targetFPS)
{
  if(!pView)
  {
    NSLog(@"PlatformView: null view!");
    return;
  }
  if(!pParent) return;
  
  // get parent view, from either NSWindow or NSView according to flag
  NSView* parentView{nullptr};
  if(platformFlags & PlatformView::kParentIsNSWindow)
  {
    auto parentWindow = (__bridge NSWindow *)(pParent);
    parentView = [parentWindow contentView];
  }
  else
  {
    parentView = (__bridge NSView *)(pParent);
  }

  NSRect boundsRectDefault = NSMakeRect(0, 0, bounds.width(), bounds.height());
  NSRect boundsRectBacking = [parentView convertRectToBacking: boundsRectDefault];
  NSRect parentFrame = [parentView frame];
  NSRect boundsRect = NSMakeRect(0, 0, bounds.width(), bounds.height());
  float displayScale = boundsRectBacking.size.width / boundsRectDefault.size.width;
  
  // make the new view
  MyMTKView* view = [[MyMTKView alloc] initWithFrame:(boundsRect) device:(MTLCreateSystemDefaultDevice())];
  
  if(!view)
  {
    NSLog(@"Failed to allocate MTK View!");
    return;
  }
  if(!view.device)
  {
    NSLog(@"Metal is not supported on this device.");
    return;
  }

  // view.framebufferOnly = NO; // set to NO if readback is needed
  view.layer.opaque = true;

  // create a new renderer for our view.
  [view setFrameSize:CGSizeMake(bounds.width(), bounds.height())];
  
  [view setAppView: pView];
  
  MetalNanoVGRenderer* renderer = [[MetalNanoVGRenderer alloc] initWithMetalKitView:view withBounds:boundsRect withScale:displayScale];
  if(!renderer)
  {
    NSLog(@"Renderer failed initialization");
    return;
  }

  // the MetalNanoVGRenderer is delegated by the MyMTKView to draw it and change size.
  view.delegate = renderer;

  // set origin here and not after resizing. important to get correct positioning in some hosts
  [view setFrameOrigin:CGPointMake(0, 0)];
  
  // We should set this to a frame rate that we think our renderer can consistently maintain.
  view.preferredFramesPerSecond = targetFPS;

  // add the new view to our parent view supplied by the host.
  [parentView addSubview: view];

  _pImpl = std::make_unique< Impl >();
  _pImpl->_mtkView = view;
  _pImpl->_renderer = renderer;
}

PlatformView::~PlatformView()
{
  if(_pImpl->_mtkView)
  {
    _pImpl->_mtkView.preferredFramesPerSecond = 0;
    _pImpl->_mtkView.paused = true;
    [_pImpl->_mtkView removeFromSuperviewWithoutNeedingDisplay];
    _pImpl->_mtkView = nullptr;
  }
}

// resize view, in pixel coordinates
void PlatformView::resizePlatformView(int w, int h)
{
  CGSize newSize = CGSizeMake(w, h);

  if (_pImpl->_mtkView)
  {
    [_pImpl->_mtkView setFrameSize:newSize];
  }
  
  if(_pImpl->_renderer)
  {
    [_pImpl->_renderer resize:newSize];
  }
}
