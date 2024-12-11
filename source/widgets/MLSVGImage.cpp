
// mlvg: GUI library for madronalib apps and plugins
// Copyright (C) 2019-2022 Madrona Labs LLC
// This software is provided 'as-is', without any express or implied warranty.
// See LICENSE.txt for details.


#include "MLSVGImage.h"
#include "MLDSPProjections.h"

using namespace ml;

void SVGImage::draw(ml::DrawContext dc)
{
  NativeDrawContext* nvg = getNativeContext(dc);
  Rect bounds = getLocalBounds(dc, *this);
  auto image = getVectorImage(dc, Path(getTextProperty("image_name")));
     
  if(image)
  {
    // get max rectangle for SVG image
    float imageAspect = image->width/image->height;
    float boundsAspect = bounds.width/bounds.height;
    float imgScale;
    Vec2 imageSize{image->width, image->height};

    if(imageAspect >= boundsAspect)
    {
      imgScale = bounds.width/imageSize.x;
    }
    else
    {
      imgScale = bounds.height/imageSize.y;
    }

    nvgSave(nvg);
    {
      nvgTranslate(nvg, getCenter(bounds) - imageSize*imgScale/2);
      nvgScale(nvg, imgScale, imgScale);
      nvgDrawSVG(nvg, image->_pImage);
    }
    nvgRestore(nvg);
  }
}
