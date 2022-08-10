// config header for juce_core module included unser ISC license

#pragma once


#define JUCE_STANDALONE_APPLICATION 0


//==============================================================================
// juce_core flags:

#define JUCE_DISABLE_JUCE_VERSION_PRINTING 1

#ifndef    JUCE_FORCE_DEBUG
 //#define JUCE_FORCE_DEBUG
#endif

#ifndef    JUCE_LOG_ASSERTIONS
 //#define JUCE_LOG_ASSERTIONS
#endif

#define JUCE_CHECK_MEMORY_LEAKS 0

#ifndef    JUCE_DONT_AUTOLINK_TO_WIN32_LIBRARIES
 //#define JUCE_DONT_AUTOLINK_TO_WIN32_LIBRARIES
#endif

#ifndef    JUCE_INCLUDE_ZLIB_CODE
 //#define JUCE_INCLUDE_ZLIB_CODE
#endif

#ifndef    JUCE_USE_CURL
 //#define JUCE_USE_CURL
#endif
