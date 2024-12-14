
// mlvg: GUI library for madronalib apps and plugins
// Copyright (C) 2019-2022 Madrona Labs LLC
// This software is provided 'as-is', without any express or implied warranty.
// See LICENSE.txt for details.


#pragma once

#include <array>
#include <cmath>
#include <algorithm>
#include <iostream>

#include "MLDSPScalarMath.h"

namespace ml {

// internal V4 type with math that should be auto-vectorized.

class alignas(16) V4
{
public:
  union
  {
    std::array<float, 4> val;
    struct {
      float x;
      float y;
      float z;
      float w;
    };
    struct {
      float left;
      float top;
      float width;
      float height;
    };
  };
  
  
  V4() : x(0.f), y(0.f), z(0.f), w(0.f) {}
  
  // the float constructor fills all elements, which allows V4 * float -> V4.
  V4(float a) : x(a), y(a), z(a), w(a) {}
  
  V4(float a, float b) : x(a), y(b), z(0.f), w(0.f) {}
  V4(float a, float b, float c) : x(a), y(b), z(c), w(0.f) {}
  constexpr V4(float a, float b, float c, float d) : x(a), y(b), z(c), w(d) {}

  static constexpr float kMinFloat = std::numeric_limits<float>::lowest();
  
  static const V4 kNullObject;

  explicit operator bool() const { return !(*this == kNullObject); }

  bool operator==(const V4& b) const
  {
    return (val[0] == b.val[0]) && (val[1] == b.val[1]) && (val[2] == b.val[2]) && (val[3] == b.val[3]);
  }
   
  bool operator!=(const V4& b) const { return !operator==(b); }
   
  inline V4 & operator+=(const V4& b)
  {
    for(unsigned int i=0; i<4; ++i) { val[i] += b.val[i]; }
    return *this;
  }
  inline V4 & operator-=(const V4& b)
  {
    for(unsigned int i=0; i<4; ++i) { val[i] -= b.val[i]; }
    return *this;
  }
  inline V4 & operator*=(const V4& b)
  {
    for(unsigned int i=0; i<4; ++i) { val[i] *= b.val[i]; }
    return *this;
  }
  inline V4 & operator/=(const V4& b)
  {
    for(unsigned int i=0; i<4; ++i) { val[i] /= b.val[i]; }
    return *this;
  }

  inline const V4 operator+ (const V4& b) const { return V4(*this) += b; }
  inline const V4 operator- (const V4& b) const { return V4(*this) -= b; }
  inline const V4 operator* (const V4& b) const { return V4(*this) *= b; }
  inline const V4 operator/ (const V4& b) const { return V4(*this) /= b; }
  
  inline V4 operator-() const
  {
    V4 r;
    for(unsigned int i=0; i<4; ++i) { r.val[i] = -val[i]; }
    return r;
  }
  
  inline V4 getIntPart() const
  {
    V4 r;
    for(unsigned int i=0; i<4; ++i) { r.val[i] = (int)val[i]; }
    return r;
  }
  
