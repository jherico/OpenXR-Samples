#pragma once

#include <interfaces.hpp>
#include <gl/framebuffer.hpp>
#include <mutex>

namespace xr_examples { namespace gl {

class ThreadedSwapchainRenderer {
public:
    void requestNewFrame();

    virtual void render() = 0;
    virtual void initContext() = 0;

    virtual void create(const xr::Extent2Di& size, const xr::Session& session, Window& window);
    const xr::Swapchain& getSwapchain() const;

private:
    using Mutex = std::mutex;
    using Lock = std::unique_lock<Mutex>;

    SwapchainFramebuffer framebuffer;
    std::condition_variable conditional;
    std::mutex mutex;
    std::unique_ptr<std::thread> thread;
    bool frameRequested{ false };
    bool quit{ false };
    void run();
};

}}  // namespace xr_examples::gl