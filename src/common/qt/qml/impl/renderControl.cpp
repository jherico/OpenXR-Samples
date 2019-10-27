//
//  Created by Bradley Austin Davis on 2018-01-04
//  Copyright 2013-2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "renderControl.hpp"

#if defined(HAVE_QT)

using namespace xr_examples::qml::impl;

RenderControl::RenderControl(QObject* parent) : QQuickRenderControl(parent) {
}

void RenderControl::setRenderWindow(QWindow* renderWindow) {
    _renderWindow = renderWindow;
}

QWindow* RenderControl::renderWindow(QPoint* offset) {
    if (nullptr == _renderWindow) {
        return QQuickRenderControl::renderWindow(offset);
    }
    if (nullptr != offset) {
        offset->rx() = offset->ry() = 0;
    }
    return _renderWindow;
}

#endif