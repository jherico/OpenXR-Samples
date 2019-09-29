//
//  Created by Bradley Austin Davis
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <ctime>

#include <algorithm>
#include <array>
#include <chrono>
#include <fstream>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <iomanip>
#include <list>
#include <memory>
#include <mutex>
#include <queue>
#include <random>
#include <set>
#include <string>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <streambuf>
#include <thread>
#include <vector>

#include <fmt/format.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/noise.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>

using glm::ivec2;
using glm::mat3;
using glm::mat4;
using glm::quat;
using glm::uvec2;
using glm::vec2;
using glm::vec3;
using glm::vec4;

#define FORMAT(s, ...) fmt::format(s, __VA_ARGS__)

#if defined(WIN32)
#define XR_USE_PLATFORM_WIN32
#elif defined(__ANDROID__)
#define XR_USE_PLATFORM_ANDROID
#elif
#define XR_USE_PLATFORM_XLIB
#endif

// Boilerplate for running an example
#if defined(__ANDROID__)

#define ENTRY_POINT_START                   \
    void android_main(android_app* state) { \
        vkx::android::androidApp = state;

#define ENTRY_POINT_END }

#else  // defined(__ANDROID__)

extern int g_argc;
extern char** g_argv;

#define ENTRY_POINT_START              \
    int main(int argc, char* argv[]) { \
        g_argc = argc;                 \
        g_argv = argv;

#define ENTRY_POINT_END \
    return 0;           \
    }

#endif

#define RUN_EXAMPLE(ExampleType) \
    ENTRY_POINT_START            \
    ExampleType().run();         \
    ENTRY_POINT_END

#define OPENXR_EXAMPLE_MAIN() RUN_EXAMPLE(OpenXRExample)
