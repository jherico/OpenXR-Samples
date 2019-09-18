set(TARGET_NAME common)

set(SHADER_DIR "${PROJECT_SOURCE_DIR}/data/shaders")
file(GLOB_RECURSE SHADERS
    ${SHADER_DIR}/*.vert
    ${SHADER_DIR}/*.frag
    ${SHADER_DIR}/*.comp
    ${SHADER_DIR}/*.tesc
    ${SHADER_DIR}/*.tese
    ${SHADER_DIR}/*.geom
)
GroupSources("data/shaders")


foreach(SHADER ${SHADERS})
    compile_spirv_shader(${SHADER})
    list(APPEND COMPILED_SHADERS ${COMPILE_SPIRV_SHADER_RETURN})
    source_group("compiled" FILES ${COMPILE_SPIRV_SHADER_RETURN})
endforeach()
add_custom_target(shaders SOURCES ${SHADERS} ${COMPILED_SHADERS})
set_target_properties(shaders PROPERTIES FOLDER "common")

file(GLOB_RECURSE COMMON_SOURCE *.c *.cpp *.h *.hpp)
add_library(${TARGET_NAME} STATIC ${COMMON_SOURCE})
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "common")
add_dependencies(${TARGET_NAME} shaders)
GroupSources("src/common")

target_include_directories(${TARGET_NAME} PUBLIC "C:/Users/bdavi/git/OpenXR-SDK-Source/include")
target_include_directories(${TARGET_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

target_basisu()
target_glfw3()
target_glm()
target_gli()
target_vulkan()
target_assimp()
target_imgui()
target_glad()
target_fmt()

if (ANDROID)
    add_dependencies(${TARGET_NAME} app-glue)
    target_link_libraries(${TARGET_NAME} PRIVATE app-glue android log m)
endif()