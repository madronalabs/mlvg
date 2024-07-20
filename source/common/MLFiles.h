
// mlvg: GUI library for madronalib apps and plugins
// Copyright (C) 2019-2022 Madrona Labs LLC
// This software is provided 'as-is', without any express or implied warranty.
// See LICENSE.txt for details.


#pragma once

#include "madronalib.h"

#include <vector>
#include "MLPath.h"
#include "MLTree.h"

namespace ml
{

using CharVector = std::vector< unsigned char >;

class File final
{
  Path fullPath_;
   
public:
  File() = default;
  File(Path p) : fullPath_(p) {}
  ~File() = default;
  
  explicit operator bool() const { return !(fullPath_ == Path()); }
  
  Path getFullPath() const { return fullPath_; }
  TextFragment getFullPathAsText() const; 
  TextFragment getShortName() const;

  bool exists() const;
  bool create() const;
  
  bool replaceWithData(const CharVector&) const;
  bool replaceWithDataCompressed(const CharVector&) const;
  bool replaceWithText(const TextFragment&) const;

  bool load(CharVector&) const;
  bool loadCompressed(CharVector&) const;
  bool loadAsText(TextFragment&) const;

  bool createDirectory();
};

Path getRelativePath(const Path& root, const Path& child);
bool exists(const File& f);
bool isDirectory(const File& f);

TextFragment filePathToText(const ml::Path& p);


class FileTree : public Tree< std::unique_ptr< File >, textUtils::SymbolCollator >
{
  Path _rootPath;
  Symbol _extension;
  std::vector< Path > _relativePathIndex;

public:
  
  FileTree(Path p, Symbol e) : _rootPath(p), _extension(e) {}
  ~FileTree() = default;
  
  void scan();
  
  // return the number of leaves.
  inline size_t size() const { return _relativePathIndex.size(); }
  
  Path getRelativePathByIndex(size_t i) const;
  File* getLeafByIndex(size_t i) const;

  int findIndexOfLeaf(Path p) const;
};


// file dialog utils

namespace FileDialog
{
  Path getFolderForLoad(Path startPath, TextFragment filters);
  Path getFilePathForLoad(Path startPath, TextFragment filtersFrag);
  Path getFilePathForSave(Path startPath, TextFragment defaultName, TextFragment filtersFrag = TextFragment());
};

namespace FileUtils
{

void test();

Path getUserDataPath();
Path getApplicationDataPath(TextFragment maker, TextFragment app, Symbol type);
File getApplicationDataFile(TextFragment maker, TextFragment app, Symbol type, Path relativeName);
bool setCurrentPath(Path p);

};


} // namespaces
