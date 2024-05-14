
// mlvg: GUI library for madronalib apps and plugins
// Copyright (C) 2019-2022 Madrona Labs LLC
// This software is provided 'as-is', without any express or implied warranty.
// See LICENSE.txt for details.


#include "MLDrawableImageView.h"
#include "MLDSPProjections.h"

using namespace ml;

MessageList DrawableImageView::animate(int elapsedTimeInMs, ml::DrawContext dc)
{
    MessageList r;

    if (!_initialized)
    {
        NativeDrawContext* nvg = getNativeContext(dc);
        Rect bounds = getLocalBounds(dc, *this);
        auto pImage = getDrawableImage(dc, Path(getTextProperty("image_name")));


        if (pImage)
        {
            int fboWidth, fboHeight;
            int w, h;
            
            float pxRatio{ 1.f };


            // DRAW SOME JUNK FOR TESTING!

            nvgImageSize(nvg, pImage->_buf->image, &fboWidth, &fboHeight);
            w = (int)(fboWidth / pxRatio);
            h = (int)(fboHeight / pxRatio);

            // std::cout << "img" << w << " x " << h << ", ratio = " << pxRatio << "\n";

            drawToImage(pImage);

            nvgBeginFrame(nvg, w, h, pxRatio);

            int strokeWidth = w / 20;

            // draw background
            //nvgSave(nvg);
            // nvgGlobalCompositeBlendFunc(nvg, NVG_ONE, NVG_ZERO);
            /*
            nvgBeginPath(nvg);
            nvgRect(nvg, 0, 0, w, h);
            nvgFillColor(nvg, rgba(0, 0, 0, 1));
            nvgFill(nvg);
            nvgRestore(nvg);
            */


            // draw X
            nvgStrokeWidth(nvg, strokeWidth);
            nvgStrokeColor(nvg, colors::red);
            nvgBeginPath(nvg);
            nvgMoveTo(nvg, 0, 0);
            nvgLineTo(nvg, w, h);
            nvgMoveTo(nvg, w, 0);
            nvgLineTo(nvg, 0, h);
            nvgStroke(nvg);

            // draw dot
            nvgBeginPath(nvg);
            nvgFillColor(nvg, colors::green);
            nvgCircle(nvg, w / 2, h /2, strokeWidth);
            nvgFill(nvg);

            nvgEndFrame(nvg);
            drawToImage(nullptr);
        }


        _initialized = true;
    }
    return r;
}

void DrawableImageView::draw(ml::DrawContext dc)
{
    NativeDrawContext* nvg = getNativeContext(dc);
    Rect bounds = getLocalBounds(dc, *this);
    auto pImage = getDrawableImage(dc, Path(getTextProperty("image_name")));

    if (pImage)
    {
        float bw = bounds.width();
        float bh = bounds.height();
        float iw = pImage->width;
        float ih = pImage->height;

        NVGpaint img = nvgImagePattern(nvg, 0, 0, bw, bh, 0, pImage->_buf->image, 1.0f);
        nvgBeginPath(nvg);
  
        nvgRect(nvg, 0, 0, bw, bh);
        nvgFillPaint(nvg, img);
        nvgFill(nvg);

    }
}
