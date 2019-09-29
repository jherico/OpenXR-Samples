# 
#  Copyright 2015 High Fidelity, Inc.
#  Created by Bradley Austin Davis on 2015/10/10
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 
macro(TARGET_BASISU)
    include(SelectLibraryConfigurations)

    find_library(BASISU_ENCODER_LIBRARY_RELEASE basisu_encoder PATHS ${EZVCPKG_DIR}/lib NO_DEFAULT_PATH)
    find_library(BASISU_ENCODER_LIBRARY_DEBUG basisu_encoder PATHS ${EZVCPKG_DIR}/debug/lib NO_DEFAULT_PATH)
    select_library_configurations(BASISU_ENCODER)

    find_library(BASISU_TRANSCODER_LIBRARY_RELEASE basisu_transcoder PATHS ${EZVCPKG_DIR}/lib NO_DEFAULT_PATH)
    find_library(BASISU_TRANSCODER_LIBRARY_DEBUG basisu_transcoder PATHS ${EZVCPKG_DIR}/debug/lib NO_DEFAULT_PATH)
    select_library_configurations(BASISU_TRANSCODER)
    target_link_libraries(${TARGET_NAME} PUBLIC ${BASISU_ENCODER_LIBRARIES} ${BASISU_TRANSCODER_LIBRARIES})
    target_compile_definitions(${TARGET_NAME} PRIVATE BASISU_NO_ITERATOR_DEBUG_LEVEL)
    target_include_directories(${TARGET_NAME} PUBLIC ${EZVCPKG_DIR}/include)
    target_include_directories(${TARGET_NAME} PUBLIC ${EZVCPKG_DIR}/include/basisu)
endmacro()
