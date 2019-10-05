// Copyright (c) 2017 The Khronos Group Inc.
// Copyright (c) 2016 Oculus VR, LLC.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Author: J.M.P. van Waveren
//

#pragma once

#include <openxr/openxr.hpp>

#include <cassert>
#include <cmath>

namespace xr {

namespace color {
static constexpr Color4f Red { 1.0f, 0.0f, 0.0f, 1.0f };
static constexpr Color4f Green { 0.0f, 1.0f, 0.0f, 1.0f };
static constexpr Color4f Blue { 0.0f, 0.0f, 1.0f, 1.0f };
static constexpr Color4f Yellow { 1.0f, 1.0f, 0.0f, 1.0f };
static constexpr Color4f Purple { 1.0f, 0.0f, 1.0f, 1.0f };
static constexpr Color4f Cyan { 0.0f, 1.0f, 1.0f, 1.0f };
static constexpr Color4f LightGrey { 0.7f, 0.7f, 0.7f, 1.0f };
static constexpr Color4f DarkGrey { 0.3f, 0.3f, 0.3f, 1.0f };

}  // namespace color

inline Vector3f operator*(const Vector3f& a, const float& f) {
    return { a.x * f, a.y * f, a.z * f };
}

inline Vector3f operator+(const Vector3f& a, const Vector3f& b) {
    return { a.x + b.x, a.y + b.y, a.z + b.z };
}

inline Vector3f operator-(const Vector3f& a, const Vector3f& b) {
    return { a.x - b.x, a.y - b.y, a.z - b.z };
}

inline Vector3f operator/(const Vector3f& a, const float& f) {
    return { a.x / f, a.y / f, a.z / f };
}

inline Vector3f operator/(const Vector3f& a, const Vector3f& b) {
    return { a.x / b.x, a.y / b.y, a.z / b.z };
}

inline Vector3f min(const Vector3f& a, const Vector3f& b) {
    return { std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z) };
}

inline Vector3f max(const Vector3f& a, const Vector3f& b) {
    return { std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z) };
}

inline float dot(const Vector3f& a, const Vector3f& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

// Compute cross product, which generates a normal vector.
// Direction vector can be determined by right-hand rule: Pointing index finder in
// direction a and middle finger in direction b, thumb will point in Cross(a, b).
inline Vector3f operator^(const Vector3f& a, const Vector3f& b) {
    Vector3f result;
    result.x = a.y * b.z - a.z * b.y;
    result.y = a.z * b.x - a.x * b.z;
    result.x = a.x * b.y - a.y * b.x;
    return result;
}

inline Vector3f mix(const Vector3f& a, const Vector3f& b, const float fraction) {
    return { a.x + fraction * (b.x - a.x), a.y + fraction * (b.y - a.y), a.z + fraction * (b.z - a.z) };
}

inline float lengthSquared(const Vector3f& v) {
    return (v.x * v.x) + (v.y * v.y) + (v.z * v.z);
}

inline float rcpSqrt(const float x) {
    const float SMALLEST_NON_DENORMAL = 1.1754943508222875e-038f;  // ( 1U << 23 )
    const float rcp = (x >= SMALLEST_NON_DENORMAL) ? 1.0f / sqrtf(x) : 1.0f;
    return rcp;
}

inline float length(const Vector3f& v) {
    return sqrt(lengthSquared(v));
}

inline Vector3f normalized(const Vector3f& v) {
    return v * rcpSqrt(lengthSquared(v));
}

}  // namespace xr
