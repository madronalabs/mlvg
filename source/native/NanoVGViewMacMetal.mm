
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

Vec2 NSPointToVec2(NSPoint p)
{
  return Vec2{float(p.x), float(p.y)};
}

// convert CoreGraphics coordinate rect from Apple (origin at bottom left of main window, y=up)
// to rest of world coords (origin = top left of main window, y=down)
ml::Rect NSToMLRect(NSRect nsr)
{
  NSRect mainWindowRect =  [[NSScreen mainScreen] frame];
  
  float x = nsr.origin.x;
  float nsRectTop = nsr.origin.y + nsr.size.height;
  float y = mainWindowRect.size.height - nsRectTop;
  float width = nsr.size.width;
  float height = nsr.size.height;
  return ml::Rect(x, y, width, height);
}

// MyMTKView

@interface MyMTKView : MTKView
- (void)setAppView:(AppView*) pView;
- (NSPoint) convertPointToScreen:(NSPoint)point;
- (void)convertEventPositions:(NSEvent *)appKitEvent toGUIEvent:(GUIEvent*)vgEvent;
- (void)convertEventPositionsWithOffset:(NSEvent *)appKitEvent withOffset:(NSPoint)offset toGUIEvent:(GUIEvent*)vgEvent;
- (void)convertEventFlags:(NSEvent *)appKitEvent toGUIEvent:(GUIEvent*)vgEvent;
- (id)initWithFrame:(CGRect)aRect device:(id<MTLDevice>) device;
- (void) setFrameSize:(NSSize)size;
- (void) setPlatformView:(PlatformView *)view;
- (float) getDisplayScale;
@end

@implementation MyMTKView
{
@private
  AppView* appView_;
  PlatformView* platformView_;
  NSPoint totalDrag_;
  float displayScale;
  bool needsResize;
}

- (id)initWithFrame:(CGRect)aRect device:(id<MTLDevice>) mtlDevice;
{
  self = [super initWithFrame:aRect device:mtlDevice];
  
  if(self)
  {
    // initialize to default values
    displayScale = 1.0f;
    needsResize = false;
  }
  
  return self;
}

- (void)setAppView:(AppView*) pView
{
  appView_ = pView;
}

- (void) setPlatformView:(PlatformView *)view
{
  platformView_ = view;
}

- (float) getDisplayScale
{
  return displayScale;
}

- (void)setFrameSize:(NSSize)size
{
  [super setFrameSize:size];
}

- (NSPoint) convertPointToScreen:(NSPoint)point
{
  NSRect screenRect = [self.window convertRectToScreen:NSMakeRect(point.x, point.y, 0.0, 0.0)];
  NSSize screenSize = [ [ self.window screen ] frame ].size;
  
  float x = screenRect.origin.x;
  float y = screenSize.height - screenRect.origin.y;
  
  return NSMakePoint(x, y);
}

