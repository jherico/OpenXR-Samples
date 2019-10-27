
macro(setup_qt)
    find_package(Qt5 5.14 QUIET COMPONENTS ${ARGN})
    if (Qt5_FOUND)
        message(STATUS "Qt ${Qt5_VERSION} found")
        if (NOT Qt5_BIN_DIR) 
            get_property(QMAKE_EXECUTABLE TARGET Qt5::qmake PROPERTY LOCATION)
            get_filename_component(Qt5_BIN_DIR ${QMAKE_EXECUTABLE} DIRECTORY CACHE)
        endif()
    
        if(WIN32)
            find_program(WINDEPLOYQT_COMMAND windeployqt PATHS ${Qt5_BIN_DIR} NO_DEFAULT_PATH)
            if (NOT WINDEPLOYQT_COMMAND)
                message(FATAL_ERROR "Could not find windeployqt at ${Qt5_BIN_DIR}. windeployqt is required.")
            endif()
        elseif(APPLE)
            find_program(MACDEPLOYQT_COMMAND macdeployqt PATHS ${Qt5_BIN_DIR} NO_DEFAULT_PATH)
            if (NOT MACDEPLOYQT_COMMAND)
                message(FATAL_ERROR "Could not find macdeployqt at ${Qt5_BIN_DIR}. macdeployqt is required.")
            endif()
        endif()
    endif()
endmacro()

macro(add_qt_executable)
	if (WIN32)
        add_executable(${TARGET_NAME} WIN32 ${TARGET_SRCS})
        set(DEPLOY_COMMANDS ${WINDEPLOYQT_COMMAND} )
        list(APPEND DEPLOY_COMMANDS --no-compiler-runtime --no-opengl-sw --no-angle -no-system-d3d-compiler)
        if (TARGET_QML_DIR)
            list(APPEND DEPLOY_COMMANDS --qmldir "${TARGET_QML_DIR}")
        endif()
        add_custom_command(
            TARGET ${TARGET_NAME}
            POST_BUILD
            COMMAND 
                ${DEPLOY_COMMANDS}
                ${EXTRA_DEPLOY_OPTIONS} 
                $<$<OR:$<CONFIG:Release>,$<CONFIG:MinSizeRel>,$<CONFIG:RelWithDebInfo>>:--release>
                "$<TARGET_FILE:${TARGET_NAME}>"
        )
	else(APPLE)
		add_executable(${TARGET_NAME} MACOSX_BUNDLE ${INTERFACE_SRCS} ${QM})
		set_target_properties(${TARGET_NAME} PROPERTIES INSTALL_RPATH "@executable_path/../Frameworks")
        add_custom_command(
            TARGET ${TARGET_NAME}
            POST_BUILD
            COMMAND 
                ${MACDEPLOYQT_COMMAND} 
                $<$<OR:$<CONFIG:Release>,$<CONFIG:MinSizeRel>,$<CONFIG:RelWithDebInfo>>:--release>
                --no-opengl-sw --no-angle
                "$<TARGET_FILE:${TARGET_NAME}>"
        )
	endif()
endmacro()





