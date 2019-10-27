#pragma once

#include <memory>
#include <functional>

#include <gl/threadedSwapchainRenderer.hpp>

namespace xr_examples { namespace imgui {

class Renderer : public xr_examples::gl::ThreadedSwapchainRenderer {
    using Parent = xr_examples::gl::ThreadedSwapchainRenderer;
    struct Private;

public:
    using Handler = std::function<void()>;
    static void init();

    void render() override;
    void initContext() override;
    void setHandler(const Handler& handler);

private:
    std::shared_ptr<Private> d;
    Handler handler;
};

}}  // namespace xr_examples::imgui

