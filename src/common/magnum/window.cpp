
#include "window.hpp"

#ifdef USE_MAGNUM

#include <stdexcept>

#pragma warning(push)
#pragma warning(disable : 4251)
#pragma warning(disable : 4267)
#include <Magnum/Platform/Sdl2Application.h>
#include <Corrade/Utility/Arguments.h>
#pragma warning(pop)

#include <magnum/math.hpp>

extern int g_argc;
extern char** g_argv;

namespace xr_examples { namespace magnum {

struct OffscreenContext : public xr_examples::Context {
    OffscreenContext(SDL_Window* window) : window(window) {
        SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
        glContext = SDL_GL_CreateContext(window);
    }

    void makeCurrent() const override { SDL_GL_MakeCurrent(window, glContext); }
    void doneCurrent() const override { SDL_GL_MakeCurrent(window, nullptr); }
    void destroy() {
        SDL_GL_DeleteContext(glContext);
        glContext = nullptr;
    }

    SDL_Window* const window;
    SDL_GLContext glContext;
};

}}  // namespace xr_examples::magnum

using namespace xr_examples::magnum;

struct xr_examples::magnum::Window::Private : public Magnum::Platform::Application {
    using Parent = Magnum::Platform::Application;
    std::function<void()> handler{ [] {} };
    Magnum::Vector2i size;
    SDL_GLContext glContext;

    Private(Window& parent, const Magnum::Vector2i& size_) :
        Parent{ Parent::Arguments{ g_argc, g_argv }, Magnum::NoCreate }, size(size_) {
        if (!tryCreate(Configuration{}.setSize(this->size).setWindowFlags({}))) {
            throw std::runtime_error("Failed to create Window");
        }
        glContext = SDL_GL_GetCurrentContext();
    };

    ~Private() {}

    void viewportEvent(ViewportEvent& event) override { size = event.windowSize(); }

    void drawEvent() override { handler(); }

    void makeCurrent() { SDL_GL_MakeCurrent(window(), glContext); }

    void doneCurrent() { SDL_GL_MakeCurrent(window(), nullptr); }

    Context::Pointer createOffscreenContext() { return std::make_shared<OffscreenContext>(window()); }
};

void Window::init() {
}

Window::~Window() {
    d.reset();
}

xr::Extent2Di Window::getSize() const {
    return { d->size.x(), d->size.y() };
}

void xr_examples::magnum::Window::create(const xr::Extent2Di& size) {
    d = std::make_shared<Private>(*this, fromXr(size));
}

void Window::makeCurrent() const {
    d->makeCurrent();
}

void Window::doneCurrent() const {
    d->doneCurrent();
}

void Window::setSwapInterval(uint32_t swapInterval) const {
    SDL_GL_SetSwapInterval(swapInterval);
}

void Window::requestClose() {
    d->exit();
}

void Window::swapBuffers() const {
    d->swapBuffers();
    d->redraw();
}

void Window::setTitle(const std::string& title) const {
    SDL_SetWindowTitle(d->window(), title.c_str());
}

void Window::runWindowLoop(const std::function<void()>& handler) {
    d->handler = handler;
    d->redraw();
    d->exec();
}

xr_examples::Context::Pointer Window::createOffscreenContext() const {
    return d->createOffscreenContext();
}

#endif
