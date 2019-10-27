//
//  Created by Bradley Austin Davis on 2018-01-04
//  Copyright 2013-2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "sharedObject.hpp"

#if defined(HAVE_QT)

#include <QtCore/qlogging.h>
#include <QtCore/QTimer>
#include <QtCore/QPointer>

#include <QtGui/QOpenGLContext>

#include <QtQuick/QQuickWindow>
#include <QtQuick/QQuickItem>

#include <QtQml/QQmlContext>
#include <QtQml/QQmlEngine>

#include "renderControl.hpp"
#include "renderEventHandler.hpp"
#include "../offscreenSurface.hpp"

// Time between receiving a request to render the offscreen UI actually triggering
// the render.  Could possibly be increased depending on the framerate we expect to
// achieve.
// This has the effect of capping the framerate at 200
static const int MIN_TIMER_MS = 5;

using namespace xr_examples::qml;
using namespace xr_examples::qml::impl;

extern QOpenGLContext* qt_gl_global_share_context();

//xr::Session g_sharedSession;
//void SharedObject::setSharedSession(const xr::Session& session) {
//    g_sharedSession = session;
//}
//
//const xr::Session& SharedObject::getSharedSession() {
//    return g_sharedSession;
//}

SharedObject::SharedObject() {
    // Create render control
    _renderControl = new RenderControl();

    // Create a QQuickWindow that is associated with our render control.
    // This window never gets created or shown, meaning that it will never get an underlying native (platform) window.
    // NOTE: Must be created on the main thread so that OffscreenQmlSurface can send it events
    // NOTE: Must be created on the rendering thread or it will refuse to render,
    //       so we wait until after its ctor to move object/context to this thread.
    QQuickWindow::setDefaultAlphaBuffer(true);
    _quickWindow = new QQuickWindow(_renderControl);
    _quickWindow->setFormat(QSurfaceFormat::defaultFormat());
    _quickWindow->setColor(QColor(255, 255, 255, 0));
    _quickWindow->setClearBeforeRendering(true);

    QObject::connect(qApp, &QCoreApplication::aboutToQuit, this, &SharedObject::onAboutToQuit);
}

SharedObject::~SharedObject() {
    // After destroy returns, the rendering thread should be gone
    destroy();

    // _renderTimer is created with `this` as the parent, so need no explicit destruction
    // Destroy the event hand
    if (_renderObject) {
        delete _renderObject;
        _renderObject = nullptr;
    }

    if (_renderControl) {
        delete _renderControl;
        _renderControl = nullptr;
    }

    // already deleted objects will be reset to null by QPointer so it should be safe just iterate here
    for (auto qmlObject : _deletionList) {
        if (qmlObject) {
            // manually delete not-deleted-yet qml items
            QQmlEngine::setObjectOwnership(qmlObject, QQmlEngine::CppOwnership);
            delete qmlObject;
        }
    }

    if (_rootItem) {
        delete _rootItem;
        _rootItem = nullptr;
    }

    if (_quickWindow) {
        _quickWindow->destroy();
        delete _quickWindow;
        _quickWindow = nullptr;
    }

    if (_qmlContext) {
        auto engine = _qmlContext->engine();
        delete _qmlContext;
        _qmlContext = nullptr;
        releaseEngine(engine);
    }
}

void SharedObject::create(OffscreenSurface* surface) {
    if (_rootItem) {
        qFatal("QML surface root item already set");
    }

    //QObject::connect(_quickWindow, &QQuickWindow::focusObjectChanged, surface, &OffscreenSurface::onFocusObjectChanged);

    // Create a QML engine.
    auto qmlEngine = acquireEngine(surface);
    _qmlContext = new QQmlContext(qmlEngine->rootContext(), qmlEngine);
    emit surface->rootContextCreated(_qmlContext);
    if (!qmlEngine->incubationController()) {
        qmlEngine->setIncubationController(_quickWindow->incubationController());
    }
    _qmlContext->setContextProperty("offscreenWindow", QVariant::fromValue(_quickWindow));
}

