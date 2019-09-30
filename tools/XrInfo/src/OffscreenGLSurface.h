//
//  Created by Bradley Austin Davis on 2014/04/09.
//  Copyright 2013-2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include <mutex>
#include <QObject>

class QOpenGLContext;
class QOffscreenSurface;
class QOpenGLDebugMessage;
class QSurfaceFormat;

class OffscreenGLSurface : public QObject {
public:
    OffscreenGLSurface();
    ~OffscreenGLSurface();
    void setFormat(const QSurfaceFormat& format);
    bool create(QOpenGLContext* sharedContext = nullptr);
    bool makeCurrent();
    void doneCurrent();
    void moveToThreadWithContext(QThread* thread);
    QOpenGLContext* getContext() {
        return _context;
    }
    QObject* getContextObject();

protected:
    QOpenGLContext* _context{ nullptr };
    QOffscreenSurface* _offscreenSurface{ nullptr };
};
