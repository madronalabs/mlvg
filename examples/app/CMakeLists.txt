# CMakeLists.txt for mlvg example app.
# for example:
# mkdir build
# cd build
# cmake -GXcode ..
#
# Windows:
# cmake -G"Visual Studio 14 2015 Win64" ..


#--------------------------------------------------------------------
# make SDL2 application target
#--------------------------------------------------------------------

 
set(target testapp)

create_resources (resources build/resources)

file(GLOB LOCAL_SOURCES "${CMAKE_SOURCE_DIR}/examples/app/*.cpp")
file(GLOB LOCAL_INCLUDES "${CMAKE_SOURCE_DIR}/examples/app/*.h")

list(APPEND test_app_sources ${LOCAL_SOURCES} )
list(APPEND test_app_sources ${LOCAL_INCLUDES} )
list(APPEND test_app_sources ${RESOURCES} )
list(APPEND test_app_sources "${MLVG_SOURCE_DIR}/native/MLSDLUtils.h" )

if(APPLE)
elseif(WIN32)
    list(APPEND test_app_sources "${MLVG_SOURCE_DIR}/native/hidpi.manifest" )
endif()

add_executable(${target} ${test_app_sources})


# find SDL headers and libraries
target_compile_definitions(${target} PRIVATE ML_INCLUDE_SDL=1)
if(APPLE)
    # to use the SDL2 framework in /Library/Frameworks, do this:
    set_target_properties(${target} PROPERTIES XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "/Library/Frameworks")
    find_package(SDL2 REQUIRED COMPONENTS SDL2)
            
    
elseif(WIN32)
    # it seems like there's no standard place to put the SDL dev tools on Windows.
    # we put them in an SDL dir alongside the main project directory.
    set(SDL2_INCLUDE_DIRS "${CMAKE_CURRENT_LIST_DIR}/../SDL/include")

    # Support 64 bit builds
    set(SDL2_LIBRARIES_DIR "${CMAKE_CURRENT_LIST_DIR}/../SDL/VisualC/x64/Release")

    # note: linking with the debug build of the SDL library made an executable that crashed.
    # So for now we link to the release version always.
    target_link_libraries(${target} PRIVATE "${SDL2_LIBRARIES_DIR}/SDL2.lib")
    target_link_libraries(${target} PRIVATE "${SDL2_LIBRARIES_DIR}/SDL2main.lib")
endif()


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

# add the package info
if(APPLE)
    set_target_properties(
        ${target} PROPERTIES
        # these settings allow the bundle version to be updated from the buildApp script.
        # the stale version from when the XCode project was made will still appear in the
        # project's "Custom macOS Application Target Properties."
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.madronalabs.guiro_example_app"
        XCODE_ATTRIBUTE_INFOPLIST_FILE ${CMAKE_CURRENT_LIST_DIR}/examples/app/mac/Info.plist
        XCODE_ATTRIBUTE_INFOPLIST_PREPROCESS NO
        )
        
elseif(APPLE)
    # target_sources(${target} PRIVATE resource/guiro_example_app.rc)
endif(APPLE)
        

if(APPLE)
    target_compile_definitions(${target} PUBLIC "$<$<CONFIG:DEBUG>:DEBUG>")
    target_compile_definitions(${target} PUBLIC "$<$<CONFIG:RELEASE>:NDEBUG>")
endif()

# add UI libs and frameworks- note that these appear under
# "other linker flags" in XCode and not in its file browser
if(APPLE)
    target_link_libraries(${target} PRIVATE "-framework Cocoa" "-framework Metal" "-framework MetalKit"
        "-framework CoreAudio" "-framework CoreServices"
        "-framework AppKit" "-framework SDL2" )
elseif(WIN32)
    find_package(OpenGL REQUIRED)
    target_link_libraries(${target} PRIVATE ${OPENGL_gl_LIBRARY})
    target_link_libraries(${target} PRIVATE "Shcore.lib")
    target_link_libraries(${target} PRIVATE "winmm.lib")
    target_link_libraries(${target} PRIVATE "setupapi.lib")
    target_link_libraries(${target} PRIVATE "version.lib")
    target_link_libraries(${target} PRIVATE "Imm32.lib")
    target_link_libraries(${target} PRIVATE debug "msvcrtd.lib" optimized "msvcrt.lib")
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

source_group(resources REGULAR_EXPRESSION "${MLVG_SOURCE_DIR}/../examples/app/build/resources/*.*")



