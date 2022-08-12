
// mlvg: GUI library for madronalib apps and plugins
// Copyright (C) 2019-2022 Madrona Labs LLC
// This software is provided 'as-is', without any express or implied warranty.
// See LICENSE.txt for details.


#pragma once

#include <array>
#include <cmath>
#include <algorithm>

#include "mldsp.h"
#include "MLMatrix.h"

namespace ml {

using V4 = std::array<float, 4>;

//inline bool operator==(const V4& a, const V4& b) { return (a == b); }

class MLVec
{
public:
  V4 val;
  
  static const V4 kZeroValue;
  
  MLVec() : val(kZeroValue) {}
  MLVec(V4 v) : val(v) {}
  MLVec(const float f) { val = {f, f, f, f}; }
  MLVec(const MLVec& b) : val(b.val) {}
  MLVec(const float a, const float b, const float c, const float d) { val = {a, b, c, d}; }
  MLVec(const float* p) { val = {p[0], p[1], p[2], p[3]}; }
  
  virtual ~MLVec() = default;
  
  //static MLVec null() { return MLVec(kNullValue); }
  explicit operator bool() const { return !(val == kZeroValue); }
  
  inline void clear() { val = {0}; }
  inline void set(float f) { val = {f, f, f, f}; }
  
  inline MLVec & operator+=(const MLVec& b)
  { val[0]+=b.val[0]; val[1]+=b.val[1]; val[2]+=b.val[2]; val[3]+=b.val[3];
    return *this; }
  inline MLVec & operator-=(const MLVec& b)
  { val[0]-=b.val[0]; val[1]-=b.val[1]; val[2]-=b.val[2]; val[3]-=b.val[3];
    return *this; }
  inline MLVec & operator*=(const MLVec& b)
  { val[0]*=b.val[0]; val[1]*=b.val[1]; val[2]*=b.val[2]; val[3]*=b.val[3];
    return *this; }
  inline MLVec & operator/=(const MLVec& b)
  { val[0]/=b.val[0]; val[1]/=b.val[1]; val[2]/=b.val[2]; val[3]/=b.val[3];
    return *this; }
  inline const MLVec operator-() const { return MLVec{-val[0], -val[1], -val[2], -val[3]}; }
  
  // inspector, return by value
  inline float operator[] (int i) const { return val[i]; }
  // mutator, return by reference
  inline float& operator[] (int i) { return val[i]; }
  
  bool operator==(const MLVec& b) const;
  bool operator!=(const MLVec& b) const;
  
  inline const MLVec operator+ (const MLVec& b) const { return MLVec(*this) += b; }
  inline const MLVec operator- (const MLVec& b) const { return MLVec(*this) -= b; }
  inline const MLVec operator* (const MLVec& b) const { return MLVec(*this) *= b; }
  inline const MLVec operator/ (const MLVec& b) const { return MLVec(*this) /= b; }
  
  inline MLVec & operator*=(const float f) { (*this) *= MLVec(f); return *this; }
  inline const MLVec operator* (const float f) const { return MLVec(*this) *= MLVec(f, f, f, f); }
  inline const MLVec operator/ (const float f) const { return MLVec(*this) /= MLVec(f, f, f, f); }
  
  void normalize();
  void quantize(int q);
  
