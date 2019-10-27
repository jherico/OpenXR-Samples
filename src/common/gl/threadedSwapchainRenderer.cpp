#include "threadedSwapchainRenderer.hpp"

#include <gl/debug.hpp>

#include <glad/glad.h>

using namespace xr_examples;
using namespace xr_examples::gl;

void ThreadedSwapchainRenderer::create(const xr::Extent2Di& size, const xr::Session& session, Window& window) {
    auto swapchain = session.createSwapchain(xr::SwapchainCreateInfo{
        {}, xr::SwapchainUsageFlagBits::TransferDst, GL_RGBA8, 1, (uint32_t)size.width, (uint32_t)size.height, 1, 1, 1 });
    framebuffer.setSwapchain(swapchain);

    auto glContext = window.createOffscreenContext();
    glContext->makeCurrent();
    gl::enableDebugLogging();
    // Framebuffers are not shared between contexts, so we have to create ours AFTER creating and making the offscreen context current
    framebuffer.create(size);
    // Always make sure the first entry in the swapchain is valid for submission in a layer BEFORE we return on the main thread
    {
        framebuffer.bind();
        framebuffer.clear();
        framebuffer.bindDefault();
        framebuffer.advance();
    }
    // Let derived class do any one-time GL context setup required
    initContext();
    // Release offscreen context on the main thread
    glContext->doneCurrent();

    // Launch the rendering thread.  The offscreen context will remain current on that thread for the duration
    thread = std::make_unique<std::thread>([=] {
        glContext->makeCurrent();
        run();
        glContext->doneCurrent();
        glContext->destroy();
    });

    window.makeCurrent();
}

void ThreadedSwapchainRenderer::requestNewFrame() {
    Lock lock{ mutex };
    frameRequested = true;
    conditional.notify_one();
}

void ThreadedSwapchainRenderer::run() {
    using namespace std::chrono_literals;
    while (!quit) {
        Lock lock(mutex);
        if (!conditional.wait_for(lock, 100ms, [this] { return frameRequested; })) {
            continue;
        }
        frameRequested = false;
        framebuffer.bind();
        render();
        framebuffer.bindDefault();
        framebuffer.advance();
    }
}

const xr::Swapchain& ThreadedSwapchainRenderer::getSwapchain() const {
    return framebuffer.swapchain;
}