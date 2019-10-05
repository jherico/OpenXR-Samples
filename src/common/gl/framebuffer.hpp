#pragma once

#include <interfaces.hpp>

namespace xr_examples { namespace gl {

class FramebufferBase : public xr_examples::Framebuffer {
public:
    void create(const xr::Extent2Di& size) override;
    void destroy() override;

    void clear(const xr::Color4f& color = { 0, 0, 0, 1 }, float depth = 1.0f, int stencil = 0) override;

    void bindDefault(Target target = Draw) override;
    void setViewport(const xr::Rect2Di& vp) override;
    void checkStatus(Target target = Draw);
    void bind(Target target = Draw) override;

    inline uint32_t id() override { return fbo; }

    uint32_t fbo{ 0 };
    uint32_t depthStencil{ 0 };
};

class Framebuffer : public FramebufferBase {
    using Parent = FramebufferBase;

public:
    void create(const xr::Extent2Di& size) override;
    void destroy() override;

    uint32_t color{ 0 };
};

class SwapchainFramebuffer : public FramebufferBase {
    using Parent = FramebufferBase;

public:
    void setSwapchain(const xr::Swapchain& swapchain);
    void setDepthSwapchain(const xr::Swapchain& swapchain);
    void bind(Target target = Draw) override;
    void advance();

    xr::Swapchain swapchain;
    std::vector<xr::SwapchainImageOpenGLKHR> images;
    xr::Swapchain depthSwapchain;
    std::vector<xr::SwapchainImageOpenGLKHR> depthImages;
    bool valid{ false };
};

}}  // namespace xr_examples::gl
