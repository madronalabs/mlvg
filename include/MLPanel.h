// mlvg: GUI library for madronalib apps and plugins
// Copyright (C) 2019-2022 Madrona Labs LLC
// This software is provided 'as-is', without any express or implied warranty.
// See LICENSE.txt for details.

#pragma once

#include "MLWidget.h"

using namespace ml;

class Panel : public Widget
{  
public:
  Panel(WithValues p) : Widget(p) {}

  // Widget implementation
  void draw(ml::DrawContext d) override;

};
