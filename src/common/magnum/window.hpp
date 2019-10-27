#pragma once

#ifdef USE_MAGNUM

#include <interfaces.hpp>
#include <memory>

namespace xr_examples { namespace magnum {

class Window : public xr_examples::Window {
    struct Private;
public:
    static void init();

    virtual ~Window();
    void create(const xr::Extent2Di& size) override;
    void makeCurrent() const override;
    void doneCurrent() const override;
    void setSwapInterval(uint32_t swapInterval) const override;
    void setTitle(const ::std::string& title) const override;
    void runWindowLoop(const ::std::function<void()>& handler) override;
    void swapBuffers() const override;
    void requestClose() override;
    xr::Extent2Di getSize() const override;
    Context::Pointer createOffscreenContext() const override;

private:
    ::std::shared_ptr<Private> d;
};

}}  // namespace xr_examples::magnum

#endif