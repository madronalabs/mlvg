
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
  
  NSRect screen0Frame = NSScreen.screens[0].frame;
  float screenMaxY = NSMaxY(screen0Frame);
  
  // allow dragging past top and bottom of screen by repositioning the mouse.
  const CGFloat dragMargin = 50;
  NSPoint moveDelta;
  bool repositioned = false;
  if(eventPos.y < 1)
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
  std::unique_ptr< Layer > _backingLayer;
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
    _appView->initializeResources(_nvg);
    _appView->setDisplayScale(scale);
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

// resize callback, called by the MTKView in native pixel coordinates
- (void) mtkView: (nonnull MTKView*)view drawableSizeWillChange:(CGSize)newSize
{
  if(_appView)
  {
    float scale = _appView->getDisplayScale();
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

    // draw the AppView to the backing layer
    //drawToLayer(_backingLayer.get());
    //nvgBeginFrame(_nvg, w, h, 1.0f);
    
    _appView->renderView(_nvg, _backingLayer.get());
    
    // end backing layer update
    //nvgEndFrame(_nvg);
    //drawToLayer(nullptr);
    
    // blit backing layer to main layer
    drawToLayer(nullptr);
    nvgBeginFrame(_nvg, w, h, 1.0f);
    auto nativeImage = getNativeImageHandle(*_backingLayer);
    NVGpaint img = nvgImagePattern(_nvg, 0, 0, w, h, 0, nativeImage, 1.0f);
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

// internal resize, in system coordinates
- (void) resize:(CGSize)size
{
  int width = size.width;
  int height = size.height;
  
  if(!_appView) return;
  if(!_nvg) return;
  
  float displayScale = _appView->getDisplayScale();
  Vec2 systemSize(width, height);
  Vec2 viewSizeInPixels = systemSize*displayScale;
  
  
  std::cout << "resize: " << viewSizeInPixels << "\n";

  if((viewSizeInPixels != _nativeSize) || (!_backingLayer.get()))
  {
    
    
    
    _backingLayer = ml::make_unique< Layer >(_nvg, viewSizeInPixels.x(), viewSizeInPixels.y());
    
    if(!_backingLayer)
    {
      std::cout << "no backing yayer!\n";
    }
    
    _appView->viewResized(_nvg, systemSize);
    _nativeSize = viewSizeInPixels;
  }
}

@end


// PlatformView

struct PlatformView::Impl
{
  MyMTKView* _mtkView{nullptr};
  MetalNanoVGRenderer* _renderer{nullptr};
};

PlatformView::PlatformView(void* pParent, ml::Rect bounds, AppView* pView, void* platformHandle, int platformFlags)
{
  if(!pView)
  {
    NSLog(@"PlatformView: null view!");
    return;
  }
  if(!pParent) return;
  
  // get parent view, from either NSWindow or NSView according to flag
  NSView* parentView{nullptr};
  NSView* parentView2{nullptr};
  if(platformFlags & PlatformView::kParentIsNSWindow)
  {
    auto parentWindow = (__bridge NSWindow *)(pParent);
    parentView = [parentWindow contentView];
  }
  else
  {
    parentView = (__bridge NSView *)(pParent);
    //parentView2  = [parentWindow contentView];
  }

  NSRect boundsRectDefault = NSMakeRect(0, 0, bounds.width(), bounds.height());
  NSRect boundsRectBacking = [parentView convertRectToBacking: boundsRectDefault];
  NSRect parentFrame = [parentView frame];
  
  std::cout << "parent frame: " << parentFrame.origin.x << ", " << parentFrame.origin.y <<
  ", " << parentFrame.size.width << ", " << parentFrame.size.height << "\n";
  
  float displayScale = boundsRectBacking.size.width / boundsRectDefault.size.width;
  
  /*
  ml::Rect pixelBounds = bounds*displayScale;
  CGRect boundsRect = NSMakeRect(parentFrame.origin.x, parentFrame.origin.y, pixelBounds.width(), pixelBounds.height());
  MyMTKView* view = [[MyMTKView alloc] initWithFrame:(boundsRect) device:(MTLCreateSystemDefaultDevice())];
*/
  CGRect boundsRect = NSMakeRect(0, 0, bounds.width(), bounds.height());
  
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
  //[view setFrameSize:CGSizeMake(pixelBounds.width(), pixelBounds.height())];
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

  // set AppView scale
  pView->setDisplayScale(displayScale);
  
  // We should set this to a frame rate that we think our renderer can consistently maintain.
  view.preferredFramesPerSecond = 60;

  // add the new view to our parent view supplied by the host.
  [parentView addSubview: view];

  _pImpl = ml::make_unique< Impl >();
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
  }
}

void PlatformView::resizeView(int w, int h)
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
