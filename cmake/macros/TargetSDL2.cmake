#
#  Created by Bradley Austin Davis on 2019/09/29
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 
macro(TARGET_SDL2)
    find_package(SDL2 CONFIG REQUIRED)
    target_link_libraries(${TARGET_NAME} PRIVATE SDL2::SDL2 SDL2::SDL2main)
endmacro()
