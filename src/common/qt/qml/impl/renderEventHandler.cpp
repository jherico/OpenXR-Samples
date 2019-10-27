//
//  Created by Bradley Austin Davis on 2018-01-04
//  Copyright 2013-2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "renderEventHandler.hpp"

#if defined(HAVE_QT)

#include <QtCore/QCoreApplication>
#include <QtQuick/QQuickWindow>
#include <QtGui/qopenglfunctions_4_5_core.h>

#include <qt/gl.hpp>
#include <qt/qml/impl/sharedObject.hpp>
#include <qt/qml/impl/renderControl.hpp>

using namespace xr_examples::qml::impl;

RenderEventHandler::RenderEventHandler(SharedObject* shared, QThread* targetThread) : _shared(shared) {
    QOpenGLContext* currentContext = QOpenGLContext::currentContext();
    auto* currentSurface = currentContext->surface();

    _context.setFormat(QSurfaceFormat::defaultFormat());
    _surface.setFormat(QSurfaceFormat::defaultFormat());
    _context.setShareContext(qt_gl_global_share_context());
    // Create the GL canvas in the same thread as the share canvas
    if (!_context.create()) {
        qFatal("Failed to create OffscreenGLCanvas context");
    }
    _surface.create();
    if (!_surface.isValid()) {
        qFatal("Offscreen surface is invalid");
    }

    _context.moveToThread(targetThread);
    moveToThread(targetThread);

    currentContext->makeCurrent(currentSurface);
}

bool RenderEventHandler::event(QEvent* e) {
    switch (static_cast<OffscreenEvent::Type>(e->type())) {
        case OffscreenEvent::Render:
            onRender();
            return true;

        case OffscreenEvent::RenderSync:
            onRenderSync();
            return true;

        case OffscreenEvent::Initialize:
            onInitalize();
            return true;

        case OffscreenEvent::Quit:
            onQuit();
            return true;

        default:
            break;
    }
    return QObject::event(e);
}

void RenderEventHandler::onInitalize() {
    if (_shared->isQuit()) {
        return;
    }

    if (_context.shareContext() != qt_gl_global_share_context()) {
        qFatal("QML rendering context has no share context");
    }

    if (!_context.makeCurrent(&_surface)) {
        qFatal("Unable to make QML rendering context current on render thread");
    }
    auto& glf = qt::getFunctions();
    glf.glCreateFramebuffers(1, &_fbo);
    glf.glCreateRenderbuffers(1, &_depthBuffer);
    glf.glNamedRenderbufferStorage(_depthBuffer, GL_DEPTH24_STENCIL8, _shared->_size.width(), _shared->_size.height());
    glf.glNamedFramebufferRenderbuffer(_fbo, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, _depthBuffer);

    _shared->_renderControl->initialize(&_context);
    _initialized = true;
}

void RenderEventHandler::onRender() {
    qmlRender(false);
}

void RenderEventHandler::onRenderSync() {
    qmlRender(true);
}

void RenderEventHandler::qmlRender(bool sceneGraphSync) {
    if (_shared->isQuit()) {
        return;
    }

    if (&_context != QOpenGLContext::currentContext()) {
        qFatal("QML rendering context not current on render thread");
    }

    if (!_shared->preRender(sceneGraphSync)) {
        return;
    }

    auto& swapchain = _shared->_swapchain;
    auto index = swapchain.acquireSwapchainImage({});
    auto& swapchainImage = _shared->_swapchainImages[index];
    swapchain.waitSwapchainImage({ xr::Duration::infinite() });

    auto& glf = qt::getFunctions();

    glf.glNamedFramebufferTexture(_fbo, GL_COLOR_ATTACHMENT0, swapchainImage.image, 0);
    glf.glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _fbo);
    glf.glClearColor(1, 0, 1, 1);
    glf.glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    _shared->_quickWindow->setRenderTarget(_fbo, _shared->_size);
    _shared->_renderControl->render();
    glf.glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glf.glFlush();
    glf.glFinish();
    swapchain.releaseSwapchainImage({});
    _shared->_quickWindow->resetOpenGLState();
    _shared->_lastRenderTime = std::chrono::high_resolution_clock::now();
}

void RenderEventHandler::onQuit() {
    if (_initialized) {
        if (&_context != QOpenGLContext::currentContext()) {
            qFatal("QML rendering context not current on render thread");
        }

        _shared->_swapchain.destroy();
        _shared->shutdownRendering();
        _context.doneCurrent();
    }
    _context.moveToThread(qApp->thread());
    moveToThread(qApp->thread());
    QThread::currentThread()->quit();
}

#endif