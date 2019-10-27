#pragma once

#include <Magnum/Magnum.h>
#include <Magnum/Math/Range.h>
#include <Magnum/Math/Vector3.h>
#include <Magnum/Math/Vector4.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/Math/Quaternion.h>

#include <interfaces.hpp>

namespace xr_examples { namespace magnum {

inline Magnum::Range2Di fromXr(const xr::Rect2Di& v) {
    return Magnum::Range2Di{ { (int32_t)v.offset.x, (int32_t)v.offset.y },
                             { (int32_t)v.extent.width + v.offset.x, (int32_t)v.extent.height + v.offset.y } };
}

inline Magnum::Vector2i fromXr(const xr::Extent2Di& v) {
    return Magnum::Vector2i{ (int32_t)v.width, (int32_t)v.height };
}

inline Magnum::Vector4 fromXr(const xr::Vector4f& v) {
    return Magnum::Vector4{ v.x, v.y, v.z, v.w };
}

inline Magnum::Vector3 fromXr(const xr::Vector3f& v) {
    return Magnum::Vector3{ v.x, v.y, v.z };
}

inline Magnum::Quaternion fromXr(const xr::Quaternionf& q) {
    return Magnum::Quaternion{ Magnum::Vector3{ q.x, q.y, q.z }, q.w };
}

inline Magnum::Matrix4 fromXrGL(const xr::Fovf& fov, float nearZ = 0.01f, float farZ = 1000.0f) {
    auto tanAngleRight = tanf(fov.angleRight);
    auto tanAngleLeft = tanf(fov.angleLeft);
    auto tanAngleUp = tanf(fov.angleUp);
    auto tanAngleDown = tanf(fov.angleDown);
    const float tanAngleWidth = tanAngleRight - tanAngleLeft;
    const float tanAngleHeight = tanAngleUp - tanAngleDown;

    Magnum::Vector4 result[4];
    const float offsetZ = nearZ;
    result[0][0] = 2 / tanAngleWidth;
    result[1][0] = 0;
    result[2][0] = (tanAngleRight + tanAngleLeft) / tanAngleWidth;
    result[3][0] = 0;

    result[0][1] = 0;
    result[1][1] = 2 / tanAngleHeight;
    result[2][1] = (tanAngleUp + tanAngleDown) / tanAngleHeight;
    result[3][1] = 0;

    result[0][2] = 0;
    result[1][2] = 0;
    result[2][2] = -(farZ + offsetZ) / (farZ - nearZ);
    result[3][2] = -(farZ * (nearZ + offsetZ)) / (farZ - nearZ);

    result[0][3] = 0;
    result[1][3] = 0;
    result[2][3] = -1;
    result[3][3] = 0;
    return Magnum::Matrix4{result[0], result[1], result[2], result[3]};
}

inline Magnum::Matrix4 fromXr(const xr::Posef& p) {
    auto translation = Magnum::Matrix4::translation(fromXr(p.position));
    auto rotation = Magnum::Matrix4{ fromXr(p.orientation).toMatrix() };
    return translation * rotation;
}

}}  // namespace xr_examples::magnum

