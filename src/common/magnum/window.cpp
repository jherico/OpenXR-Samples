
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

using namespace xr_examples::magnum;

struct xr_examples::magnum::Window::Private : public Magnum::Platform::Application {
    using Parent = Magnum::Platform::Application;
    std::function<void()> handler{ [] {} };
    Magnum::Vector2i size;

    Private(Window& parent, const Magnum::Vector2i& size_) :
        Parent{ Parent::Arguments{ g_argc, g_argv }, Magnum::NoCreate }, size(size_) {
        if (!tryCreate(Configuration{}.setSize(this->size).setWindowFlags({}))) {
            throw std::runtime_error("Failed to create Window");
        }
    };

    ~Private() {}

    void viewportEvent(ViewportEvent& event) override { size = event.windowSize(); }

    void drawEvent() override { handler(); }
};

void Window::init() {
}

Window::Window() {
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

void Window::makeCurrent() {
}

void Window::doneCurrent() {
}

void Window::setSwapInterval(uint32_t swapInterval) {
}

void Window::requestClose() {
	d->exit();
}

void Window::swapBuffers() {
    d->swapBuffers();
    d->redraw();
}

void Window::setTitle(const std::string& title) {
}

void Window::runWindowLoop(const std::function<void()>& handler) {
    d->handler = handler;
    d->redraw();
    d->exec();
}

#endif
