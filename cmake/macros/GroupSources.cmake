# 
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 

macro(GroupSources basedir curdir)
    file(GLOB children RELATIVE ${curdir} ${curdir}/*)
    foreach(child ${children})
        if(IS_DIRECTORY ${curdir}/${child})
            GroupSources(${basedir} ${curdir}/${child})
        elseif(NOT ("${child}" STREQUAL "CMakeLists.txt"))
            file(TO_NATIVE_PATH childFile "${curdir}/${child}")
            file(RELATIVE_PATH groupname ${basedir} ${curdir})
            set(groupname "src\\${groupname}")
            string(REPLACE "/" "\\" groupname ${groupname})
            source_group(${groupname} FILES ${curdir}/${child})
        endif()
    endforeach()
endmacro()
