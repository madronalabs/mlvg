// mlvg: GUI library for madronalib apps and plugins
// Copyright (C) 2019-2022 Madrona Labs LLC
// This software is provided 'as-is', without any express or implied warranty.
// See LICENSE.txt for details.

#pragma once

#include "MLWidget.h"
#include "MLValue.h"
#include "MLTree.h"

using namespace ml;
constexpr size_t kWaveformPoints{64};

class DialBasic : public Widget
{
  float _dragY1{0.f};
  float _indicatorNormalizedValue{0.f};
  float _trackPositionToNormalValue(Vec2 p);
  float _quantizeNormalizedValue(float v);
  
  ml::Timer _scrollTimer;
  bool _doEndScroll{false};
  std::vector< float > _normDetents;
  Vec2 _clickAndHoldStartPosition;

public:
  DialBasic(WithValues p) : Widget(p) {}

  // Widget implementation
  void setupParams() override;
  MessageList processGUIEvent(const GUICoordinates& gc, GUIEvent e) override;
  MessageList animate(int elapsedTimeInMs, ml::DrawContext dc) override;
  void draw(ml::DrawContext d) override;
};
