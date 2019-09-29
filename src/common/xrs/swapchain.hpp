#pragma once

#define XR_USE_GRAPHICS_API_OPENGL

#include <openxr/openxr.hpp>
#if defined(XR_USE_GRAPHICS_API_OPENGL)
#pragma warning(push)
#pragma warning(disable : 4251)
#pragma warning(disable : 4267)
#include <Magnum/GL/GL.h>
#include <Magnum/GL/Framebuffer.h>
#pragma warning(pop)
#endif

namespace xrs {

#if defined(XR_USE_GRAPHICS_API_VULKAN)
using DefaultSwapchainImageType = xr::SwapchainImageVulkanKHR;
using SwapchainFormatType = vk::Format;
constexpr vk::Format DEFAULT_SWAPCHAIN_FORMAT{ vk::Format::eB8G8R8A8Srgb };
#define DEFAULT_SWAPCHAIN_FORMAT
#elif defined(XR_USE_GRAPHICS_API_OPENGL)
using DefaultSwapchainImageType = xr::SwapchainImageOpenGLKHR;
using SwapchainFormatType = uint32_t;
constexpr GLenum DEFAULT_SWAPCHAIN_FORMAT{ GL_SRGB8_ALPHA8 };
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
                const glm::uvec2& size,
                SwapchainFormatType format = DEFAULT_SWAPCHAIN_FORMAT,
                xr::SwapchainUsageFlags usageFlags = xr::SwapchainUsageFlagBits::TransferDst,
                uint32_t samples = 1,
                uint32_t arrayCount = 1,
                uint32_t faceCount = 1,
                uint32_t mipCount = 1) {
        createSwapchain(session,
                        xr::SwapchainCreateInfo{
                            {}, usageFlags, (int64_t)format, samples, size.x, size.y, faceCount, arrayCount, mipCount });
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

struct FramebufferSwapchain : public Swapchain<xr::SwapchainImageOpenGLKHR> {
private:
    using Parent = Swapchain;

public:
    void createSwapchain(const xr::Session& session, const xr::SwapchainCreateInfo& ci) override {
        Parent::createSwapchain(session, ci);
        glCreateFramebuffers(1, &object);
        glCreateRenderbuffers(1, &depthStencil);
        glBindRenderbuffer(GL_RENDERBUFFER, depthStencil);
        if (createInfo.sampleCount == 1) {
            glNamedRenderbufferStorage(depthStencil, GL_DEPTH24_STENCIL8, ci.width, ci.height);
        } else {
            glNamedRenderbufferStorageMultisample(depthStencil, ci.sampleCount, GL_DEPTH24_STENCIL8, ci.width, ci.height);
        }
        glNamedFramebufferRenderbuffer(object, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthStencil);
    }

    void destroy() override {
        glDeleteFramebuffers(1, &object);
        object = 0;

        glDeleteRenderbuffers(1, &depthStencil);
        depthStencil = 0;

        // stuff
        Parent::destroy();
    }

    xr::SwapchainImageOpenGLKHR& acquireImage() override {
        auto& result = Parent::acquireImage();
        glNamedFramebufferTexture(object, GL_COLOR_ATTACHMENT0, result.image, 0);
        return result;
    }

    void releaseImage() override {
        glNamedFramebufferTexture(object, GL_COLOR_ATTACHMENT0, 0, 0);
        Parent::releaseImage();
    }

    uint32_t object{ 0 };
    uint32_t depthStencil{ 0 };
};

}  // namespace gl

#endif

}  // namespace xrs