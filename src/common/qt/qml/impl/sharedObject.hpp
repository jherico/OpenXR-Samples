//
//  Created by Bradley Austin Davis on 2018-01-04
//  Copyright 2013-2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#if defined(HAVE_QT)

#include <QtCore/QObject>
#include <QtCore/QSharedPointer>
#include <QtCore/QThread>
#include <QtCore/QWaitCondition>
#include <QtCore/QMutex>
#include <QtCore/QSize>

#include <xrs/swapchain.hpp>

namespace xr {
class Session;
}

class QWindow;
class QTimer;
class QQuickWindow;
class QQuickItem;
class QOpenGLContext;
class QQmlEngine;
class QQmlContext;

namespace xr_examples { namespace qml {

class OffscreenSurface;

namespace impl {

class RenderControl;
class RenderEventHandler;

class SharedObject : public QObject {
    Q_OBJECT
    friend class RenderEventHandler;
    using time_point = std::chrono::time_point<std::chrono::steady_clock>;

public:
    SharedObject();
    virtual ~SharedObject();

    void create(OffscreenSurface* surface);
    void setRootItem(QQuickItem* rootItem);
    void destroy();
    bool isQuit() const;

    QSize getSize() const;
    void setSwapchain(const xr::Swapchain& swapchain);
    void setSize(const QSize& size);
    void setMaxFps(uint8_t maxFps);

    QQuickWindow* getWindow() { return _quickWindow; }
    QQuickItem* getRootItem() { return _rootItem; }
    QQmlContext* getContext() { return _qmlContext; }
    void setProxyWindow(QWindow* window);

    void pause();
    void resume();
    bool isPaused() const;
    void addToDeletionList(QObject* object);
    const xr::Swapchain& getSwapchain() const { return _swapchain; }

private:
    bool event(QEvent* e) override;
    bool preRender(bool sceneGraphSync);
    void shutdownRendering();

    QQmlEngine* acquireEngine(OffscreenSurface* surface);
    void releaseEngine(QQmlEngine* engine);

    void requestRender();
    void requestRenderSync();
    void wait();
    void wake();
    void onInitialize();
    void onRender();
    void onTimer();
    void onAboutToQuit();

    xr::Swapchain _swapchain;
    std::vector<::xr::SwapchainImageOpenGLKHR> _swapchainImages;

    QList<QPointer<QObject>> _deletionList;

    // Texture management
    QQuickItem* _rootItem{ nullptr };
    QQuickWindow* _quickWindow{ nullptr };
    QQmlContext* _qmlContext{ nullptr };
    mutable QMutex _mutex;
    QWaitCondition _cond;

    QWindow* _proxyWindow{ nullptr };
    RenderControl* _renderControl{ nullptr };
    RenderEventHandler* _renderObject{ nullptr };

    QTimer* _renderTimer{ nullptr };
    QThread* _renderThread{ nullptr };

    time_point _lastRenderTime;
    QSize _size{ 100, 100 };
    uint8_t _maxFps{ 60 };

    bool _renderRequested{ false };
    bool _syncRequested{ false };
    bool _quit{ false };
    bool _paused{ false };
};

}  // namespace impl
}}  // namespace xr_examples::qml

#endif