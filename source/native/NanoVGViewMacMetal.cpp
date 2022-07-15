// mlvg: GUI library for madronalib apps and plugins
// Copyright (C) 2019-2022 Madrona Labs LLC
// This software is provided 'as-is', without any express or implied warranty.
// See LICENSE.txt for details.

#define NS_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#define MTK_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION

#include <Metal/Metal.hpp>
#include <MetalKit/MetalKit.hpp>

#include "MLVGView.h"
#include "MLRenderer.h"


class MetalNanoVGRenderer
{
public:
  MetalNanoVGRenderer( MTL::Device* pDevice );
  ~MetalNanoVGRenderer();
  void draw( MTK::View* pView );
  void resize( MTK::View* pView, CGSize size );
  
private:
  MTL::Device* _pDevice;
  MTL::CommandQueue* _pCommandQueue;
};

class MyMTKViewDelegate : public MTK::ViewDelegate
{
public:
  MyMTKViewDelegate( MTL::Device* pDevice );
  virtual ~MyMTKViewDelegate() override;
  virtual void drawInMTKView( MTK::View* pView ) override;
  virtual void drawableSizeWillChange( MTK::View* pView, CGSize size ) override;

private:
  MetalNanoVGRenderer* _pRenderer;
};

#pragma mark - MetalNanoVGRenderer


MetalNanoVGRenderer::MetalNanoVGRenderer( MTL::Device* pDevice )
: _pDevice( pDevice->retain() )
{
  _pCommandQueue = _pDevice->newCommandQueue();
}

MetalNanoVGRenderer::~MetalNanoVGRenderer()
{
  _pCommandQueue->release();
  _pDevice->release();
}

void MetalNanoVGRenderer::draw( MTK::View* pView )
{
  NS::AutoreleasePool* pPool = NS::AutoreleasePool::alloc()->init();
  
  MTL::CommandBuffer* pCmd = _pCommandQueue->commandBuffer();
  MTL::RenderPassDescriptor* pRpd = pView->currentRenderPassDescriptor();
  MTL::RenderCommandEncoder* pEnc = pCmd->renderCommandEncoder( pRpd );
  pEnc->endEncoding();
  pCmd->presentDrawable( pView->currentDrawable() );
  pCmd->commit();
  
  pPool->release();
}

void MetalNanoVGRenderer::resize( MTK::View* pView, CGSize size )
{
  int width = size.width;
  int height = size.height;
  
  
  /*
  if(!_rnd) return;
  if(!_nvg) return;
  
  float displayScale = _rnd->_GUICoordinates.displayScale;
  Vec2 newSize(width*displayScale, height*displayScale);
  
  if((newSize != _nativeSize) || (!_backingLayer.get()))
  {
    _backingLayer = ml::make_unique<Layer>(_nvg, width*displayScale, height*displayScale);
    _rnd->viewResized(width, height);
    _nativeSize = newSize;
  }
   */
  
}

#pragma mark - MyMTKViewDelegate

MyMTKViewDelegate::MyMTKViewDelegate( MTL::Device* pDevice )
: MTK::ViewDelegate()
, _pRenderer( new MetalNanoVGRenderer( pDevice ) )
{
}

MyMTKViewDelegate::~MyMTKViewDelegate()
{
  delete _pRenderer;
}

void MyMTKViewDelegate::drawInMTKView( MTK::View* pView )
{
  _pRenderer->draw( pView );
}

void MyMTKViewDelegate::drawableSizeWillChange( MTK::View* pView, CGSize size )
{
  _pRenderer->resize( pView, size );
}

#pragma mark - AppView

struct AppView::AppData
{
  NS::Window* _pWindow{nullptr};
  MTK::View* _pMtkView{nullptr};
  MTL::Device* _pDevice{nullptr};
  MyMTKViewDelegate* _pViewDelegate{nullptr};
  ml::AppRenderer* _pRenderer{nullptr};
};

AppView::AppView(void* pParent, ml::Rect bounds, AppRenderer* pR, void* platformHandle)
{
  _pData = make_unique<AppData>();
  _pData->_pRenderer = pR;
  
  CGRect frame = (CGRect){ {100.0, 100.0}, {512.0, 512.0} };
  
  _pData->_pDevice = MTL::CreateSystemDefaultDevice();
  
  _pData->_pMtkView = MTK::View::alloc()->init( frame, _pData->_pDevice );
  _pData->_pMtkView->setColorPixelFormat( MTL::PixelFormat::PixelFormatBGRA8Unorm_sRGB );
  _pData->_pMtkView->setClearColor( MTL::ClearColor::Make( 1.0, 0.0, 0.0, 1.0 ) );

  _pData->_pViewDelegate = new MyMTKViewDelegate( _pData->_pDevice );
  _pData->_pMtkView->setDelegate( _pData->_pViewDelegate );
  
  NS::View* pParentView = static_cast< NS::View* >(pParent);
  pParentView->addSubview(_pData->_pMtkView);
}

void AppView::resizeCanvas(int w, int h)
{
  CGSize newSize = CGSizeMake(w, h);
  
  if (_pData->_pMtkView)
  {
    _pData->_pMtkView->setDrawableSize(newSize);
  }
  if(_pData->_pRenderer)
  {
    _pData->_pViewDelegate->drawableSizeWillChange(_pData->_pMtkView, newSize);
  }
}

AppView::~AppView()
{
  _pData->_pMtkView->release();
  _pData->_pWindow->release();
  _pData->_pDevice->release();
  delete _pData->_pViewDelegate;
}
