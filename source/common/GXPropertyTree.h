
// mlvg: GUI library for madronalib apps and plugins
// Copyright (C) 2019-2022 Madrona Labs LLC
// This software is provided 'as-is', without any express or implied warranty.
// See LICENSE.txt for details.

#pragma once

#include "MLGUICoordinates.h"
#include "MLGUIEvent.h"
#include "MLValue.h"
#include "MLValueChange.h"
#include "MLTree.h"
#include "MLPropertyTree.h"
#include "MLMessage.h"
#include "MLParameters.h"
#include "GXTypes.h"

namespace gx {

// A GXPropertyTree is a PropertyTree with utility methods for converting
// drawing-related objects to and from Values.
//
class GXPropertyTree : public PropertyTree
{
public:
  
  GXPropertyTree(WithValues p) : PropertyTree(p) {}
  GXPropertyTree() : PropertyTree() {}

  ml::Rect getRectProperty(Path p) const { return valueToPODType<ml::Rect>(getProperty(p)); }
  ml::Rect getPointPropertyWithDefault(Path p, ml::Rect d) const { return hasProperty(p) ? getRectProperty(p) : d; }
  void setRectProperty(Path p, ml::Rect v) { setProperty(p, valueFromPODType<ml::Rect>(v)); }

  ml::Rect getBounds() const { return getRectProperty("bounds"); }
  void setBounds(ml::Rect r) { setRectProperty("bounds", r); }

  Point getPointProperty(Path p) const { return valueToPODType<Point>(getProperty(p)); }
  Point getPointPropertyWithDefault(Path p, Point d) const { return hasProperty(p) ? getPointProperty(p) : d; }
  void setPointProperty(Path p, Point v) { setProperty(p, valueFromPODType<Point>(v)); }
  
  Color getColorProperty(Path p) const { return valueToPODType<Color>(getProperty(p)); }
  Color getColorPropertyWithDefault(Path p, Color d) const { return hasProperty(p) ? getColorProperty(p) : d; }
  void setColorProperty(Path p, Color c) { setProperty(p, valueFromPODType<Color>(c)); }
};


} // namespace ml

