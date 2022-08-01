// mlvg: GUI library for madronalib apps and plugins
// Copyright (C) 2019-2022 Madrona Labs LLC
// This software is provided 'as-is', without any express or implied warranty.
// See LICENSE.txt for details.

#pragma once

#include "MLWidget.h"
#include "MLValue.h"
#include "MLTree.h"

using namespace ml;

class Resizer : public Widget
{
  Vec2 _dragStart{};
  Vec2 _sizeStart{};
  Vec2 _dragDelta{};

public:
  Resizer(WithValues p) : Widget(p) {}

  // Widget interface
  MessageList processGUIEvent(const GUICoordinates& gc, GUIEvent e) override;
  void draw(ml::DrawContext d) override;

};
