
// mlvg: GUI library for madronalib apps and plugins
// Copyright (C) 2019-2022 Madrona Labs LLC
// This software is provided 'as-is', without any express or implied warranty.
// See LICENSE.txt for details.


#pragma once

#include "madronalib.h"

#include "external/osdialog/osdialog.h"

namespace ml
{

using BinaryVector = std::vector< uint8_t >;

class File final
{
  bool _isDirectory;
  Path _fullPath;
   
public:
  File() = default;
  File(Path p);
  ~File();
  
  explicit operator bool() const { return !(_fullPath == Path()); }
  inline bool isDirectory() const { return _isDirectory; }
  
  Path getFullPath() const;
  TextFragment getFullPathAsText() const; 
  TextFragment getShortName() const;
  Path getRelativePathFrom(const File& other) const;

  bool exists() const;
  bool create() const;
  bool hasWriteAccess() const;
  
  bool replaceWithData(const BinaryVector&) const;
  bool replaceWithText(const TextFragment&) const;

  bool load(BinaryVector&) const;
  bool loadAsText(TextFragment&) const;

  Symbol createDirectory();
};

class FileTree : public Tree< std::unique_ptr< File >, textUtils::SymbolCollator >
{
  Path _rootPath;
  Symbol _extension;
  std::vector< Path > _relativePathIndex;

public:
  FileTree(Path p, Symbol e);
  ~FileTree();
  
  void scan();
  
  // return the number of leaves.
  inline size_t size() const { return _relativePathIndex.size(); }
  
  Path getRelativePathByIndex(size_t i) const;
  File* getLeafByIndex(size_t i) const;

  int findIndexOfLeaf(Path p) const;
};

Path getApplicationDataRoot(TextFragment maker, TextFragment app, Symbol type);
File getApplicationDataFile(TextFragment maker, TextFragment app, Symbol type, Path relativeName);


// file dialog utils

struct FileDialog
{
  static Path getFolderForLoad(Path startPath, TextFragment filters)
  {
    Path r;
    auto pathText = pathToText(startPath);
    pathText = TextFragment("/", pathText);
    
    char* filename = osdialog_file(OSDIALOG_OPEN_DIR, pathText.getText(), nullptr, nullptr);

    r = textToPath(filename);
    free(filename);
    return r;
  }
  
  static Path getFilePathForLoad(Path startPath, TextFragment filtersFrag)
  {
    Path r;
    osdialog_filters* filters = osdialog_filters_parse(filtersFrag.getText());
    auto pathText = pathToText(startPath);
    pathText = TextFragment("/", pathText);

    char* filename = osdialog_file(OSDIALOG_OPEN, pathText.getText(), nullptr, filters);
    
    r = textToPath(filename);
    free(filename);
    osdialog_filters_free(filters);
    return r;
  }
  
  static Path getFilePathForSave(Path startPath, TextFragment defaultName)
  {
    Path r;
    auto pathText = pathToText(startPath);
    pathText = TextFragment("/", pathText);
    
    char* filename = osdialog_file(OSDIALOG_SAVE, pathText.getText(), defaultName.getText(), nullptr);

    r = textToPath(filename);
    free(filename);
    return r;
  }
  
};

} // namespaces
