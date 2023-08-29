
// mlvg: GUI library for madronalib apps and plugins
// Copyright (C) 2019-2022 Madrona Labs LLC
// This software is provided 'as-is', without any express or implied warranty.
// See LICENSE.txt for details.


#include "MLFiles.h"

using namespace ml;

// use juce_core for implementation.
#define JUCE_APP_CONFIG_HEADER "external/juce_core/JuceCoreConfig.h"
#include "external/juce_core/juce_core.h"

TextFragment fullPathToText(Path p)
{
  char separator;

#if JUCE_MAC || JUCE_IOS
  separator = '/';
  TextFragment tf(separator, pathToText(p, separator));
#elif JUCE_LINUX || JUCE_ANDROID
  separator = '/';
  TextFragment tf(separator, pathToText(p, separator));
#elif JUCE_WINDOWS
  separator = '\\';
  TextFragment tf(pathToText(p, separator));
#endif
  return tf;
}

// File

File::File(Path p)
{
  char separator;
  TextFragment tf(fullPathToText(p));
  juce::File jf(juce::CharPointer_UTF8(tf.getText()));
  auto fullPathName = jf.getFullPathName();
  _fullPath = textToPath(TextFragment(fullPathName.toUTF8()));
  _isDirectory = jf.isDirectory();
}

File::~File(){}

Path File::getFullPath() const
{
  return _fullPath;
}

TextFragment File::getFullPathAsText() const
{

#if JUCE_MAC || JUCE_IOS
  char separator = '/';
  TextFragment tf(separator, pathToText(_fullPath));
#elif JUCE_LINUX || JUCE_ANDROID
  char separator = '/';
  TextFragment tf(separator, pathToText(_fullPath));
#elif JUCE_WINDOWS
  TextFragment tf(pathToText(_fullPath));
#endif

  return tf;
}

TextFragment File::getShortName() const
{
  Symbol nameSym = last(_fullPath);
  return nameSym.getTextFragment();
}


Path File::getRelativePathFrom(const File& other) const
{
  auto pathText = getFullPathAsText();
  juce::File thisFile(juce::CharPointer_UTF8(pathText.getText()));
  
  auto otherPathText = other.getFullPathAsText();
  juce::File otherFile(juce::CharPointer_UTF8(otherPathText.getText()));
  
  auto relativePathStr = thisFile.getRelativePathFrom(otherFile);
  return textToPath(TextFragment(relativePathStr.toUTF8()));
}

bool File::exists() const
{
  auto pathText = getFullPathAsText();
  juce::File jf(juce::CharPointer_UTF8(pathText.getText()));
  return jf.exists();
}

bool File::create() const
{
  auto pathText = getFullPathAsText();
  juce::File jf(juce::CharPointer_UTF8(pathText.getText()));
  return jf.create();
}

bool File::hasWriteAccess() const
{ 
  auto pathText = getFullPathAsText();
  juce::File jf(juce::CharPointer_UTF8(pathText.getText()));
  return jf.hasWriteAccess();
}

bool File::replaceWithData(const BinaryVector& dataVec) const
{
  auto pathText = getFullPathAsText();
  juce::File jf(juce::CharPointer_UTF8(pathText.getText()));
  const void* pData = static_cast< const void* > (dataVec.data());
  return jf.replaceWithData(pData, dataVec.size());
}

bool File::replaceWithText(const TextFragment& text) const
{
  auto pathText = getFullPathAsText();
  juce::File jf(juce::CharPointer_UTF8(pathText.getText()));
  juce::String juceStr(juce::CharPointer_UTF8(text.getText()));
  return jf.replaceWithText(juceStr, false, false, nullptr);
}

bool File::load(BinaryVector& dataVec) const
{
  bool r{false};
  constexpr size_t kMaxFileBlock{ 1000*1000*256 }; // TODO

  auto pathText = getFullPathAsText();
  juce::File jf(juce::CharPointer_UTF8(pathText.getText()));
  if(!jf.exists()) { return r; }

  juce::MemoryBlock jmb;
  if(jf.loadFileAsData(jmb))
  {
    auto newSize = jmb.getSize();
    if(within(newSize, size_t(0), kMaxFileBlock))
    {
      dataVec.resize(newSize);
      std::memcpy(dataVec.data(), jmb.getData(), newSize);
      r = true;
    }
  }
  return r;
}

bool File::loadAsText(TextFragment& fileAsText) const
{
  bool r{false};
  constexpr size_t kMaxFileBlock{ 1000*1000*256 }; // TODO

  auto pathText = getFullPathAsText();
  juce::File jf(juce::CharPointer_UTF8(pathText.getText()));
  if(!jf.exists()) { return r; }
  
  juce::String juceFileStr = jf.loadFileAsString();
  
  if(juceFileStr != juce::String())
  {
    juce::CharPointer_UTF8 utfStr = juceFileStr.toUTF8();
    fileAsText = TextFragment(utfStr);
    r = true;
  }
  return r;
}

