#include "framebuffer.hpp"

#ifdef USE_MAGNUM

#pragma warning(push)
#pragma warning(disable : 4251)
#pragma warning(disable : 4267)
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/Renderbuffer.h>
#include <Magnum/GL/RenderbufferFormat.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/GL/Framebuffer.h>
#pragma warning(pop)

#include <magnum/math.hpp>

using namespace Magnum;
using namespace xr_examples;

struct magnum::Framebuffer::Private {
    using Formats = std::vector<GL::TextureFormat>;
    const Vector2i size;
    GL::Framebuffer object;
    std::vector<GL::Texture2D> colors;
    GL::Renderbuffer depthStencil;

    Private(const Vector2i& size_, const Formats& colorFormats = Formats{ { GL::TextureFormat::RGBA8 } }) :
        size(size_), object({ { 0, 0 }, size }) {
        auto count = colorFormats.size();
        colors.resize(colorFormats.size());
        for (uint32_t i = 0; i < count; ++i) {
            auto& color = colors[i];
            color.setStorage(1, colorFormats[i], size);
            object.attachTexture(GL::Framebuffer::ColorAttachment{ i }, color, 0);
        }
        depthStencil.setStorage(GL::RenderbufferFormat::Depth24Stencil8, size);
        object.attachRenderbuffer(GL::Framebuffer::BufferAttachment::DepthStencil, depthStencil);
    }
};

magnum::Framebuffer::~Framebuffer() {
    d.reset();
}

void magnum::Framebuffer::create(const xr::Extent2Di& size) {
    this->size = size;
    this->eyeSize = size;
    eyeSize.width /= 2;
    d = std::make_shared<Private>(fromXr(size));
}

void magnum::Framebuffer::clear(const xr::Color4f& color, float depth, int stencil) {
    d->object.clear(GL::FramebufferClear::Color | GL::FramebufferClear::Depth | GL::FramebufferClear::Stencil);
}

void magnum::Framebuffer::bind(Target target) {
    if (target == Draw) {
        d->object.bind();
    } else {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, d->object.id());
    }
}

void magnum::Framebuffer::bindDefault(Target target) {
    if (target == Draw) {
        GL::defaultFramebuffer.bind();
    } else {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    }
}

void magnum::Framebuffer::setViewport(const xr::Rect2Di& viewport) {
    d->object.setViewport(fromXr(viewport));
}

uint32_t magnum::Framebuffer::id() {
    return d->object.id();
}


#endif
