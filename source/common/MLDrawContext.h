
// mlvg: GUI library for madronalib apps and plugins
// Copyright (C) 2019-2022 Madrona Labs LLC
// This software is provided 'as-is', without any express or implied warranty.
// See LICENSE.txt for details.


#pragma once

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "mldsp.h"
#include "madronalib.h"
#include "MLMath2D.h"
#include "MLGUICoordinates.h"

// TODO clean up cross-platform code

#ifdef __APPLE__

#include "nanovg_mtl.h"
// TODO runtime metal / GL switch? #include <OpenGL/gl.h> // OpenGL v.2
#define NANOVG_GL2 1
#define NANOVG_FBO_VALID 1
#include <OpenGL/gl.h> 
#include "nanovg.h"
#include "nanovg_gl_utils.h"
#include "nanosvg.h"
using NativeDrawBuffer = MNVGframebuffer;

#define nvgCreateContext(flags) nvgCreateGL2(flags)
#define nvgDeleteContext(context) nvgDeleteGL2(context)
#define nvgBindFramebuffer(fb) mnvgBindFramebuffer(fb)
#define nvgCreateFramebuffer(ctx, w, h, flags) mnvgCreateFramebuffer(ctx, w, h, flags)
#define nvgDeleteFramebuffer(fb) mnvgDeleteFramebuffer(fb)

#elif defined WIN32
  
#define NANOVG_GL3 1
#define NANOVG_FBO_VALID 1

#include "glad.h"
#include "nanovg.h"
#include "nanovg_gl_utils.h"
#include "nanosvg.h"
using NativeDrawBuffer = NVGLUframebuffer;

#define nvgCreateContext(flags) nvgCreateGL3(flags)
#define nvgDeleteContext(context) nvgDeleteGL3(context)
#define nvgBindFramebuffer(fb) nvgluBindFramebuffer(fb)
#define nvgCreateFramebuffer(ctx, w, h, flags) nvgluCreateFramebuffer(ctx, w, h, flags)
#define nvgDeleteFramebuffer(fb) nvgluDeleteFramebuffer(fb)

#elif defined LINUX // TODO

#endif

namespace ml {

class VectorImage
{
public:
  VectorImage(char* dataStart)
  {
    _pImage = nsvgParse(dataStart, "px", 96);
    if(_pImage)
    {
      width = _pImage->width;
      height = _pImage->height;
    }
  }
  
  ~VectorImage()
  {
    if(_pImage) { nsvgDelete(_pImage); }
  }
  
  float width{0};
  float height{0};
  NSVGimage* _pImage{nullptr};
};

using Resource = std::vector< uint8_t >;
using ResourceTree = Tree< std::unique_ptr< Resource > >;
using VectorImageTree = Tree< std::unique_ptr< VectorImage > >;
using NativeDrawContext = NVGcontext;

constexpr int kNullHandle{-1};
using NativeImageHandle = int;
using NativeFontHandle = NativeImageHandle;
inline bool isValid(NativeImageHandle h) { return h > kNullHandle; }

struct DrawContext
{
  void* pNativeContext;
  ResourceTree* pResources;
  PropertyTree* pProperties;
  VectorImageTree* pVectorImages;
  GUICoordinates coords;
};

inline DrawContext translate(const DrawContext& dc, Vec2 topLeft)
{
  auto dc2 = dc;
  dc2.coords.origin += topLeft;
  return dc2;
}

class Layer
{
public:
  void* _buf{nullptr};
  NativeDrawContext* _nvg{nullptr};
  size_t width{0};
  size_t height{0};
  
