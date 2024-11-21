
// mlvg: GUI library for madronalib apps and plugins
// Copyright (C) 2019-2022 Madrona Labs LLC
// This software is provided 'as-is', without any express or implied warranty.
// See LICENSE.txt for details.

#ifdef __OBJC__

#import <Foundation/NSFileManager.h>

#include "MLFiles.h"

namespace ml
{
namespace FileUtils
{

Path getUserDataPath()
{
  NSString *filePath{nullptr};
  Path p;
  
  filePath = [NSSearchPathForDirectoriesInDomains(NSMusicDirectory, NSUserDomainMask, YES) objectAtIndex:0] ;

  if(filePath)
  {
    const char* pStr = [filePath UTF8String];
    p = textToPath(TextFragment(pStr));
  }
  
  return p;
}



Path getApplicationDataPath(TextFragment maker, TextFragment app, Symbol type)
{
  Path result;
  
  // everything is now in ~/Music/Madrona Labs on Mac. We can write and read here without entitlements, it seems?
  // so: used for testing until we look into that more. 
  Path musicPath = getUserDataPath();
  
  if(musicPath)
  {
    Path makerPath (musicPath, maker);
    Path appPath (musicPath, maker, app);
    switch(hash(type))
    {
      case(hash("patches")):
      {
        // patches are directly in for example .../Madrona Labs/Aalto
        result = Path(appPath);
        break;
      }
      case(hash("scales")):
      case(hash("mappings")):
      {
        result = Path(makerPath, "Scales");
        break;
      }
      case(hash("licenses")):
      {
        result = Path(makerPath, "Licenses");
        break;
      }
      case(hash("samples")):
      {
        result = Path(appPath, "Samples");
        break;
      }
      case(hash("partials")):
      {
        result = Path(appPath, "Partials");
        break;
      }
    }
  }
  return result;
}


}}

#endif // __OBJC__
