 # root CMakeLists.txt for mlvg examples.
 # for example:
 # mkdir build
 # cd build
 # cmake -GXcode ..
 #
 # Windows:
 # cmake -G"Visual Studio 14 2015 Win64" ..
 
 # for windows validator debug:
 # debugging / command: C:\Users\Randy\dev\mlvg\build\bin\Debug\validator.exe
 # debugging / command arguments: C:\Users\Randy\dev\mlvg\build\VST3\Debug\aaltoverb.vst3\Contents\x86_64-win\aaltoverb.vst3
 
 cmake_minimum_required (VERSION 3.16)

  
 IF(APPLE)
  SET(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "Build architectures for Mac OS X" FORCE)
  SET(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum OS X deployment version")
 ENDIF(APPLE)

#--------------------------------------------------------------------
# project and version
#--------------------------------------------------------------------

set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)

project(mlvg)

set(VERSION_MAJOR "0")
set(VERSION_MINOR "1")
set(VERSION_PATCH "0")
set(VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

option(BUILD_SDL2_APP "Build SDL2 example app" ON)

 #--------------------------------------------------------------------
 # Compiler flags
 #--------------------------------------------------------------------
 
 set(CMAKE_CXX_STANDARD 17)
 set(CMAKE_CXX_STANDARD_REQUIRED True)
 
 if(APPLE)
   # For now, explicitly disable C++17 alignment feature
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-aligned-new")
 elseif(WIN32)
   # no unknown pragma warning
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4068")
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zc:alignedNew-")
 endif()

if(MSVC)
    # arcane thing about setting runtime library flags
    cmake_policy(SET CMP0091 NEW)

    add_compile_options(
        $<$<CONFIG:>:/MT>
        $<$<CONFIG:Debug>:/MTd>
        $<$<CONFIG:Release>:/MT>
    )
endif()

 #--------------------------------------------------------------------
 # Choose library output name
 #--------------------------------------------------------------------
 
 # creates the library mlvg-debug in debug configuration
 set(mlvg_NAME mlvg$<$<CONFIG:Debug>:-debug>)

#--------------------------------------------------------------------
# find madronalib
# MacOS: /usr/local/include/madronalib
# Windows: C:/Program Files/madronalib/include
#--------------------------------------------------------------------

if(APPLE)
    include(GNUInstallDirs)
    set (MADRONALIB_INCLUDE_DIR "${CMAKE_INSTALL_FULL_INCLUDEDIR}/madronalib")
    set (MADRONALIB_LIBRARY_DIR ${CMAKE_INSTALL_FULL_LIBDIR})
elseif(WIN32)
    if(EXISTS "C:/Program Files/madronalib/lib/madrona.lib")
        set (MADRONALIB_INCLUDE_DIR "C:/Program Files/madronalib/include")
        set (MADRONALIB_LIBRARY_DIR "C:/Program Files/madronalib/lib")
    else()
        set (MADRONALIB_INCLUDE_DIR "C:/Program Files (x86)/madronalib/include")
        set (MADRONALIB_LIBRARY_DIR "C:/Program Files (x86)/madronalib/lib")
    endif()
else()
endif()

 # add -debug suffix to link debug madronalib for debug builds
set(madronalib_NAME madrona$<$<CONFIG:Debug>:-debug>)

message("madronalib headers should be in: " ${MADRONALIB_INCLUDE_DIR} )
message("madronalib library should be in: " ${MADRONALIB_LIBRARY_DIR}/${madronalib_NAME} )


 #--------------------------------------------------------------------
 # find mlvg sources including our juce-core subset 
 #--------------------------------------------------------------------
 
 set (MLVG_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/source")
 
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
          ${MLVG_SOURCE_DIR}/external/glad/glad.c
          ${MLVG_SOURCE_DIR}/external/glad/glad.h
          ${MLVG_SOURCE_DIR}/external/glad/khrplatform.h
      )
      set(NANOVG_INCLUDE_DIRS
          ${MLVG_SOURCE_DIR}/external/nanovg/src
          ${MLVG_SOURCE_DIR}/external/nanosvg/src
          ${MLVG_SOURCE_DIR}/external/glad
      )
      if (MSVC)
          set_source_files_properties(source/external/glad/glad.c
          PROPERTIES COMPILE_FLAGS /wd4055)
      endif()
  endif()
 
 #--------------------------------------------------------------------
 # gather osdialog sources
 #--------------------------------------------------------------------
 
 set(osdialog_sources
    ${MLVG_SOURCE_DIR}/external/osdialog/osdialog.c
 )
  
 if(WIN32)
   set(osdialog_sources_native
       ${MLVG_SOURCE_DIR}/external/osdialog/osdialog_win.c
   )
 elseif(APPLE)
   set(osdialog_sources_native
      ${MLVG_SOURCE_DIR}/external/osdialog/osdialog_mac.m
   )
 elseif(UNIX AND NOT APPLE)
   set(osdialog_sources_native
       ${MLVG_SOURCE_DIR}/external/osdialog/osdialog_gtk3.c
   )
 endif()
 
 list(APPEND osdialog_sources ${osdialog_sources_native} )

#--------------------------------------------------------------------
# gather cJSON sources
#--------------------------------------------------------------------

set(cjson_sources
${MLVG_SOURCE_DIR}/external/cJSON/cJSON.c
${MLVG_SOURCE_DIR}/external/cJSON/cJSON.h)

set(cjson_include_dirs ${MLVG_SOURCE_DIR}/external/cJSON)

#--------------------------------------------------------------------
# add filesystem library
#--------------------------------------------------------------------

add_subdirectory(${MLVG_SOURCE_DIR}/external/filesystem)

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
   ${MLVG_SOURCE_DIR}/external
)

