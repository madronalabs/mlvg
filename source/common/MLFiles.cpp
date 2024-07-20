
// mlvg: GUI library for madronalib apps and plugins
// Copyright (C) 2019-2022 Madrona Labs LLC
// This software is provided 'as-is', without any express or implied warranty.
// See LICENSE.txt for details.


#include <filesystem>
#include <iostream>
#include <fstream>

#include "MLPlatform.h"
#include "MLFiles.h"
#include "external/miniz/miniz.h"

#include "external/osdialog/osdialog.h"
#include "external/filesystem/include/ghc/filesystem.hpp"

using namespace ml;
namespace fs = ghc::filesystem;

// NOTE: this is the beginning of a rewrite using ghc::filesystem, because we needed to ditch our previous
// library. Tests and error handling are needed!

const char kPlatformFileSeparator{
    #if ML_MAC
         '/'
#elif ML_WINDOWS
        '\\'
#endif
};


// TODO move to native code
ml::Path fsToMLPath(const fs::path& p)
{

#if ML_MAC
    char separator{ '/' };
#elif ML_WINDOWS
    char separator{ '\\' };
#endif
    std::string pathStr(p.string());
    TextFragment filePathAsText(pathStr.c_str());
    return textToPath(filePathAsText, separator);
}

// TODO move to native code, clean up
fs::path mlToFSPath(const ml::Path& p)
{
#if ML_MAC
  TextFragment pathTxt = rootPathToText(p);
#elif ML_WINDOWS
  TextFragment pathTxt = pathToText(p);
#endif
  
  return fs::path(pathTxt.getText());
}

TextFragment ml::filePathToText(const ml::Path& p)
{
#if ML_MAC
    return rootPathToText(p);
#elif ML_WINDOWS
    return pathToText(p, kPlatformFileSeparator);
#endif
}

// File


TextFragment File::getFullPathAsText() const
{
    return filePathToText(getFullPath());
}

TextFragment File::getShortName() const
{
  Symbol nameSym = last(fullPath_);
  return nameSym.getTextFragment();
}

bool File::exists() const
{
  return fs::exists(fs::status(mlToFSPath(fullPath_)));
}

bool File::create() const
{
  fs::path p(mlToFSPath(fullPath_));
  fs::create_directories(p.parent_path());
  std::ofstream ofs;
  ofs.open(p);
  ofs.close();
  return exists();
}

bool File::replaceWithData(const CharVector& dataVec) const
{
  fs::path p(mlToFSPath(fullPath_));
  std::ofstream ofs;
  ofs.open(p, std::ios::trunc | std::ios::binary);
  ofs.write(reinterpret_cast<const char*>(dataVec.data()), dataVec.size());
  ofs.close();
  
  // TODO catch error
  return true;
}

bool File::replaceWithDataCompressed(const CharVector& uncompressedData) const
{
    // compress
    size_t uncompressedSize = uncompressedData.size();
    size_t compressedSizeEstimate = compressBound(uncompressedSize);
    mz_ulong compressedSize = compressedSizeEstimate;
    CharVector compressedData;
    compressedData.resize(compressedSizeEstimate);

    int compressResult = compress(compressedData.data(), &compressedSize, uncompressedData.data(), uncompressedSize);

    // printf("Compressed from %u to %u bytes\n", (mz_uint32)uncompressedSize, (mz_uint32)compressedSize); // TEMP
    compressedData.resize(compressedSize);

    // replace
    return replaceWithData(compressedData);
}

bool File::replaceWithText(const TextFragment& text) const
{
  fs::path p(mlToFSPath(fullPath_));
  std::ofstream ofs;
  ofs.open(p, std::ios::trunc);
  ofs.write(text.getText(), text.lengthInBytes());
  ofs.close();
  
  // TODO catch error
  return true;
}

bool File::load(CharVector& dataVec) const
{
  if(!exists()) return false;
  
  fs::path p(mlToFSPath(fullPath_));
  size_t size = fs::file_size(p);
  dataVec.resize(size);
  
  std::ifstream ifs;
  ifs.open(p, std::ios::in | std::ios::binary);
  if(ifs.is_open())
  {
    ifs.read(reinterpret_cast<char*>(dataVec.data()), size);
  }
  // TODO catch error
  return true;
}

