
file(GLOB_RECURSE EXAMPLES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

if (WIN32)
    #set(CMAKE_WIN32_EXECUTABLE TRUE)
endif()

foreach(EXAMPLE ${EXAMPLES})
    get_filename_component(TARGET_NAME ${EXAMPLE} NAME_WE)

    set(TARGET_QML_DIR ${CMAKE_SOURCE_DIR}/data/qml)

    if ((${TARGET_NAME} MATCHES ".*_qt_.*" ) OR (${TARGET_NAME} MATCHES "^qt_.*" ))
		if (NOT Qt5_FOUND)
			continue()
		endif()
        set(CMAKE_AUTOMOC ON)
        set(CMAKE_AUTORCC ON)
        add_qt_executable(${TARGET_NAME})
		target_qt()
    else()
        add_executable(${TARGET_NAME})
    endif()

    target_sources(${TARGET_NAME} PRIVATE ${EXAMPLE})
    set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "examples")
    add_dependencies(${TARGET_NAME} common)
    target_link_libraries(${TARGET_NAME} PUBLIC common)
    target_link_libraries(${TARGET_NAME} PUBLIC ${CMAKE_THREAD_LIBS_INIT})
    target_compile_definitions(${TARGET_NAME} PRIVATE EXAMPLE_NAME="${EXAMPLE_NAME}")
	# FIXME remove this when the openxr.hpp header no longer freaks out
	target_compile_definitions(${TARGET_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)

    # String formatting library
    target_fmt()
    # 3D math library
    target_glm()
    # OpenXR loader
    target_openxr()
    # Cross-platform window management
    # target_glfw3()
    # target_sdl2()
    # Cross-platform UI
	target_imgui()
    # GPU compressed images
    target_basisu()
    # OpenGL function loader
	target_glad()

	target_glfw3()

    if ((${TARGET_NAME} MATCHES ".*_magnum_.*" ) OR (${TARGET_NAME} MATCHES "^magnum_.*" ))
        target_magnum()
    endif()
    

    if ((${TARGET_NAME} MATCHES ".*_qt_.*" ) OR (${TARGET_NAME} MATCHES "^qt_.*" ))
		target_qt()
        set(CMAKE_AUTOMOC OFF)
        set(CMAKE_AUTORCC OFF)
    endif()

    if ((${TARGET_NAME} MATCHES ".*_gl_.*" ) OR (${TARGET_NAME} MATCHES "^gl_.*" ))
        target_glad()
    endif()

    if ((${TARGET_NAME} MATCHES ".*_sdl2_.*" ) OR (${TARGET_NAME} MATCHES "^sdl2_.*" ))
        target_sdl2()
    endif()

    if ((${TARGET_NAME} MATCHES ".*_vk_.*" ) OR (${TARGET_NAME} MATCHES "vk_.*" ))
        target_vulkan()
    endif()

    if (${TARGET_NAME} MATCHES ".*_d3d11_.*" )
    endif()

    if (${TARGET_NAME} MATCHES ".*_d3d12_.*" )
    endif()


endforeach()


