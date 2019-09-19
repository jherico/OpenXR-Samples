//
//  Created by Bradley Austin Davis
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include <functional>
#include <glm/glm.hpp>
#include <set>
#include <string>
#include <vector>

#if defined(XR_USE_GRAPHICS_API_VULKAN)
#include <vulkan/vulkan.hpp>
#endif

#include <GLFW/glfw3.h>

#if defined(WIN32)
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
namespace glfw {
using NativeContext = HGLRC;
using NativeWindow = HWND;
}  // namespace glfw
#else
#define GLFW_EXPOSE_NATIVE_X11
#define GLFW_EXPOSE_NATIVE_GLX
namespace glfw {
using NativeContext = GLXContext;
using nativeWindow = ::Window
}  // namespace glfw
#endif

#include <GLFW/glfw3native.h>

namespace glfw {

using ResizeHandler = std::function<void(const glm::uvec2&)>;
using CloseHandler = std::function<void()>;
using KeyHandler = std::function<void(int, int, int, int)>;
using MouseButtonHandler = std::function<void(int, int, int)>;
using MouseMoveHandler = std::function<void(glm::vec2)>;
using MouseScrollHandler = std::function<void(float)>;

class Window {
public:
    static bool init() { return GLFW_TRUE == glfwInit(); }
    static void terminate() { glfwTerminate(); }

#ifdef XR_USE_GRAPHICS_API_VULKAN
    static std::vector<std::string> getRequiredInstanceExtensions() {
        std::vector<std::string> result;
        uint32_t count = 0;
        const char** names = glfwGetRequiredInstanceExtensions(&count);
        if (names && count) {
            for (uint32_t i = 0; i < count; ++i) {
                result.emplace_back(names[i]);
            }
        }
        return result;
    }
    static vk::SurfaceKHR createWindowSurface(GLFWwindow* window,
                                              const vk::Instance& instance,
                                              const vk::AllocationCallbacks* pAllocator = nullptr) {
        VkSurfaceKHR rawSurface;
        vk::Result result = static_cast<vk::Result>(
            glfwCreateWindowSurface((VkInstance)instance, window, reinterpret_cast<const VkAllocationCallbacks*>(pAllocator),
                                    &rawSurface));
        return vk::createResultValue(result, rawSurface, "vk::CommandBuffer::begin");
    }

    vk::SurfaceKHR createSurface(const vk::Instance& instance, const vk::AllocationCallbacks* pAllocator = nullptr) {
        return createWindowSurface(window, instance, pAllocator);
    }
#endif

    NativeWindow getNativeWindowHandle() {
#ifdef GLFW_EXPOSE_NATIVE_WIN32
        return glfwGetWin32Window(window);
#elif defined(GLFW_EXPOSE_NATIVE_X11)
#endif
    }

    NativeContext getNativeContextHandle() {
#ifdef GLFW_EXPOSE_NATIVE_WGL
        return glfwGetWGLContext(window);
#elif defined(GLFW_EXPOSE_NATIVE_GLX)
        return glfwGetGLXContext(window);
#endif
    }

    void swapBuffers() const { glfwSwapBuffers(window); }

    void requestClose() { glfwSetWindowShouldClose(window, GLFW_TRUE); }

    void createWindow(const glm::uvec2& size, const glm::ivec2& position = { INT_MIN, INT_MIN }) {
        // Disable window resize
        window = glfwCreateWindow(size.x, size.y, "Window Title", nullptr, nullptr);
        if (position != glm::ivec2{ INT_MIN, INT_MIN }) {
            glfwSetWindowPos(window, position.x, position.y);
        }
        glfwSetWindowUserPointer(window, this);
        glfwSetWindowCloseCallback(window, CLOSE_CALLBACK);
        glfwSetFramebufferSizeCallback(window, RESIZE_HANDLER);
        glfwSetKeyCallback(window, KEY_CALLBACK);
        glfwSetMouseButtonCallback(window, MOUSE_BUTTON_CALLBACK);
        glfwSetCursorPosCallback(window, MOUSE_MOVE_CALLBACK);
        glfwSetScrollCallback(window, MOUSE_SCROLL_CALLBACK);
    }

    void destroyWindow() {
        glfwDestroyWindow(window);
        window = nullptr;
    }

    void makeCurrent() const { glfwMakeContextCurrent(window); }

    void present() const { glfwSwapBuffers(window); }

    void showWindow(bool show = true) {
        if (show) {
            glfwShowWindow(window);
        } else {
            glfwHideWindow(window);
        }
    }

    void setSwapInterval(int swapInterval) { glfwSwapInterval(swapInterval); }

    void setTitle(const std::string& title) { glfwSetWindowTitle(window, title.c_str()); }

    void setSizeLimits(const glm::uvec2& minSize, const glm::uvec2& maxSize = {}) {
        glfwSetWindowSizeLimits(window, minSize.x, minSize.y, (maxSize.x != 0) ? maxSize.x : minSize.x,
                                (maxSize.y != 0) ? maxSize.y : minSize.y);
    }

    void runWindowLoop(const std::function<void()>& frameHandler) {
        while (0 == glfwWindowShouldClose(window)) {
            glfwPollEvents();
            frameHandler();
        }
    }

    bool shouldClose() const { return 0 != glfwWindowShouldClose(window); }

    CloseHandler closeHandler = []() {};
    ResizeHandler resizeHandler = [](const glm::uvec2&) {};
    KeyHandler keyHandler = [](int, int, int, int) {};
    MouseButtonHandler mouseButtonHandler = [](int, int, int) {};
    MouseMoveHandler mouseMoveHandler = [](glm::vec2) {};
    MouseScrollHandler mouseScrollHandler = [](float) {};

private:
    static void KEY_CALLBACK(GLFWwindow* window, int key, int scancode, int action, int mods) {
        reinterpret_cast<Window*>(glfwGetWindowUserPointer(window))->keyHandler(key, scancode, action, mods);
    }
    static void MOUSE_BUTTON_CALLBACK(GLFWwindow* window, int button, int action, int mods) {
        reinterpret_cast<Window*>(glfwGetWindowUserPointer(window))->mouseButtonHandler(button, action, mods);
    }
    static void MOUSE_MOVE_CALLBACK(GLFWwindow* window, double posx, double posy) {
        reinterpret_cast<Window*>(glfwGetWindowUserPointer(window))->mouseMoveHandler({ posx, posy });
    }
    static void MOUSE_SCROLL_CALLBACK(GLFWwindow* window, double xoffset, double yoffset) {
        reinterpret_cast<Window*>(glfwGetWindowUserPointer(window))->mouseScrollHandler((float)yoffset);
    }
    static void CLOSE_CALLBACK(GLFWwindow* window) {
        reinterpret_cast<Window*>(glfwGetWindowUserPointer(window))->closeHandler();
    }
    static void RESIZE_HANDLER(GLFWwindow* window, int width, int height) {
        reinterpret_cast<Window*>(glfwGetWindowUserPointer(window))->resizeHandler({ width, height });
    }

    GLFWwindow* window{ nullptr };
};
}  // namespace glfw
