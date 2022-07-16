// VST3 example code for madronalib
// (c) 2020, Madrona Labs LLC, all rights reserved
// see LICENSE.txt for details


// use clipboard code from VST3 SDK
#include "public.sdk/source/common/systemclipboard.h"

#include "testAppController.h"
#include "testAppView.h"

// param definitions for this plugin
#include "testAppParameters.h"

#include <cmath>
#include <iostream>
#include <chrono>

#include "MLSerialization.h"

// include miniz - in mlvg/external directory
#include "external/miniz/miniz.h"
#include "external/miniz/miniz.c"

// include nfd
#include "external/nativefiledialog-extended/src/include/nfd.h"

#define JUCE_APP_CONFIG_HEADER "external/juce_core/JuceCoreConfig.h"
#include "external/juce_core/juce_core.h"

using namespace ml;

Path _incrementVersion(Path currentPath)
{
  using namespace ml::textUtils;
  
  Symbol finalSym = last(currentPath);
  int currVersion = getFinalNumber(finalSym);
  
  // if there is no current version, currVersion = 0 and we start at 1
  int nextVersion = currVersion + 1;
  Symbol nextFinalSym = addFinalNumber(stripFinalNumber(finalSym), nextVersion);
  Path nextPath(butLast(currentPath), nextFinalSym);
  return nextPath;
}

//-----------------------------------------------------------------------------
// testAppController implementation

testAppController::testAppController()
{
#ifdef DEBUG
#ifdef ML_WINDOWS
  AllocConsole();
  freopen("CONOUT$", "w", stdout);
  freopen("CONOUT$", "w", stderr);
#endif
#endif

  // make parameters and projections
  ParameterDescriptionList pdl;
  readParameterDescriptions(pdl);
  buildParameterTree(pdl, _params);
  
  // store IDs by name and param names by ID
  for(size_t i=0; i < pdl.size(); ++i)
  {
    ParameterDescription& pd = *pdl[i];
    Path paramName = pd.getProperty("name").getTextValue();
    _paramIDsByName[paramName] = i;
    _paramNamesByID.push_back(paramName);
  }
  
  getProcessorStateParams(pdl, _processorStateParams);
  getControllerStateParams(pdl, _controllerStateParams);

  // start timers in main thread.
  _timers->start(true);
  
  // register and start Actor
  _instanceNum = _controllerRegistry->getUniqueID();
  _instanceName = TextFragment(getAppName(), "controller", ml::textUtils::naturalNumberToText(_instanceNum));
  registerActor(_instanceName, this);
  Actor::start();
  
  _debugTimer.start([=]() { debug(); }, milliseconds(1000));

  Path patchesRoot = getApplicationDataRoot(getMakerName(), getAppName(), "patches");
  _fileTreeIndex["patches"] = ml::make_unique< FileTree >(patchesRoot, "mlpreset");
}

testAppController::~testAppController()
{
  // don't stop the master Timers-- there may be other plugin instances using it!
  // std::cout << "testAppController: BYE!\n";
}

#pragma mark mlvg

void testAppController::sendMessageToView(Message msg)
{
  sendMessageToActor(_viewName, msg);
}

void testAppController::sendParamToView(Path pname)
{
  sendMessageToActor(_viewName, {Path("set_param", pname), _params[pname], kMsgFromController});
}

void testAppController::sendAllParamsToView()
{
  for(auto& pname : _paramNamesByID)
  {
    sendParamToView(pname);
  }
}

void testAppController::sendAllParamsToProcessor()
{
  for(auto& pname : _paramNamesByID)
  {
    sendParamToProcessor(pname, kMsgSequenceStart | kMsgSequenceEnd);
  }
}


TextFragment testAppController::getPatchHeader(size_t bytes)
{
  TextFragment pn2(getAppName(), ":", _params["current_patch"].getTextValue());
  TextFragment pn3(pn2, ":", textUtils::naturalNumberToText(bytes), " bytes");
  TextFragment pn4(pn3, "\n");
  return pn4;
}

TextFragment testAppController::getPatchAsText()
{
  // TODO manage memory here for patches that might be
  // too big for stack
  
  // get params data in plain projections as binary
  auto procStateValueTree = keepNodesInList(getPlainValues(_params), _processorStateParams);
  auto t = JSONToText(valueTreeToJSON(procStateValueTree));

  
  return t;
}

void testAppController::setPatchFromText(TextFragment t)
{
  auto newParams = JSONToValueTree(textToJSON(t));
  _params.setFromPlainValues(newParams);
}


