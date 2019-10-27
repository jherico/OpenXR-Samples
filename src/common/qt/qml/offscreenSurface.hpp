//
//  Created by Bradley Austin Davis on 2015-04-04
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#if defined(HAVE_QT)

#include <atomic>
#include <queue>
#include <map>
#include <functional>

#include <QtCore/QUrl>
#include <QtCore/QSize>
#include <QtCore/QPointF>
#include <QtCore/QSharedPointer>
#include <QtCore/QTimer>

#include <QtQml/QJSValue>

class QWindow;
class QOpenGLContext;
class QQmlContext;
class QQmlEngine;
class QQmlComponent;
class QQuickWindow;
class QQuickItem;
class OffscreenQmlSharedObject;
class QQmlFileSelector;

namespace xr {
class Session;
class Swapchain;
}
namespace xr_examples { namespace qml {

namespace impl {
class SharedObject;
}

using QmlContextCallback = ::std::function<void(QQmlContext*)>;
using QmlContextObjectCallback = ::std::function<void(QQmlContext*, QQuickItem*)>;
using MouseTranslator = std::function<QPoint(const QPointF&)>;

class OffscreenSurface : public QObject {
    Q_OBJECT

public:
    static const QmlContextObjectCallback DEFAULT_CONTEXT_OBJECT_CALLBACK;
    static const QmlContextCallback DEFAULT_CONTEXT_CALLBACK;

    OffscreenSurface();
    virtual ~OffscreenSurface();

    QSize size() const;
    virtual void resize(const QSize& size);
    void clearCache();

    void setMaxFps(uint8_t maxFps);
    // Optional values for event handling
    void setSwapchain(const xr::Swapchain& swapchain);
    void setProxyWindow(QWindow* window);
    void setMouseTranslator(const MouseTranslator& mouseTranslator) { _mouseTranslator = mouseTranslator; }

    void pause();
    void resume();
    bool isPaused() const;

    QQuickItem* getRootItem();
    QQuickWindow* getWindow();
    QObject* getEventHandler();
    QQmlContext* getSurfaceContext();
    QQmlFileSelector* getFileSelector();
    QPointF mapToVirtualScreen(const QPointF& originalPoint);

    // For use from C++
    void load(const QUrl& qmlSource,
              const QmlContextObjectCallback& callback = DEFAULT_CONTEXT_OBJECT_CALLBACK,
              const QmlContextCallback& contextCallback = DEFAULT_CONTEXT_CALLBACK);

public slots:
    virtual void onFocusObjectChanged(QObject* newFocus) {}

signals:
    void rootContextCreated(QQmlContext* rootContext);
    void rootItemCreated(QQuickItem* rootContext);

protected:
    bool eventFilter(QObject* originalDestination, QEvent* event) override;
    bool filterEnabled(QObject* originalDestination, QEvent* event) const;

    virtual void initializeEngine(QQmlEngine* engine);
    virtual void finishQmlLoad(QQmlComponent* qmlComponent, const QmlContextObjectCallback& onQmlLoadedCallback) final;

private:
    MouseTranslator _mouseTranslator{ [](const QPointF& p) { return p.toPoint(); } };
    friend class impl::SharedObject;
    impl::SharedObject* const _sharedObject;
};

}}  // namespace xr_examples::qml

#endif