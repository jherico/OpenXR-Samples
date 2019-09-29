
file(GLOB_RECURSE EXAMPLES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

if (WIN32)
    #set(CMAKE_WIN32_EXECUTABLE TRUE)
endif()

foreach(EXAMPLE ${EXAMPLES})
    get_filename_component(TARGET_NAME ${EXAMPLE} NAME_WE)

    add_executable(${TARGET_NAME})
    target_sources(${TARGET_NAME} PRIVATE ${EXAMPLE})
    set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "examples")
    add_dependencies(${TARGET_NAME} common)
    target_link_libraries(${TARGET_NAME} PUBLIC common)
    target_link_libraries(${TARGET_NAME} PUBLIC ${CMAKE_THREAD_LIBS_INIT})
    target_compile_definitions(${TARGET_NAME} PRIVATE EXAMPLE_NAME="${EXAMPLE_NAME}")
	# FIXME remove this when the openxr.hpp header no longer freaks out
	target_compile_definitions(${TARGET_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)
	# FIXME remove this when Oculus starts working with debug utils ext
    target_compile_definitions(${TARGET_NAME} PRIVATE SUPPRESS_DEBUG_UTILS)

    target_fmt()
    target_glm()
    target_openxr()
	target_basisu()
    target_magnum()

    if ((${TARGET_NAME} MATCHES ".*_gl_.*" ) OR (${TARGET_NAME} MATCHES "^gl_.*" ))
        target_glad()
		target_sdl2()
		set_target_properties(${TARGET_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
    endif()

    if ((${TARGET_NAME} MATCHES ".*_vk_.*" ) OR (${TARGET_NAME} MATCHES "vk_.*" ))
        target_vulkan()
    endif()

    if (${TARGET_NAME} MATCHES ".*_d3d11_.*" )
    endif()

    if (${TARGET_NAME} MATCHES ".*_d3d12_.*" )
    endif()


endforeach()