- (void)convertEventPositions:(NSEvent *)appKitEvent toGUIEvent:(GUIEvent*)vgEvent
{
  NSPoint eventLocation = [appKitEvent locationInWindow];
  
  NSPoint pt = [self convertPoint:eventLocation fromView:nil];
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

// send new display scale to the delegate. Will be called on PlatformView creation and when scale changes thereafter.
- (void)viewDidChangeBackingProperties
{
  displayScale = [[self window] backingScaleFactor];
  platformView_->setPlatformViewScale(displayScale);
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
  e.screenPos = NSPointToVec2(pt);
  totalDrag_ = NSMakePoint(0, 0);
  
  if(appView_)
  {
    appView_->pushEvent(e);
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
  e.screenPos = NSPointToVec2(pt);
  
  // reset drag
  totalDrag_ = NSMakePoint(0, 0);
  
  // rename to delegate? event handler? UIView?
  if(appView_)
  {
    appView_->pushEvent(e);
  }
}

- (void) mouseDragged:(NSEvent*)pEvent
{
  NSPoint eventPos = [self convertPointToScreen:[pEvent locationInWindow]];
  
  bool repositioned = false;
  /*
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
   */
  
  if(repositioned)
  {
    /*
     // don't send an event when repositioning
     NSPoint moveToPos = NSMakePoint(eventPos.x + moveDelta.x, eventPos.y + moveDelta.y);
     [self setMousePosition:moveToPos];
     totalDrag_ = NSMakePoint(totalDrag_.x - moveDelta.x, totalDrag_.y - moveDelta.y);
     */
  }
  else if(appView_)
  {
    GUIEvent e{"drag"};
    
    // add total drag offset to event and send
    [self convertEventPositionsWithOffset:pEvent withOffset:totalDrag_ toGUIEvent:&e];
    [self convertEventFlags:pEvent toGUIEvent:&e];
    e.screenPos = NSPointToVec2(eventPos);
    appView_->pushEvent(e);
  }
}

- (void)mouseUp:(NSEvent*) pEvent
{
  GUIEvent e{"up"};
  
  [self convertEventPositions:pEvent toGUIEvent:&e];
  [self convertEventFlags:pEvent toGUIEvent:&e];
  
  NSPoint pt = [self convertPointToScreen:[pEvent locationInWindow]];
  e.screenPos = NSPointToVec2(pt);
  
  
  if(appView_)
  {
    appView_->pushEvent(e);
  }
}

- (void)rightMouseUp:(NSEvent*) pEvent
{
  GUIEvent e{"up"};
  
  [self convertEventPositions:pEvent toGUIEvent:&e];
  [self convertEventFlags:pEvent toGUIEvent:&e];
  
  NSPoint pt = [self convertPointToScreen:[pEvent locationInWindow]];
  e.screenPos = NSPointToVec2(pt);
  
  // set control down for right click. TODO different event!
  e.keyFlags |= controlModifier;
  
  if(appView_)
  {
    appView_->pushEvent(e);
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
    
    NSPoint pt = [self convertPointToScreen:[pEvent locationInWindow]];
    e.screenPos = NSPointToVec2(pt);
    
    // restore device values if user has "natural scrolling" off in Prefs
    bool scrollDirectionNatural = pEvent.directionInvertedFromDevice;
    if(!scrollDirectionNatural) e.delta *= Vec2(-1, -1);
    
    if(appView_)
    {
      appView_->pushEvent(e);
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
  totalDrag_ = NSMakePoint(0, 0);
  
  if(appView_->willHandleEvent(e))
  {
    appView_->pushEvent(e);
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
- (nonnull instancetype) initWithMetalKitView:(nonnull MyMTKView*)mtkView withBounds:(NSRect)b withScale:(float)scale;
- (void) setAppView:(AppView*) pView;
- (void) resize:(CGSize)size;
- (void) setScale:(float)scale;
- (void) doResize;
- (NVGcontext*) getNativeContext;
@end

@implementation MetalNanoVGRenderer
{
@private
  id<MTLDevice> _device;
  NVGcontext* _nvg;
  AppView* appView_; // non-owning pointer
  std::unique_ptr< DrawableImage > _backingLayer;
  Vec2 _nativeSize;
  Vec2 _systemSize;
  float displayScale;
  bool needsResize;
}

- (nonnull instancetype)initWithMetalKitView:(nonnull MyMTKView*)mtkView withBounds:(NSRect)bounds withScale:(float)scale
{
  if (self = [super init])
  {
    _device = mtkView.device;
    int w = bounds.size.width;
    int h = bounds.size.height;
    
    // NVG_STENCIL_STROKES looks bad
    // NVG_TRIPLE_BUFFER has been recommended, NVG_DOUBLE_BUFFER seems to work as well
    _nvg = nvgCreateMTL((__bridge void *)mtkView.layer, NVG_ANTIALIAS | NVG_TRIPLE_BUFFER);
    if(!_nvg)
    {
      NSLog(@"Could not create nanovg.");
      return self;
    }
    
    appView_ = nullptr;
    displayScale = scale;
    needsResize = true;
  }
  return self;
}

- (void)setAppView:(AppView*) pView
{
  appView_ = pView;
}

-(void)dealloc
{
  _backingLayer = nullptr;
  if(_nvg)
  {
    nvgDeleteMTL(_nvg);
  }
}

// MTKViewDelegate protocol (renderer)

// resize callback, called by the MTKView in pixel coordinates
- (void) mtkView: (nonnull MTKView*)view drawableSizeWillChange:(CGSize)newSize
{
  // nothing now, an app resize here may have the menubar size added to it, which is not what we want
}

// draw callback, called by the MTKView to draw itself
- (void)drawInMTKView:(nonnull MTKView*)view
{
  if(needsResize)
  {
    [self doResize];
  }
  
  if(appView_ && _nvg && _backingLayer)
  {
    // give the view a chance to animate
    appView_->animate(_nvg);
    
    // draw the AppView to the backing layer
    drawToImage(_backingLayer.get());
    size_t w = _backingLayer->width;
    size_t h = _backingLayer->height;
        
    appView_->render(_nvg);
      
    // blit backing layer to main layer
    drawToImage(nullptr);
    nvgBeginFrame(_nvg, w, h, 1.0f);
    
    // get image pattern for 1:1 blit
    NVGpaint img = nvgImagePattern(_nvg, 0, 0, w, h, 0, _backingLayer->_buf->image, 1.0f);
        
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

- (void) resize:(CGSize)size
{
  _systemSize = Vec2(size.width, size.height);
  needsResize = true;
}

- (void) setScale:(float)scale
{
  displayScale = scale;
  needsResize = true;
}

// internal resize
- (void) doResize
{
  needsResize = false;
  _nativeSize = _systemSize*displayScale;
    
  Vec2 layerSize(0, 0);
  if(_backingLayer.get()) layerSize = Vec2(_backingLayer->width, _backingLayer->height);
  
  if(_nativeSize != layerSize)
  {
    _backingLayer = std::make_unique< DrawableImage >(_nvg, _nativeSize.x(), _nativeSize.y());
  }
  
  if(appView_ && _nvg)
  {
    appView_->viewResized(_nvg, _nativeSize, displayScale);
  }
}

- (NVGcontext*) getNativeContext
{
  return _nvg;
}

@end


#pragma mark PlatformView

// PlatformView static helpers (move to Utils module)

// get default point to put the center of a new window
Vec2 PlatformView::getPrimaryMonitorCenter()
{
  NSScreen* screen = [NSScreen mainScreen]; // NSScreen.screens[1]; //
  NSRect frm = [screen frame];
  return getCenter(NSToMLRect(frm));
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
  NSView* parentView{nullptr};
  AppView* pAppView{nullptr};
  Vec2 rendererSize;
};

PlatformView::PlatformView(void* pParent, AppView* pView, void* /*platformHandle*/, int platformFlags, int targetFPS)
{
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
  
  NSRect parentFrame = [parentView frame];
  
  ml::Rect bounds{(float)parentFrame.origin.x, (float)parentFrame.origin.y,
    (float)parentFrame.size.width, (float)parentFrame.size.height};
  NSRect boundsRect = NSMakeRect(0, 0, bounds.width(), bounds.height());
  
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
  
  [view setPlatformView:this];
  float initialScale = [view getDisplayScale];
  
  // view.framebufferOnly = NO; // set to NO if readback is needed
  view.layer.opaque = true;
  
  // create a new renderer for our view.
  
  CGSize cgRendererSize = CGSizeMake(bounds.width(), bounds.height());
  
  [view setFrameSize:cgRendererSize];
  
  MetalNanoVGRenderer* renderer = [[MetalNanoVGRenderer alloc] initWithMetalKitView:view withBounds:boundsRect withScale:initialScale];
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
  
  _pImpl = std::make_unique< Impl >();
  _pImpl->_mtkView = view;
  _pImpl->_renderer = renderer;
  _pImpl->parentView = parentView;
  _pImpl->pAppView = pView;
  _pImpl->rendererSize = Vec2(bounds.width(), bounds.height());
  [_pImpl->_mtkView setAppView: pView];
  [_pImpl->_renderer setAppView: pView];
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

void PlatformView::attachViewToParent()
{
  // add the new view to our parent view supplied by the host.
  [_pImpl->parentView addSubview: _pImpl->_mtkView];
  
  // initial resize
  NSRect parentFrame = [_pImpl->parentView frame];
  setPlatformViewSize(parentFrame.size.width, parentFrame.size.height);
}

// resize view, in system coordinates
// the Controller uses this API to resize e.g. by dragging
void PlatformView::setPlatformViewSize(int w, int h)
{
  Vec2 newSizeVec(w, h);
  _pImpl->rendererSize = newSizeVec;
  
  CGSize newSize = CGSizeMake(w, h);
  
  if (_pImpl->_mtkView)
  {
    [_pImpl->_mtkView setFrameSize:newSize];
  }
  
  if (_pImpl->_renderer)
  {
    [_pImpl->_renderer resize:newSize];
  }
}

void PlatformView::setPlatformViewScale(float newScale)
{
  // scale changes will come from the MTKView, so we need to set the scale of the renderer
  if (_pImpl->_renderer)
  {
    [_pImpl->_renderer setScale:newScale];
  }
  
  // re-setting the view's frame size is needed to correct the parent size
  if (_pImpl->_mtkView)
  {
    CGSize newSize = CGSizeMake(_pImpl->rendererSize[0], _pImpl->rendererSize[1]);
    [_pImpl->_mtkView setFrameSize:newSize];
  }
}

NativeDrawContext* PlatformView::getNativeDrawContext()
{
  return [_pImpl->_renderer getNativeContext];
}
