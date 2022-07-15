// ml-gui: GUI library for madronalib apps
// Copyright (c) 2019 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

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
