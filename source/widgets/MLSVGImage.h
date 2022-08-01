
// mlvg: GUI library for madronalib apps and plugins
// Copyright (C) 2019-2022 Madrona Labs LLC
// This software is provided 'as-is', without any express or implied warranty.
// See LICENSE.txt for details.


#pragma once

#include "MLWidget.h"
#include "MLValue.h"
#include "MLTree.h"

#include "MLDrawContext.h"

using namespace ml;

class SVGImage : public Widget
{
  bool _initialized{false};
  //NSVGimage* _image{nullptr};

public:
  SVGImage(WithValues p) : Widget(p) {}

  // Widget implementation
  void draw(ml::DrawContext d) override;
  virtual MessageList processGUIEvent(const GUICoordinates& gc, GUIEvent e) override {return MessageList();}
};
