//
//  Created by Bradley Austin Davis on 2019/09/29
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "framebuffer.hpp"

#ifdef HAVE_QT

#include <mutex>

#include <QtCore/QProcessEnvironment>
#include <QtCore/QDebug>
#include <QtCore/QThread>
#include <QtCore/QThreadStorage>
#include <QtCore/QPointer>

#include <QtGui/QOffscreenSurface>
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLDebugLogger>
#include <QtGui/QSurfaceFormat>
#include <QtGui/QOpenGLDebugLogger>
#include <QtGui/qopenglfunctions_4_5_core.h>

#include <gl/framebuffer.hpp>

#include <qt/math.hpp>
#include <qt/gl.hpp>


namespace xr_examples { namespace qt {

QObject* g_sceneSurface;
QOpenGLContext* g_sceneContext;

class SizedOffscreenSurface : public QOffscreenSurface {
public:
    void resize(const QSize& newSize) { _size = newSize; }

    QSize _size;
    QSize size() const override { return _size; }
};

struct Framebuffer::Private {
    QOpenGLContext* parentContext{ nullptr };
    QSurface* parentSurface{ nullptr };

    QOpenGLContext* context{ nullptr };
    SizedOffscreenSurface* offscreenSurface{ nullptr };
    gl::Framebuffer windowFramebuffer;
    uint32_t offscreenFbo{ 0 };
    uint32_t offscreenDepthStencil{ 0 };

    bool makeCurrent() { return context->makeCurrent(offscreenSurface); }

    bool makeParentCurrent() { return parentContext->makeCurrent(parentSurface); }

    Private(const xr::Extent2Di& size) {
        parentContext = QOpenGLContext::currentContext();
        parentSurface = parentContext->surface();
        windowFramebuffer.create(size);

        context = new QOpenGLContext();

        context->setFormat(QSurfaceFormat::defaultFormat());
        offscreenSurface = new SizedOffscreenSurface();
        offscreenSurface->setFormat(QSurfaceFormat::defaultFormat());
        offscreenSurface->resize(fromXr(size));
        auto shareContext = qt_gl_global_share_context();
        if (shareContext) {
            context->setShareContext(shareContext);
        }
        if (!context->create()) {
            qFatal("Failed to create OffscreenSurface context");
        }
        if (!shareContext) {
            qt_gl_set_global_share_context(context);
        }
        offscreenSurface->create();
        if (!offscreenSurface->isValid()) {
            qFatal("Offscreen surface is invalid");
        }
        if (!makeCurrent()) {
            qFatal("Offscreen surface can't be made current");
        }

        auto& glf = getFunctions();
        glf.glCreateRenderbuffers(1, &offscreenDepthStencil);
        glf.glNamedRenderbufferStorage(offscreenDepthStencil, GL_DEPTH24_STENCIL8, size.width, size.height);

        glf.glCreateFramebuffers(1, &offscreenFbo);
        glf.glNamedFramebufferRenderbuffer(offscreenFbo, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, offscreenDepthStencil);
        glf.glNamedFramebufferTexture(offscreenFbo, GL_COLOR_ATTACHMENT0, windowFramebuffer.color, 0);

        if (!makeParentCurrent()) {
            qFatal("Offscreen surface can't restore old context");
        }


        g_sceneContext = context;
        g_sceneSurface = offscreenSurface;
    }

    ~Private() {
        context->doneCurrent();
        context->deleteLater();
        offscreenSurface->destroy();
        offscreenSurface->deleteLater();
    }
};

void Framebuffer::create(const xr::Extent2Di& size) {
    this->size = size;
    this->eyeSize = size;
    eyeSize.width /= 2;
    d = std::make_shared<Private>(size);
}

void Framebuffer::bind(Target target) {
    d->makeCurrent();
    auto& glf = getFunctions();
    auto gltarget = target == Draw ? GL_DRAW_FRAMEBUFFER : GL_READ_FRAMEBUFFER;
    glf.glBindFramebuffer(gltarget, d->offscreenFbo);
}
void Framebuffer::clear(const xr::Color4f& color, float depth, int stencil) {
    auto& glf = getFunctions();
    glf.glClearColor(color.r, color.g, color.b, color.a);
    glf.glClearDepth(depth);
    glf.glClearStencil(stencil);
    glf.glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void Framebuffer::bindDefault(Target target) {
    auto& glf = getFunctions();
    glf.glFlush();
    glf.glFinish();

    d->makeParentCurrent();
    auto gltarget = target == Draw ? GL_DRAW_FRAMEBUFFER : GL_READ_FRAMEBUFFER;
    glf.glBindFramebuffer(gltarget, d->windowFramebuffer.fbo);
}
void Framebuffer::setViewport(const xr::Rect2Di& vp) {
    auto& glf = getFunctions();
    glf.glViewport(vp.offset.x, vp.offset.y, vp.extent.width, vp.extent.height);
}

uint32_t Framebuffer::id() {
    return d->windowFramebuffer.id();
}

}}  // namespace xr_examples::qt
#endif