bool File::loadCompressed(CharVector& uncompressedData) const
{
    // arbitrary, TODO something real
    constexpr size_t kMaxUncompressedSize{ 20 * 1024 * 1024 };

    // load
    CharVector compressedInput;
    load(compressedInput);

    if (!compressedInput.size())
    {
        return false;
    }
 
    // uncompress
    const unsigned char* pData{ compressedInput.data() };
    mz_ulong uncompSize = compressedInput.size();

    // with our best guess of buffer size, try to uncompress. if the result doesn't
    // fit, double the buffer size and try again.
    int cmpResult{ MZ_BUF_ERROR };
    while ((MZ_BUF_ERROR == cmpResult) && (uncompSize < kMaxUncompressedSize))
    {
        uncompSize *= 2;
        uncompressedData.resize(uncompSize);
        cmpResult = uncompress(uncompressedData.data(), &uncompSize, compressedInput.data(), compressedInput.size());
    }

    if (MZ_BUF_ERROR == cmpResult)
    {
        // TODO something real
        std::cout << "loadCompressed: decompress failed!\n";
        return false;
    }

    uncompressedData.resize(uncompSize);
    return true;
}

bool File::loadAsText(TextFragment& fileAsText) const
{
  if(!exists()) return false;
  
  std::ifstream file(mlToFSPath(fullPath_));
  if(file.is_open())
  {
    std::string contents((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    fileAsText = TextFragment(contents.c_str(), contents.size());
  }
  
  // TODO catch error
  return true;
}

bool File::createDirectory()
{
  fs::path p(mlToFSPath(fullPath_));
  fs::create_directories(p);
  return exists();
}


// functions on Files

bool ml::isDirectory(const File& f)
{
  auto pathText = f.getFullPathAsText();
  return fs::is_directory(fs::status(pathText.getText()));
}


// functions on ml::Path and fs::path

Path ml::getRelativePath(const Path& root, const Path& child)
{
  Path relPath;
  
  if (root == child)
  {
    relPath = Path();
  }
  
  if(child.beginsWith(root))
  {
    relPath = lastN(child, child.getSize() - root.getSize());
  }
  else
  {
    // child is not in root
    relPath = child;
  }
  
  return relPath;
}

bool isHidden(const fs::path &p)
{
  fs::path::string_type name = p.filename();
  if(name[0] == '.')
  {
    return true;
  }
  
  return false;
}

// FileTree

// scan the files in our root directory and build a current leaf index.
void FileTree::scan()
{
  clear();
  _relativePathIndex.clear();
  auto rootPath(mlToFSPath(_rootPath));
  if (!fs::exists(fs::status(rootPath)))
  {
      return;
  }
  
  for(const fs::directory_entry& entry : fs::recursive_directory_iterator(rootPath))
  {
    // if file matches our search type, make a new file and add it to the tree
    if(!isHidden(entry.path()) && entry.is_regular_file())
    {
      auto filePath = fsToMLPath(entry.path());
      if(last(filePath).endsWith(_extension))
      {
          Path relPath = getRelativePath(_rootPath, filePath);
          add(relPath, std::make_unique< File >(filePath));
      }
    }
  }

  // add all leaves to the index in sorted order
  for (auto it = begin(); it != end(); ++it)
  {
    auto filePtr = (*it).get();
    auto relPath = it.getCurrentPath();
    _relativePathIndex.push_back(relPath);
  }

  return;
}

Path FileTree::getRelativePathByIndex(size_t i) const
{
  if( within(i, size_t(0), _relativePathIndex.size()) )
  {
    return _relativePathIndex[i];
  }
  else
  {
    return Path();
  }
}

File* FileTree::getLeafByIndex(size_t i) const
{
  File* filePtr{nullptr};
  if( within(i, size_t(0), _relativePathIndex.size()) )
  {
    auto leafNode = getNode(_relativePathIndex[i]);
    if(leafNode)
    {
      filePtr = leafNode->getValue().get();
    }
  }

  return filePtr;
}

int FileTree::findIndexOfLeaf(Path p) const
{
  size_t n = size();
  for(int i = 0; i < n; ++i)
  {
    if(_relativePathIndex[i] == p)
    {
      return i;
    }
  }
  return -1;
}

// helper functions

using namespace FileUtils;

File ml::FileUtils::getApplicationDataFile(TextFragment maker, TextFragment app, Symbol type, Path relativeName)
{
  File ret;
  Path rootPath = getApplicationDataPath(maker, app, type);
  Path typePath, fullPath;
  
  TextFragment extension;
  if(type == "patches")
  {
    extension = "mlpreset";
  }
  else if(type == "licenses")
  {
    extension = "txt";
  }
  else if(type == "scales")
  {
    extension = "scl";
  }
  else if(type == "partials")
  {
    extension = "sumu";
  }

  if(rootPath)
  {
    TextFragment nameWithExtension;
    if(extension)
    {
      nameWithExtension = TextFragment(pathToText(relativeName, kPlatformFileSeparator), ".", extension);
    }
    else
    {
      nameWithExtension = pathToText(relativeName, kPlatformFileSeparator);
    }
    
    fullPath = Path(rootPath, nameWithExtension);
    ret = File(fullPath);
  }
    
  return ret;
}


// file dialog utils

Path FileDialog::getFolderForLoad(Path startPath, TextFragment filters)
{
  Path r;
  auto pathText = filePathToText(startPath);
  
  if (char* filename = osdialog_file(OSDIALOG_OPEN_DIR, pathText.getText(), nullptr, nullptr))
  {
      r = textToPath(filename, kPlatformFileSeparator);
      free(filename);
  }

  return r;
}

Path FileDialog::getFilePathForLoad(Path startPath, TextFragment filtersFrag)
{
  Path r;
  osdialog_filters* filters = osdialog_filters_parse(filtersFrag.getText());
  auto pathText = filePathToText(startPath);
  
  if (char* filename = osdialog_file(OSDIALOG_OPEN, pathText.getText(), nullptr, filters))
  {
      r = textToPath(filename, kPlatformFileSeparator);
      free(filename);
      osdialog_filters_free(filters);
  }
  return r;
}

Path FileDialog::getFilePathForSave(Path startPath, TextFragment defaultName, TextFragment userFilters)
{
    using namespace textUtils;
    Path returnPath;
    osdialog_filters* filters{ nullptr };
    TextFragment pathText = filePathToText(startPath);
    TextFragment defaultExtension = textUtils::getExtension(defaultName);

    TextFragment filtersFrag;
    if (userFilters)
    {
        // use filters param
        filtersFrag = userFilters;
    }
    else
    {
        // determine filters from file extension
        std::vector< TextFragment > knownFilters
            { "WAV audio:wav", "Vutu partials:utu" };
        for (auto filter : knownFilters)
        {
            auto extIdx = findLast(filter, ':');
            auto ext = subText(filter, extIdx + 1, filter.lengthInCodePoints());

            if (defaultExtension == ext)
            {
                filtersFrag = filter;
                break;
            }
        }
    }

    if (filtersFrag)
    {
        filters = osdialog_filters_parse(filtersFrag.getText());
    }
 
    const char* dialogStartPath = pathText.getText();

    if (char* filename = osdialog_file(OSDIALOG_SAVE, dialogStartPath, defaultName.getText(), filters))
    {
        returnPath = textToPath(filename, kPlatformFileSeparator);
        free(filename);
    }
    
    if (returnPath)
    {
        // attach extension if there is none
        TextFragment shortName = (last(returnPath).getTextFragment());
        auto newExtension = (textUtils::getExtension(shortName));
        if (!newExtension)
        {
            shortName = TextFragment(shortName, ".", defaultExtension);
        }

        returnPath = Path(butLast(returnPath), shortName);
    }

    if(filters)
    {
        osdialog_filters_free(filters);
    }

    return returnPath;
}

using namespace FileUtils;

bool FileUtils::setCurrentPath(Path p)
{
  bool r{true};
  try
  {
    fs::current_path(mlToFSPath(p));
  }
  catch (fs::filesystem_error)
  {
    std::cout << "could not set current path " << p << "\n";
    r = false;
  }
  return r;
}
