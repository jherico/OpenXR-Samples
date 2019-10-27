//
//  Created by Bradley Austin Davis on 2014/04/09.
//  Copyright 2013-2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "OffscreenGLSurface.h"

#include <QtCore/QProcessEnvironment>
#include <QtCore/QDebug>
#include <QtCore/QThread>
#include <QtCore/QThreadStorage>
#include <QtCore/QPointer>
#include <QtGui/QOffscreenSurface>
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLDebugLogger>

OffscreenGLSurface::OffscreenGLSurface() : _context(new QOpenGLContext), _offscreenSurface(new QOffscreenSurface) {
}

OffscreenGLSurface::~OffscreenGLSurface() {
    // A context with logging enabled needs to be current when it's destroyed
    _context->makeCurrent(_offscreenSurface);
    delete _context;
    _context = nullptr;

    _offscreenSurface->destroy();
    delete _offscreenSurface;
    _offscreenSurface = nullptr;
}

void OffscreenGLSurface::setFormat(const QSurfaceFormat& format) {
    _context->setFormat(format);
}

bool OffscreenGLSurface::create(QOpenGLContext* sharedContext) {
    if (nullptr != sharedContext) {
        sharedContext->doneCurrent();
        _context->setShareContext(sharedContext);
    }
    if (!_context->create()) {
        qFatal("Failed to create OffscreenGLSurface context");
    }
    _offscreenSurface->setFormat(_context->format());
    _offscreenSurface->create();
    if (!_offscreenSurface->isValid()) {
        qFatal("Offscreen surface is invalid");
    }
    return true;
}

bool OffscreenGLSurface::makeCurrent() {
    return _context->makeCurrent(_offscreenSurface);
}

void OffscreenGLSurface::doneCurrent() {
    _context->doneCurrent();
}

QObject* OffscreenGLSurface::getContextObject() {
    return _context;
}

void OffscreenGLSurface::moveToThreadWithContext(QThread* thread) {
    moveToThread(thread);
    _context->moveToThread(thread);
}