Symbol File::createDirectory()
{
  auto pathText = getFullPathAsText();
  juce::File jf(juce::CharPointer_UTF8(pathText.getText()));
  auto r = jf.createDirectory();
  if(r == juce::Result::ok())
  {
    return "OK";
  }
  else
  {
    return Symbol(r.getErrorMessage().toUTF8());
  }
}

// FileTree

FileTree::FileTree(Path p, Symbol e) : _rootPath(p), _extension(e)
{
}

FileTree::~FileTree()
{
}

// scan the files in our root directory and build a current leaf index.
void FileTree::scan()
{
  clear();
  _relativePathIndex.clear();
  
  TextFragment t = fullPathToText(_rootPath);
  juce::File root (juce::CharPointer_UTF8(t.getText()));
  bool recurse = true;
  const juce::String& wildCard = "*";
  const int whatToLookFor = juce::File::findFilesAndDirectories | juce::File::ignoreHiddenFiles;  
  juce::RangedDirectoryIterator di (root, recurse, wildCard, whatToLookFor);
  
  for(auto dirEntry : di)
  {
    auto f = dirEntry.getFile();
    // TODO clean up
    juce::String relPath = f.getRelativePathFrom(root);
    juce::CharPointer_UTF8 relUTF8Path = relPath.toUTF8();
    ml::Path relativePath (relUTF8Path);
    
    juce::String fullPathStr = f.getFullPathName();
    juce::CharPointer_UTF8 UTF8Path = fullPathStr.toUTF8();
    ml::Path fullPath (UTF8Path);
    
    // if file matches our search type, make a new file and add it to the tree
    if(!f.isDirectory())
    {
      Symbol shortName = last(relativePath);
      if(shortName.endsWith(_extension))
      {
        add(relativePath, ml::make_unique< File >(fullPath));
      }
    }
  }

  // add all leaves to the index in sorted order
  for (auto it = begin(); it != end(); ++it)
  {
    auto filePtr = (*it).get();
    auto relPath = it.getCurrentNodePath();
    _relativePathIndex.push_back(removeExtensionFromPath(relPath));
  }
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
  int n = size();
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





// TODO generalize these for user types!


// BLEAH



Path ml::getApplicationDataRoot(TextFragment maker, TextFragment app, Symbol type)
{
  Path dataRoot, typeRoot;
  char separator;
  
#if JUCE_MAC || JUCE_IOS
  // everything is now in ~/Music/Madrona Labs on Mac
  auto pt = pathToText(Path("~/Music", maker, app));
  juce::File rootDir(juce::CharPointer_UTF8(pt.getText()));
  separator = '/';
#elif JUCE_LINUX || JUCE_ANDROID
  auto pt = pathToText(ath("~/", ".", maker, app));
  juce::File rootDir(juce::CharPointer_UTF8(pt.getText()));
  separator = '/';
#elif JUCE_WINDOWS
  juce::File appDataDir = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory);
  juce::File makerDir = appDataDir.getChildFile(juce::CharPointer_UTF8(maker.getText()));
  juce::File rootDir = makerDir.getChildFile(juce::CharPointer_UTF8(app.getText()));
  separator = '\\';
#endif
  
  const juce::String& fullName = rootDir.getFullPathName();
  dataRoot = textToPath(TextFragment(fullName.toUTF8()), separator);
  
  if(dataRoot)
  {
    if(type == "patches")
    {
      typeRoot = dataRoot;
    }
    else if(type == "scales")
    {
      typeRoot = Path(dataRoot, "..", "Scales");
    }
    else if(type == "samples")
    {
      typeRoot = Path(dataRoot, "Samples");
    }
    else if(type == "partials")
    {
      typeRoot = Path(dataRoot, "Partials");
    }
    else if(type == "licenses")
    {
      typeRoot = Path(dataRoot, "..", "Licenses");
    }
    else
    {
      typeRoot = Path(dataRoot); // default to app dir
    }
  }
    
  return typeRoot;
}

File ml::getApplicationDataFile(TextFragment maker, TextFragment app, Symbol type, Path relativeName)
{
  File ret;
  Path rootPath = getApplicationDataRoot(maker, app, type);
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
      nameWithExtension = TextFragment(pathToText(relativeName), ".", extension);
    }
    else
    {
      nameWithExtension = pathToText(relativeName);
    }
    
    fullPath = Path(rootPath, nameWithExtension);
    ret = File(fullPath);
  }
    
  return ret;
}
