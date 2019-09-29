#pragma once

#include <Magnum/Magnum.h>
#include <Magnum/Math/Range.h>
#include <Magnum/Math/Vector3.h>
#include <Magnum/Math/Vector4.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/Math/Quaternion.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

#include <interfaces.hpp>

namespace xr_examples { namespace magnum {

inline Magnum::Range2Di fromGlm(const glm::uvec4& v) {
    return Magnum::Range2Di{ { (int32_t)v.x, (int32_t)v.y }, { (int32_t)v.z, (int32_t)v.w } };
}

inline Magnum::Vector2i fromGlm(const glm::uvec2& v) {
    return Magnum::Vector2i{ (int32_t)v.x, (int32_t)v.y };
}

inline Magnum::Vector4 fromGlm(const glm::vec4& v) {
    return Magnum::Vector4{ v.x, v.y, v.z, v.w };
}

inline Magnum::Matrix4 fromGlm(const glm::mat4& m) {
    return Magnum::Matrix4(fromGlm(m[0]), fromGlm(m[1]), fromGlm(m[2]), fromGlm(m[3]));
}

inline Magnum::Matrix4 fromGlm(const xrs::pose& p) {
    return fromGlm(p.toMat4());
}

inline Magnum::Vector3 fromGlm(const glm::vec3& v) {
    return Magnum::Vector3{ v.x, v.y, v.z };
}

inline Magnum::Quaternion fromGlm(const glm::quat& q) {
    return Magnum::Quaternion{ Magnum::Vector3{ q.x, q.y, q.z }, q.w };
}

}}  // namespace xr_examples::magnum
