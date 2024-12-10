
// mlvg: GUI library for madronalib apps and plugins
// Copyright (C) 2019-2022 Madrona Labs LLC
// This software is provided 'as-is', without any express or implied warranty.
// See LICENSE.txt for details.


#pragma once

#include <array>
#include <cmath>
#include <algorithm>

#include "MLDSPScalarMath.h"

namespace ml {

// internal V4 type with math that should be auto-vectorized.

struct alignas(16) V4
{
  union
  {
    std::array<float, 4> val;
    struct {
      float x;
      float y;
      float z;
      float w;
    };
  };
  
  V4() : x(0.f), y(0.f), z(0.f), w(0.f) {}
  V4(float a) : x(a), y(0.f), z(0.f), w(0.f) {}
  V4(float a, float b) : x(a), y(b), z(0.f), w(0.f) {}
  V4(float a, float b, float c) : x(a), y(b), z(c), w(0.f) {}
  V4(float a, float b, float c, float d) : x(a), y(b), z(c), w(d) {}

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
  
  inline const V4 operator-() const { return V4{-val[0], -val[1], -val[2], -val[3]}; }
  
  void quantize(int q);
  
  V4 getIntPart() const;
  V4 getFracPart() const;
  void getIntAndFracParts(V4& intPart, V4& fracPart) const;
};

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
  Vec2(float x, float y) : V4(x, y) {}
  Vec2(V4 v) : V4(v.x, v.y) {}
};

struct Vec3 : public V4
{
  Vec3(float x, float y, float z) : V4(x, y, z) {}
  Vec3(V4 v) : V4(v.x, v.y, v.z) {}
};

struct Vec4 : public V4
{
  Vec4(float x, float y, float z, float w) : V4(x, y, z, w) {}
  Vec4(V4 v) : V4(v.x, v.y, v.z, v.w) {}
};

struct Mat22 : public V4
{
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
  Mat22 m_{angleToMatrix(0.f)};

  Mat22 angleToMatrix(float t)
  {
    return Mat22{cosf(t), -sinf(t), sinf(t), cosf(t)};
  }

  void setAngle(float t)
  {
    if(theta_ != t)
    {
      m_ = angleToMatrix(t);
      theta_ = t;
    }
  }
  
  Vec2 operator()(Vec2 a)
  {
    return multiply(m_, a);
  }
};

Vec2 intersect(const LineSegment& a, const LineSegment& b);

// rectangle stored in left / top / width / height format.
class Rect : public V4
{
public:
  Rect() : V4() {}
  Rect(const V4& b) : V4(b) {}
  Rect(float w, float h) : V4(0, 0, w, h) {}
  Rect(float x, float y, float w, float h) : V4(x, y, w, h) {}
  
  Rect(const Vec2& corner1, const Vec2& corner2);
  float left() const { return val[0]; }
  float top() const { return val[1]; }
  float right() const { return val[0] + val[2]; }
  float bottom() const { return val[1] + val[3]; }
  
  float width() const { return val[2]; }
  float height() const { return val[3]; }
  
  float& left() { return val[0]; }
  float& top() { return val[1]; }
  float& width() { return val[2]; }
  float& height() { return val[3]; }

  inline float area() const { return width()*height(); }
  inline bool contains(const Vec2& p) const { return (ml::within(p.x, left(), right()) && ml::within(p.y, top(), bottom())); }
  Rect intersect(const Rect& b) const;
  Rect unionWith(const Rect& b) const;
  bool intersects(const Rect& p) const;
  
  // TODO turn into pure functions
  void setToIntersectionWith(const Rect& b);
  void setToUnionWith(const Rect& b);
  
  inline void setOrigin(Vec2 b){ val[0] = b.val[0]; val[1] = b.val[1]; }
  inline void setLeft(float px){ val[0] = px; }
  inline void setTop(float py){ val[1] = py; }
  inline void setWidth(float w){ val[2] = w; }
  inline void setHeight(float h){ val[3] = h; }
  
  inline void setRight(float px){ val[0] = px - val[2]; }
  inline void setBottom(float py){ val[1] = py - val[3]; }
  void setCenter(const Vec2& b);
  void centerInRect(const Rect& b);
   
  Rect withTopLeft(const Vec2& b) const;
  Rect withTopLeft(const float cx, const float cy);
  
  Vec2 topLeft() const;
  Vec2 topCenter() const;
  Vec2 topRight() const;
  Vec2 middleLeft() const;
  Vec2 center() const;
  Vec2 middleRight() const;
  Vec2 bottomLeft() const;
  Vec2 bottomCenter() const;
  Vec2 bottomRight() const;
  Vec2 dims() const;
  
  inline bool contains(int px, int py) const { return (ml::within(px, (int)left(), (int)right()) && ml::within(py, (int)top(), (int)bottom())); }
};

inline Vec2 getTopLeft(Rect r) { return Vec2(r.left(), r.top()); }
inline Vec2 getTopCenter(Rect r) { return Vec2(r.left() + r.width()*0.5f, r.top()); }
inline Vec2 getTopRight(Rect r) { return Vec2(r.right(), r.top()); }

inline Vec2 getMiddleLeft(Rect r) { return Vec2(r.left(), r.top() + r.height()*0.5f); }
inline Vec2 getCenter(Rect r) { return Vec2(r.left() + r.width()*0.5f, r.top() + r.height()*0.5f);}
inline Vec2 getMiddleRight(Rect r) { return Vec2(r.right(), r.top() + r.height()*0.5f); }

