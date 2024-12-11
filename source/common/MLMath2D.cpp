
// mlvg: GUI library for madronalib apps and plugins
// Copyright (C) 2019-2022 Madrona Labs LLC
// This software is provided 'as-is', without any express or implied warranty.
// See LICENSE.txt for details.


#include <limits>
#include <iostream>

#include "MLMath2D.h"

namespace ml
{


#if 0



void Rect::setCenter(const Vec2& b)
{
	val[0] = b.val[0] - val[2]*0.5f;
	val[1] = b.val[1] - val[3]*0.5f;
}

void Rect::centerInRect(const Rect& b)
{
	setCenter(b.center());
}

/*
Rect Rect::translated(const Vec2& b) const
{
	return *this + b; 
}
*/

/*
Rect Rect::withCenter(const Vec2& b) const
{
	return (translated(b - Vec2(left() + width()*0.5f, top() + height()*0.5f))); 
}

Rect Rect::withCenter(const float cx, const float cy)
{
	return (translated(Vec2(cx - left() - width()*0.5f, cy - top() - height()*0.5f))); 
}
*/

Rect Rect::withTopLeft(const Vec2& b) const
{
	return (Rect(b.val[0], b.val[1], width(), height()));
}

Rect Rect::withTopLeft(const float cx, const float cy) 
{
	return (Rect(cx, cy, width(), height()));
}


Rect intersectRects(const Rect& a, const Rect& b)
{
  Rect ret{};
  float l, r, t, bot;
  l = ml::max(a.left, b.left);
  r = ml::min(a.right(), b.right());
  if (r > l)
  {
    t = ml::max(a.top, b.top);
    bot = ml::min(a.bottom(), b.bottom());
    if (bot > t)
    {
      ret = Rect(l, t, r - l, bot - t);
    }
  }
  return ret;
}


Rect unionRects(const Rect& a, const Rect& b)
{
  if (a.area() > 0.)
  {
    float l, r, t, bot;
    l = ml::min(a.left, b.left);
    r = ml::max(a.right(), b.right());
    t = ml::min(a.top, b.top);
    bot = ml::max(a.bottom(), b.bottom());
    return Rect(l, t, r - l, bot - t);
  }
  else
  {
    return b;
  }
}

Rect grow(const Rect& a, float m)
{
  return a + Rect(-m, -m, 2*m, 2*m);
}

Rect growWidth(const Rect& a, float m)
{
  return a + Rect(-m, 0, 2*m, 0);
}

Rect growHeight(const Rect& a, float m)
{
  return a + Rect(0, -m, 0, 2*m);
}

Rect shrink(const Rect& a, float m)
{
  return a + Rect(m, m, -2*m, -2*m);
}

Rect shrinkWidth(const Rect& a, float m)
{
  return a + Rect(m, 0, -2*m, 0);
}

Rect shrinkHeight(const Rect& a, float m)
{
  return a + Rect(0, m, 0, -2*m);
}


Rect constrainInside(Rect a, Rect b)
{
  Rect r{a};
  r.top -= max(r.bottom() - b.bottom(), 0.f);
  r.top = max(r.top, b.top);
  r.left -= max(r.right() - b.right(), 0.f);
  r.left = max(r.left, b.left);
  return r;
}

Rect rectEnclosing(const Rect& a, const Rect& b)
{
  Rect ret;
  if (a.area() > 0.)
  {
    float l, r, t, bot;
    l = ml::min(a.left, b.left);
    r = ml::max(a.right(), b.right());
    
    t = ml::min(a.top, b.top);
    bot = ml::max(a.bottom(), b.bottom());
    ret = Rect(l, t, r - l, bot - t);
  }
  else
  {
    ret = b;
  }
  return ret;
}

Rect alignRect(const Rect& a, const Rect& b, alignFlags flags)
{
  float x, y;
  switch(flags)
  {
    case kTopLeft:
      x = b.left;
      y = b.top;
      break;
    case kTopCenter:
      x = b.left + b.width/2 - a.width/2;
      y = b.top;
      break;
    case kTopRight:
      x = b.right() - a.width;
      y = b.top;
      break;
    case kLeft:
      x = b.left;
      y = b.top + b.height/2 - a.height/2;
      break;
    case kCenter:
      x = b.left + b.width/2 - a.width/2;
      y = b.top + b.height/2 - a.height/2;
      break;
    case kRight:
      x = b.right() - a.width;
      y = b.top + b.height/2 - a.height/2;
      break;
    case kBottomLeft:
      x = b.left;
      y = b.bottom() - a.height;
      break;
    case kBottomCenter:
      x = b.left + b.width/2 - a.width/2;
      y = b.bottom() - a.height;
      break;
    case kBottomRight:
      x = b.right() - a.width;
      y = b.bottom() - a.height;
      break;
  }
  return {x, y, a.width, a.height};
}


/*
std::ostream& operator<< (std::ostream& out, const Vec2& r)
{
	out << "[";
	out << r.x;
	out << ", ";
	out << r.y;
	out << "]";
	return out;
}

std::ostream& operator<< (std::ostream& out, const Vec3& r)
{
	out << "[";
	out << r.x;
	out << ", ";
	out << r.y;
	out << ", ";
	out << r.z();
	out << "]";
	return out;
}

std::ostream& operator<< (std::ostream& out, const Vec4& r)
{
	out << "[";
	out << r.x;
	out << ", ";
	out << r.y;
	out << ", ";
	out << r.z();
	out << ", ";
	out << r.w();
	out << "]";
	return out;
}

*/

std::ostream& operator<< (std::ostream& out, const Rect& r)
{
	out << "[";
	out << r.left;
	out << ", ";
	out << r.top;
	out << ", ";
	out << r.width;
	out << ", ";
	out << r.height;
	out << "]";
	return out;
}


#endif 


// namespace ml
}
