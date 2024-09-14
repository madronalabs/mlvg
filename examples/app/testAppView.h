// mlvg test application
// Copyright (C) 2019-2022 Madrona Labs LLC
// This software is provided 'as-is', without any express or implied warranty.
// See LICENSE.txt for details.

#pragma once

#include "madronalib.h"
#include "mlvg.h"

#include "SDL.h"
#include "SDL_syswm.h"
#include "native/MLSDLUtils.h"

#define TEST_RESIZER 1
#define TEST_FIXED_RATIO 1

const ml::Vec2 kDefaultGridUnits{ 16, 9 };
const int kDefaultGridUnitSize{ 60 };

class TestAppView final:
  public ml::AppView
{
public:
  TestAppView(TextFragment appName, size_t instanceNum);
  ~TestAppView() override;

  // AppView interface
  void initializeResources(NativeDrawContext* nvg) override;
  void clearResources() override;
  void layoutView(DrawContext dc) override;
  void onGUIEvent(const GUIEvent& event) override {};
  void onResize(Vec2 newSize) override {};

  // Actor interface
  void onMessage(Message m) override;
  
  void makeWidgets(const ParameterDescriptionList& pdl);

  void attachToWindow(SDL_Window* window);
  void stop();

private:
	std::unique_ptr< PlatformView > _platformView;
	ResizingEventWatcherData watcherData_;
};
