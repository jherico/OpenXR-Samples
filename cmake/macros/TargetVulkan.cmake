# 
#  Created by Bradley Austin Davis on 2016/02/16
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 
macro(TARGET_VULKAN)
    find_package(Vulkan REQUIRED)
    target_link_libraries(${TARGET_NAME} PUBLIC Vulkan::Vulkan)
	if (WIN32) 
		target_compile_definitions(${TARGET_NAME} PUBLIC VK_USE_PLATFORM_WIN32_KHR)
	endif()

    if (ANDROID)
        add_definitions(-DVK_USE_PLATFORM_ANDROID_KHR)
    elseif (WIN32)
        add_definitions(-DVK_USE_PLATFORM_WIN32_KHR)
    else()
        add_definitions(-DVK_USE_PLATFORM_XCB_KHR)
        find_package(XCB REQUIRED)
        target_link_libraries(${TARGET_NAME} PRIVATE ${XCB_LIBRARIES})
    endif()
endmacro()