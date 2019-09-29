#pragma once

#include <openxr/openxr.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

namespace xrs {

struct pose {
    glm::vec3 position;
    glm::quat rotation;

	glm::mat4 toMat4() const {
		return glm::translate(position) * glm::mat4_cast(rotation);
	}
};

inline XrFovf toTanFovf(const XrFovf& fov) {
    return { tanf(fov.angleLeft), tanf(fov.angleRight), tanf(fov.angleUp), tanf(fov.angleDown) };
}

inline glm::mat4 toGlmGL(const XrFovf& fov, float nearZ = 0.01f, float farZ = 10000.0f) {
    auto tanFov = toTanFovf(fov);
    const auto& tanAngleRight = tanFov.angleRight;
    const auto& tanAngleLeft = tanFov.angleLeft;
    const auto& tanAngleUp = tanFov.angleUp;
    const auto& tanAngleDown = tanFov.angleDown;
    const float tanAngleWidth = tanAngleRight - tanAngleLeft;
    const float tanAngleHeight = tanAngleUp - tanAngleDown;

    // Set to nearZ for a [-1,1] Z clip space (OpenGL / OpenGL ES).
    // Set to zero for a [0,1] Z clip space (Vulkan / D3D / Metal).
    const float offsetZ = nearZ;
    glm::mat4 resultm{};
    float* result = &resultm[0][0];
    result[0] = 2 / tanAngleWidth;
    result[4] = 0;
    result[8] = (tanAngleRight + tanAngleLeft) / tanAngleWidth;
    result[12] = 0;

    result[1] = 0;
    result[5] = 2 / tanAngleHeight;
    result[9] = (tanAngleUp + tanAngleDown) / tanAngleHeight;
    result[13] = 0;

    result[2] = 0;
    result[6] = 0;
    result[10] = -(farZ + offsetZ) / (farZ - nearZ);
    result[14] = -(farZ * (nearZ + offsetZ)) / (farZ - nearZ);

    result[3] = 0;
    result[7] = 0;
    result[11] = -1;
    result[15] = 0;
    return resultm;
}

inline glm::mat4 toGlmVK(const XrFovf& fov, float nearZ = 0.01f, float farZ = 10000.0f) {
    auto tanFov = toTanFovf(fov);
    const auto& tanAngleRight = tanFov.angleRight;
    const auto& tanAngleLeft = tanFov.angleLeft;
    const auto& tanAngleUp = tanFov.angleUp;
    const auto& tanAngleDown = tanFov.angleDown;

    const float tanAngleWidth = tanAngleRight - tanAngleLeft;
    const float tanAngleHeight = (tanAngleDown - tanAngleUp);
    const float offsetZ = 0;

    glm::mat4 resultm{};
    float* result = &resultm[0][0];
    // normal projection
    result[0] = 2 / tanAngleWidth;
    result[4] = 0;
    result[8] = (tanAngleRight + tanAngleLeft) / tanAngleWidth;
    result[12] = 0;

    result[1] = 0;
    result[5] = 2 / tanAngleHeight;
    result[9] = (tanAngleUp + tanAngleDown) / tanAngleHeight;
    result[13] = 0;

    result[2] = 0;
    result[6] = 0;
    result[10] = -(farZ + offsetZ) / (farZ - nearZ);
    result[14] = -(farZ * (nearZ + offsetZ)) / (farZ - nearZ);

    result[3] = 0;
    result[7] = 0;
    result[11] = -1;
    result[15] = 0;

    return resultm;
}

inline glm::quat toGlm(const XrQuaternionf& q) {
    return glm::make_quat(&q.x);
}

inline glm::vec3 toGlm(const XrVector3f& v) {
    return glm::make_vec3(&v.x);
}

inline glm::mat4 toGlm(const XrPosef& p) {
    glm::mat4 orientation = glm::mat4_cast(toGlm(p.orientation));
    glm::mat4 translation = glm::translate(glm::mat4(1.0), toGlm(p.position));
    return translation * orientation;
}

inline pose toGlmPose(const XrPosef& p) {
	return { toGlm(p.position), toGlm(p.orientation) };
}

inline glm::ivec2 toGlm(const XrExtent2Di& e) {
    return { e.width, e.height };
}

inline xr::Vector3f fromGlm(const glm::vec3& v) {
    return { v.x, v.y, v.z };
}

static inline xr::Vector2f fromGlm(const glm::vec2& v) {
    return { v.x, v.y };
}

static inline xr::Quaternionf fromGlm(const glm::quat& q) {
    return { q.x, q.y, q.z, q.w };
}

static inline xr::Posef poseFromGlm(const glm::mat4& m) {
    glm::vec3 translation = glm::vec3(m[3]) / m[3].w;
    glm::quat orientation = glm::quat_cast(m);
    return xr::Posef{ fromGlm(orientation), fromGlm(translation) };
}

}  // namespace xrs