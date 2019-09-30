#pragma once

#ifdef HAVE_QT

#include <interfaces.hpp>

namespace xr_examples { namespace qt {

class Window : public ::xr_examples::Window {
    struct Private;

public:
    static void init();
    Window();
    virtual ~Window();
    void create(const xr::Extent2Di& size) override;
    void makeCurrent() override;
    void doneCurrent() override;
    void setSwapInterval(uint32_t swapInterval) override;

    void* getNativeWindowHandle();
    void* getNativeContextHandle();

    void setTitle(const std::string& title) override;
    void runWindowLoop(const std::function<void()>& handler) override;
    void swapBuffers() override;
    void requestClose() override;
    xr::Extent2Di getSize() const override;

private:
    std::shared_ptr<Private> d;
};

}}  // namespace xr_examples::qt

#endif
