
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
            int w = pImage->width;
            int h = pImage->height;

            int qw, qh;
            nvgImageSize(nvg, pImage->_buf->image, &qw, &qh);

            std::cout << "img" << qw << " " << qh << "\n";

            drawToImage(pImage);
            nvgBeginFrame(nvg, w, h, 1.0f);

            // draw background
            nvgSave(nvg);
            // nvgGlobalCompositeBlendFunc(nvg, NVG_ONE, NVG_ZERO);
            nvgBeginPath(nvg);
            nvgRect(nvg, 0, 0, w, h);
            nvgFillColor(nvg, rgba(0, 0, 0, 1));
            nvgFill(nvg);
            nvgRestore(nvg);

            // draw X
            nvgStrokeWidth(nvg, 8);
            nvgStrokeColor(nvg, colors::red);
            nvgBeginPath(nvg);
            nvgMoveTo(nvg, 0, 0);
            nvgLineTo(nvg, w, h);
            nvgMoveTo(nvg, w, 0);
            nvgLineTo(nvg, 0, h);
            nvgStroke(nvg);

            // draw dot
            nvgBeginPath(nvg);
            nvgFillColor(nvg, colors::blue);
            nvgCircle(nvg, w / 8, h - h / 4, 16);
            nvgFill(nvg);

            // draw dot
            nvgBeginPath(nvg);
            nvgFillColor(nvg, colors::green);
            nvgCircle(nvg, w / 2, h /2, 16);
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
        nvgScale(nvg, bw/iw, bh/ih);
        nvgRect(nvg, 0, 0, bw, bh);
        nvgFillPaint(nvg, img);
        nvgFill(nvg);

    }
}
