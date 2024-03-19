
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

	namespace FileUtils
	{

		Path getUserDataPath()
		{
			Path resultPath;
			PWSTR ppszPath = nullptr;

			// everything is somewhere in (home)/AppData/Roaming on Windows
			HRESULT hr = SHGetKnownFolderPath(FOLDERID_LocalDocuments, 0, nullptr, &ppszPath);

			if (SUCCEEDED(hr)) {
				std::wstring wstr(ppszPath);

				// wide to UTF-8
				std::wstring_convert<std::codecvt_utf8<wchar_t>> conv1;
				std::string str = conv1.to_bytes(wstr);

				CoTaskMemFree(ppszPath);

				TextFragment frag(str.c_str());
				resultPath = textToPath(frag, '\\');
			}
			return resultPath;
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
				default:
				case(hash("root")): {
					resultPath = Path(appPath);
					break;
				}	      
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


	}
}
