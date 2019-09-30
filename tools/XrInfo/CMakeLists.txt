set(TARGET_NAME "XrInfo")

setup_qt(Gui Qml Quick QuickControls2)

message(STATUS "Qt5_FOUND ${Qt5_FOUND}")
if (Qt5_FOUND) 
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)
    project(${TARGET_NAME})

    file(GLOB TARGET_SRCS src/*)
    if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${TARGET_NAME}.qrc)
        list(APPEND TARGET_SRCS ${TARGET_NAME}.qrc)
    endif()
    file(GLOB_RECURSE QML_SRCS qml/*)
    list(APPEND TARGET_SRCS ${QML_SRCS})
    foreach(_FILE ${QML_SRCS})
        if(IS_DIRECTORY ${_FILE})
            continue()
        endif()
        source_group(QML FILES ${_FILE})
    endforeach()
    set(TARGET_QML_DIR "${CMAKE_CURRENT_SOURCE_DIR}/qml")
    file(TO_NATIVE_PATH "${TARGET_QML_DIR}" TARGET_QML_DIR)
	add_qt_executable()
    set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "tools")
    target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/include)
    target_link_libraries(${TARGET_NAME} PRIVATE Qt5::Gui Qt5::Qml Qt5::Quick Qt5::QuickControls2)
    target_openxr()
	target_vulkan()
	target_glad()
	target_fmt()

    set(CMAKE_AUTOMOC OFF)
    set(CMAKE_AUTORCC OFF)
endif()





