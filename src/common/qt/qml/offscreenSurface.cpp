//
//  Created by Bradley Austin Davis on 2015-05-13
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "offscreenSurface.hpp"

#if defined(HAVE_QT)

#include <unordered_set>
#include <unordered_map>

#include <QtCore/QThread>
#include <QtQml/QtQml>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlFileSelector>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>

#include "impl/sharedObject.hpp"

using namespace xr_examples::qml;
using namespace xr_examples::qml::impl;

const QmlContextObjectCallback OffscreenSurface::DEFAULT_CONTEXT_OBJECT_CALLBACK = [](QQmlContext*, QQuickItem*) {};
const QmlContextCallback OffscreenSurface::DEFAULT_CONTEXT_CALLBACK = [](QQmlContext*) {};


QQmlFileSelector* OffscreenSurface::getFileSelector() {
    auto context = getSurfaceContext();
    if (!context) {
        return nullptr;
    }
    auto engine = context->engine();
    if (!engine) {
        return nullptr;
    }

    return QQmlFileSelector::get(engine);
}

void OffscreenSurface::initializeEngine(QQmlEngine* engine) {
    new QQmlFileSelector(engine);
}

OffscreenSurface::OffscreenSurface() : _sharedObject(new impl::SharedObject()) {
}

OffscreenSurface::~OffscreenSurface() {
    _sharedObject->deleteLater();
}

void OffscreenSurface::setSwapchain(const xr::Swapchain& swapchain) {
    _sharedObject->setSwapchain(swapchain);
}

void OffscreenSurface::resize(const QSize& newSize_) {
    _sharedObject->setSize(newSize_);
}

QQuickItem* OffscreenSurface::getRootItem() {
    return _sharedObject->getRootItem();
}

void OffscreenSurface::clearCache() {
    _sharedObject->getContext()->engine()->clearComponentCache();
}

QPointF OffscreenSurface::mapToVirtualScreen(const QPointF& originalPoint) {
    return _mouseTranslator(originalPoint);
}

///////////////////////////////////////////////////////
//
// Event handling customization
//

bool OffscreenSurface::filterEnabled(QObject* originalDestination, QEvent* event) const {
    if (!_sharedObject || !_sharedObject->getWindow() || _sharedObject->getWindow() == originalDestination) {
        return false;
    }
    // Only intercept events while we're in an active state
    if (_sharedObject->isPaused()) {
        return false;
    }
    return true;
}

bool OffscreenSurface::eventFilter(QObject* originalDestination, QEvent* event) {
    if (!filterEnabled(originalDestination, event)) {
        return false;
    }

#ifdef DEBUG
    // Don't intercept our own events, or we enter an infinite recursion
    {
        auto rootItem = _sharedObject->getRootItem();
        auto quickWindow = _sharedObject->getWindow();
        QObject* recurseTest = originalDestination;
        while (recurseTest) {
            Q_ASSERT(recurseTest != rootItem && recurseTest != quickWindow);
            recurseTest = recurseTest->parent();
        }
    }
#endif

    switch (event->type()) {
        case QEvent::KeyPress:
        case QEvent::KeyRelease: {
            event->ignore();
            if (QCoreApplication::sendEvent(_sharedObject->getWindow(), event)) {
                return event->isAccepted();
            }
            break;
        }

        case QEvent::Wheel: {
            QWheelEvent* wheelEvent = static_cast<QWheelEvent*>(event);
            QPointF transformedPos = mapToVirtualScreen(wheelEvent->pos());
            QWheelEvent mappedEvent(transformedPos, wheelEvent->delta(), wheelEvent->buttons(), wheelEvent->modifiers(),
                                    wheelEvent->orientation());
            mappedEvent.ignore();
            if (QCoreApplication::sendEvent(_sharedObject->getWindow(), &mappedEvent)) {
                return mappedEvent.isAccepted();
            }
            break;
        }
        case QEvent::MouseMove: {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            QPointF transformedPos = mapToVirtualScreen(mouseEvent->localPos());
            QMouseEvent mappedEvent(mouseEvent->type(), transformedPos, mouseEvent->screenPos(), mouseEvent->button(),
                                    mouseEvent->buttons(), mouseEvent->modifiers());
            mappedEvent.ignore();
            if (QCoreApplication::sendEvent(_sharedObject->getWindow(), &mappedEvent)) {
                return mappedEvent.isAccepted();
            }
            break;
        }

#if defined(Q_OS_ANDROID)
        case QEvent::TouchBegin:
        case QEvent::TouchUpdate:
        case QEvent::TouchEnd: {
            QTouchEvent* originalEvent = static_cast<QTouchEvent*>(event);
            QEvent::Type fakeMouseEventType = QEvent::None;
            Qt::MouseButton fakeMouseButton = Qt::LeftButton;
            Qt::MouseButtons fakeMouseButtons = Qt::NoButton;
            switch (event->type()) {
                case QEvent::TouchBegin:
                    fakeMouseEventType = QEvent::MouseButtonPress;
                    fakeMouseButtons = Qt::LeftButton;
                    break;
                case QEvent::TouchUpdate:
                    fakeMouseEventType = QEvent::MouseMove;
                    fakeMouseButtons = Qt::LeftButton;
                    break;
                case QEvent::TouchEnd:
                    fakeMouseEventType = QEvent::MouseButtonRelease;
                    fakeMouseButtons = Qt::NoButton;
                    break;
                default:
                    Q_UNREACHABLE();
            }
            // Same case as OffscreenUi.cpp::eventFilter: touch events are always being accepted so we now use mouse events and consider one touch, touchPoints()[0].
            QMouseEvent fakeMouseEvent(fakeMouseEventType, originalEvent->touchPoints()[0].pos(), fakeMouseButton,
                                       fakeMouseButtons, Qt::NoModifier);
            fakeMouseEvent.ignore();
            if (QCoreApplication::sendEvent(_sharedObject->getWindow(), &fakeMouseEvent)) {
                /*qInfo() << __FUNCTION__ << "sent fake touch event:" << fakeMouseEvent.type()
                        << "_quickWindow handled it... accepted:" << fakeMouseEvent.isAccepted();*/
                return fakeMouseEvent.isAccepted();
            }
            break;
        }
        case QEvent::InputMethod:
        case QEvent::InputMethodQuery: {
            auto window = getWindow();
            if (window && window->activeFocusItem()) {
                event->ignore();
                if (QCoreApplication::sendEvent(window->activeFocusItem(), event)) {
                    bool eventAccepted = event->isAccepted();
                    if (event->type() == QEvent::InputMethodQuery) {
                        QInputMethodQueryEvent* imqEvent = static_cast<QInputMethodQueryEvent*>(event);
                        // this block disables the selection cursor in android which appears in
                        // the top-left corner of the screen
                        if (imqEvent->queries() & Qt::ImEnabled) {
                            imqEvent->setValue(Qt::ImEnabled, QVariant(false));
                        }
                    }
                    return eventAccepted;
                }
                return false;
            }
            break;
        }
#endif
        default:
            break;
    }

    return false;
}

