set(TARGET_NAME common)

add_library(${TARGET_NAME})

target_compile_definitions(${TARGET_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)
file(GLOB_RECURSE COMMON_SOURCE *.hpp *.cpp)
target_sources(${TARGET_NAME} PRIVATE ${COMMON_SOURCE})
target_include_directories(${TARGET_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "common")
GroupSources(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

# String formatting library
target_fmt()

target_magnum()

# 3D math library
target_glm()
# OpenXR loader
target_openxr()

target_basisu()



# set(SHADER_DIR "${PROJECT_SOURCE_DIR}/data/shaders")
# file(GLOB_RECURSE SHADERS
#     ${SHADER_DIR}/*.vert
#     ${SHADER_DIR}/*.frag
#     ${SHADER_DIR}/*.comp
#     ${SHADER_DIR}/*.tesc
#     ${SHADER_DIR}/*.tese
#     ${SHADER_DIR}/*.geom
# )
# GroupSources("data/shaders")

# foreach(SHADER ${SHADERS})
#     compile_spirv_shader(${SHADER})
#     list(APPEND COMPILED_SHADERS ${COMPILE_SPIRV_SHADER_RETURN})
#     source_group("compiled" FILES ${COMPILE_SPIRV_SHADER_RETURN})
# endforeach()

# add_custom_target(shaders SOURCES ${SHADERS} ${COMPILED_SHADERS})
# set_target_properties(shaders PROPERTIES FOLDER "common")

# add_dependencies(${TARGET_NAME} shaders)
# GroupSources("src/common")

# if (ANDROID)
#     add_dependencies(${TARGET_NAME} app-glue)
#     target_link_libraries(${TARGET_NAME} PRIVATE app-glue android log m)
# endif()