  Layer(NativeDrawContext* nvg, int width, int height);
  ~Layer();
};
  
// resource helpers

inline NativeDrawContext* getNativeContext(const DrawContext& dc) { return static_cast<NativeDrawContext*>(dc.pNativeContext); }

inline Resource* getResource(const DrawContext& dc, Path resourceName)
{
  const ResourceTree& rTree = *(dc.pResources);
  auto& pr (rTree[resourceName]);
  if(pr)
  {
    return pr.get();
  }
  else
  {
    return nullptr;
  }
}

inline NativeImageHandle getImageHandleResource(const DrawContext& dc, Path resourceName)
{
  ResourceTree& rTree = *(dc.pResources);
  auto& pr (rTree[resourceName]);
  if(pr)
  {
    if(pr->size() == sizeof(NativeImageHandle))
    {
      return *(reinterpret_cast<NativeImageHandle *>(pr->data()));
    }
  }
  return NativeImageHandle();
}

inline NativeFontHandle getFontHandleResource(const DrawContext& dc, Path resourceName)
{
  return getImageHandleResource(dc, resourceName);
}

NativeImageHandle getNativeImageHandle(const Layer& layer);



// vector images
// TODO make vector images a type of Resource

VectorImage* getVectorImage(const DrawContext& dc, Path imageName);

void drawVectorImage(const DrawContext& dc, const VectorImage& v);


// drawing helpers

// draw to the Layer's buffer. If pLayer is null, draw to the main buffer of the context.
void drawToLayer(const Layer* pLayer);


// nanovg + mlvg helpers

inline Vec2 nvgAngle2Vec(float a)
{
  return Vec2{ cosf(-a), -sinf(-a)};
}

inline void nvgLineTo(NativeDrawContext* nvg, Vec2 p)
{
  nvgLineTo(nvg, p.x(), p.y());
}

inline void nvgMoveTo(NativeDrawContext* nvg, Vec2 p)
{
  nvgMoveTo(nvg, p.x(), p.y());
}

inline void nvgRect(NativeDrawContext* nvg, ml::Rect r)
{
  nvgRect(nvg, r.left(), r.top(), r.width(), r.height());
}

inline void nvgX(NativeDrawContext* nvg, ml::Rect r)
{
  nvgMoveTo(nvg, r.left(), r.top());
  nvgLineTo(nvg, r.right(), r.bottom());
  
  nvgMoveTo(nvg, r.right(), r.top());
  nvgLineTo(nvg, r.left(), r.bottom());
}

inline void nvgRoundedRect(NativeDrawContext* nvg, ml::Rect r, float radius)
{
  nvgRoundedRect(nvg, r.left(), r.top(), r.width(), r.height(), radius);
}

inline void nvgScissor(NativeDrawContext* nvg, ml::Rect r)
{
  nvgScissor(nvg, r.left(), r.top(), r.width(), r.height());
}

inline void nvgIntersectScissor(NativeDrawContext* nvg, ml::Rect r)
{
  nvgIntersectScissor(nvg, r.left(), r.top(), r.width(), r.height());
}

inline void nvgTranslate(NativeDrawContext* nvg, Vec2 p)
{
  nvgTranslate(nvg, p.x(), p.y());
}

constexpr inline NVGcolor rgba(float pr, float pg, float pb, float pa)
{
  return NVGcolor{{{pr, pg, pb, pa}}};
}

inline void drawBrackets(NativeDrawContext* nvg, ml::Rect b, int width)
{
  float bl = b.left();
  float bt = b.top();
  float br = b.left() + b.width();
  float bb = b.top() + b.height();
  nvgBeginPath(nvg);
  
  nvgMoveTo(nvg, bl + width, bb);
  nvgLineTo(nvg, bl, bb);
  nvgLineTo(nvg, bl, bt);
  nvgLineTo(nvg, bl + width, bt);

  nvgMoveTo(nvg, br - width, bt);
  nvgLineTo(nvg, br, bt);
  nvgLineTo(nvg, br, bb);
  nvgLineTo(nvg, br - width, bb);

  nvgStroke(nvg);
 }

constexpr float kLabelScale { 0.4375f };

const Vec2 kDefaultLabelPos{0, 1.125f};

// draw label at bottom of bounds rect
inline void drawLabel(NativeDrawContext* nvg, TextFragment t, int gridSize, Rect nativeBounds, int fontID = 1, Vec2 offset = {0, 0})
{
  if(t)
  {
    float labelSize  = gridSize*kLabelScale;
    
    nvgFontFaceId(nvg, fontID);
    nvgFontSize(nvg, labelSize);
    
    nvgTextLetterSpacing(nvg, 0);// labelSize*0.05f);
    nvgTextAlign(nvg, NVG_ALIGN_CENTER | NVG_ALIGN_BOTTOM);
    
    Vec2 center = getCenter(nativeBounds);
    nvgText(nvg, center.x() + offset.x(), nativeBounds.bottom() + offset.y(), t.getText(), nullptr);
  }
}

// geometry transformations

using VGMatrix = std::array<float, 6>;

inline void nvgTransform(NativeDrawContext* nvg, VGMatrix t)
{
  nvgTransform(nvg, t[0], t[1], t[2], t[3], t[4], t[5]);
}


//ml::Projection

// typedef std::function<Vec2(Vec2)> Transform;

struct VGTransform
{
  VGMatrix transMatrix;
  VGTransform() { nvgTransformIdentity(transMatrix.data()); }
  VGTransform(VGMatrix m) : transMatrix(m){}
  Vec2 operator()(Vec2 srcPoint)
  {
    Vec2 destPoint;
    nvgTransformPoint(&destPoint[0], &destPoint[1], transMatrix.data(), srcPoint.x(), srcPoint.y());
    return destPoint;
  }
};

// TODO: compose(a, b, c...)

inline VGTransform compose(VGTransform a, VGTransform b)
{
  VGMatrix m = a.transMatrix;
  nvgTransformMultiply(m.data(), b.transMatrix.data());
  return VGTransform(m);
}

inline VGTransform translate(Vec2 p)
{
  VGTransform t;
  nvgTransformTranslate(t.transMatrix.data(), p.x(), p.y());
  return t;
}

inline VGTransform rotate(float theta)
{
  VGTransform t;
  nvgTransformRotate(t.transMatrix.data(), theta);
  return t;
}

inline VGTransform scale(Vec2 xy)
{
  VGTransform t;
  nvgTransformScale(t.transMatrix.data(), xy.x(), xy.y());
  return t;
}


// RAII-style objects for better drawing state management
// example:
// {
//    UsingTransform t(nvg, translate(newOrigin));
//    // ... draw some stuff ...
// }

struct UsingTransform
{
  NativeDrawContext* _nvg;
  UsingTransform(NativeDrawContext* nvg, VGTransform tx) : _nvg(nvg)
  {
    nvgSave(_nvg);
    float* pt = tx.transMatrix.data();
    nvgTransform(_nvg, pt[0], pt[1], pt[2], pt[3], pt[4], pt[5]);
  }
  ~UsingTransform()
  {
    nvgRestore(_nvg);
  }
};

struct UsingScissor
{
  NativeDrawContext* _nvg;
  UsingScissor(NativeDrawContext* nvg, Rect r) : _nvg(nvg)
  {
    nvgSave(_nvg);
    nvgIntersectScissor(nvg, r.left(), r.top(), r.width(), r.height());
  }
  ~UsingScissor()
  {
    nvgRestore(_nvg);
  }
};

void drawText(NativeDrawContext* nvg, Vec2 location, ml::Text t, int align = NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);

// draw one line of text, making the font size smaller if needed to fit into the specified width.
void drawTextToFit(NativeDrawContext* nvg, const ml::Text& t, Vec2 location, float desiredSize, float spacing, float width, int align);

// draw a multi-line text, using whatever algorithm nanovg uses for line breaking
void drawTextBox(NativeDrawContext* nvg, Vec2 location, float rowWidth, ml::Text t, int align = NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);

inline float getNvgLabelKerning(float textSize)
{
  static auto p(projections::linear({0, 128}, {0.05f, -0.1f}));
  //float kc = k / textSize;
  //std::cout << " text size: " << textSize << " -> " << kc << "\n";
  return p(textSize);
}

inline NVGcolor matrixToColor(const ml::Matrix& m)
{
  return nvgRGBAf(m[0], m[1], m[2], m[3]);
}

inline ml::Matrix colorToMatrix(const NVGcolor& c)
{
  return {c.r, c.g, c.b, c.a};
}

inline NVGcolor multiplyAlpha(const NVGcolor& c, float p)
{
  return nvgRGBAf(c.r, c.g, c.b, c.a*p);
}

using HSVAcolor = std::array<float, 4>;

// untested
inline HSVAcolor rgb2hsv(NVGcolor c)
{
  auto hi = std::max(c.r, std::max(c.g, c.b));
  auto lo = std::min(c.r, std::min(c.g, c.b));
  auto hue{0.f};
  auto sat{0.f};
  auto val{hi};
  const auto range = (hi - lo);
  
  if (range > 0.f)
  {
    if (c.r == hi)
    {
      hue = (c.g - c.b)/range;
    }
    else if (c.g == hi)
    {
      hue = 2.0f + (c.b - c.r)/range;
    }
    else
    {
      hue = 4.0f + (c.r - c.g)/range;
    }
    hue /= 6.0f;
    if (hue < 0.f) hue += 1.f;
    
    sat = range/hi;
  }

  return HSVAcolor{ hue, sat, val, c.a };
}

inline NVGcolor colorWithAlpha(NVGcolor c, float alpha)
{
  return rgba(c.r, c.g, c.b, alpha);
}



// shadow helpers

// no center?
inline void drawShadowArc(NativeDrawContext* nvg, float a0, float a1, float r0, float r1, NVGcolor shadowColor, float alpha)
{
  auto r2Alpha = projections::intervalMap( {r0, r1}, {alpha, 0.f}, projections::easeOutCubic );
  nvgStrokeWidth(nvg, 1.0f);
  if(r1 > r0)
  {
    for(float r = r0; r < r1; r += 1.0f)
    {
      auto c = shadowColor;
      c.a *= r2Alpha(r);
      nvgStrokeColor(nvg, c);
      nvgBeginPath(nvg);
      nvgArc(nvg, 0, 0, r, a0, a1, NVG_CW);
      nvgStroke(nvg);
    }
  }
  else
  {
    for(float r = r0; r > r1; r -= 1.0f)
    {
      auto c = shadowColor;
      c.a *= r2Alpha(r);
      nvgStrokeColor(nvg, c);
      nvgBeginPath(nvg);
      nvgArc(nvg, 0, 0, r, a0, a1, NVG_CW);
      nvgStroke(nvg);
    }
  }
}

inline void drawCircleShadow(NativeDrawContext* nvg, Vec2 center, float r0, float r1, NVGcolor shadowColor, float alpha)
{
  auto r2Alpha = projections::intervalMap( {r0, r1}, {alpha, 0.f}, projections::easeOutCubic );
  nvgStrokeWidth(nvg, 1.0f);
  for(float r = r0; r < r1; r += 1.0f)
  {
    nvgStrokeColor(nvg, multiplyAlpha(shadowColor, r2Alpha(r)));
    nvgBeginPath(nvg);
    nvgCircle(nvg, center.x(), center.y(), r);
    nvgStroke(nvg);
  }
}

inline void drawRoundRectShadow(NativeDrawContext* nvg, ml::Rect r, int width, int radius, NVGcolor shadowColor, float alpha)
{
  auto r2Alpha = projections::intervalMap( {0, width + 0.f}, {alpha, 0.f}, projections::easeOutQuartic );
  nvgStrokeWidth(nvg, 1);
  for(int i=0; i<width; ++i)
  {
    auto br = grow(r, i);
    nvgBeginPath(nvg);
    nvgRoundedRect(nvg, br, radius + i);
    nvgStrokeColor(nvg, multiplyAlpha(shadowColor, r2Alpha(i)));
    nvgStroke(nvg);
  }
}

// draw a centered grid over the given Rect, with the current stroke width and color.

inline void drawGrid(NativeDrawContext* nvg, float gridSize, Rect bounds)
{
  const int xIters = bounds.width() / gridSize;
  const int yIters = bounds.height() / gridSize;

  float xStart = -xIters/2*gridSize;
  float yStart = -yIters/2*gridSize;

  for(int i=0; i<xIters; ++i)
  {
    nvgBeginPath(nvg);
    float x = xStart + gridSize*i;
    nvgMoveTo(nvg, Vec2(x, bounds.top()));
    nvgLineTo(nvg, Vec2(x, bounds.bottom()));
    nvgStroke(nvg);
  }
  for(int i=0; i<yIters; ++i)
  {
    nvgBeginPath(nvg);
    float y = yStart + gridSize*i;
    nvgMoveTo(nvg, Vec2(bounds.left(), y));
    nvgLineTo(nvg, Vec2(bounds.right(), y));
    nvgStroke(nvg);
  }
}
/*
 
 // return brighter, less saturated vesion of a fill color-- for drawing lines etc.
 inline NVGcolor brightLineColor (const NVGcolor c)
 {
 NVGcolor hsv = rgb2hsv(c);
 float sat = std::min(hsv.g, 0.60f);
 float val = 1.f;
 
 return NVGcolor(c.getHue(), sat, val, 1.f);
 }
 
 */

/*
 // special things for zero
 if (fabs(number) < 0.00001f)
 {
 switch(mode)
 {
 case eMLNumFloat:
 if (doSign) numBuf[0] = ' ';
 break;
 case eMLNumZeroIsOff:
 snprintf(numBuf, bufLength, "off");
 break;
 break;
 default:
 break;
 }
 }
 */
/*
 if(mode == eMLNumDecibels)
 {
 if (number < -60.5f)
 {
 snprintf(numBuf, bufLength, "-inf.");
 }
 }
 */

// //debug() << "N" << numBuf << "\n";


void nvgDrawSVG(NVGcontext *vg, NSVGimage *svg);


Rect floatToSide(Rect fixedRect, Rect floatingRect, float margin, float windowWidth, float windowHeight, Symbol side);


// drawing property helpers

inline float getFloat(DrawContext t, Path p) { return t.pProperties->getFloatProperty(p); }

inline NVGcolor getColor(DrawContext t, Path p) { return matrixToColor(t.pProperties->getMatrixProperty(p)); }

//inline NVGcolor getColorWithDefault(DrawContext t, Path p, NVGcolor r) { return matrixToColor(t.pProperties->getMatrixPropertyWithDefault(p, colorToMatrix(r))); }

inline NVGcolor lerp(NVGcolor a, NVGcolor b, float mix) { return nvgLerpRGBA(a, b, mix); }


// void setColorProperty(Path p, NVGcolor r) { setProperty(p, colorToMatrix(r)); }
 




} // namespace ml