void OffscreenSurface::pause() {
    _sharedObject->pause();
}

void OffscreenSurface::resume() {
    _sharedObject->resume();
}

bool OffscreenSurface::isPaused() const {
    return _sharedObject->isPaused();
}

void OffscreenSurface::setProxyWindow(QWindow* window) {
    _sharedObject->setProxyWindow(window);
}

QObject* OffscreenSurface::getEventHandler() {
    return getWindow();
}

QQuickWindow* OffscreenSurface::getWindow() {
    return _sharedObject->getWindow();
}

QSize OffscreenSurface::size() const {
    return _sharedObject->getSize();
}

QQmlContext* OffscreenSurface::getSurfaceContext() {
    return _sharedObject->getContext();
}

void OffscreenSurface::setMaxFps(uint8_t maxFps) {
    _sharedObject->setMaxFps(maxFps);
}

void OffscreenSurface::load(const QUrl& qmlSource,
                            const QmlContextObjectCallback& callback,
                            const QmlContextCallback& contextCallback) {
    if (QThread::currentThread() != thread()) {
        qFatal("Called load on a non-surface thread");
    }

    _sharedObject->create(this);

    QUrl finalQmlSource = qmlSource;
    if ((qmlSource.isRelative() && !qmlSource.isEmpty()) || qmlSource.scheme() == QLatin1String("file")) {
        finalQmlSource = getSurfaceContext()->resolvedUrl(qmlSource);
    }
    _sharedObject->setObjectName(finalQmlSource.toString());

    auto context = getSurfaceContext();
    contextCallback(context);
    auto engine = context->engine();
    QQmlComponent* qmlComponent = new QQmlComponent(engine, finalQmlSource, QQmlComponent::PreferSynchronous);
    if (qmlComponent->isLoading()) {
        QObject::connect(qmlComponent, &QQmlComponent::statusChanged, this,
                         [=](QQmlComponent::Status) { finishQmlLoad(qmlComponent, callback); });
        return;
    }
    finishQmlLoad(qmlComponent, callback);
}

void OffscreenSurface::finishQmlLoad(QQmlComponent* qmlComponent, const QmlContextObjectCallback& callback) {
    disconnect(qmlComponent, &QQmlComponent::statusChanged, this, 0);
    if (qmlComponent->isError()) {
        for (const auto& error : qmlComponent->errors()) {
            qWarning() << error.url() << error.line() << error;
        }
        qmlComponent->deleteLater();
        return;
    }

    QObject* newObject = qmlComponent->beginCreate(getSurfaceContext());
    if (qmlComponent->isError()) {
        for (const auto& error : qmlComponent->errors()) {
            qWarning() << error.url() << error.line() << error;
        }
        if (!getRootItem()) {
            qFatal("Unable to finish loading QML root");
        }
        qmlComponent->deleteLater();
        return;
    }

    if (!newObject) {
        qFatal("Could not load object as root item");
        return;
    }

    auto context = getSurfaceContext();
    context->engine()->setObjectOwnership(this, QQmlEngine::CppOwnership);
    // All quick items should be focusable
    QQuickItem* newItem = qobject_cast<QQuickItem*>(newObject);
    if (newItem) {
        // Make sure we make items focusable (critical for supporting keyboard shortcuts)
        newItem->setFlag(QQuickItem::ItemIsFocusScope, true);
    }

    _sharedObject->setRootItem(newItem);
    emit rootItemCreated(newItem);
    callback(context, newItem);
    qmlComponent->completeCreate();
    qmlComponent->deleteLater();
}

#endif