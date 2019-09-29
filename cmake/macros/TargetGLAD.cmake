#
#  Created by Bradley Austin Davis on 2019/09/29
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 
macro(TARGET_GLAD)
    find_package(glad CONFIG REQUIRED)
    target_link_libraries(${TARGET_NAME} PRIVATE glad::glad)
endmacro()
