cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

include("cmake/defaults.cmake")
set(NAME OpenXRExamples)

# Dependency management
include(${CMAKE_SOURCE_DIR}/cmake/ezvcpkg/ezvcpkg.cmake)
ezvcpkg_fetch(
    COMMIT fa7a3eaec35409d60dbc456d5229b54c192f11de
    PACKAGES assimp basisu fmt glad glm glfw imgui openxr-loader vulkan magnum "magnum-plugins[tinygltfimporter]"
    UPDATE_TOOLCHAIN
    USE_HOST_VCPKG
    SAVE_BUILD
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


setup_qt(Gui Qml Quick QuickControls2 3DCore 3DExtras 3DQuick)

add_subdirectory(data/shaders)
add_subdirectory(src/common)
add_subdirectory(src/examples)