inline Vec2 getBottomLeft(Rect r) { return Vec2(r.left(), r.bottom()); }
inline Vec2 getBottomCenter(Rect r) { return Vec2(r.left() + r.width()*0.5f, r.bottom()); }
inline Vec2 getBottomRight(Rect r) { return Vec2(r.right(), r.bottom()); }

inline Vec2 getDims(Rect r) { return Vec2(r.width(), r.height()); }

inline Vec2 getCenterOffset(Rect r)
{
  return Vec2(r.width()*0.5f, r.height()*0.5f);
}

inline Vec2 getSize(Rect r)
{
  return Vec2(r.width(), r.height());
}

inline Rect translate(Rect r, Vec2 p)
{
  return r + p;
}

inline bool within(Vec2 p, Rect r)
{
  return (ml::within(p.x, r.left(), r.right()) && ml::within(p.y, r.top(), r.bottom()));
}

Rect intersectRects(const Rect& a, const Rect& b);
Rect unionRects(const Rect& a, const Rect& b);
Rect rectEnclosing(const Rect& a, const Rect& b);
Rect grow(const Rect& a, float amount);
Rect growWidth(const Rect& a, float amount);
Rect growHeight(const Rect& a, float amount);
Rect shrink(const Rect& a, float amount);
Rect shrinkWidth(const Rect& a, float amount);
Rect shrinkHeight(const Rect& a, float amount);

Rect constrainInside(Rect a, Rect b);

inline Rect centerOnOrigin(const Rect& a) { return Rect(-a.width()/2, -a.height()/2, a.width(), a.height()); }
inline Rect alignTopLeftToOrigin(const Rect& a) { return Rect(0, 0, a.width(), a.height()); }

#if(0)
inline Rect alignTopLeftToPoint(Rect a, Vec2 b) { return Rect(b.x(), b.y(), a.width(), a.height()); }
inline Rect alignTopRightToPoint(Rect a, Vec2 b) { return Rect(b.x() - a.width(), b.y(), a.width(), a.height()); }
inline Rect alignTopCenterToPoint(Rect a, Vec2 b) { return Rect(b.x() - a.width()/2, b.y(), a.width(), a.height()); }

inline Rect alignMiddleLeftToPoint(Rect a, Vec2 b) { return Rect(b.x(), b.y() - a.height()/2, a.width(), a.height()); }
inline Rect alignMiddleRightToPoint(Rect a, Vec2 b) { return Rect(b.x() - a.width(), b.y() - a.height()/2, a.width(), a.height()); }



inline Rect alignBottomLeftToPoint(Rect a, Vec2 b) { return Rect(b.x(), b.y() - a.height(), a.width(), a.height()); }
inline Rect alignBottomRightToPoint(Rect a, Vec2 b) { return Rect(b.x() - a.width(), b.y() - a.height(), a.width(), a.height()); }
inline Rect alignBottomCenterToPoint(Rect a, Vec2 b) { return Rect(b.x() - a.width()/2, b.y() - a.height(), a.width(), a.height()); }

//inline Rect centerRectOnPoint(Rect r, Vec2 p)
// changed to
inline Rect alignCenterToPoint(Rect r, Vec2 p)
{
  return Rect(p[0] - r[2]*0.5f, p[1] - r[3]*0.5f, r[2], r[3]);
}

inline Rect alignTopLeftToRect(const Rect& a, const Rect& b) { return Rect(b.left(), b.top(), a.width(), a.height()); }
inline Rect alignTopRightToRect(const Rect& a, const Rect& b) { return Rect(b.right() - a.width(), b.top(), a.width(), a.height()); }

inline Rect alignMiddleLeftToRect(const Rect& a, const Rect& b) { return alignMiddleLeftToPoint(a, getMiddleLeft(b)); }
inline Rect alignMiddleRightToRect(const Rect& a, const Rect& b) { return alignMiddleRightToPoint(a, getMiddleRight(b)); }

inline Rect alignBottomLeftToRect(const Rect& a, const Rect& b) { return Rect(b.left(), b.bottom() - a.height(), a.width(), a.height()); }
inline Rect alignBottomRightToRect(const Rect& a, const Rect& b) { return Rect(b.right() - a.width(), b.bottom() - a.height(), a.width(), a.height()); }

inline Rect alignCenterToRect(const Rect& a, const Rect& b) { return alignCenterToPoint(a, getCenter(b)); }


inline Rect portionOfRect(Rect ru, Rect bounds)
{
  Rect r;
  r.left() = bounds.left() + ru.left()*bounds.width();
  r.top() = bounds.top() + ru.top()*bounds.height();
  r.width() =  ru.width()*bounds.width();
  r.height() =  ru.height()*bounds.height();
  return r;
}

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

Rect alignRect(const Rect& rectToAlign, const Rect& fixedRect, alignFlags flags);
/*
 std::ostream& operator<< (std::ostream& out, const Vec2& r);
 std::ostream& operator<< (std::ostream& out, const Vec3& r);
 std::ostream& operator<< (std::ostream& out, const Vec4& r);
 */
std::ostream& operator<< (std::ostream& out, const ml::Rect& r);

inline Rect roundToInt(const Rect& r)
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


#endif

} // namespace ml




