# 
#  Created by Bradley Austin Davis on 2016/02/16
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 
macro(TARGET_GLFW3)
    if (NOT ANDROID)
        find_package(glfw3 CONFIG REQUIRED)
        target_link_libraries(${TARGET_NAME} PUBLIC glfw)
    endif()
endmacro()