void SharedObject::setRootItem(QQuickItem* rootItem) {
    if (_quit) {
        return;
    }

    _rootItem = rootItem;
    _rootItem->setSize(_quickWindow->size());

    // Create the render thread
    _renderThread = new QThread();
    _renderThread->setObjectName(objectName());
    _renderThread->start();

    // Create event handler for the render thread
    _renderObject = new RenderEventHandler(this, _renderThread);
    QCoreApplication::postEvent(this, new OffscreenEvent(OffscreenEvent::Initialize));

    QObject::connect(_renderControl, &QQuickRenderControl::renderRequested, this, &SharedObject::requestRender);
    QObject::connect(_renderControl, &QQuickRenderControl::sceneChanged, this, &SharedObject::requestRenderSync);
}

void SharedObject::destroy() {
    // `destroy` is idempotent, it can be called multiple times without issues
    if (_quit) {
        return;
    }

    if (!_rootItem) {
        return;
    }

    _paused = true;
    if (_renderTimer) {
        _renderTimer->stop();
        QObject::disconnect(_renderTimer);
    }

    if (_renderControl) {
        QObject::disconnect(_renderControl);
    }

    QObject::disconnect(qApp);

    {
        QMutexLocker lock(&_mutex);
        _quit = true;
        if (_renderObject) {
            QCoreApplication::postEvent(_renderObject, new OffscreenEvent(OffscreenEvent::Quit), Qt::HighEventPriority);
        }
    }
    // Block until the rendering thread has stopped
    // FIXME this is undesirable because this is blocking the main thread,
    // but I haven't found a reliable way to do this only at application
    // shutdown
    if (_renderThread) {
        _renderThread->wait();
        delete _renderThread;
        _renderThread = nullptr;
    }
}

#define SINGLE_QML_ENGINE 0

#if SINGLE_QML_ENGINE
static QQmlEngine* globalEngine{ nullptr };
static size_t globalEngineRefCount{ 0 };
#endif

QQmlEngine* SharedObject::acquireEngine(OffscreenSurface* surface) {
    Q_ASSERT(QThread::currentThread() == qApp->thread());

    QQmlEngine* result = nullptr;
    if (QThread::currentThread() != qApp->thread()) {
        qWarning() << "Cannot acquire QML engine on any thread but the main thread";
    }

#if SINGLE_QML_ENGINE
    if (!globalEngine) {
        Q_ASSERT(0 == globalEngineRefCount);
        globalEngine = new QQmlEngine();
        surface->initializeEngine(result);
    }
    ++globalEngineRefCount;
    result = globalEngine;
#else
    result = new QQmlEngine();
    surface->initializeEngine(result);
#endif

    return result;
}

void SharedObject::releaseEngine(QQmlEngine* engine) {
    Q_ASSERT(QThread::currentThread() == qApp->thread());
#if SINGLE_QML_ENGINE
    Q_ASSERT(0 != globalEngineRefCount);
    if (0 == --globalEngineRefCount) {
        globalEngine->deleteLater();
        globalEngine = nullptr;
    }
#else
    engine->deleteLater();
#endif
}

bool SharedObject::event(QEvent* e) {
    switch (static_cast<OffscreenEvent::Type>(e->type())) {
        case OffscreenEvent::Initialize:
            onInitialize();
            return true;

        case OffscreenEvent::Render:
            onRender();
            return true;

        default:
            break;
    }
    return QObject::event(e);
}

void SharedObject::setSwapchain(const xr::Swapchain& swapchain) {
    _swapchain = swapchain;
    _swapchainImages = _swapchain.enumerateSwapchainImages<xr::SwapchainImageOpenGLKHR>();
}

QSize SharedObject::getSize() const {
    QMutexLocker locker(&_mutex);
    return _size;
}