if(APPLE)
  set(MLVG_SOURCES_NATIVE
    ${MLVG_SOURCE_DIR}/native/MLFilesMac.mm
    ${MLVG_SOURCE_DIR}/native/NanoVGViewMacGL.mm
    ${MLVG_SOURCE_DIR}/native/NanoVGViewMacMetal.mm
   )
elseif(WIN32)
  set(MLVG_SOURCES_NATIVE
    ${MLVG_SOURCE_DIR}/native/MLFilesWin.cpp
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
list(APPEND MLVG_INCLUDE_DIRS "${NANOVG_INCLUDE_DIRS}")
list(APPEND MLVG_INCLUDE_DIRS "${MLVG_SOURCE_DIR}/include")
list(APPEND MLVG_INCLUDE_DIRS "${MADRONALIB_SOURCE_DIR}")
list(APPEND MLVG_INCLUDE_DIRS "${MADRONALIB_INCLUDE_DIR}")
list(APPEND MLVG_INCLUDE_DIRS "${MADRONALIB_INCLUDE_DIR}/madronalib")
list(APPEND MLVG_INCLUDE_DIRS "${cjson_include_dirs}")

#--------------------------------------------------------------------
# make and install mlvg library
#--------------------------------------------------------------------
  
set(target mlvg)

# Define the library
add_library(${target} STATIC ${mlvg_sources} ${osdialog_sources} ${cjson_sources})

set_target_properties(${target} PROPERTIES
  OUTPUT_NAME "${mlvg_NAME}"
  VERSION ${VERSION}
  SOVERSION ${VERSION_MAJOR}
  POSITION_INDEPENDENT_CODE ON
  FOLDER "mlvg")

target_include_directories(${target} PRIVATE ${MLVG_INCLUDE_DIRS})

if(APPLE)
    target_compile_options(${target} PRIVATE "-fobjc-arc")
elseif(WIN32)
endif()


target_link_libraries(${target} ghc_filesystem)

include(GNUInstallDirs)

if(WIN32)
    set(CMAKE_INSTALL_LIBDIR "C:/Program Files/mlvg/lib")

    if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
      set(CMAKE_INSTALL_PREFIX "C:/Program Files/mlvg" CACHE PATH "..." FORCE)
    endif()
endif()

install(
    TARGETS ${target}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

message("mlvg destination: " ${CMAKE_INSTALL_LIBDIR} )

#set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_SOURCE_DIR}) % wrong for Windows. needed for Mac?
set(CMAKE_SKIP_INSTALL_ALL_DEPENDENCY true)
install(TARGETS mlvg DESTINATION ${CMAKE_INSTALL_LIBDIR})

#--------------------------------------------------------------------
# install headers
#--------------------------------------------------------------------

if(APPLE)
    set(HEADERS_INCLUDE_DIR "include/mlvg")
elseif(WIN32)
    set(HEADERS_INCLUDE_DIR "include")
endif()


file(GLOB COMMON_HEADERS "source/common/*.h")
file(GLOB WIDGETS_HEADERS "source/widgets/*.h")

# TODO clean this up! should not be externally visible
file(GLOB NATIVE_HEADERS "source/native/*.h")
file(GLOB NATIVE_HEADERS_HACK "source/external/MetalNanoVG/src/*.h")
file(GLOB NATIVE_HEADERS_HACK2 "source/external/nanovg/src/*.h")
file(GLOB NATIVE_HEADERS_HACK3 "source/external/nanosvg/src/*.h")
file(GLOB NATIVE_HEADERS_HACK4 "source/external/miniz/*.h")
file(GLOB NATIVE_HEADERS_HACK5 "source/external/osdialog/*.h")
file(GLOB NATIVE_HEADERS_HACK6 "source/external/glad/*.h")


install(FILES
    source/include/mlvg.h
    ${COMMON_HEADERS}
    ${WIDGETS_HEADERS}
    ${NATIVE_HEADERS}
    ${NATIVE_HEADERS_HACK}
    ${NATIVE_HEADERS_HACK2}
    ${NATIVE_HEADERS_HACK3}
    ${NATIVE_HEADERS_HACK4}
    ${NATIVE_HEADERS_HACK5}
    ${NATIVE_HEADERS_HACK6}
    DESTINATION
    # PERMISSIONS OWNER_READ GROUP_READ WORLD_READ
    ${HEADERS_INCLUDE_DIR}
  )

#--------------------------------------------------------------------
# make SDL2 application target
#--------------------------------------------------------------------

if(BUILD_SDL2_APP)
    create_resources (examples/app/resources build/resources/testapp)
     
    set(target testapp)

    file(GLOB LOCAL_SOURCES "${CMAKE_SOURCE_DIR}/examples/app/*.cpp")
    file(GLOB LOCAL_INCLUDES "${CMAKE_SOURCE_DIR}/examples/app/*.h")

    list(APPEND test_app_sources ${LOCAL_SOURCES} )
    list(APPEND test_app_sources ${LOCAL_INCLUDES} )

    add_executable(${target} ${test_app_sources})
    target_compile_definitions(${target} PRIVATE ML_INCLUDE_SDL=1)

    # find SDL headers and libraries
    if(APPLE)
        # to use the SDL2 framework in /Library/Frameworks, do this:
        find_package(SDL2 REQUIRED COMPONENTS SDL2)
    elseif(WIN32)
        # it seems like there's no standard place to put the SDL dev tools on Windows.
        # we put them in an SDL dir alongside the main project directory.
        set(SDL2_INCLUDE_DIRS "${CMAKE_CURRENT_LIST_DIR}/../SDL/include")
        # Support both 32 and 64 bit builds
        if (${CMAKE_SIZEOF_VOID_P} MATCHES 8)
          set(SDL2_LIBRARIES_DIR "${CMAKE_CURRENT_LIST_DIR}/../SDL/lib/x64")
        else ()
          set(SDL2_LIBRARIES_DIR "${CMAKE_CURRENT_LIST_DIR}/../SDL/lib/x86")
        endif ()
        target_link_libraries(${target} PRIVATE "${SDL2_LIBRARIES_DIR}/SDL2.lib")
        target_link_libraries(${target} PRIVATE "${SDL2_LIBRARIES_DIR}/SDL2main.lib")
    endif()
    message("SDL2 headers should be in: " ${SDL2_INCLUDE_DIRS} )
    message("SDL2 libraries should be in: " ${SDL2_LIBRARIES_DIR} )
    target_include_directories(${target} PRIVATE ${SDL2_INCLUDE_DIRS})


    # add madronalib
    target_include_directories(${target} PRIVATE ${MADRONALIB_INCLUDE_DIR})
    target_include_directories(${target} PRIVATE ${MADRONALIB_INCLUDE_DIR}/madronalib)
    if(APPLE)
        target_link_libraries(${target} PRIVATE "${MADRONALIB_LIBRARY_DIR}/lib${madronalib_NAME}.a")
    elseif(WIN32)
        target_link_libraries(${target} PRIVATE "${MADRONALIB_LIBRARY_DIR}/${madronalib_NAME}.lib")
    endif()

    # add mlvg library
    target_include_directories(${target} PRIVATE ${MLVG_INCLUDE_DIRS})
    target_link_libraries(${target} PRIVATE "mlvg")


    if(APPLE)
        # ARC must be off for juce
        # target_compile_options(${target} PRIVATE "-fno-objc-arc")

        target_compile_definitions(${target} PUBLIC ML_INCLUDE_SDL=1)
        target_compile_definitions(${target} PUBLIC "$<$<CONFIG:DEBUG>:DEBUG>")
        target_compile_definitions(${target} PUBLIC "$<$<CONFIG:RELEASE>:NDEBUG>")

        # add UI libs and frameworks- note that these appear under
        # "other linker flags" in XCode and not in its file browser
        if(APPLE)
            target_link_libraries(${target} PRIVATE "-framework Cocoa" "-framework Metal" "-framework MetalKit"
              "-framework CoreAudio" "-framework CoreServices"
              "-framework AppKit" "-framework SDL2" )
        elseif(WIN32)
            find_package(OpenGL REQUIRED)
            target_link_libraries(${target} PRIVATE ${OPENGL_gl_LIBRARY})
        endif()
    endif()
endif()

#--------------------------------------------------------------------
# set source groups for some source above
#--------------------------------------------------------------------

source_group(nanovg REGULAR_EXPRESSION "${MLVG_SOURCE_DIR}/external/nanovg/src.*|${MLVG_SOURCE_DIR}/external/MetalNanoVG/src.*")
source_group(nanosvg REGULAR_EXPRESSION "${MLVG_SOURCE_DIR}/external/nanosvg/src.*")
set(NATIVE1 "${MLVG_SOURCE_DIR}/native/*.*")
set(NATIVE2 "${MLVG_SOURCE_DIR}/external/osdialog/osdialog_mac.m")
set(NATIVE3 "${MLVG_SOURCE_DIR}/external/MetalNanoVG/src/*.*")
source_group(native REGULAR_EXPRESSION "${NATIVE1}|${NATIVE2}|${NATIVE3}")
# source_group(native REGULAR_EXPRESSION "*.m")



