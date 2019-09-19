cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

include("cmake/defaults.cmake")
set(NAME OpenXRExamples)

include(${CMAKE_SOURCE_DIR}/cmake/ezvcpkg/ezvcpkg.cmake)

ezvcpkg_fetch(
    REPO jherico/vcpkg
    COMMIT cfb3b69551964c583c4917509ea37c76efc49a4d
    PACKAGES assimp basisu fmt imgui glad glfw3 gli glm openxr-loader vulkan
    UPDATE_TOOLCHAIN
    USE_HOST_VCPKG
)

project(${NAME})

if (NOT "${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
    message( FATAL_ERROR "Only 64 bit builds supported." )
endif()

include("cmake/compiler.cmake")

find_package(Threads REQUIRED)

if (NOT ANDROID)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/bin")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/bin_debug")
    set(CMAKE_DEBUG_POSTFIX "d")

    add_custom_target(SetupRelease ALL ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/bin)
    set_target_properties(SetupRelease PROPERTIES FOLDER "CMakeTargets")
    add_custom_target(SetupDebug ALL ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/bin_debug)
    set_target_properties(SetupDebug PROPERTIES FOLDER "CMakeTargets")
endif()

if (ANDROID)
    add_definitions(-DVULKAN_HPP_NO_SMART_HANDLE)
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")
    set(APP_GLUE_DIR ${ANDROID_NDK}/sources/android/native_app_glue)
    include_directories(${APP_GLUE_DIR})
    add_library(app-glue STATIC ${APP_GLUE_DIR}/android_native_app_glue.c)
endif()

add_subdirectory(src/common)
add_subdirectory(src/examples)
