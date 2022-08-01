
// mlvg: GUI library for madronalib apps and plugins
// Copyright (C) 2019-2022 Madrona Labs LLC
// This software is provided 'as-is', without any express or implied warranty.
// See LICENSE.txt for details.


#pragma once

#include "MLSymbol.h"
#include "MLMath2D.h"
using namespace ml;

#include <chrono>
using namespace std::chrono;

struct GUIEvent
{
  Symbol type{};
  Vec2 position{}; // position in grid coordinates
  Vec2 delta{};
  uint32_t keyFlags{};
  int sourceIndex{}; // for multiple touches etc.

//  time_point<system_clock> time;
  
  GUIEvent() = default;
  GUIEvent(Symbol t, Vec2 p=Vec2(), Vec2 d=Vec2(), int k=0, int s=0) : type(t), position(p), delta(d), keyFlags(k), sourceIndex(s) {}
};


inline GUIEvent translate(GUIEvent e, Vec2 p)
{
  GUIEvent t{e};
  t.position += p;
  return t;
}

enum KeyFlags
{
  /** Indicates no modifier keys. */
  noModifiers                             = 0,

  /** Shift key flag. */
  shiftModifier                           = 1,

  /** CTRL key flag. */
  controlModifier                         = 2,

  /** ALT key flag. */
  altModifier                             = 4,

  /** Command key flag - on windows this is the same as the CTRL key flag. */
  commandModifier                         = 8,

  /** Left mouse button flag. */
  leftButtonModifier                      = 16,

  /** Right mouse button flag. */
  rightButtonModifier                     = 32,

  /** Middle mouse button flag. */
  middleButtonModifier                    = 64

};



// TEMP for navigation
#if 0
MouseInputSource kSource;
MouseEvent m;
#endif

