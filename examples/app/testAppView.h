// mlvg test application
// Copyright (C) 2019-2022 Madrona Labs LLC
// This software is provided 'as-is', without any express or implied warranty.
// See LICENSE.txt for details.


#pragma once

#include "MLAppView.h"
#include "MLPlatformView.h"
#include "MLWidget.h"
#include "MLView.h"


class TestAppView final:
  public ml::AppView
{
public:
  TestAppView(TextFragment appName, size_t instanceNum);
  ~TestAppView() override;

  // Actor interface
  void onMessage(Message m) override;

  // from ml::AppView
  void initializeResources(NativeDrawContext* nvg) override;
  void makeWidgets() override;
  void renderView(NativeDrawContext* nvg, Layer* backingLayer) override;
  void pushEvent(GUIEvent g) override;
  void layoutView() override;
  
  void doAttached (void* pParent, int flags);
  void doResize(Vec2 newSize);

  
private:





  int getElapsedTime();
  
  
  
  std::unique_ptr< PlatformView > _platformView;
  
  int _animCounter{0};
  NVGcolor _popupIndicatorColor;

  
  NativeDrawContext* _drawContext{};


  void* _parent{nullptr};


};