  inline V4 getFracPart() const
  {
    return *this - getIntPart();
  }
};

inline constexpr V4 V4::kNullObject( kMinFloat, kMinFloat, kMinFloat, kMinFloat );

inline V4 roundToInt(V4 a)
{
  V4 r;
  for(unsigned int i=0; i<4; ++i) { r.val[i] = (int)a.val[i]; }
  return r;
}

inline V4 vmin(V4 a, V4 b)
{
  V4 r;
  for(unsigned int i=0; i<4; ++i) { r.val[i] = std::min(a.val[i], b.val[i]); }
  return r;
}

inline V4 vmax(V4 a, V4 b)
{
  V4 r;
  for(unsigned int i=0; i<4; ++i) { r.val[i] = std::max(a.val[i], b.val[i]); }
  return r;
}

inline const V4 vclamp(const V4&a, const V4&b, const V4&c) { return vmin(c, vmax(a, b)); }

inline V4 vsqrt(V4 a)
{
  V4 r;
  for(unsigned int i=0; i<4; ++i) { r.val[i] = sqrtf(a.val[i]); }
  return r;
}

inline const V4 lerp(const V4& a, const V4&b, const float m) { return a + V4{m, m, m, m}*(b - a); }

inline float magnitudeSquared(V4 v)
{
  float a = v.val[0];
  float b = v.val[1];
  float c = v.val[2];
  float d = v.val[3];
  return (a*a + b*b + c*c + d*d);
}

inline float sum(V4 v)
{
  float a = v.val[0];
  float b = v.val[1];
  float c = v.val[2];
  float d = v.val[3];
  return (a + b + c + d);
}

inline float magnitude(V4 v) { return sqrtf(magnitudeSquared(v)); }
inline V4 normalize(V4 v) { return (v/magnitude(v)); }
inline float dotProduct(V4 v, V4 w) { return sum(v*w); }

// derived types

struct Vec2 : public V4
{
  Vec2() : V4() {}
  Vec2(float x, float y) : V4(x, y) {}
  Vec2(V4 v) : V4(v.x, v.y) {}
};

struct Vec3 : public V4
{
  Vec3() : V4() {}
  Vec3(float x, float y, float z) : V4(x, y, z) {}
  Vec3(V4 v) : V4(v.x, v.y, v.z) {}
};

struct Vec4 : public V4
{
  Vec4() : V4() {}
  Vec4(float x, float y, float z, float w) : V4(x, y, z, w) {}
  Vec4(V4 v) : V4(v.x, v.y, v.z, v.w) {}
};

struct Mat22 : public V4
{
  Mat22() : V4() {}
  Mat22(float x, float y, float z, float w) : V4(x, y, z, w) {}
  Mat22(V4 v) : V4(v.x, v.y, v.z, v.w) {}
};

struct LineSegment
{
  LineSegment(Vec2 a, Vec2 b) : start(a), end(b) {}
  Vec2 start;
  Vec2 end;
};

inline Vec2 multiply(Mat22 m, Vec2 a)
{
  return Vec2(m.x*a.x + m.y*a.y, m.z*a.x + m.w*a.y);
}

inline LineSegment multiply(Mat22 m, LineSegment a)
{
  return LineSegment(multiply(m, a.start), multiply(m, a.end));
}

inline LineSegment translate(LineSegment a, Vec2 p)
{
  return LineSegment(a.start + p, a.end + p);
}

inline float length(LineSegment a)
{
  return magnitude(Vec2(a.end - a.start));
}

struct Rotation
{
  float theta_{0.f};
  Mat22 m{angleToMatrix(0.f)};

  Mat22 angleToMatrix(float t)
  {
    return Mat22{cosf(t), -sinf(t), sinf(t), cosf(t)};
  }

  void setAngle(float t)
  {
    if(theta_ != t)
    {
      m = angleToMatrix(t);
      theta_ = t;
    }
  }
  
  Vec2 operator()(Vec2 a)
  {
    return multiply(m, a);
  }
};

// returns the point where a intersects b, or a null object if no intersection exists.

inline Vec2 intersect(LineSegment a, LineSegment b)
{
  if((a.start == a.end) || (b.start == b.end))
  {
    return V4::kNullObject;
  }
  
  if( (a.start == b.start) || (a.start == b.end) ||
     (a.end == b.start) || (a.end == b.end) )
  {
    return V4::kNullObject;
  }
  
  LineSegment a2 = translate(a, -a.start);
  LineSegment b2 = translate(b, -a.start);
  float aLen = length(a2);
  float cosTheta = a2.end.x/aLen;
  float sinTheta = -(a2.end.y/aLen);
  LineSegment b3 = multiply(Mat22(cosTheta, -sinTheta, sinTheta, cosTheta), b2);
  if(ml::sign(b3.start.y) == ml::sign(b3.end.y)) return V4::kNullObject;
  float sectX = b3.end.x + (b3.start.x - b3.end.x)*b3.end.y/(b3.end.y - b3.start.y);
  if(!ml::within(sectX, 0.f, aLen)) return V4::kNullObject;
  return Vec2(a.start + Vec2(sectX*cosTheta, sectX*-sinTheta));
}

// rectangle stored in left / top / width / height format.

struct Rect : public V4
{
public:
  Rect() : V4() {}
  Rect(const V4& b) : V4(b) {}
  Rect(float w, float h) : V4(0, 0, w, h) {}
  Rect(float a, float b, float c, float d) : V4(a, b, c, d) {}
  
