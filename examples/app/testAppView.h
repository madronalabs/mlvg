// mlvg test application
// Copyright (C) 2019-2022 Madrona Labs LLC
// This software is provided 'as-is', without any express or implied warranty.
// See LICENSE.txt for details.

#pragma once

#include "madronalib.h"
#include "mlvg.h"

class TestAppView final:
  public ml::AppView
{
public:
  TestAppView(TextFragment appName, size_t instanceNum);
  ~TestAppView() override;

  // AppView interface
  void initializeResources(NativeDrawContext* nvg) override;
  void layoutView() override;
  void onGUIEvent(const GUIEvent& event) override {};
  void onResize(Vec2 newSize) override {};

  // Actor interface
  void onMessage(Message m) override;
  
  void makeWidgets(const ParameterDescriptionList& pdl);
};
