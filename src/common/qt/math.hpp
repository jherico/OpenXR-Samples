#pragma once

#ifdef HAVE_QT

#include <openxr/openxr.hpp>
#include <QtGui/QVector3D>
#include <QtGui/QMatrix4x4>
#include <QtGui/QQuaternion>

namespace xr_examples { namespace qt {

inline QVector3D fromXr(const xr::Vector3f& v) {
    return QVector3D{ v.x, v.y, v.z };
}

inline QQuaternion fromXr(const xr::Quaternionf& q) {
    return QQuaternion{ q.w, { q.x, q.y, q.z } };
}

inline QMatrix4x4 fromXr(const xr::Posef& m) {
    QMatrix4x4 translation;
    translation.translate(fromXr(m.position));
    QMatrix4x4 rotation{ fromXr(m.orientation).toRotationMatrix() };
    return translation * rotation;
}

inline QSize fromXr(const xr::Extent2Di& e) {
    return { e.width, e.height };
}

inline QMatrix4x4 fromXrGL(const xr::Fovf& fov, float nearZ = 0.01f, float farZ = 1000.0f) {
    auto tanAngleRight = tanf(fov.angleRight);
    auto tanAngleLeft = tanf(fov.angleLeft);
    auto tanAngleUp = tanf(fov.angleUp);
    auto tanAngleDown = tanf(fov.angleDown);
    const float tanAngleWidth = tanAngleRight - tanAngleLeft;
    const float tanAngleHeight = tanAngleUp - tanAngleDown;

    QMatrix4x4 result;
    const float offsetZ = nearZ;
    result(0, 0) = 2 / tanAngleWidth;
    result(0, 1) = 0;
    result(0, 2) = (tanAngleRight + tanAngleLeft) / tanAngleWidth;
    result(0, 3) = 0;

    result(1, 0) = 0;
    result(1, 1) = 2 / tanAngleHeight;
    result(1, 2) = (tanAngleUp + tanAngleDown) / tanAngleHeight;
    result(1, 3) = 0;

    result(2, 0) = 0;
    result(2, 1) = 0;
    result(2, 2) = -(farZ + offsetZ) / (farZ - nearZ);
    result(2, 3) = -(farZ * (nearZ + offsetZ)) / (farZ - nearZ);

    result(3, 0) = 0;
    result(3, 1) = 0;
    result(3, 2) = -1;
    result(3, 3) = 0;
    return result;
}

}}  // namespace xr_examples::qt

#endif
