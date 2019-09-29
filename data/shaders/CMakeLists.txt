file(GLOB_RECURSE SHADERS_SOURCES *.vert *.frag *.glsl)
add_custom_target(shaders SOURCES ${SHADERS_SOURCES})
set_target_properties(shaders PROPERTIES FOLDER "common")
