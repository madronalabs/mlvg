
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

// MyMTKView

@interface MyMTKView : MTKView

- (void)setAppView:(AppView*) pView;
//- (AppView*)getAppView;
- (NSPoint) convertPointToScreen:(NSPoint)point;
- (void)convertEventPositions:(NSEvent *)appKitEvent toGUIEvent:(GUIEvent*)vgEvent;
- (void)convertEventPositionsWithOffset:(NSEvent *)appKitEvent withOffset:(NSPoint)offset toGUIEvent:(GUIEvent*)vgEvent;
- (void)convertEventFlags:(NSEvent *)appKitEvent toGUIEvent:(GUIEvent*)vgEvent;
- (id)initWithFrame:(CGRect)aRect device:(id<MTLDevice>) device;
- (void) setFrameSize:(NSSize)size;
- (void) setPlatformView:(PlatformView *)view;

@end

@implementation MyMTKView
{
@private
  AppView* appView_;
  PlatformView* platformView_;
  NSPoint totalDrag_;
  float displayScale;
  NSSize displaySize_;
  bool needsResize;
}

- (id)initWithFrame:(CGRect)aRect device:(id<MTLDevice>) mtlDevice;
{
  self = [super initWithFrame:aRect device:mtlDevice];
  
  if(self)
  {
    // initialize to default values
    displayScale = 1.0f;
    displaySize_ = NSMakeSize(0, 0);
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


- (void)setFrameSize:(NSSize)size
{
  displaySize_ = size;
  [super setFrameSize:size];
}

- (NSPoint) convertPointToScreen:(NSPoint)point
{
  NSRect screenRect = [self.window convertRectToScreen:NSMakeRect(point.x, point.y, 0.0, 0.0)];
  NSSize screenSize = [ [ self.window screen ] frame ].size;
  
  float x = screenRect.origin.x;
  float y = screenSize.height - screenRect.origin.y;
  x = clamp(x, 0.f, float(screenSize.width));
  y = clamp(y, 0.f, float(screenSize.height));

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
  
  NSLog(@"NEW display scale %f", displayScale);
  
  // send scale up to platform view, this is why we had to set the pointer with setPlatformView()
  if(platformView_)
  {
    platformView_->setPlatformViewDisplayScale(displayScale);
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
  e.screenPos = NSPointToVec2(pt) * displayScale;
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
  e.screenPos = NSPointToVec2(pt) * displayScale;
  
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
    e.screenPos = NSPointToVec2(eventPos) * displayScale;
    appView_->pushEvent(e);
  }
}

- (void)mouseUp:(NSEvent*) pEvent
{
  GUIEvent e{"up"};
  
  [self convertEventPositions:pEvent toGUIEvent:&e];
  [self convertEventFlags:pEvent toGUIEvent:&e];
  
  NSPoint pt = [self convertPointToScreen:[pEvent locationInWindow]];
  e.screenPos = NSPointToVec2(pt) * displayScale;

  
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
  e.screenPos = NSPointToVec2(pt) * displayScale;

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
    e.screenPos = NSPointToVec2(pt) * displayScale;
    
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
- (nonnull instancetype)initWithMetalKitView:(nonnull MyMTKView*)mtkView withBounds:(NSRect)b withScale:(float)scale;
- (void)setAppView:(AppView*) pView;
- (void) resize:(CGSize)size;
- (void) setDisplayScale:(float)scale;
- (void) resizeAppView;
@end

@implementation MetalNanoVGRenderer
{
@private
  id<MTLDevice> _device;
  NVGcontext* _nvg;
  AppView* appView_;
  std::unique_ptr< DrawableImage > _backingLayer;
  Vec2 _nativeSize;
  float displayScale;
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
    
    appView_ = nullptr;
    displayScale = scale;
    
    CGSize pixelSize = CGSizeMake(width*displayScale, height*displayScale);
    
    // dont resize here when creating because app view is not attached yet
    // [self resize:pixelSize];
    [self setDisplayScale:displayScale];
    
  }
  return self;
}

- (void)setAppView:(AppView*) pView
{
  appView_ = pView;
  appView_->initializeResources(_nvg);
}

- (void)setDisplayScale:(float)scale
{
  std::cout << "renderer setDisplayScale: " << displayScale << "\n";

  if(displayScale != scale)
  {
    displayScale = scale;
    [self resizeAppView];
  }
}

-(void)dealloc
{
  if(_nvg)
  {
    _backingLayer = nullptr;
    nvgDeleteMTL(_nvg);
  }
}


// MTKViewDelegate protocol (renderer)

// resize callback, called by the MTKView in pixel coordinates
- (void) mtkView: (nonnull MTKView*)view drawableSizeWillChange:(CGSize)newSize
{
  // std::cout << "renderer drawableSizeWillChange: " << newSize.width << " x " << newSize.height << "\n";
  [self resize: newSize];
}

// draw callback, called by the MTKView to draw itself
- (void)drawInMTKView:(nonnull MTKView*)view
{
  if(appView_ && _nvg && _backingLayer)
  {
    size_t w = _backingLayer->width;
    size_t h = _backingLayer->height;

    // give the view a chance to animate
    appView_->animate(_nvg);
    
    // draw the AppView to the backing layer
    drawToImage(_backingLayer.get());
    appView_->render(_nvg);
    
    // blit backing layer to main layer
    drawToImage(nullptr);
    nvgBeginFrame(_nvg, w, h, 1.0f);
    
    // get image pattern for 1:1 blit
    NVGpaint img;
    img = nvgImagePattern(_nvg, 0, 0, w, h, 0, _backingLayer->_buf->image, 1.0f);
    
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

// internal resize, in pixel size
- (void) resize:(CGSize)size
{
  Vec2 newNativeSize(size.width, size.height);

//  if((newNativeSize != _nativeSize) || (!_backingLayer.get()))
  {
    _nativeSize = newNativeSize;
    _backingLayer = std::make_unique< DrawableImage >(_nvg, _nativeSize.x(), _nativeSize.y());
    [self resizeAppView];
  }
}

- (void) resizeAppView
{
  if(appView_ && _nvg)
  {
    appView_->viewResized(_nvg, _nativeSize, displayScale);
  }
}


@end

#pragma mark PlatformView

// PlatformView static helpers (move to Utils module)

// get default point to put the center of a new window
Vec2 PlatformView::getPrimaryMonitorCenter()
{
  NSScreen* screen =[NSScreen mainScreen];
  NSRect frm = [screen frame];
  float x = frm.origin.x + frm.size.width/2;
  float y = frm.origin.y + frm.size.height/2;
  return Vec2{x, y};
}

// get the scale the OS considers the window's device to be at, compared to "usual" DPI
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

PlatformView::PlatformView(void* pParent, void* /*platformHandle*/, int platformFlags)
{
  if(!pParent) return;
  
  // Rect bounds = getWindowRect(pParent, platformFlags);
  
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

  NSRect boundsRectDefault = NSMakeRect(0, 0, bounds.width(), bounds.height());
  NSRect boundsRectBacking = [parentView convertRectToBacking: boundsRectDefault];
  
  NSRect boundsRect = NSMakeRect(0, 0, bounds.width(), bounds.height());
  displayScale_ = boundsRectBacking.size.width / boundsRectDefault.size.width;
  
  // make the new view
  MyMTKView* view = [[MyMTKView alloc] initWithFrame:(boundsRect) device:(MTLCreateSystemDefaultDevice())];
  [view setPlatformView:this];
  
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
  CGSize rendererSize = CGSizeMake(bounds.width(), bounds.height());

  [view setFrameSize:rendererSize];
  
  MetalNanoVGRenderer* renderer = [[MetalNanoVGRenderer alloc] initWithMetalKitView:view withBounds:boundsRect withScale:displayScale_];
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
  view.preferredFramesPerSecond = ml::kTargetFPS;


  _pImpl = std::make_unique< Impl >();
  _pImpl->_mtkView = view;
  _pImpl->_renderer = renderer;
  
  // add the new view to our parent view supplied by the host.
  // will call viewDidChangeBackingProperties.
  [parentView addSubview: view];

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

void PlatformView::setAppView(AppView* pView)
{
  [_pImpl->_mtkView setAppView: pView];
  [_pImpl->_renderer setAppView: pView];
}

void PlatformView::setPlatformViewDisplayScale(float scale)
{
  if (_pImpl->_renderer)
  {
    [_pImpl->_renderer setDisplayScale:scale];
  }
}
                     
// resize view, in pixel coordinates
void PlatformView::resizePlatformView(int w, int h)
{
  Vec2 newSizeVec(w, h);
  
  std::cout << "resizePlatformView: " << newSizeVec << "\n";
  
  //if(newSizeVec != displaySize_) // TEMP
  {
    displaySize_ = newSizeVec;
    CGSize newScreenSize = CGSizeMake(w, h);
    CGSize newPixelSize = CGSizeMake(w*displayScale_, h*displayScale_);
    
    if (_pImpl->_mtkView)
    {
      // set view frame size, which will trigger the renderer's drawableSizeWillChange call
      [_pImpl->_mtkView setFrameSize:newScreenSize];
    }
    
    if (_pImpl->_renderer)
    {
      // this will resize the renderer if it exists but is not yet connected via drawableSizeWillChange (first time)
      [_pImpl->_renderer resize:newPixelSize];
    }
  }
}
