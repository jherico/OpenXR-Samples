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

void RenderEventHandler::onInitalize() {
    if (_shared->isQuit()) {
        return;
    }

    if (!_context.makeCurrent(&_surface)) {
        qFatal("Unable to make QML rendering context current on render thread");
    }
    _shared->initializeRenderControl(&_context);
    _initialized = true;
}

void RenderEventHandler::resize() {
    auto targetSize = _shared->getSize();
    if (_currentSize != targetSize) {
		_shared->_swapchain.destroy();
        _currentSize = targetSize;
	    _shared->_swapchain.create(SharedObject::getSharedSession(), { _currentSize.width(), _currentSize.height() });
    }
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

    resize();

    auto swapchainImage = _shared->_swapchain.acquireImage();
	_shared->_swapchain.waitImage();
	auto& glf = qt::getFunctions();
	glf.glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _shared->_swapchain.fbo);
	
	glClearColor(1, 0, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    _shared->setRenderTarget(_shared->_swapchain.fbo, _currentSize);
    _shared->_renderControl->render();
    glf.glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glf.glFlush();
	glf.glFinish();
	_shared->_swapchain.releaseImage();
	_shared->_quickWindow->resetOpenGLState();
    _shared->_lastRenderTime = std::chrono::high_resolution_clock::now();
}

void RenderEventHandler::onQuit() {
    if (_initialized) {
        if (&_context != QOpenGLContext::currentContext()) {
            qFatal("QML rendering context not current on render thread");
        }

		_shared->_swapchain.destroy();
        _shared->shutdownRendering(_currentSize);
		_context.doneCurrent();
    }
    _context.moveToThread(qApp->thread());
    moveToThread(qApp->thread());
    QThread::currentThread()->quit();
}

#endif