std::vector< uint8_t > testAppController::getPatchAsBinary()
{
  // TODO manage memory here for patches that might be
  // too big for stack
  
  // get params data in plain projections as binary
  auto procStateValueTree = keepNodesInList(getPlainValues(_params), _processorStateParams);
  auto binaryData = valueTreeToBinary(procStateValueTree);
  
  // compress it
  size_t srcLen = binaryData.size();
  size_t cmpEstimateLen = compressBound(srcLen);
  size_t uncompLen = srcLen;
  mz_ulong cmpActualLen = cmpEstimateLen;
  std::vector< uint8_t > compressedData;
  
  compressedData.resize(cmpEstimateLen);
  
  int cmpResult = compress(compressedData.data(), &cmpActualLen, binaryData.data(), srcLen);
  //printf("Compressed from %u to %u bytes\n", (mz_uint32)srcLen, (mz_uint32)cmpActualLen);
  compressedData.resize(cmpActualLen);
  
  return compressedData;
}

void testAppController::setPatchFromBinary(const std::vector< uint8_t >& p)
{
  mz_ulong uncompSize = p.size();
  std::vector< uint8_t > uncompressedData;
  size_t maxSize = 1024*1024;
  int cmpResult{MZ_BUF_ERROR};
  
  // with our best guess of buffer size, try to uncompress. if the result doesn't
  // fit, double the buffer size and try again.
  while((MZ_BUF_ERROR == cmpResult) && (uncompSize < maxSize))
  {
    uncompSize *= 2;
    uncompressedData.resize(uncompSize);
    cmpResult = uncompress(uncompressedData.data(), &uncompSize, p.data(), p.size());
  }
  
  if(MZ_BUF_ERROR != cmpResult)
  {
    auto newProcParams = binaryToValueTree(uncompressedData);
    _params.setFromPlainValues(newProcParams);
  }
  else
  {
    std::cout << "setPatchFromBinary: decompress failed!\n";
  }
}

void testAppController::sendParamToProcessor(Path pname, uint32_t flags)
{
 
}
 
void testAppController::sendMessageToProcessor(Message msg)
{

}

void testAppController::handleFullQueue()
{
  std::cout << "Controller: full queue! \n";
}

FileTree* testAppController::updateCollection(Path which)
{
  FileTree* pTree {_fileTreeIndex[which].get()};
  if(pTree)
  {
    pTree->scan();
  }
  
  return pTree;
}

void testAppController::debug()
{
  //std::cout << "testAppController: " << getMessagesAvailable() << " messages in queue. max: " << _maxQueueSize << "\n";
//  std::cout << "testAppController @ " << std::hex << (this) << std::dec << " : \n";
//  std::cout << "        timers @ " << std::hex << (&_timers.get()) << std::dec << "\n";
}