  MLVec getIntPart() const;
  MLVec getFracPart() const;
  void getIntAndFracParts(MLVec& intPart, MLVec& fracPart) const;
};


inline const MLVec vmin(const MLVec&a, const MLVec&b)
{ return MLVec(std::min(a.val[0],b.val[0]),std::min(a.val[1],b.val[1]),
               std::min(a.val[2],b.val[2]),std::min(a.val[3],b.val[3])); }
inline const MLVec vmax(const MLVec&a, const MLVec&b)
{ return MLVec(std::max(a.val[0],b.val[0]),std::max(a.val[1],b.val[1]),
               std::max(a.val[2],b.val[2]),std::max(a.val[3],b.val[3])); }
inline const MLVec vclamp(const MLVec&a, const MLVec&b, const MLVec&c) { return vmin(c, vmax(a, b)); }

inline const MLVec vsqrt(const MLVec& a)
{ return MLVec(sqrtf(a.val[0]), sqrtf(a.val[1]), sqrtf(a.val[2]), sqrtf(a.val[3])); }

inline const MLVec lerp(const MLVec& a, const MLVec&b, const float m)
{ return a + MLVec(m)*(b - a); }

class Vec2 : public MLVec
{
public:
  Vec2() : MLVec() {}
  Vec2(const MLVec& b) : MLVec(b) {};
  Vec2(float px, float py) { val[0] = px; val[1] = py; val[2] = 0.; val[3] = 0.; }
  float x() const { return val[0]; }
  float y() const { return val[1]; }
  void setX(float f) { val[0] = f; }
  void setY(float f) { val[1] = f; }
};

class Vec3 : public MLVec
{
public:
  Vec3() : MLVec() {}
  Vec3(const MLVec& b) : MLVec(b) {};
  Vec3(float px, float py, float pz) { val[0] = px; val[1] = py; val[2] = pz; val[3] = 0.; }
  float x() const { return val[0]; }
  float y() const { return val[1]; }
  Vec2 xy() const { return Vec2(val[0], val[1]); }
  float z() const { return val[2]; }
  void setX(float f) { val[0] = f; }
  void setY(float f) { val[1] = f; }
  void setZ(float f) { val[2] = f; }
};

class Vec4 : public MLVec
{
public:
  Vec4() : MLVec() {}
  Vec4(const MLVec& b) : MLVec(b) {};
  Vec4(float px, float py, float pz, float pw) { val[0] = px; val[1] = py; val[2] = pz; val[3] = pw; }
  float x() const { return val[0]; }
  float y() const { return val[1]; }
  Vec2 xy() const { return Vec2(val[0], val[1]); }
  float z() const { return val[2]; }
  float w() const { return val[3]; }
  void setX(float f) { val[0] = f; }
  void setY(float f) { val[1] = f; }
  void setZ(float f) { val[2] = f; }
  void setW(float f) { val[3] = f; }
};

const Vec2 kHalfPixel{0.5f, 0.5f};

inline float magnitude(Vec2 v)
{
  float a = v.val[0];
  float b = v.val[1];
  return sqrtf(a*a + b*b);
}

inline float magnitude(Vec3 v)
{
  float a = v.val[0];
  float b = v.val[1];
  float c = v.val[2];
  return sqrtf(a*a + b*b + c*c);
}

inline float magnitude(Vec4 v)
{
  float a = v.val[0];
  float b = v.val[1];
  float c = v.val[2];
  float d = v.val[3];
  return sqrtf(a*a + b*b + c*c + d*d);
}


struct LineSegment
{
  LineSegment(Vec2 a, Vec2 b) : start(a), end(b) {}
  Vec2 start;
  Vec2 end;
};

struct Mat22
{
  Mat22(float a, float b, float c, float d) : a00(a), a10(b), a01(c), a11(d) {}
  float a00, a10, a01, a11;
};

inline Vec2 multiply(Mat22 m, Vec2 a)
{
  return Vec2(m.a00*a.x() + m.a10*a.y(), m.a01*a.x() + m.a11*a.y());
}

inline LineSegment multiply(Mat22 m, LineSegment a)
{
  return LineSegment(multiply(m, a.start), multiply(m, a.end));
}

inline LineSegment translate(LineSegment a, Vec2 p)
{
  return LineSegment(a.start + p, a.end + p);
}

inline bool lengthIsZero(LineSegment a)
{
  return((a.start.x() == a.end.x())&&(a.start.y() == a.end.y()));
}

inline float length(LineSegment a)
{
  return magnitude(Vec2(a.end - a.start)); // TODO fix type for operator- and other operations
}

Vec2 intersect(const LineSegment& a, const LineSegment& b);

// rectangle stored in left / top / width / height format.
class Rect : public MLVec
{
public:
  Rect() : MLVec() {}
  Rect(const MLVec& b) : MLVec(b) {};
  Rect(float w, float h) : MLVec(0, 0, w, h) {}
  Rect(float x, float y, float w, float h) : MLVec(x, y, w, h) {}
  
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
  inline bool contains(const Vec2& p) const { return (ml::within(p.x(), left(), right()) && ml::within(p.y(), top(), bottom())); }
  Rect intersect(const Rect& b) const;
  Rect unionWith(const Rect& b) const;
  bool intersects(const Rect& p) const;
  
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
  
  Vec2 center() const;
  Vec2 topLeft() const;
  Vec2 topRight() const;
  Vec2 topCenter() const;
  Vec2 bottomLeft() const;
  Vec2 bottomRight() const;
  Vec2 bottomCenter() const;
  Vec2 dims() const;
  
  inline bool contains(int px, int py) const { return (ml::within(px, (int)left(), (int)right()) && ml::within(py, (int)top(), (int)bottom())); }
};

inline Rect matrixToRect(Matrix m) { return Rect(m[0], m[1], m[2], m[3]); }
inline Matrix rectToMatrix(Rect r) { return Matrix{r.left(), r.top(), r.width(), r.height()}; }

inline Vec2 matrixToVec2(Matrix m) { return Vec2(m[0], m[1]); }
inline Matrix vec2ToMatrix(Vec2 v) { return Matrix{ v[0], v[1] }; }

inline Vec2 getTopLeft(Rect r)
{
  return Vec2(r.left(), r.top());
}

inline Vec2 getCenter(Rect r)
{
  return Vec2(r.left() + r.width()*0.5f, r.top() + r.height()*0.5f);
}

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
  return Rect(p[0] + r[0], p[1] + r[1], r[2], r[3]);
}

inline bool within(const Vec2& p, const Rect& r)
{
  return (ml::within(p.x(), r.left(), r.right()) && ml::within(p.y(), r.top(), r.bottom()));
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

inline Rect centerOnOrigin(const Rect& a) { return Rect(-a.width()/2, -a.height()/2, a.width(), a.height()); }
inline Rect alignTopLeftToOrigin(const Rect& a) { return Rect(0, 0, a.width(), a.height()); }

inline Rect alignTopLeftToPoint(Rect a, Vec2 b) { return Rect(b.x(), b.y(), a.width(), a.height()); }
inline Rect alignTopRightToPoint(Rect a, Vec2 b) { return Rect(b.x() - a.width(), b.y(), a.width(), a.height()); }
inline Rect alignTopCenterToPoint(Rect a, Vec2 b) { return Rect(b.x() - a.width()/2, b.y(), a.width(), a.height()); }
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

} // namespace ml




