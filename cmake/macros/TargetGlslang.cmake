# 
#  Copyright 2015 High Fidelity, Inc.
#  Created by Bradley Austin Davis on 2015/10/10
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 
macro(TARGET_GLSLANG)
    add_dependency_external_projects(glslang)
    list(APPEND EXTERNALS glslang)
    include_directories(${GLSLANG_INCLUDE_DIRS})
    link_libraries(${GLSLANG_LIBRARIES})
endmacro()



