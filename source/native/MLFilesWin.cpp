
// mlvg: GUI library for madronalib apps and plugins
// Copyright (C) 2019-2022 Madrona Labs LLC
// This software is provided 'as-is', without any express or implied warranty.
// See LICENSE.txt for details.

#include "MLFiles.h"
#include <ShlObj.h>
#include <string>
#include <locale>
#include <codecvt>


namespace ml
{

    TextFragment File::getFullPathAsText() const
    {
        return "hello";
    }

    bool isWriteable(const File& f)
    {
        // TEMP
        return false;
    }

    bool isDirectory(const File& f)
    {
        // TEMP
        return false;
    }


namespace FileUtils
{

Path getUserPath(Symbol name)
{
    Path p;
    /*
  NSString *filePath{nullptr};

  
  switch(hash(name))
  {
    case(hash("music")):
    {
      filePath = [NSSearchPathForDirectoriesInDomains(NSMusicDirectory, NSUserDomainMask, YES) objectAtIndex:0] ;
      break;
    }
    default:
    {
      break;
    }
  }
  
  if(filePath)
  {
    const char* pStr = [filePath UTF8String];
    p = textToPath(TextFragment(pStr));
  }
  */

  return p;
}


Path getApplicationDataRoot(TextFragment maker, TextFragment app, Symbol type)
{
    return Path();
}

Path getApplicationDataPath(TextFragment maker, TextFragment app, Symbol type)
{
  Path appDataPath;
  PWSTR ppszPath = nullptr;
  HRESULT hr = SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &ppszPath);

  if (SUCCEEDED(hr)) {
      std::wstring wstr(ppszPath);

      // wide to UTF-8
      std::wstring_convert<std::codecvt_utf8<wchar_t>> conv1;
      std::string str = conv1.to_bytes(wstr);

      TextFragment frag(str.c_str());
      appDataPath = textToPath(frag);

      std::cout << "got path! " << appDataPath << "\n";
      CoTaskMemFree(ppszPath);
  }




  /*
  // everything is now in ~/Music/Madrona Labs on Mac

  Path musicPath = getUserPath("music");
  
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
  
  }*/
  return appDataPath;
}


}}
