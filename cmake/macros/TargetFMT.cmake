# 
#  Created by Bradley Austin Davis on 2019/08/13
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 
macro(TARGET_FMT)
    find_package(fmt CONFIG REQUIRED)                                        
    target_link_libraries(${TARGET_NAME} PUBLIC fmt::fmt fmt::fmt-header-only)        
    target_compile_definitions(${TARGET_NAME} PUBLIC HAVE_FMT_FORMAT)
endmacro()
