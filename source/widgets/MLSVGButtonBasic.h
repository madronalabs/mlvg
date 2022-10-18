// mlvg: GUI library for madronalib apps and plugins
// Copyright (C) 2019-2022 Madrona Labs LLC
// This software is provided 'as-is', without any express or implied warranty.
// See LICENSE.txt for details.

#pragma once

#include "MLWidget.h"

using namespace ml;

class SVGButtonBasic : public Widget
{
  bool _down{false};
  bool _initialized{false};
  NSVGimage* _image{nullptr};
  
public:
  SVGButtonBasic(WithValues p) : Widget(p) {}

  // Widget implementation
  MessageList processGUIEvent(const GUICoordinates& gc, GUIEvent e) override;
  void draw(ml::DrawContext d) override;

};
