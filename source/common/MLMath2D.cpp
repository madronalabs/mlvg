
// mlvg: GUI library for madronalib apps and plugins
// Copyright (C) 2019-2022 Madrona Labs LLC
// This software is provided 'as-is', without any express or implied warranty.
// See LICENSE.txt for details.


#include <limits>
#include <iostream>

#include "MLMath2D.h"

namespace ml
{

// const float kMLMinSample = std::numeric_limits<float>::lowest();
// const V4 MLVec::kZeroValue = { {0, 0, 0, 0} };
// MLTESTconst V4 MLVec::kNullValue = { {kMLMinSample, kMLMinSample, kMLMinSample, kMLMinSample} };

/*

MLVec MLVec::getIntPart() const
{
	return MLVec((int)val[0],(int)val[1],(int)val[2],(int)val[3]);
}

MLVec MLVec::getFracPart() const
{
	return *this - getIntPart();
}

void MLVec::getIntAndFracParts(MLVec& intPart, MLVec& fracPart) const
{
	MLVec ip = getIntPart();
	intPart = ip;
	fracPart = *this - ip;
}

bool MLVec::operator==(const MLVec& b) const
{	
	return (val[0] == b.val[0]) && (val[1] == b.val[1]) && (val[2] == b.val[2]) && (val[3] == b.val[3]);
}

bool MLVec::operator!=(const MLVec& b) const
{	
	return !operator==(b);  
}
*/

#if 0


void MLVec::quantize(int q)
{
	int i0, i1, i2, i3;
	i0 = val[0];
	i1 = val[1];
	i2 = val[2];
	i3 = val[3];
	i0 = (i0/q)*q;
	i1 = (i1/q)*q;
	i2 = (i2/q)*q;
	i3 = (i3/q)*q;
	val[0] = i0;
	val[1] = i1;
	val[2] = i2;
	val[3] = i3;
}

// line segment intersection.
// returns Vec2::null() if no intersection exists.

Vec2 intersect(const LineSegment& a, const LineSegment& b)
{
	if(a.start == a.end) || (b.start == b.end))
	{
		return Vec2();
	}
	
	if( (a.start == b.start) || (a.start == b.end) || 
	   (a.end == b.start) || (a.end == b.end) )
	{
		return Vec2();
	}
	
	LineSegment a2 = translate(a, -a.start);
	LineSegment b2 = translate(b, -a.start);
	
	float aLen = length(a2);
	float cosTheta = a2.end.x()/aLen;
	float sinTheta = -(a2.end.y()/aLen);
	
	LineSegment b3 = multiply(Mat22(cosTheta, -sinTheta, sinTheta, cosTheta), b2);
	
	if(ml::sign(b3.start.y()) == ml::sign(b3.end.y())) return Vec2();
	
	float sectX = b3.end.x() + (b3.start.x() - b3.end.x())*b3.end.y()/(b3.end.y() - b3.start.y());
	
	if(!ml::within(sectX, 0.f, aLen)) return Vec2();
	
	return Vec2(a.start + Vec2(sectX*cosTheta, sectX*-sinTheta));
}

//
#pragma mark Rect



/*
Rect::Rect(int px, int py, int w, int h)
{
  val[0] = px;
  val[1] = py;
  val[2] = w;
  val[3] = h;
}
*/

Rect::Rect(const Vec2& corner1, const Vec2& corner2)
{ 
	float x1, x2, y1, y2;
	x1 = ml::min(corner1.x(), corner2.x());
	x2 = ml::max(corner1.x(), corner2.x());
	y1 = ml::min(corner1.y(), corner2.y());
	y2 = ml::max(corner1.y(), corner2.y());
	val[0] = x1;
	val[1] = y1;
	val[2] = x2 - x1;
	val[3] = y2 - y1;
}

Rect Rect::intersect(const Rect& b) const
{
	Rect ret;
	float l, r, t, bot;
	l = ml::max(left(), b.left());
	r = ml::min(right(), b.right());
	if (r > l)
	{
		t = ml::max(top(), b.top());
		bot = ml::min(bottom(), b.bottom());
		if (bot > t)
		{
			ret = Rect(l, t, r - l, bot - t);
		}
	}
	return ret;
}

bool Rect::intersects(const Rect& b) const
{ 
	Rect rx = intersect(b);
	return (rx.area() > 0);
}