void testAppController::handleMessage(Message m)
{
  if(!m.address) return;
  
  std::cout << "testAppController::handleMessage:" << m.address << " " << m.value << " \n ";
  
  Path addr = m.address;
  switch(hash(head(addr)))
  {
    case(hash("set_param")):
    {
      // set a parameter from the normalized value in the message.
      Path whatParam = tail(addr);
      switch(hash(head(whatParam)))
      {
          // TODO refactor, set params here and send to proc if needed
          
        case(hash("current_patch")):
        {
          // if the editor changes the current_patch parameter, load the new patch.
          if(m.value && (m.value != _params["current_patch"]))
          {
            _params["current_patch"] = m.value;
            
            FileTree* pTree {_fileTreeIndex["patches"].get()};
            if(pTree)
            {
              Path newPatch = textToPath(m.value.getTextValue());
              
              // load the file
              // TODO DRY share loading code
              File fileToLoad = getApplicationDataFile(getMakerName(), getAppName(), "patches", newPatch);
              BinaryVector fileData;
              if(fileToLoad.load(fileData))
              {
                setPatchFromBinary(fileData);
                sendAllParamsToProcessor();
                sendAllParamsToView();
                
                _revertState = getNormalizedValues(_params);
                _changedFromRevertValues = false;
              }
            }
          }
          break;
        }
          
        case(hash("view_size")):
        {
          //std::cout << "testAppController::handleMessage: view_size = " << m.value << " \n ";
          _params["view_size"] = m.value;
          break;
        }
          
        default:
        {
          // usual set_param messages
 
          // TODO
          // make undo-able value change here
          // then coalesce and add to history
          // undo will be another message
          
          // save in our ParameterTree
          _params.setParamFromNormalizedValue(whatParam, m.value);
          
          // send to processor
          sendParamToProcessor(whatParam, m.flags);
          
          break;
        }
      }
      break;
    }
      
    case(hash("set_prop")):
    {
      Path whatProp = tail(addr);
      switch(hash(head(whatProp)))
      {
        case(hash("learn_param")):
        {
          //std::cout << "testAppController:: let's learn param: " << m.value << " \n ";
          _currentLearnParam = textToPath(m.value.getTextValue());
          break;
        }
        case(hash("midi_learn_source")):
        {
          // midi_learn_source is sent from the Processor when in MIDI learn mode.
          // ask the learn popup to add an item with name "cc[n]" and the MIDI source as its value.
          MIDISource source = indexToMIDISource(m.value.getUnsignedLongValue());
          TextFragment sourceText{"cc", textUtils::naturalNumberToText(source.cc) };
          sendMessageToView(Message{Path("popup/learn_source_menu/do/add_item", sourceText), m.value});
          
          break;
        }
      }
      break;
    }
      
    case(hash("do")):
    {
      Path whatAction = tail(addr);
      switch(hash(head(whatAction)))
      {
        case(hash("subscribe_to_signal")):
        {
          Path sigPath = textToPath(m.value.getTextValue());
          _signalsSubscribedByView[sigPath] = 1;
          break;
        }
        case(hash("start_learn")):
        {
          sendMessageToProcessor(Message{"learn_mode", pathToText(_currentLearnParam)});
          break;
        }
        case(hash("stop_learn")):
        {
          sendMessageToProcessor(Message{"learn_mode", Text()});
          break;
        }
        case(hash("paste_license")):
        {
          Symbol statusSym;
          if(_params["status"].getTextValue() == "ok")
          {
            statusSym = "ok";
          }
          else
          {
            auto lt(_pasteClipboardText());
            auto newLicense = decryptLicense(lt);
            
            Symbol newLicenseStatus(newLicense["status"].getTextValue());
            if(newLicenseStatus == "ok")
            {
              // sets status and licensor before write.
              setStatusFromLicense(newLicense);
              
              // write and show status.
              statusSym = writeLicenseToDisk(lt).getTextFragment();
            }
            else
            {
              statusSym = newLicenseStatus;
            }
          }

          showStatusInPopup(statusSym);
          updateRegLabel();
          break;
        }
        case(hash("check_status")):
        {
          setStatusFromLicense(decryptLicense(readLicenseFromDisk()));
          updateRegLabel();
          break;
        }
        case(hash("copy_to_clipboard")):
        {
          auto patch = getPatchAsBinary();
          size_t bytes = patch.size();
          TextFragment t(getPatchHeader(bytes), textUtils::base64Encode(patch));
          Steinberg::SystemClipboard::copyTextToClipboard(t.getText());
          
          break;
        }
        case(hash("send_current_file_path")):
        {
          // TODO broadcast_param general
          sendParamToView("current_patch");
          break;
        }
        case(hash("save_over")):
        {
          // save over current file path
          Path currentPath = textToPath(_params["current_patch"].getTextValue()); // TODO Path properties
          File saveFile = getApplicationDataFile(getMakerName(), getAppName(), "patches", currentPath);
          
          auto patch = getPatchAsBinary();
          size_t bytes = patch.size();
          // TextFragment t(getPatchHeader(bytes), textUtils::base64Encode(patch));
          
          if(saveFile.hasWriteAccess())
          {
            saveFile.replaceWithData(patch);
            _revertState = getNormalizedValues(_params);
            _changedFromRevertValues = false;
          }
          else
          {
            std::cout << "save to file: no write access!\n";
            // TODO
          }
          
          sendMessageToActor(_viewName, {"editor/do/update_collection", "patches"});
          sendParamToView("current_patch");
          break;
        }
        case(hash("save_as")):
        {
          File patchDir(getApplicationDataRoot(getMakerName(), getAppName(), "patches"));
          
          if(!patchDir.exists())
          {
            // make directory
            Symbol r = patchDir.createDirectory();
            if(r != "OK")
            {
              // TODO present error
              std::cout << "create directory failed: " << r << "\n";
            }
          }
          
          if(!patchDir.exists()) return;
          
          auto patchDirPath = patchDir.getFullPath();
          auto patchDirText = pathToText(patchDirPath);
          auto patchDirUTF = patchDirText.getText();
          
          NFD_Init();
          nfdchar_t* savePathAsString;
          Path savePath;
          
          Path currentPath = textToPath(_params["current_patch"].getTextValue());
          Symbol currentName = last(currentPath);
          TextFragment defaultName (currentName.getTextFragment(), ".mlpreset");
          
          // prepare filters for the dialog
          const int nFilters = 1;
          nfdfilteritem_t filterItem[nFilters] = {{"Aaltoverb presets", "mlpreset"}};
          
          // show the dialog
          nfdresult_t result = NFD_SaveDialog(&savePathAsString, filterItem, nFilters, patchDirUTF, defaultName.getText());
          if (result == NFD_OKAY)
          {
            puts("Success!");
            puts(savePathAsString);
            savePath = Path(savePathAsString);
            NFD_FreePath(savePathAsString);
          }
          else if (result == NFD_CANCEL)
          {
            puts("User pressed cancel.");
          }
          else
          {
            printf("Error: %s\n", NFD_GetError());
          }
          
          // Quit NFD
          NFD_Quit();
          
          // save the file
          if(savePath)
          {
            File saveFile (savePath);
            
            // TODO DRY
            auto patch = getPatchAsBinary();
            
            if(saveFile.hasWriteAccess())
            {
              if(saveFile.replaceWithData(patch))
              {
                // set saved file as current file path
                Path relPath = saveFile.getRelativePathFrom(patchDir);
                _params["current_patch"] = pathToText(removeExtensionFromPath(relPath));
                
                // tell editor to broadcast the updated collection to any
                // Widgets that need it (this will call our updateCollection())
                sendMessageToActor(_viewName, {"editor/do/update_collection", "patches"});
                sendParamToView("current_patch");
                
                _revertState = getNormalizedValues(_params);
                _changedFromRevertValues = false;
              }
              else
              {
                // TODO other save error
              }
            }
            else
            {
              std::cout << "save to file: no write access!\n";
              // TODO
            }
          }
          
          break;
        }
        case(hash("save_version")):
        {
          // TODO DRY!
          
          // get version path
          Path currentPath = textToPath(_params["current_patch"].getTextValue()); // TODO Path properties
          Path nextVersionPath = _incrementVersion(currentPath);
          
          // try incrementing file path until the path no longer exists
          size_t tries{0};
          File saveFile;
          while(tries < 10)
          {
            saveFile = getApplicationDataFile(getMakerName(), getAppName(), "patches", nextVersionPath);
            if(!saveFile.exists()) break;
            nextVersionPath = _incrementVersion(nextVersionPath);
          }
          
          if(!saveFile.exists())
          {
            auto patch = getPatchAsBinary();
            size_t bytes = patch.size();
            TextFragment t(getPatchHeader(bytes), textUtils::base64Encode(patch));
            
            if(saveFile.hasWriteAccess())
            {
              saveFile.replaceWithData(patch);
              _revertState = getNormalizedValues(_params);
              _changedFromRevertValues = false;
            }
            else
            {
              std::cout << "save version: no write access!\n";
            }
            
            _params["current_patch"] = pathToText(nextVersionPath);
            
            sendMessageToActor(_viewName, {"editor/do/update_collection", "patches"});
            sendParamToView("current_patch");
            
            _revertState = getNormalizedValues(_params);
            _changedFromRevertValues = false;
            break;
          }
        }
        case(hash("paste_from_clipboard")):
        {
          std::string clipText;
          auto OK = Steinberg::SystemClipboard::getTextFromClipboard (clipText);
          if(OK)
          {
            TextFragment tf(clipText.c_str());
            auto lines = textUtils::split(tf, '\n');
            size_t n = lines.size();
            auto binaryPatch = textUtils::base64Decode(lines[n - 1]);
            
            setPatchFromBinary(binaryPatch);
            sendAllParamsToProcessor();
            sendAllParamsToView();
            
            _revertState = getNormalizedValues(_params);
            _changedFromRevertValues = false;
            break;
          }
        }
        case(hash("revert_to_saved")):
        {
          // would be nicer to have just = here, maybe refactor params
          _params.setFromNormalizedValues(_revertState);
          
          sendAllParamsToProcessor();
          sendAllParamsToView();
          break;
        }
        default:
          break;
      }
    }
      break;
      
    default:
    {
      // TODO forward?
      std::cout << "testAppController: unhandled message: " << m << " \n ";
      break;
    }
  }
}

