# 
#  Copyright 2015 High Fidelity, Inc.
#  Created by Bradley Austin Davis on 2015/10/10
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 
macro(TARGET_MAGNUM)
    if (NOT Corrade_FOUND)
        find_package(Corrade CONFIG QUIET REQUIRED)                                        
        find_package(Magnum CONFIG QUIET REQUIRED Trade MeshTools Primitives SceneGraph GL Shaders Sdl2Application)
    endif()

    target_link_libraries(${TARGET_NAME} PUBLIC Magnum::Magnum Magnum::Trade Magnum::MeshTools Magnum::Primitives Magnum::SceneGraph Magnum::GL Magnum::Shaders Magnum::Sdl2Application)
    target_compile_definitions(${TARGET_NAME} PRIVATE USE_MAGNUM)
endmacro()
