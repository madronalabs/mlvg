
// mlvg: GUI library for madronalib apps and plugins
// Copyright (C) 2019-2022 Madrona Labs LLC
// This software is provided 'as-is', without any express or implied warranty.
// See LICENSE.txt for details.


#include "MLAppView.h"

namespace ml {


void AppView::createVectorImage(Path newImageName, const unsigned char* dataStart, size_t dataSize)
{
  // horribly, nsvgParse will clobber the data it reads! So let's copy that data before parsing.
  auto tempData = malloc(dataSize);
  
  if(tempData)
  {
    memcpy(tempData, dataStart, dataSize);
    _vectorImages[newImageName] = ml::make_unique< VectorImage >(static_cast< char* >(tempData));
  }
  else
  {
    std::cout << "createVectorImage: alloc failed! " << newImageName << ", " << dataSize << " bytes \n";
    return;
  }

  free(tempData);
}


void AppView::deleteWidgets()
{
  _rootWidgets.clear();
}

}
