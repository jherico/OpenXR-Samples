macro(TARGET_OPENXR)
    find_library(OPENXR_LIBRARY_RELEASE openxr_loader PATHS ${EZVCPKG_DIR}/lib NO_DEFAULT_PATH)
    find_library(OPENXR_LIBRARY_DEBUG openxr_loader PATHS ${EZVCPKG_DIR}/debug/lib NO_DEFAULT_PATH)
    include(SelectLibraryConfigurations)
    select_library_configurations(OPENXR)
    target_link_libraries(${TARGET_NAME} PRIVATE ${OPENXR_LIBRARIES})
    target_include_directories(${TARGET_NAME} PRIVATE "C:\\Users\\bdavi\\git\\OpenXR-SDK-Source\\include")
endmacro()


