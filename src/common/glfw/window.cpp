
#include "window.hpp"

#include <stdexcept>
#include <GLFW/glfw3.h>

extern int g_argc;
extern char** g_argv;

namespace xr_examples { namespace glfw {

struct OffscreenContext : xr_examples::Context {
    OffscreenContext(GLFWwindow* parentContext) {
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        glfwWindowHint(GLFW_FOCUSED, GLFW_FALSE);
        context = glfwCreateWindow(100, 100, "", nullptr, parentContext);
    }

    void makeCurrent() const override { glfwMakeContextCurrent(context); }

    void doneCurrent() const override { glfwMakeContextCurrent(nullptr); }

    void destroy() {
        glfwDestroyWindow(context);
        context = nullptr;
    }

    GLFWwindow* context{ nullptr };
};

}}  // namespace xr_examples::glfw

using namespace xr_examples::glfw;

struct Window::Private {
    //using ResizeHandler = std::function<void(const xr::Extent2Di&)>;
    //using CloseHandler = std::function<void()>;
    //using KeyHandler = std::function<void(int, int, int, int)>;
    //using MouseButtonHandler = std::function<void(int, int, int)>;
    //using MouseMoveHandler = std::function<void(const xr::Extent2Df&)>;
    //using MouseScrollHandler = std::function<void(float)>;
    //static void KEY_CALLBACK(GLFWwindow* window, int key, int scancode, int action, int mods) {
    //    reinterpret_cast<Private*>(glfwGetWindowUserPointer(window))->keyHandler(key, scancode, action, mods);
    //}
    //static void MOUSE_BUTTON_CALLBACK(GLFWwindow* window, int button, int action, int mods) {
    //    reinterpret_cast<Private*>(glfwGetWindowUserPointer(window))->mouseButtonHandler(button, action, mods);
    //}
    //static void MOUSE_MOVE_CALLBACK(GLFWwindow* window, double posx, double posy) {
    //    reinterpret_cast<Private*>(glfwGetWindowUserPointer(window))->mouseMoveHandler({ posx, posy });
    //}
    //static void MOUSE_SCROLL_CALLBACK(GLFWwindow* window, double xoffset, double yoffset) {
    //    reinterpret_cast<Private*>(glfwGetWindowUserPointer(window))->mouseScrollHandler((float)yoffset);
    //}
    //static void CLOSE_CALLBACK(GLFWwindow* window) {
    //    reinterpret_cast<Private*>(glfwGetWindowUserPointer(window))->closeHandler();
    //}
    //static void RESIZE_HANDLER(GLFWwindow* window, int width, int height) {
    //    reinterpret_cast<Private*>(glfwGetWindowUserPointer(window))->resizeHandler({ width, height });
    //}

    Private(const xr::Extent2Di& size) {
        // Disable window resize
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
        window = glfwCreateWindow(size.width, size.height, "Window Title", nullptr, nullptr);
        glfwSetWindowSizeLimits(window, size.width, size.height, size.width, size.height);
        glfwSetWindowUserPointer(window, this);
        glfwShowWindow(window);
        //glfwSetWindowCloseCallback(window, CLOSE_CALLBACK);
        //glfwSetFramebufferSizeCallback(window, RESIZE_HANDLER);
        //glfwSetKeyCallback(window, KEY_CALLBACK);
        //glfwSetMouseButtonCallback(window, MOUSE_BUTTON_CALLBACK);
        //glfwSetCursorPosCallback(window, MOUSE_MOVE_CALLBACK);
        //glfwSetScrollCallback(window, MOUSE_SCROLL_CALLBACK);
    }

    ~Private() {
        glfwDestroyWindow(window);
        window = nullptr;
    }

    GLFWwindow* window{ nullptr };
};

void Window::init() {
    glfwInit();
}

void Window::deinit() {
    glfwTerminate();
}

Window::Window() {
}

Window::~Window() {
    d.reset();
}

xr::Extent2Di Window::getSize() const {
    xr::Extent2Di result;
    glfwGetFramebufferSize(d->window, &result.width, &result.height);
    return result;
}

void xr_examples::glfw::Window::create(const xr::Extent2Di& size) {
    d = std::make_shared<Private>(size);
}

void Window::makeCurrent() const {
    glfwMakeContextCurrent(d->window);
}

void Window::doneCurrent() const {
    glfwMakeContextCurrent(nullptr);
}

void Window::setSwapInterval(uint32_t swapInterval) const {
    glfwSwapInterval(swapInterval);
}

void Window::requestClose() {
    glfwSetWindowShouldClose(d->window, GLFW_TRUE);
}

void Window::swapBuffers() const {
    glfwSwapBuffers(d->window);
}

void Window::setTitle(const std::string& title) const {
    glfwSetWindowTitle(d->window, title.c_str());
}

xr_examples::Context::Pointer Window::createOffscreenContext() const {
    return std::make_shared<OffscreenContext>(d->window);
}

void Window::runWindowLoop(const std::function<void()>& handler) {
    while (0 == glfwWindowShouldClose(d->window)) {
        glfwPollEvents();
        handler();
    }
}
