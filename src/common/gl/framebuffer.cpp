#define XR_USE_GRAPHICS_API_OPENGL
#include "framebuffer.hpp"

#include <glad/glad.h>

#include <logging.hpp>

namespace xr_examples { namespace gl {

void FramebufferBase::create(const xr::Extent2Di& size) {
    this->size = size;
    this->eyeSize = size;
    eyeSize.width /= 2;
    glCreateRenderbuffers(1, &depthStencil);
    glNamedRenderbufferStorage(depthStencil, GL_DEPTH24_STENCIL8, size.width, size.height);
    glCreateFramebuffers(1, &fbo);
    glNamedFramebufferRenderbuffer(fbo, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthStencil);
}

void FramebufferBase::checkStatus(Target target) {
    GLenum result = glCheckNamedFramebufferStatus(fbo, GL_DRAW_FRAMEBUFFER);
    if (result != GL_FRAMEBUFFER_COMPLETE) {
        LOG_ERROR("Framebuffer incomplete at %s:%i\n", __FILE__, __LINE__);
    }
}

void FramebufferBase::destroy() {
    glDeleteFramebuffers(1, &fbo);
    fbo = 0;

    glDeleteRenderbuffers(1, &depthStencil);
    depthStencil = 0;
}

void FramebufferBase::clear(const xr::Color4f& color, float depth, int stencil) {
    glClearColor(color.r, color.g, color.b, color.a);
    glClearDepth(depth);
    glClearStencil(stencil);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void FramebufferBase::bindDefault(Target target) {
    glBindFramebuffer(target == Draw ? GL_DRAW_FRAMEBUFFER : GL_READ_FRAMEBUFFER, 0);
}

void FramebufferBase::setViewport(const xr::Rect2Di& vp) {
    glViewport(vp.offset.x, vp.offset.y, vp.extent.width, vp.extent.height);
}

void FramebufferBase::bind(Target target) {
    glBindFramebuffer(target == Draw ? GL_DRAW_FRAMEBUFFER : GL_READ_FRAMEBUFFER, fbo);
}

void Framebuffer::create(const xr::Extent2Di& size) {
    Parent::create(size);
    glCreateTextures(GL_TEXTURE_2D, 1, &color);
    glTextureStorage2D(color, 1, GL_RGBA8, size.width, size.height);
    glNamedFramebufferTexture(fbo, GL_COLOR_ATTACHMENT0, color, 0);
}

void Framebuffer::destroy() {
    glDeleteTextures(1, &color);
    color = 0;
    Parent::destroy();
}

void SwapchainFramebuffer::setSwapchain(const xr::Swapchain& swapchain) {
    this->swapchain = swapchain;
    this->images = swapchain.enumerateSwapchainImages<xr::SwapchainImageOpenGLKHR>();
}

void SwapchainFramebuffer::setDepthSwapchain(const xr::Swapchain& swapchain) {
    this->depthSwapchain = swapchain;
    this->depthImages = swapchain.enumerateSwapchainImages<xr::SwapchainImageOpenGLKHR>();
}

void SwapchainFramebuffer::bind(Target target) {
    if (!valid) {
        {
            auto swapchainIndex = swapchain.acquireSwapchainImage({});
            const auto& image = images[swapchainIndex];
            swapchain.waitSwapchainImage({ xr::Duration::infinite() });
            glNamedFramebufferTexture(fbo, GL_COLOR_ATTACHMENT0, image.image, 0);
        }
        if (depthSwapchain) {
            auto depthSwapchainIndex = depthSwapchain.acquireSwapchainImage({});
            const auto& depthImage = depthImages[depthSwapchainIndex];
            depthSwapchain.waitSwapchainImage({ xr::Duration::infinite() });
            glNamedFramebufferTexture(fbo, GL_DEPTH_STENCIL_ATTACHMENT, depthImage.image, 0);
        }
        valid = true;
    }
    Parent::bind(target);
}

void SwapchainFramebuffer::advance() {
    glNamedFramebufferTexture(fbo, GL_COLOR_ATTACHMENT0, 0, 0);
    if (depthSwapchain) {
        glNamedFramebufferTexture(fbo, GL_DEPTH_STENCIL_ATTACHMENT, 0, 0);
        depthSwapchain.releaseSwapchainImage({});
    }
    swapchain.releaseSwapchainImage({});
    valid = false;
}

}}  // namespace xr_examples::gl
