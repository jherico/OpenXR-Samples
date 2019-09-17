macro(TARGET_GLAD)
    find_package(glad CONFIG REQUIRED)
    target_link_libraries(${TARGET_NAME} PUBLIC glad::glad)
endmacro()