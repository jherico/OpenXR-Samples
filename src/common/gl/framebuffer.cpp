#include "framebuffer.hpp"

#include <glad/glad.h>

#include <logging.hpp>

namespace xr_examples { namespace gl {

void Framebuffer::create(const xr::Extent2Di& size) {
    this->size = size;
    this->eyeSize = size;
    eyeSize.width /= 2;
    glCreateRenderbuffers(1, &depthStencil);
    glNamedRenderbufferStorage(depthStencil, GL_DEPTH24_STENCIL8, size.width, size.height);
    glCreateTextures(GL_TEXTURE_2D, 1, &color);
    glTextureStorage2D(color, 1, GL_RGBA8, size.width, size.height);

    glCreateFramebuffers(1, &fbo);
    //glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
    //glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthStencil);
    //glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, color, 0);
    glNamedFramebufferRenderbuffer(fbo, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthStencil);
    glNamedFramebufferTexture(fbo, GL_COLOR_ATTACHMENT0, color, 0);
    GLenum result = glCheckNamedFramebufferStatus(fbo, GL_DRAW_FRAMEBUFFER);
    if (result != GL_FRAMEBUFFER_COMPLETE) {
        LOG_ERROR("Framebuffer incomplete at %s:%i\n", __FILE__, __LINE__);
    }
}

void Framebuffer::destroy() {
    glDeleteFramebuffers(1, &fbo);
    fbo = 0;

    glDeleteTextures(1, &color);
    color = 0;

    glDeleteRenderbuffers(1, &depthStencil);
    depthStencil = 0;
}

void Framebuffer::bind(Target target ) {
    glBindFramebuffer(target == Draw ? GL_DRAW_FRAMEBUFFER : GL_READ_FRAMEBUFFER, fbo);
}
void Framebuffer::clear() {
    glClearColor(1, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void Framebuffer::bindDefault(Target target ) {
    glBindFramebuffer(target == Draw ? GL_DRAW_FRAMEBUFFER : GL_READ_FRAMEBUFFER, 0);
}
void Framebuffer::setViewport(const xr::Rect2Di& vp) {
    glViewport(vp.offset.x, vp.offset.y, vp.extent.width, vp.extent.height);
}

}}  // namespace xr_examples::gl
