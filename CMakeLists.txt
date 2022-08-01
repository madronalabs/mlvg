 # root CMakeLists.txt for mlvg examples.
 # to run, set VST3_SDK_ROOT to inform CMake where the VST3 SDK is, for example:
 # mkdir build
 # cd build
 # cmake -DVST3_SDK_ROOT=~/dev/VST3_SDK -DSMTG_RUN_VST_VALIDATOR=0 -GXcode ..
 #
 # Windows:
 # cmake -DVST3_SDK_ROOT=~/dev/VST3_SDK -DSMTG_RUN_VST_VALIDATOR=0 -DSMTG_ADD_VSTGUI=0 -G"Visual Studio 14 2015 Win64" ..
 
 # for windows validator debug:
 # debugging / command: C:\Users\Randy\dev\mlvg\build\bin\Debug\validator.exe
 # debugging / command arguments: C:\Users\Randy\dev\mlvg\build\VST3\Debug\aaltoverb.vst3\Contents\x86_64-win\aaltoverb.vst3
 
 cmake_minimum_required (VERSION 3.16)
 
 set(VST3_SDK_ROOT "" CACHE PATH "Location of VST3 SDK")
 set(public_sdk_SOURCE_DIR ${VST3_SDK_ROOT}/public.sdk)
 
 set(CMAKE_OSX_DEPLOYMENT_TARGET "10.10" CACHE STRING "Minimum OS X deployment version")
 
 IF(APPLE)
    # uncomment this to build a universal binary for both platforms.
    # SET(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "Build architectures for Mac OS X" FORCE)
 ENDIF(APPLE)
 
 project(mlvg)
 set(PLUGIN_NAME "llllPluginNamellll")
 
 set(llllpluginnamellll_sources
     source/factoryDefinition.cpp
     source/pluginController.cpp
     source/pluginController.h
     source/llllpluginnamellll.cpp
     source/llllpluginnamellll.h
     source/pluginProcessor.cpp
     source/pluginProcessor.h
     source/signalProcessor.h
     source/processorParameters.h
     source/version.h
     source/wrapper.h
 )
 
 #--------------------------------------------------------------------
 # Compiler flags
 #--------------------------------------------------------------------
 
 set(CMAKE_CXX_STANDARD 14)
 set(CMAKE_CXX_STANDARD_REQUIRED True)
 
 #--------------------------------------------------------------------
 # find madronalib
 #--------------------------------------------------------------------
 
 if(APPLE)
     include(GNUInstallDirs)
     set (MADRONALIB_INCLUDE_DIR ${CMAKE_INSTALL_FULL_INCLUDEDIR})
     set (MADRONALIB_LIBRARY_DIR ${CMAKE_INSTALL_FULL_LIBDIR})
 elseif(WIN32)
     set (MADRONALIB_INCLUDE_DIR "C:/Program Files/madronalib/include")
     set (MADRONALIB_LIBRARY_DIR "C:/Program Files/madronalib/lib")
 else()
 endif()
 
 # add -debug suffix to link debug madronalib for debug builds
 set(madronalib_NAME madrona$<$<CONFIG:Debug>:-debug>)
 
 message("madronalib headers should be in: " ${MADRONALIB_INCLUDE_DIR} )
 message("madronalib library should be at: " ${MADRONALIB_LIBRARY_DIR}/${madronalib_NAME} )
 
 #--------------------------------------------------------------------
 # find mlvg sources - including our juce-core subset and nfd
 #--------------------------------------------------------------------
 
 set (MLVG_SOURCE_DIR "source")
 
 #--------------------------------------------------------------------
 # compile binary resources
 #--------------------------------------------------------------------
 
 # Creates C resources file from files in given directory
 function(create_resources dir outputdir)
 
     # Collect input files
     file(GLOB bins ${dir}/*)
 
     # Create empty main include file
     set(includefile "${outputdir}/resources.c")
     file(WRITE "${includefile}" "")
 
     # Iterate through input files
     foreach(bin ${bins})
     
         # Get short filename
         string(REGEX MATCH "([^/]+)$" filename ${bin})
 
         # Replace filename spaces & extension separator for C compatibility
         string(REGEX REPLACE "\\.| |-" "_" filename ${filename})
 
         # Read hex data from file
         file(READ ${bin} filedata HEX)
 
         # Convert hex data for C compatibility
         string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," filedata ${filedata})
 
         # Create empty output file
         set(outputfile "${outputdir}/${filename}.c")
         file(WRITE "${outputfile}" "namespace resources \n{\n")
 
         # Append data to output file
         file(APPEND "${outputfile}" "const unsigned char ${filename}[] = {${filedata}};\nconst unsigned ${filename}_size = sizeof(${filename});\n")
         file(APPEND "${outputfile}" "\n}")
 
         # Append filename to main include file
         file(APPEND "${includefile}" "#include \"${filename}.c\"\n")
 
     endforeach()
 
 endfunction()
 
 create_resources (resource build/resources)
 
 #--------------------------------------------------------------------
 # gather nanovg and nanosvg sources
 #--------------------------------------------------------------------
 
 if(APPLE)
     set(NANOVG_SOURCES
         ${MLVG_SOURCE_DIR}/external/nanovg/src/nanovg.c
         ${MLVG_SOURCE_DIR}/external/nanovg/src/nanovg.h
         ${MLVG_SOURCE_DIR}/external/nanosvg/src/nanosvg.h
         ${MLVG_SOURCE_DIR}/external/MetalNanoVG/src/nanovg_mtl.h
         ${MLVG_SOURCE_DIR}/external/MetalNanoVG/src/nanovg_mtl.m
     )
     set(NANOVG_INCLUDE_DIRS
         ${MLVG_SOURCE_DIR}/external/nanovg/src
         ${MLVG_SOURCE_DIR}/external/nanosvg/src
         ${MLVG_SOURCE_DIR}/external/MetalNanoVG/src
     )
 elseif(WIN32)
     set(NANOVG_SOURCES
         ${MLVG_SOURCE_DIR}/external/nanovg/src/nanovg.c
         ${MLVG_SOURCE_DIR}/external/nanovg/src/nanovg.h
         ${MLVG_SOURCE_DIR}/external/nanosvg/src/nanosvg.h
         ${MLVG_SOURCE_DIR}/external/nanovg/src/nanovg_gl.h
         ${MLVG_SOURCE_DIR}/external/nanovg/src/nanovg_gl_utils.h
         ${MLVG_SOURCE_DIR}/external/glad/src/glad.c
         ${MLVG_SOURCE_DIR}/external/glad/include/glad/glad.h
         ${MLVG_SOURCE_DIR}/external/glad/include/KHR/khrplatform.h
     )
     set(NANOVG_INCLUDE_DIRS
         ${MLVG_SOURCE_DIR}/external/nanovg/src
         ${MLVG_SOURCE_DIR}/external/nanosvg/src
         ${MLVG_SOURCE_DIR}/external/glad/include
         ${MLVG_SOURCE_DIR}/external/glad/include/glad
     )
     if (MSVC)
         set_source_files_properties(source/external/glad/src/glad.c
         PROPERTIES COMPILE_FLAGS /wd4055)
     endif()
 endif()
 
 #--------------------------------------------------------------------
 # gather juce-core sources
 #--------------------------------------------------------------------
 
 if(APPLE)
     set(juce_core_sources
         ${MLVG_SOURCE_DIR}/external/juce_core/juce_core.mm
         ${MLVG_SOURCE_DIR}/external/juce_core/JuceCoreConfig.h
     )
     set(juce_core_include_dirs
     ${MLVG_SOURCE_DIR}/external/juce_core
     ${MLVG_SOURCE_DIR}
     )
 else()
     set(juce_core_sources
         ${MLVG_SOURCE_DIR}/external/juce_core/juce_core.cpp
         ${MLVG_SOURCE_DIR}/external/juce_core/JuceCoreConfig.h
     )
     set(juce_core_include_dirs
     ${MLVG_SOURCE_DIR}/external/juce_core
     ${MLVG_SOURCE_DIR}
     )
 endif()
 
#--------------------------------------------------------------------
# gather mlvg sources
#--------------------------------------------------------------------

file(GLOB MLVG_SOURCES_COMMON "${MLVG_SOURCE_DIR}/common/*.cpp")
file(GLOB MLVG_HEADERS_COMMON "${MLVG_SOURCE_DIR}/common/*.h")
file(GLOB MLVG_SOURCES_WIDGETS "${MLVG_SOURCE_DIR}/widgets/*.cpp")
file(GLOB MLVG_HEADERS_WIDGETS "${MLVG_SOURCE_DIR}/widgets/*.h")
  
set(MLVG_INCLUDE_DIRS 
   source
   ${MLVG_SOURCE_DIR}
   ${MLVG_SOURCE_DIR}/external/glad/src
)
 
 if(APPLE)
     set(MLVG_SOURCES_NATIVE
         ${MLVG_SOURCE_DIR}/native/NanoVGViewMacGL.mm
         ${MLVG_SOURCE_DIR}/native/NanoVGViewMacMetal.mm
     )
 elseif(WIN32)
     set(MLVG_SOURCES_NATIVE
         ${MLVG_SOURCE_DIR}/native/NanoVGViewWindowsGL.cpp
     )
 endif()
 
list(APPEND mlvg_sources ${MLVG_SOURCES_COMMON} )
list(APPEND mlvg_sources ${MLVG_HEADERS_COMMON} )
list(APPEND mlvg_sources ${MLVG_SOURCES_WIDGETS} )
list(APPEND mlvg_sources ${MLVG_HEADERS_WIDGETS} )
list(APPEND mlvg_sources ${NANOVG_SOURCES} )
list(APPEND mlvg_sources ${MLVG_SOURCES_NATIVE} )

list(APPEND MLVG_INCLUDE_DIRS "${MLVG_SOURCE_DIR}/common")
list(APPEND MLVG_INCLUDE_DIRS "${MLVG_SOURCE_DIR}/widgets")
list(APPEND MLVG_INCLUDE_DIRS ${NANOVG_INCLUDE_DIRS})
list(APPEND MLVG_INCLUDE_DIRS ${MADRONALIB_INCLUDE_DIR}/madronalib)


#--------------------------------------------------------------------
# make juce-core library
#--------------------------------------------------------------------

set(target juce-core)

add_library(${target} STATIC ${juce_core_sources})

target_include_directories(${target} PRIVATE ${juce_core_include_dirs})

# set juce-core configuration for this target
target_compile_definitions(${target} PRIVATE JUCE_APP_CONFIG_HEADER="external/juce_core/JuceCoreConfig.h")
target_compile_definitions(${target} PUBLIC "$<$<CONFIG:DEBUG>:DEBUG>")
target_compile_definitions(${target} PUBLIC "$<$<CONFIG:RELEASE>:NDEBUG>")

if(APPLE)
  # ARC must be off for juce
  target_compile_options(${target} PRIVATE "-fno-objc-arc")

  # build only active arch for Debug builds
  set_target_properties(${target} PROPERTIES XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH[variant=Debug] "YES")

  # set debug info format
  set_target_properties(${target} PROPERTIES XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT "dwarf-with-dsym")
endif()

#--------------------------------------------------------------------
# gather nfd sources
#--------------------------------------------------------------------

set(nfd_sources
${MLVG_SOURCE_DIR}/external/nativefiledialog-extended/src/include/nfd.h
${MLVG_SOURCE_DIR}/external/nativefiledialog-extended/src/include/nfd.hpp)

if(WIN32)
  set(nfd_sources_native
      ${MLVG_SOURCE_DIR}/external/nativefiledialog-extended/src/nfd_win.cpp
  )
elseif(APPLE)
  set(nfd_sources_native
      ${MLVG_SOURCE_DIR}/external/nativefiledialog-extended/src/nfd_cocoa.m
  )
elseif(UNIX AND NOT APPLE)
  set(nfd_sources_native
      ${MLVG_SOURCE_DIR}/external/nativefiledialog-extended/src/nfd_gtk.cpp
  )
endif()

list(APPEND nfd_sources ${nfd_sources_native} )

#--------------------------------------------------------------------
# make nfd library
#--------------------------------------------------------------------

set(target nfd)

# Define the library
add_library(${target} STATIC ${nfd_sources})

# Allow includes from include/
target_include_directories(${target} PUBLIC ${MLVG_SOURCE_DIR}/external/nativefiledialog-extended/src/include/)

# find and link libraries 
if(WIN32)

elseif(APPLE)
  find_library(APPKIT_LIBRARY AppKit)
  target_link_libraries(${target} PRIVATE ${APPKIT_LIBRARY})
elseif(UNIX AND NOT APPLE)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
  message("Using GTK version: ${GTK3_VERSION}")
  target_include_directories(${target} PRIVATE ${GTK3_INCLUDE_DIRS})
  target_link_libraries(${target} PRIVATE ${GTK3_LIBRARIES})
endif()

# compiler settings
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
string(REPLACE "/EHsc" "/EHs-c-" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
string(REPLACE "/GR" "/GR-" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
set_property(TARGET ${target} APPEND_STRING PROPERTY STATIC_LIBRARY_OPTIONS /NODEFAULTLIB)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
target_compile_options(${target} PRIVATE -nostdlib -fno-exceptions -fno-rtti)
endif()

if(APPLE)
  # build only active arch for Debug builds
  set_target_properties(${target} PROPERTIES XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH[variant=Debug] "YES")

  # set debug info format
  set_target_properties(${target} PROPERTIES XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT "dwarf-with-dsym")
endif()
 
#--------------------------------------------------------------------
# make mlvg target 
# contains all our graphics / UI source
#--------------------------------------------------------------------
  
set(target mlvg)

# Define the library
add_library(${target} STATIC ${mlvg_sources})

target_include_directories(${target} PRIVATE ${MLVG_INCLUDE_DIRS})


#--------------------------------------------------------------------
# make SDL2 application target 
#--------------------------------------------------------------------

create_resources (examples/app/resources build/resources/testapp)
 
set(target testapp)

file(GLOB LOCAL_SOURCES "${CMAKE_SOURCE_DIR}/examples/app/*.cpp")
file(GLOB LOCAL_INCLUDES "${CMAKE_SOURCE_DIR}/examples/app/*.h")

list(APPEND test_app_sources ${LOCAL_SOURCES} )
list(APPEND test_app_sources ${LOCAL_INCLUDES} )

add_executable(${target} ${test_app_sources})

find_package(SDL2 REQUIRED COMPONENTS SDL2::SDL2)
target_link_libraries(${target} PRIVATE SDL2::SDL2 mlvg nfd juce-core)

target_include_directories(${target} PRIVATE ${MLVG_INCLUDE_DIRS})

# add madronalib includes
target_include_directories(${target} PRIVATE ${MADRONALIB_INCLUDE_DIR}/madronalib)

if(APPLE)
    # ARC must be off for juce
    target_compile_options(${target} PRIVATE "-fno-objc-arc")
    
    # build only active arch for Debug builds
    set_target_properties(${target} PROPERTIES XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH[variant=Debug] "YES")
    
    # set debug info format
    set_target_properties(${target} PROPERTIES XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT "dwarf-with-dsym")
endif()
 
# set compiler options
if(APPLE)
    target_compile_options(${target} PRIVATE "-Werror=return-type")
elseif(WIN32)
    target_compile_options(${target} PRIVATE "/wd4068") # silence unknown pragma warning
    target_compile_options(${target} PRIVATE "/EHa") # standard C++ stack unwinding
endif()

target_compile_definitions(${target} PUBLIC "$<$<CONFIG:DEBUG>:DEBUG>")
target_compile_definitions(${target} PUBLIC "$<$<CONFIG:RELEASE>:NDEBUG>")

# add UI frameworks- note that these appear under 
# "other linker flags" in XCode and not in its file browser
if(APPLE)
    target_link_libraries(${target} PRIVATE "-framework Cocoa")
    target_link_libraries(${target} PRIVATE "-framework Metal")
    target_link_libraries(${target} PRIVATE "-framework MetalKit")
     target_link_libraries(${target} PRIVATE "${MADRONALIB_LIBRARY_DIR}/lib${madronalib_NAME}.a")   
elseif(WIN32)
    find_package(OpenGL REQUIRED)
    target_link_libraries(${target} PRIVATE ${OPENGL_gl_LIBRARY})
    target_link_libraries(${target} PRIVATE "${MADRONALIB_LIBRARY_DIR}/${madronalib_NAME}.lib")
endif()

if(APPLE)
    # build only active arch for Debug builds
    set_target_properties(${target} PROPERTIES XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH[variant=Debug] "YES")

    # set debug info format
    set_target_properties(${target} PROPERTIES XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT "dwarf-with-dsym")
endif() 

message("SDL2 include dirs: " ${SDL2_INCLUDE_DIRS} )

 
 #--------------------------------------------------------------------
 # set VST library options
 #--------------------------------------------------------------------
# 
# SET(SMTG_ADD_VSTGUI OFF CACHE BOOL "Add VSTGUI support")
# SET(VSTGUI_TOOLS OFF CACHE BOOL "Build VSTGUI Tools")
# 
# if(APPLE)
#      set(SMTG_PLUGIN_TARGET_PATH "/Library/Audio/Plug-Ins/VST3" )
# endif()
# 
# include(examples/plugin/VST3_SDK.cmake)
# 
#--------------------------------------------------------------------
# make VST plugin base target 
# contains our plugin code and the vst sdk and base libraries
# and graphics libraries
#--------------------------------------------------------------------

# set(target pluginbase)
# 
# add_library(${target} STATIC ${llllpluginnamellll_sources})
# 
# smtg_set_vst_win_architecture_name(${target})
# 
# target_link_libraries(${target} PRIVATE base sdk)
# 
# if(SMTG_ADD_VSTGUI)
#     target_include_directories(${target} PUBLIC ${VSTGUI_ROOT}/vstgui4)
#     target_link_libraries(${target} PRIVATE vstgui_support)
# endif()
# 
# set(PLUGINBASE_INCLUDE_DIRS
#     source
#     ${MLVG_SOURCE_DIR}
#     ${MLVG_SOURCE_DIR}/vg
#     ${MLVG_SOURCE_DIR}/external/nanovg/src
#     ${MLVG_SOURCE_DIR}/external/nanosvg/src
#     ${MLVG_SOURCE_DIR}/external/MetalNanoVG/src
#     ${MLVG_SOURCE_DIR}/external/glad/include/glad
#     ${MLVG_SOURCE_DIR}/external/glad/include
# )
# 
# target_include_directories(${target} PRIVATE ${PLUGINBASE_INCLUDE_DIRS})
# 
# # set compiler options
# if(APPLE)
#     target_compile_options(${target} PRIVATE "-Werror=return-type")
# elseif(WIN32)
#     target_compile_options(${target} PRIVATE "/wd4068") # silence unknown pragma warning
#     target_compile_options(${target} PRIVATE "/EHa") # standard C++ stack unwinding
# endif()
# 
# target_compile_definitions(${target} PUBLIC "$<$<CONFIG:DEBUG>:DEBUG>")
# target_compile_definitions(${target} PUBLIC "$<$<CONFIG:RELEASE>:NDEBUG>")
# 
# # add UI frameworks- note that these appear under 
# # "other linker flags" in XCode and not in its file browser
# if(APPLE)
#     target_link_libraries(${target} PRIVATE "-framework Cocoa")
#     target_link_libraries(${target} PRIVATE "-framework Metal")
#     target_link_libraries(${target} PRIVATE "-framework MetalKit")
#      target_link_libraries(${target} PRIVATE "${MADRONALIB_LIBRARY_DIR}/lib${madronalib_NAME}.a")   
# elseif(WIN32)
#     find_package(OpenGL REQUIRED)
#     target_link_libraries(${target} PRIVATE ${OPENGL_gl_LIBRARY})
#     target_link_libraries(${target} PRIVATE "${MADRONALIB_LIBRARY_DIR}/${madronalib_NAME}.lib")
# endif()
# 
# # add madronalib 
# target_include_directories(${target} PRIVATE ${MADRONALIB_INCLUDE_DIR}/madronalib)
# 
# if(APPLE)
#     # build only active arch for Debug builds
#     set_target_properties(${target} PROPERTIES XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH[variant=Debug] "YES")
# 
#     # set debug info format
#     set_target_properties(${target} PROPERTIES XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT "dwarf-with-dsym")
# endif() 
# 
# # get output directories
# set(VST3_OUTPUT_DIR ${CMAKE_BINARY_DIR}/VST3)
# set(outputdir ${VST3_OUTPUT_DIR}/$<CONFIG>)
 
 #--------------------------------------------------------------------
 # add vst target
 #--------------------------------------------------------------------
 
# set(target llllpluginnamellll-vst)
# 
# # Get the default VST3 folder. on Mac, override Steinberg fn and use the
# # root /Library not the user's. 
# if(SMTG_MAC)
#     set(DEFAULT_VST3_FOLDER "/Library/Audio/Plug-Ins/VST3" )
# else()
#     smtg_get_default_vst3_path()
# endif()
# 
# message("VST3 folder: " ${DEFAULT_VST3_FOLDER} )
# 
# set(SMTG_CREATE_PLUGIN_LINK OFF)
# smtg_add_vst3plugin(${target})
# 
# # make sure the name of the vst executable inside the vst folder
# # matches the plugin name! Some hosts like Live require this.
# set_target_properties( ${target} PROPERTIES OUTPUT_NAME "${PLUGIN_NAME}" )
# 
# 
# # paths used by install target below
# if(SMTG_MAC)
#     set(VST_INSTALL_SRC ${outputdir}/${PLUGIN_NAME}.vst3)
#     set(VST_INSTALL_DEST ${DEFAULT_VST3_FOLDER}/${PLUGIN_NAME}.vst3)
# elseif(WIN32)
#     set(VST_INSTALL_SRC ${outputdir}/${target}.vst3)
#     set(VST_INSTALL_DEST ${DEFAULT_VST3_FOLDER}/${PLUGIN_NAME}.vst3)
# endif()
# 
# # add all libraries
# target_link_libraries(${target} PRIVATE pluginbase juce-core nfd)
# 
# target_include_directories(${target} PRIVATE ${MLVG_INCLUDE_DIRS})
# 
# # set compiler options
# if(APPLE)
#     target_compile_options(${target} PRIVATE "-Werror=return-type")
# elseif(WIN32)
#     target_compile_options(${target} PRIVATE "/wd4068") # silence unknown pragma warning
#     target_compile_options(${target} PRIVATE "/EHa") # standard C++ stack unwinding
# endif()
# 
# # add the package info
# if(SMTG_MAC)
#     set_target_properties( ${target} PROPERTIES
#         XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.madronalabs.vst3.llllpluginnamellll"
#         MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_LIST_DIR}/source/mac/Info.plist
#         XCODE_ATTRIBUTE_INFOPLIST_PREPROCESS YES )
# elseif(SMTG_WIN)
#     target_sources(${target} PRIVATE resource/llllpluginnamellll.rc)
# endif(SMTG_MAC)
# 
# # add mlvg as source
# target_sources(${target} PRIVATE ${mlvg_sources})
# 
# # add VST wrapper
# target_sources(${target} PRIVATE source/wrapper_vst.cpp)
# 
# # set VST API
# target_compile_definitions(${target} PRIVATE VST3_API=1)
# 
# if(APPLE)
#     # build only active arch for Debug builds
#     set_target_properties(${target} PROPERTIES XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH[variant=Debug] "YES")
# 
#     # set debug info format
#     set_target_properties(${target} PROPERTIES XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT "dwarf-with-dsym")
# endif()
# 
# # based on the API setting this header will create unique Obj-C symbols for the VST. 
# # Any Obj-C modules need to be included as source.
# set_target_properties (${target} PROPERTIES XCODE_ATTRIBUTE_GCC_PREFIX_HEADER "source/MLVGPrefixHeader.h")
# 
# 
# if(APPLE)
#     # ARC must be on for nanovg-metal
#     target_compile_options(${target} PRIVATE "-fobjc-arc")
#     target_compile_options(${target} PRIVATE "-Werror=return-type")
# endif()

 #--------------------------------------------------------------------
 # add vst-for-au target
 # makes the vst plugin used inside the au wrapper, with
 # symbol names changed that would otherwise conflict with the VST.
 # Creates llllPluginNamellll VST4AU.vst3 package
 #--------------------------------------------------------------------
 
# if(APPLE)
#     set(target llllpluginnamellll-vst-for-au)
# 
#     set(SMTG_CREATE_PLUGIN_LINK OFF)
#     smtg_add_vst3plugin(${target})
#     
#     # make sure the name of the vst executable inside the vst folder
#     # matches the plugin name! Some hosts like Live require this.
#     set_target_properties( ${target} PROPERTIES OUTPUT_NAME "${PLUGIN_NAME} VST4AU" )
# 
#     target_include_directories(${target} PRIVATE "source")
# 
#     # add mlvg as source
#     target_sources(${target} PRIVATE ${mlvg_sources})
#     target_include_directories(${target} PRIVATE ${MLVG_INCLUDE_DIRS})
# 
#     # add the package info
#     set_target_properties( ${target} PROPERTIES
#         XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.madronalabs.vst3.llllpluginnamellll"
#         MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_LIST_DIR}/source/mac/Info.plist
#         XCODE_ATTRIBUTE_INFOPLIST_PREPROCESS YES )
# 
#     target_sources(${target} PRIVATE source/au/wrapper_au.cpp)
#     target_compile_definitions(${target} PRIVATE AU_API=1)
#     set_target_properties (${target} PROPERTIES XCODE_ATTRIBUTE_GCC_PREFIX_HEADER "source/MLVGPrefixHeader.h")
# 
#     target_link_libraries(${target} PRIVATE pluginbase juce-core nfd)
# 
#     # build only active arch for Debug builds
#     set_target_properties(${target} PROPERTIES XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH[variant=Debug] "YES")
# 
#     # set debug info format
#     set_target_properties(${target} PROPERTIES XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT "dwarf-with-dsym")
# 
#     target_compile_options(${target} PRIVATE "-fobjc-arc")
#     target_compile_options(${target} PRIVATE "-Werror=return-type")
# 
# endif()
# 
 
 #--------------------------------------------------------------------
 # add au target using auwrapper.
 # Creates llllPluginNamellll.component package containing  
 # wrapper code "llllPluginNamellll AU"
 #--------------------------------------------------------------------
 
# if(SMTG_MAC AND XCODE AND SMTG_COREAUDIO_SDK_PATH)
# 
#     set(target llllpluginnamellll-au)
# 
#     set(DEFAULT_AU_FOLDER "/Library/Audio/Plug-Ins/Components" )
# 
#     set(${target}_sources
#         source/au/audiounitconfig.h
#         source/au/wrapper_au.cpp
#     )
# 
#     add_library(${target} MODULE ${${target}_sources})
#     
#     # set name of component and executable wrapper inside it
#     set_target_properties( ${target} PROPERTIES OUTPUT_NAME "${PLUGIN_NAME}" )
# 
#     add_dependencies(${target} llllpluginnamellll-vst-for-au)
# 
#     smtg_setup_universal_binary(${target})
# 
#     target_include_directories(${target} PRIVATE "source")
# 
#     set_target_properties(${target} PROPERTIES BUNDLE TRUE)
#     set_target_properties(${target} PROPERTIES XCODE_ATTRIBUTE_GENERATE_MASTER_OBJECT_FILE "YES")
#     set_target_properties(${target} PROPERTIES XCODE_ATTRIBUTE_OTHER_LDFLAGS "-all_load")
#     set_target_properties(${target} PROPERTIES XCODE_ATTRIBUTE_GENERATE_PKGINFO_FILE "YES")
#     set_target_properties(${target} PROPERTIES XCODE_ATTRIBUTE_WRAPPER_EXTENSION "component")
#     set_target_properties(${target} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${VST3_OUTPUT_DIR})
# 
#     # build only active arch for Debug builds
#     set_target_properties(${target} PROPERTIES XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH[variant=Debug] "YES")
# 
#     # set debug info format
#     set_target_properties(${target} PROPERTIES XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT "dwarf-with-dsym")
# 
#     target_link_libraries(${target} PRIVATE auwrapper)
# 
#     # preprocess Info.plist for AU wrapper - uses definitions in audiounitconfig.h
#     smtg_set_bundle(${target}
#        BUNDLE_IDENTIFIER "com.madronalabs.llllpluginnamellll"
#        INFOPLIST "${CMAKE_CURRENT_LIST_DIR}/source/au/Info.plist" PREPROCESS)
# 
#     # paths used by commands below
#     set(VST4AU "${outputdir}/${PLUGIN_NAME} VST4AU.vst3")
#     set(AUCOMPONENT "${outputdir}/${PLUGIN_NAME}.component")
# 
#     # copy the whole VST4AU package into wrapper component folder
#     add_custom_command(
#         TARGET ${target} POST_BUILD 
#         COMMAND echo "copying VST into AU wrapper"
#         COMMAND /bin/mkdir "-p" "${AUCOMPONENT}/Contents/Resources"
#         COMMAND /bin/rm "-rf" "${AUCOMPONENT}/Contents/Resources/plugin.vst3"
#         COMMAND ditto "-v" "${VST4AU}" "${AUCOMPONENT}/Contents/Resources/plugin.vst3" 
#         COMMAND chmod -R 777 "${AUCOMPONENT}/Contents/Resources/plugin.vst3" 
#     )
# 
# endif()
 
 
 #--------------------------------------------------------------------
 # install the VST to the VST3 folder.
 #--------------------------------------------------------------------
 
# set(target install-vst)
# 
# add_custom_target(
#     install-vst
#     ALL
#     DEPENDS llllpluginnamellll-vst
# )
# 
# if(APPLE)
# add_custom_command(
#     TARGET ${target} POST_BUILD
#     COMMAND echo "installing VST3 from ${VST_INSTALL_SRC} to ${VST_INSTALL_DEST}"
#     COMMAND ditto "-v" "${VST_INSTALL_SRC}" "${VST_INSTALL_DEST}"
# )
# elseif(WIN32)
# add_custom_command(
#     TARGET ${target} POST_BUILD
#     COMMAND echo "installing VST3 from ${VST_INSTALL_SRC} to ${VST_INSTALL_DEST}"
#     COMMAND ${CMAKE_COMMAND} -E copy_directory "${VST_INSTALL_SRC}" "${VST_INSTALL_DEST}"
# )
# endif()
# 
 
 #--------------------------------------------------------------------
 # install the AU to the Components folder.
 #--------------------------------------------------------------------
 
# if(APPLE)
# set(target install-au)
# 
# add_custom_target(
#     install-au
#     ALL
#     DEPENDS llllpluginnamellll-au
# )
# 
# set(DEFAULT_AU_FOLDER "/Library/Audio/Plug-Ins/Components")
# 
# add_custom_command(
#     TARGET ${target} POST_BUILD 
#     COMMAND echo "Installing AU into /Library"
#     COMMAND /bin/rm "-rf" "${DEFAULT_AU_FOLDER}/${PLUGIN_NAME}.component"
#     COMMAND ditto "-v" "${outputdir}/${PLUGIN_NAME}.Component" "${DEFAULT_AU_FOLDER}/${PLUGIN_NAME}.component"
#     COMMAND chmod -R 777 "${DEFAULT_AU_FOLDER}/${PLUGIN_NAME}.component"
# )
# 
# endif()
#

#--------------------------------------------------------------------
# set source groups for some source above
#--------------------------------------------------------------------

source_group(vstsdk REGULAR_EXPRESSION "vst3sdk+")
source_group(mlvg-native REGULAR_EXPRESSION "${MLVG_SOURCE_DIR}/vg/native.*")
source_group(nanovg REGULAR_EXPRESSION "${MLVG_SOURCE_DIR}/external/nanovg/src.*|${MLVG_SOURCE_DIR}/external/MetalNanoVG/src.*")
source_group(nanosvg REGULAR_EXPRESSION "${MLVG_SOURCE_DIR}/external/nanosvg/src.*")


