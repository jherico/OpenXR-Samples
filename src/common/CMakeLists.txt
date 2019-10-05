set(TARGET_NAME common)

if (Qt5_FOUND)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)
endif()

add_library(${TARGET_NAME})

target_compile_definitions(${TARGET_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)
file(GLOB_RECURSE COMMON_SOURCE *.hpp *.cpp)
target_sources(${TARGET_NAME} PRIVATE ${COMMON_SOURCE})
target_include_directories(${TARGET_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "common")
GroupSources(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

# String formatting library
target_fmt()
# 3D math library
target_glm()
# OpenXR loader
target_openxr()
# Cross-platform window management
#target_glfw3()
target_basisu()
target_glad()
target_qt()
target_magnum()
target_vulkan()

if (Qt5_FOUND)
    set(CMAKE_AUTOMOC OFF)
    set(CMAKE_AUTORCC OFF)
endif()