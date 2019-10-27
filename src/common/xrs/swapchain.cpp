#include "swapchain.hpp"

#include <glad/glad.h>

void xrs::gl::FramebufferSwapchain::bind(Target target) {
    glBindFramebuffer(target == Draw ? GL_DRAW_FRAMEBUFFER : GL_READ_FRAMEBUFFER, fbo);
}

void xrs::gl::FramebufferSwapchain::bindDefault(Target target) {
    glBindFramebuffer(target == Draw ? GL_DRAW_FRAMEBUFFER : GL_READ_FRAMEBUFFER, 0);
}

void xrs::gl::FramebufferSwapchain::createFramebuffer() {
    glCreateFramebuffers(1, &fbo);
    glCreateRenderbuffers(1, &depthStencil);
    glBindRenderbuffer(GL_RENDERBUFFER, depthStencil);
    if (createInfo.sampleCount == 1) {
        glNamedRenderbufferStorage(depthStencil, GL_DEPTH24_STENCIL8, createInfo.width, createInfo.height);
    } else {
        glNamedRenderbufferStorageMultisample(depthStencil, createInfo.sampleCount, GL_DEPTH24_STENCIL8, createInfo.width, createInfo.height);
    }
    glNamedFramebufferRenderbuffer(fbo, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthStencil);
}


void xrs::gl::FramebufferSwapchain::destroyFramebuffer() {
    glDeleteFramebuffers(1, &fbo);
    fbo = 0;

    glDeleteRenderbuffers(1, &depthStencil);
    depthStencil = 0;
}

void xrs::gl::FramebufferSwapchain::createSwapchain(const xr::Session& session, const xr::SwapchainCreateInfo& ci) {
    Parent::createSwapchain(session, ci);
    createFramebuffer();
}

void xrs::gl::FramebufferSwapchain::destroy() {
    destroyFramebuffer();
    Parent::destroy();
}

::xr::SwapchainImageOpenGLKHR& xrs::gl::FramebufferSwapchain::acquireImage() {
    auto& result = Parent::acquireImage();
    glNamedFramebufferTexture(fbo, GL_COLOR_ATTACHMENT0, result.image, 0);
    return result;
}

void xrs::gl::FramebufferSwapchain::releaseImage() {
    glNamedFramebufferTexture(fbo, GL_COLOR_ATTACHMENT0, 0, 0);
    Parent::releaseImage();
}

void xrs::gl::FramebufferSwapchain::clear(const xr::Color4f& color) {
    glClearColor(color.r, color.g, color.b, color.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}
