
// mlvg: GUI library for madronalib apps and plugins
// Copyright (C) 2019-2022 Madrona Labs LLC
// This software is provided 'as-is', without any express or implied warranty.
// See LICENSE.txt for details.

#pragma once

namespace ml {


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
using NativeDrawContext = NVGcontext;

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
using NativeDrawContext = NVGcontext;

#define nvgCreateContext(flags) nvgCreateGL3(flags)
#define nvgDeleteContext(context) nvgDeleteGL3(context)
#define nvgBindFramebuffer(fb) nvgluBindFramebuffer(fb)
#define nvgCreateFramebuffer(ctx, w, h, flags) nvgluCreateFramebuffer(ctx, w, h, flags)
#define nvgDeleteFramebuffer(fb) nvgluDeleteFramebuffer(fb)

#elif defined LINUX // TODO

#endif

} // namespace ml