Rect Rect::unionWith(const Rect& b) const
{
	Rect ret;
	if (area() > 0.)
	{
		float l, r, t, bot;
		l = ml::min(left(), b.left());
		r = ml::max(right(), b.right());
		
		t = ml::min(top(), b.top());
		bot = ml::max(bottom(), b.bottom());
		ret = Rect(l, t, r - l, bot - t);
	}
	else
	{
		ret = b;
	}
	return ret;
}

void Rect::setToIntersectionWith(const Rect& b)
{
	*this = intersect(b);
}

void Rect::setToUnionWith(const Rect& b)
{
	*this = unionWith(b);
}

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




Vec2 Rect::center() const
{
	return Vec2(left() + width()*0.5f, top() + height()*0.5f);
}

Vec2 Rect::topLeft() const
{
  return Vec2(left(), top());
}

Vec2 Rect::topRight() const
{
  return Vec2(right(), top());
}

Vec2 Rect::topCenter() const
{
  return Vec2(left() + width()*0.5f, top());
}

Vec2 Rect::middleLeft() const
{
  return Vec2(left(), top() + height()*0.5f);
}

Vec2 Rect::middleRight() const
{
  return Vec2(right(), top() + height()*0.5f);
}

Vec2 Rect::bottomLeft() const
{
  return Vec2(left(), bottom());
}

Vec2 Rect::bottomRight() const
{
  return Vec2(right(), bottom());
}

Vec2 Rect::bottomCenter() const
{
  return Vec2(left() + width()*0.5f, bottom());
}


Rect intersectRects(const Rect& a, const Rect& b)
{
  Rect ret{};
  float l, r, t, bot;
  l = ml::max(a.left(), b.left());
  r = ml::min(a.right(), b.right());
  if (r > l)
  {
    t = ml::max(a.top(), b.top());
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
    l = ml::min(a.left(), b.left());
    r = ml::max(a.right(), b.right());
    t = ml::min(a.top(), b.top());
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
  r.top() -= max(r.bottom() - b.bottom(), 0.f);
  r.top() = max(r.top(), b.top());
  r.left() -= max(r.right() - b.right(), 0.f);
  r.left() = max(r.left(), b.left());
  return r;
}

Rect rectEnclosing(const Rect& a, const Rect& b)
{
  Rect ret;
  if (a.area() > 0.)
  {
    float l, r, t, bot;
    l = ml::min(a.left(), b.left());
    r = ml::max(a.right(), b.right());
    
    t = ml::min(a.top(), b.top());
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
      x = b.left();
      y = b.top();
      break;
    case kTopCenter:
      x = b.left() + b.width()/2 - a.width()/2;
      y = b.top();
      break;
    case kTopRight:
      x = b.right() - a.width();
      y = b.top();
      break;
    case kLeft:
      x = b.left();
      y = b.top() + b.height()/2 - a.height()/2;
      break;
    case kCenter:
      x = b.left() + b.width()/2 - a.width()/2;
      y = b.top() + b.height()/2 - a.height()/2;
      break;
    case kRight:
      x = b.right() - a.width();
      y = b.top() + b.height()/2 - a.height()/2;
      break;
    case kBottomLeft:
      x = b.left();
      y = b.bottom() - a.height();
      break;
    case kBottomCenter:
      x = b.left() + b.width()/2 - a.width()/2;
      y = b.bottom() - a.height();
      break;
    case kBottomRight:
      x = b.right() - a.width();
      y = b.bottom() - a.height();
      break;
  }
  return {x, y, a.width(), a.height()};
}


/*
std::ostream& operator<< (std::ostream& out, const Vec2& r)
{
	out << "[";
	out << r.x();
	out << ", ";
	out << r.y();
	out << "]";
	return out;
}

std::ostream& operator<< (std::ostream& out, const Vec3& r)
{
	out << "[";
	out << r.x();
	out << ", ";
	out << r.y();
	out << ", ";
	out << r.z();
	out << "]";
	return out;
}

std::ostream& operator<< (std::ostream& out, const Vec4& r)
{
	out << "[";
	out << r.x();
	out << ", ";
	out << r.y();
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
	out << r.left();
	out << ", ";
	out << r.top();
	out << ", ";
	out << r.width();
	out << ", ";
	out << r.height();
	out << "]";
	return out;
}


#endif 


// namespace ml
}