  Rect(Vec2 corner1, Vec2 corner2)
  {
    float x1, x2, y1, y2;
    x1 = ml::min(corner1.x, corner2.x);
    x2 = ml::max(corner1.x, corner2.x);
    y1 = ml::min(corner1.y, corner2.y);
    y2 = ml::max(corner1.y, corner2.y);
    val[0] = x1;
    val[1] = y1;
    val[2] = x2 - x1;
    val[3] = y2 - y1;
  }
  
  float right() { return left + width; }
  float bottom() { return top + height; }
};


inline float area(Rect r) { return r.width*r.height; }
inline bool contains(Rect r, Vec2 p) { return (ml::within(p.x, r.left, r.right()) && ml::within(p.y, r.top, r.bottom())); }

inline Rect getIntersection(Rect a, Rect b)
{
  Rect ret;
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

inline bool intersects(Rect a, Rect b)
{
  return (area(getIntersection(a, b)) > 0);
}

inline Rect getUnion(Rect a, Rect b)
{
  Rect ret;
  if (area(a) > 0.)
  {
    float l, r, top, btm;
    l = ml::min(a.left, b.left);
    r = ml::max(a.right(), b.right());
    top = ml::min(a.top, b.top);
    btm = ml::max(a.bottom(), b.bottom());
    ret = Rect(l, top, r - l, btm - top);
  }
  else
  {
    ret = b;
  }
  return ret;
}

inline Vec2 getTopLeft(Rect r) { return Vec2(r.left, r.top); }
inline Vec2 getTopCenter(Rect r) { return Vec2(r.left + r.width*0.5f, r.top); }
inline Vec2 getTopRight(Rect r) { return Vec2(r.right(), r.top); }

inline Vec2 getMiddleLeft(Rect r) { return Vec2(r.left, r.top + r.height*0.5f); }
inline Vec2 getCenter(Rect r) { return Vec2(r.left + r.width*0.5f, r.top + r.height*0.5f);}
inline Vec2 getMiddleRight(Rect r) { return Vec2(r.right(), r.top + r.height*0.5f); }

inline Vec2 getBottomLeft(Rect r) { return Vec2(r.left, r.bottom()); }
inline Vec2 getBottomCenter(Rect r) { return Vec2(r.left + r.width*0.5f, r.bottom()); }
inline Vec2 getBottomRight(Rect r) { return Vec2(r.right(), r.bottom()); }

inline Vec2 getDims(Rect r) { return Vec2(r.width, r.height); }

inline Vec2 getCenterOffset(Rect r)
{
  return Vec2(r.width*0.5f, r.height*0.5f);
}

inline Vec2 getSize(Rect r)
{
  return Vec2(r.width, r.height);
}

inline Rect translate(Rect r, Vec2 p)
{
  return r + p;
}

inline bool within(Vec2 p, Rect r)
{
  return (ml::within(p.x, r.left, r.right()) && ml::within(p.y, r.top, r.bottom()));
}

inline Rect rectEnclosing(Rect a, Rect b)
{
  Rect ret;
  if (area(a) > 0.)
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

inline Rect grow(Rect a, float m)
{
  return a + Rect(-m, -m, 2*m, 2*m);
}

inline Rect growWidth(Rect a, float m)
{
  return a + Rect(-m, 0, 2*m, 0);
}

inline Rect growHeight(Rect a, float m)
{
  return a + Rect(0, -m, 0, 2*m);
}

inline Rect shrink(Rect a, float m)
{
  return a + Rect(m, m, -2*m, -2*m);
}

inline Rect shrinkWidth(Rect a, float m)
{
  return a + Rect(m, 0, -2*m, 0);
}

inline Rect shrinkHeight(Rect a, float m)
{
  return a + Rect(0, m, 0, -2*m);
}

inline Rect constrainInside(Rect a, Rect b)
{
  Rect r{a};
  r.top -= max(r.bottom() - b.bottom(), 0.f);
  r.top = max(r.top, b.top);
  r.left -= max(r.right() - b.right(), 0.f);
  r.left = max(r.left, b.left);
  return r;
}

inline Rect centerOnOrigin(Rect a) { return Rect(-a.width/2, -a.height/2, a.width, a.height); }
inline Rect alignTopLeftToOrigin(Rect a) { return Rect(0, 0, a.width, a.height); }

inline Rect alignTopLeftToPoint(Rect a, Vec2 b) { return Rect(b.x, b.y, a.width, a.height); }
inline Rect alignTopRightToPoint(Rect a, Vec2 b) { return Rect(b.x - a.width, b.y, a.width, a.height); }
inline Rect alignTopCenterToPoint(Rect a, Vec2 b) { return Rect(b.x - a.width/2, b.y, a.width, a.height); }

inline Rect alignMiddleLeftToPoint(Rect a, Vec2 b) { return Rect(b.x, b.y - a.height/2, a.width, a.height); }
inline Rect alignMiddleRightToPoint(Rect a, Vec2 b) { return Rect(b.x - a.width, b.y - a.height/2, a.width, a.height); }

inline Rect alignBottomLeftToPoint(Rect a, Vec2 b) { return Rect(b.x, b.y - a.height, a.width, a.height); }
inline Rect alignBottomRightToPoint(Rect a, Vec2 b) { return Rect(b.x - a.width, b.y - a.height, a.width, a.height); }
inline Rect alignBottomCenterToPoint(Rect a, Vec2 b) { return Rect(b.x - a.width/2, b.y - a.height, a.width, a.height); }

inline Rect alignCenterToPoint(Rect r, Vec2 p)
{
  return Rect(p.x - r.width*0.5f, p.y - r.height*0.5f, r.width, r.height);
}

inline Rect alignTopLeftToRect(Rect a, Rect b) { return Rect(b.left, b.top, a.width, a.height); }
inline Rect alignTopRightToRect(Rect a, Rect b) { return Rect(b.right() - a.width, b.top, a.width, a.height); }

inline Rect alignMiddleLeftToRect(Rect a, Rect b) { return alignMiddleLeftToPoint(a, getMiddleLeft(b)); }
inline Rect alignCenterToRect(Rect a, Rect b) { return alignCenterToPoint(a, getCenter(b)); }
inline Rect alignMiddleRightToRect(Rect a, Rect b) { return alignMiddleRightToPoint(a, getMiddleRight(b)); }

inline Rect alignBottomLeftToRect(Rect a, Rect b) { return Rect(b.left, b.bottom() - a.height, a.width, a.height); }
inline Rect alignBottomRightToRect(Rect a, Rect b) { return Rect(b.right() - a.width, b.bottom() - a.height, a.width, a.height); }

enum alignFlags
{
  kTopLeft = 0,
  kTopCenter,
  kTopRight,
  kLeft,
  kCenter,
  kRight,
  kBottomLeft,
  kBottomCenter,
  kBottomRight
};

inline Rect alignRect(Rect a, Rect b, alignFlags flags)
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


// return the portion ru of the whole rect bounds. each coordinate in ru is relative to the width or height of the whole.
// so portionOfRect({0, 0, 1, 1}, whole) is the whole rect. portionOfRect({0.5, 0, 0.5, 1}, whole) is the right half, and so on.

inline Rect portionOfRect(Rect ru, Rect whole)
{
  Rect r;
  r.left = whole.left + ru.left*whole.width;
  r.top = whole.top + ru.top*whole.height;
  r.width = ru.width*whole.width;
  r.height = ru.height*whole.height;
  return r;
}

inline std::ostream& operator<< (std::ostream& out, const Vec2& r)
{
  out << "[";
  out << r.x;
  out << ", ";
  out << r.y;
  out << "]";
  return out;
}

inline std::ostream& operator<< (std::ostream& out, const Vec3& r)
{
  out << "[";
  out << r.x;
  out << ", ";
  out << r.y;
  out << ", ";
  out << r.z;
  out << "]";
  return out;
}

inline std::ostream& operator<< (std::ostream& out, const Vec4& r)
{
  out << "[";
  out << r.x;
  out << ", ";
  out << r.y;
  out << ", ";
  out << r.z;
  out << ", ";
  out << r.w;
  out << "]";
  return out;
}

inline std::ostream& operator<< (std::ostream& out, const Rect& r)
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

inline Rect roundToInt(Rect r)
{
  return Rect(r.getIntPart());
}

// very fast sin approx for graphics, not DSP!
// its period is 1.0 and its amplitude is 0.25.
inline float sinApprox1(float x)
{
  float m = x - floorf(x) - 0.5f;
  return 2*m*(1.f - fabs(2*m));
}



} // namespace ml




