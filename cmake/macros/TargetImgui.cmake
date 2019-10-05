macro(TARGET_IMGUI)
    find_package(imgui CONFIG REQUIRED)
    target_link_libraries(${TARGET_NAME} PUBLIC imgui::imgui)
endmacro()