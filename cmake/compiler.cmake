set(CMAKE_CXX_FLAGS_DEBUG  "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT (CMAKE_SIZEOF_VOID_P EQUAL 8))
  message( FATAL_ERROR "Only 64 bit builds supported." )
endif()

if (WIN32)
  add_definitions(-DNOMINMAX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zi")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /DEBUG /OPT:REF /OPT:ICF")
else ()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -fno-strict-aliasing -Wno-unused-parameter")
  if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ggdb -Woverloaded-virtual -Wdouble-promotion")
      if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER "5.1") # gcc 5.1 and on have suggest-override
          set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wsuggest-override")
      endif ()
      if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "5.3")
          # GLM 0.9.8 on Ubuntu 14 (gcc 4.4) has issues with the simd declarations
          add_definitions(-DGLM_FORCE_PURE)
      endif()
  endif ()
endif()

if(APPLE)
  set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --stdlib=libc++")
  if (CMAKE_GENERATOR STREQUAL "Xcode")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
    set(CMAKE_XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS[variant=Release] "YES")
    set(CMAKE_XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT[variant=Release] "dwarf-with-dsym")
    set(CMAKE_XCODE_ATTRIBUTE_DEPLOYMENT_POSTPROCESSING[variant=Release] "YES")
  endif()

  exec_program(sw_vers ARGS -productVersion  OUTPUT_VARIABLE OSX_VERSION)
  string(REGEX MATCH "^[0-9]+\\.[0-9]+" OSX_VERSION ${OSX_VERSION})
  message(STATUS "Detected OS X version = ${OSX_VERSION}")
  set(OSX_SDK "${OSX_VERSION}" CACHE String "OS X SDK version to look for inside Xcode bundle or at OSX_SDK_PATH")
  # set our OS X deployment target
  set(CMAKE_OSX_DEPLOYMENT_TARGET 10.8)
  # find the SDK path for the desired SDK
  find_path(
    _OSX_DESIRED_SDK_PATH
    NAME MacOSX${OSX_SDK}.sdk
    HINTS ${OSX_SDK_PATH}
    PATHS /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/
          /Applications/Xcode-beta.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/
  )

  if (NOT _OSX_DESIRED_SDK_PATH)
    message(STATUS "Could not find OS X ${OSX_SDK} SDK. Will fall back to default. If you want a specific SDK, please pass OSX_SDK and optionally OSX_SDK_PATH to CMake.")
  else ()
    message(STATUS "Found OS X ${OSX_SDK} SDK at ${_OSX_DESIRED_SDK_PATH}/MacOSX${OSX_SDK}.sdk")

    # set that as the SDK to use
    set(CMAKE_OSX_SYSROOT ${_OSX_DESIRED_SDK_PATH}/MacOSX${OSX_SDK}.sdk)
  endif ()
endif()
