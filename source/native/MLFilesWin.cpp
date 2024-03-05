
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

  // everything is somewhere in (home)/AppData/Roaming on Windows
  HRESULT hr = SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &ppszPath);

  if (SUCCEEDED(hr)) {
      std::wstring wstr(ppszPath);

      // wide to UTF-8
      std::wstring_convert<std::codecvt_utf8<wchar_t>> conv1;
      std::string str = conv1.to_bytes(wstr);

      CoTaskMemFree(ppszPath);

      TextFragment frag(str.c_str());
      appDataPath = textToPath(frag, '\\');
  }

  Path resultPath;
  if (appDataPath)
  {
	  Path makerPath(appDataPath, maker);
	  Path appPath(appDataPath, maker, app);

	  switch (hash(type)) {
	      case(hash("patches")): {
              resultPath = Path(appPath);
		      break;
	      }
	      case(hash("scales")): {
              resultPath = Path(makerPath, "Scales");
		      break;
	      }
	      case(hash("licenses")): {
              resultPath = Path(makerPath, "Licenses");
		      break;
	      }
	      case(hash("samples")): {
              resultPath = Path(appPath, "Samples");
		      break;
	      }
	      case(hash("partials")): {
              resultPath = Path(appPath, "Partials");
		      break;
	      }
	  }
  }


  return resultPath;
}


}}
