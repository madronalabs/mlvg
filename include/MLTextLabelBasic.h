// mlvg: GUI library for madronalib apps and plugins
// Copyright (C) 2019-2022 Madrona Labs LLC
// This software is provided 'as-is', without any express or implied warranty.
// See LICENSE.txt for details.

#pragma once

#include "MLWidget.h"

using namespace ml;

class TextLabelBasic : public Widget
{
public:
  TextLabelBasic(WithValues p) : Widget(p) {}

  // Widget implementation
  MessageList processGUIEvent(const GUICoordinates& gc, GUIEvent e) override { return MessageList(); }
  void draw(ml::DrawContext d) override;
};