void SharedObject::setSize(const QSize& size) {
    if (getSize() == size) {
        return;
    }

    {
        QMutexLocker locker(&_mutex);
        _size = size;
    }

    qDebug() << "Offscreen UI resizing to " << size.width() << "x" << size.height();
    _quickWindow->setGeometry(QRect(QPoint(), size));
    _quickWindow->contentItem()->setSize(size);

    if (_rootItem) {
        _qmlContext->setContextProperty("surfaceSize", size);
        _rootItem->setSize(size);
    }

    requestRenderSync();
}

void SharedObject::setMaxFps(uint8_t maxFps) {
    QMutexLocker locker(&_mutex);
    _maxFps = maxFps;
}

bool SharedObject::preRender(bool sceneGraphSync) {
    QMutexLocker lock(&_mutex);
    if (_paused) {
        if (sceneGraphSync) {
            wake();
        }
        return false;
    }

    if (sceneGraphSync) {
        bool syncResult = _renderControl->sync();
        wake();
        if (!syncResult) {
            return false;
        }
    }

    return true;
}

void SharedObject::shutdownRendering() {
    QMutexLocker locker(&_mutex);
    _renderControl->invalidate();
    wake();
}

bool SharedObject::isQuit() const {
    QMutexLocker locker(&_mutex);
    return _quit;
}

void SharedObject::requestRender() {
    if (_quit) {
        return;
    }
    _renderRequested = true;
}

void SharedObject::requestRenderSync() {
    if (_quit) {
        return;
    }
    _renderRequested = true;
    _syncRequested = true;
}

void SharedObject::addToDeletionList(QObject* object) {
    _deletionList.append(QPointer<QObject>(object));
}

void SharedObject::setProxyWindow(QWindow* window) {
    _proxyWindow = window;
    _renderControl->setRenderWindow(window);
}

void SharedObject::wait() {
    _cond.wait(&_mutex);
}

void SharedObject::wake() {
    _cond.wakeOne();
}

void SharedObject::onInitialize() {
    // Associate root item with the window.
    _rootItem->setParentItem(_quickWindow->contentItem());
    _renderControl->prepareThread(_renderThread);

    // Set up the render thread
    QCoreApplication::postEvent(_renderObject, new OffscreenEvent(OffscreenEvent::Initialize));

    requestRender();

    // Set up timer to trigger renders
    _renderTimer = new QTimer(this);
    QObject::connect(_renderTimer, &QTimer::timeout, this, &SharedObject::onTimer);

    _renderTimer->setTimerType(Qt::PreciseTimer);
    _renderTimer->setInterval(MIN_TIMER_MS);  // 5ms, Qt::PreciseTimer required
    _renderTimer->start();
}

void SharedObject::onRender() {
    if (_quit) {
        return;
    }

    if (_syncRequested) {
        _renderControl->polishItems();
        QMutexLocker lock(&_mutex);
        QCoreApplication::postEvent(_renderObject, new OffscreenEvent(OffscreenEvent::RenderSync));
        // sync and render request, main and render threads must be synchronized
        wait();
        _syncRequested = false;
    } else {
        QCoreApplication::postEvent(_renderObject, new OffscreenEvent(OffscreenEvent::Render));
    }
    _renderRequested = false;
}

void SharedObject::onTimer() {
    if (!_renderRequested) {
        return;
    }

    {
        //QMutexLocker locker(&_mutex);
        //if (!_maxFps) {
        //    return;
        //}
        //auto minRenderInterval = USECS_PER_SECOND / _maxFps;
        //auto lastInterval = usecTimestampNow() - _lastRenderTime;
        //// Don't exceed the framerate limit
        //if (lastInterval < minRenderInterval) {
        //    return;
        //}
    }

    QCoreApplication::postEvent(this, new OffscreenEvent(OffscreenEvent::Render));
}

void SharedObject::onAboutToQuit() {
    destroy();
}

void SharedObject::pause() {
    _paused = true;
}

void SharedObject::resume() {
    _paused = false;
    requestRender();
}

bool SharedObject::isPaused() const {
    return _paused;
}

#endif