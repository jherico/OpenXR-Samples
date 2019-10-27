#pragma once

#define XR_USE_GRAPHICS_API_OPENGL

#include <openxr/openxr.hpp>

namespace xrs {

#if defined(XR_USE_GRAPHICS_API_VULKAN)
using DefaultSwapchainImageType = xr::SwapchainImageVulkanKHR;
using SwapchainFormatType = vk::Format;
constexpr vk::Format DEFAULT_SWAPCHAIN_FORMAT{ vk::Format::eB8G8R8A8Srgb };
constexpr vk::Format DEFAULT_SWAPCHAIN_DEPTH_FORMAT{ vk::Format::eB8G8R8A8Srgb };
#elif defined(XR_USE_GRAPHICS_API_OPENGL)
using DefaultSwapchainImageType = xr::SwapchainImageOpenGLKHR;
using SwapchainFormatType = uint32_t;
// GL_RGBA8
constexpr uint32_t DEFAULT_SWAPCHAIN_FORMAT{ 0x8C43 };
// GL_DEPTH24_STENCIL8
constexpr uint32_t DEFAULT_SWAPCHAIN_DEPTH_FORMAT{ 0x88F0 };
#endif

template <typename SwapchainImageType = DefaultSwapchainImageType>
struct Swapchain {
    static constexpr uint32_t INVALID_SWAPCHAIN_INDEX = (uint32_t)-1;
    xr::SwapchainCreateInfo createInfo;
    xr::Swapchain swapchain;
    std::vector<SwapchainImageType> images;
    uint32_t currentIndex{ INVALID_SWAPCHAIN_INDEX };

    virtual void createSwapchain(const xr::Session& session, const xr::SwapchainCreateInfo& createInfo_) {
        createInfo = createInfo_;
        // Explicitly abandon the chain
        createInfo.next = nullptr;
        swapchain = session.createSwapchain(createInfo_);
        images = swapchain.enumerateSwapchainImages<SwapchainImageType>();
    }

    void create(const xr::Session& session,
                const xr::Extent2Di& size,
                SwapchainFormatType format = DEFAULT_SWAPCHAIN_FORMAT,
                xr::SwapchainUsageFlags usageFlags = xr::SwapchainUsageFlagBits::TransferDst,
                uint32_t samples = 1,
                uint32_t arrayCount = 1,
                uint32_t faceCount = 1,
                uint32_t mipCount = 1) {
        xr::SwapchainCreateInfo ci;
        ci.usageFlags = usageFlags;
        ci.format = (int64_t)format;
        ci.sampleCount = samples;
        ci.width = size.width;
        ci.height = size.height;
        ci.faceCount = faceCount;
        ci.arraySize = arrayCount;
        ci.mipCount = mipCount;
        createSwapchain(session, ci);
    }

    virtual void destroy() {
        if (swapchain) {
            swapchain.destroy();
            swapchain = nullptr;
        }
        images.clear();
    }

    virtual SwapchainImageType& acquireImage() {
        if (currentIndex != INVALID_SWAPCHAIN_INDEX) {
            throw std::runtime_error("Acquire called without release");
        }
        swapchain.acquireSwapchainImage(xr::SwapchainImageAcquireInfo{}, &currentIndex);
        return images[currentIndex];
    }

    virtual void waitImage(const xr::Duration& duration = xr::Duration::infinite()) {
        swapchain.waitSwapchainImage(xr::SwapchainImageWaitInfo{ duration });
    }

    virtual void releaseImage() {
        if (currentIndex == INVALID_SWAPCHAIN_INDEX) {
            throw std::runtime_error("Releasing image when none is acquired");
        }
        swapchain.releaseSwapchainImage(xr::SwapchainImageReleaseInfo{});
        currentIndex = INVALID_SWAPCHAIN_INDEX;
    }
};

#if defined(XR_USE_GRAPHICS_API_OPENGL)

namespace gl {

struct FramebufferSwapchain : public ::xrs::Swapchain<::xr::SwapchainImageOpenGLKHR> {
private:
    using Parent = xrs::Swapchain<xr::SwapchainImageOpenGLKHR>;

public:
    enum Target
    {
        Draw = 1,
        Read = 2,
    };

    void createSwapchain(const xr::Session& session, const xr::SwapchainCreateInfo& ci) override;
    void destroy() override;
    ::xr::SwapchainImageOpenGLKHR& acquireImage() override;
    void releaseImage() override;
    void bind(Target target = Draw);
    void clear(const xr::Color4f& color = { 0, 0, 0, 1 });
    static void bindDefault(Target target = Draw);
    void createFramebuffer();
    void destroyFramebuffer();

    uint32_t fbo{ 0 };
    uint32_t depthStencil{ 0 };
};

}  // namespace gl

#endif

}  // namespace xrs