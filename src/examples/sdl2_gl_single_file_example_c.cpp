//
//  Created by Bradley Austin Davis on 2019/09/18
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#define XR_USE_GRAPHICS_API_OPENGL
#if defined(WIN32)
#define XR_USE_PLATFORM_WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#elif defined(__ANDROID__)
#define XR_USE_PLATFORM_ANDROID
#else
#define XR_USE_PLATFORM_XLIB
#endif

#include <cstdint>
#include <unordered_map>
#include <functional>

#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <array>

#include <SDL2/SDL.h>

#include <glad/glad.h>
#include <logging.hpp>

PFN_xrCreateDebugUtilsMessengerEXT pxrCreateDebugUtilsMessengerEXT{ nullptr };
PFN_xrDestroyDebugUtilsMessengerEXT pxrDestroyDebugUtilsMessengerEXT{ nullptr };
PFN_xrGetOpenGLGraphicsRequirementsKHR pxrGetOpenGLGraphicsRequirementsKHR{ nullptr };

namespace xrs {

enum Side : uint32_t
{
    Left = 0,
    Right = 1,
};

template <typename SideHandler>
static inline void for_each_side(SideHandler&& handler) {
    handler(Left);
    handler(Right);
}

template <typename IndexHandler>
static inline void for_each_side_index(IndexHandler&& handler) {
    handler(0);
    handler(1);
}

namespace DebugUtilsEXT {

using MessageSeverityFlagBits = XrDebugUtilsMessageSeverityFlagsEXT;
using MessageTypeFlagBits = XrDebugUtilsMessageTypeFlagsEXT;
using MessageSeverityFlags = XrDebugUtilsMessageSeverityFlagsEXT;
constexpr MessageSeverityFlags ALL_SEVERITIES =
    XR_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | XR_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
    XR_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | XR_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
using MessageTypeFlags = XrDebugUtilsMessageTypeFlagsEXT;
constexpr MessageTypeFlags ALL_TYPES =
    XR_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | XR_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
    XR_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | XR_DEBUG_UTILS_MESSAGE_TYPE_CONFORMANCE_BIT_EXT;
using CallbackData = XrDebugUtilsMessengerCallbackDataEXT;
using Messenger = XrDebugUtilsMessengerEXT;

// Raw C callback
static XrBool32 debugCallback(XrDebugUtilsMessageSeverityFlagsEXT sev_,
                              XrDebugUtilsMessageTypeFlagsEXT type_,
                              const XrDebugUtilsMessengerCallbackDataEXT* data_,
                              void* userData) {
    LOG_FORMATTED((logging::Level)sev_, "{}: {}", data_->functionName, data_->message);
    return XR_TRUE;
}

#define CHECK_XR_RESULT(x)                            \
    {                                                 \
        XrResult xr_result;                           \
        if (!XR_SUCCEEDED(xr_result = x)) {           \
            throw new std::runtime_error("XR error"); \
        }                                             \
    }

Messenger create(const XrInstance& instance,
                 const MessageSeverityFlags& severityFlags = ALL_SEVERITIES,
                 const MessageTypeFlags& typeFlags = ALL_TYPES,
                 void* userData = nullptr) {
    XrDebugUtilsMessengerCreateInfoEXT createInfo{
        XR_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT, nullptr, severityFlags, typeFlags, debugCallback, userData
    };
    Messenger result;
    CHECK_XR_RESULT(pxrCreateDebugUtilsMessengerEXT(instance, &createInfo, &result));
    return result;
}

}  // namespace DebugUtilsEXT

inline XrFovf toTanFovf(const XrFovf& fov) {
    return { tanf(fov.angleLeft), tanf(fov.angleRight), tanf(fov.angleUp), tanf(fov.angleDown) };
}

inline glm::mat4 toGlm(const XrFovf& fov, float nearZ = 0.01f, float farZ = 10000.0f) {
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
    glm::mat4 translation = glm::translate(glm::mat4{ 1 }, toGlm(p.position));
    return translation * orientation;
}

}  // namespace xrs

inline void debugMessageCallback(GLenum source,
                                 GLenum type,
                                 GLuint id,
                                 GLenum severity,
                                 GLsizei length,
                                 const GLchar* message,
                                 const void* userParam) {
    std::cout << message << std::endl;
}

static std::string formatToString(GLenum format) {
    switch (format) {
        case GL_COMPRESSED_R11_EAC:
            return "COMPRESSED_R11_EAC";
        case GL_COMPRESSED_RED_RGTC1:
            return "COMPRESSED_RED_RGTC1";
        case GL_COMPRESSED_RG_RGTC2:
            return "COMPRESSED_RG_RGTC2";
        case GL_COMPRESSED_RG11_EAC:
            return "COMPRESSED_RG11_EAC";
        case GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT:
            return "COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT";
        case GL_COMPRESSED_RGB8_ETC2:
            return "COMPRESSED_RGB8_ETC2";
        case GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2:
            return "COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2";
        case GL_COMPRESSED_RGBA8_ETC2_EAC:
            return "COMPRESSED_RGBA8_ETC2_EAC";
        case GL_COMPRESSED_SIGNED_R11_EAC:
            return "COMPRESSED_SIGNED_R11_EAC";
        case GL_COMPRESSED_SIGNED_RG11_EAC:
            return "COMPRESSED_SIGNED_RG11_EAC";
        case GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM:
            return "COMPRESSED_SRGB_ALPHA_BPTC_UNORM";
        case GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC:
            return "COMPRESSED_SRGB8_ALPHA8_ETC2_EAC";
        case GL_COMPRESSED_SRGB8_ETC2:
            return "COMPRESSED_SRGB8_ETC2";
        case GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2:
            return "COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2";
        case GL_DEPTH_COMPONENT16:
            return "DEPTH_COMPONENT16";
        case GL_DEPTH_COMPONENT24:
            return "DEPTH_COMPONENT24";
        case GL_DEPTH_COMPONENT32:
            return "DEPTH_COMPONENT32";
        case GL_DEPTH_COMPONENT32F:
            return "DEPTH_COMPONENT32F";
        case GL_DEPTH24_STENCIL8:
            return "DEPTH24_STENCIL8";
        case GL_R11F_G11F_B10F:
            return "R11F_G11F_B10F";
        case GL_R16_SNORM:
            return "R16_SNORM";
        case GL_R16:
            return "R16";
        case GL_R16F:
            return "R16F";
        case GL_R16I:
            return "R16I";
        case GL_R16UI:
            return "R16UI";
        case GL_R32F:
            return "R32F";
        case GL_R32I:
            return "R32I";
        case GL_R32UI:
            return "R32UI";
        case GL_R8_SNORM:
            return "R8_SNORM";
        case GL_R8:
            return "R8";
        case GL_R8I:
            return "R8I";
        case GL_R8UI:
            return "R8UI";
        case GL_RG16_SNORM:
            return "RG16_SNORM";
        case GL_RG16:
            return "RG16";
        case GL_RG16F:
            return "RG16F";
        case GL_RG16I:
            return "RG16I";
        case GL_RG16UI:
            return "RG16UI";
        case GL_RG32F:
            return "RG32F";
        case GL_RG32I:
            return "RG32I";
        case GL_RG32UI:
            return "RG32UI";
        case GL_RG8_SNORM:
            return "RG8_SNORM";
        case GL_RG8:
            return "RG8";
        case GL_RG8I:
            return "RG8I";
        case GL_RG8UI:
            return "RG8UI";
        case GL_RGB10_A2:
            return "RGB10_A2";
        case GL_RGB8:
            return "RGB8";
        case GL_RGB9_E5:
            return "RGB9_E5";
        case GL_RGBA16_SNORM:
            return "RGBA16_SNORM";
        case GL_RGBA16:
            return "RGBA16";
        case GL_RGBA16F:
            return "RGBA16F";
        case GL_RGBA16I:
            return "RGBA16I";
        case GL_RGBA16UI:
            return "RGBA16UI";
        case GL_RGBA2:
            return "RGBA2";
        case GL_RGBA32F:
            return "RGBA32F";
        case GL_RGBA32I:
            return "RGBA32I";
        case GL_RGBA32UI:
            return "RGBA32UI";
        case GL_RGBA8_SNORM:
            return "RGBA8_SNORM";
        case GL_RGBA8:
            return "RGBA8";
        case GL_RGBA8I:
            return "RGBA8I";
        case GL_RGBA8UI:
            return "RGBA8UI";
        case GL_SRGB8_ALPHA8:
            return "SRGB8_ALPHA8";
        case GL_SRGB8:
            return "SRGB8";
        case GL_RGB16F:
            return "RGB16F";
        case GL_DEPTH32F_STENCIL8:
            return "DEPTH32F_STENCIL8";
        case GL_BGR:
            return "BGR (Out of spec)";
        case GL_BGRA:
            return "BGRA (Out of spec)";
    }
    return "unknown";
}

struct OpenXrExample {
    bool quit{ false };

    // Application main function
    void run() {
        // Startup work
        prepare();

        // Loop
        while (!quit) {
            frame();
        }

        // Teardown work
        destroy();
    }

    //////////////////////////////////////
    // One-time setup work              //
    //////////////////////////////////////

    // The top level prepare function, which is broken down by task
    void prepare() {
        // The OpenXR instance and the OpenXR system provide information we'll require to create our window
        // and rendering backend, so it has to come first
        prepareXrInstance();
        prepareXrSystem();

        prepareWindow();
        prepareXrSession();
        prepareXrSwapchain();
        prepareXrCompositionLayers();
        prepareGlFramebuffer();
    }

    bool enableDebug{ true };
    XrInstance instance;
    xrs::DebugUtilsEXT::Messenger messenger;
    void prepareXrInstance() {
        uint32_t extensionCount{ 0 };
        CHECK_XR_RESULT(xrEnumerateInstanceExtensionProperties(nullptr, 0, &extensionCount, nullptr));
        std::vector<XrExtensionProperties> vector;
        vector.resize(extensionCount);
        for (int i = 0; i < extensionCount; ++i) {
            auto& extenionProperties = vector[i];
            extenionProperties = XrExtensionProperties{ XR_TYPE_EXTENSION_PROPERTIES, nullptr };
        }
        auto result = xrEnumerateInstanceExtensionProperties(nullptr, extensionCount, &extensionCount, vector.data());
        CHECK_XR_RESULT(result);
        std::unordered_map<std::string, XrExtensionProperties> discoveredExtensions;
        for (const auto& extensionProperties : vector) {
            discoveredExtensions.insert({ extensionProperties.extensionName, extensionProperties });
        }

#if !defined(SUPPRESS_DEBUG_UTILS)
        if (0 == discoveredExtensions.count(XR_EXT_DEBUG_UTILS_EXTENSION_NAME)) {
            enableDebug = false;
        }
#else
        enableDebug = false;
#endif

        std::vector<const char*> requestedExtensions;
        if (0 == discoveredExtensions.count(XR_KHR_OPENGL_ENABLE_EXTENSION_NAME)) {
            throw std::runtime_error(
                fmt::format("Required Graphics API extension not available: {}", XR_KHR_OPENGL_ENABLE_EXTENSION_NAME));
        }
        requestedExtensions.push_back(XR_KHR_OPENGL_ENABLE_EXTENSION_NAME);

        if (enableDebug) {
            requestedExtensions.push_back(XR_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }
        XrApplicationInfo appInfo{ "gl_single_file_example", 0, "openXrSamples", 0, XR_MAKE_VERSION(1, 0, 9) };

        XrInstanceCreateInfo ici{
            XR_TYPE_INSTANCE_CREATE_INFO, nullptr, 0, appInfo, 0, nullptr, (uint32_t)requestedExtensions.size(),
            requestedExtensions.data()
        };

        XrDebugUtilsMessengerCreateInfoEXT dumci{
            XR_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT, nullptr
        };
        if (enableDebug) {
            dumci.messageSeverities = xrs::DebugUtilsEXT::ALL_SEVERITIES;
            dumci.messageTypes = xrs::DebugUtilsEXT::ALL_TYPES;
            dumci.userData = this;
            dumci.userCallback = &xrs::DebugUtilsEXT::debugCallback;
            ici.next = &dumci;
        }

        // Create the actual instance
        CHECK_XR_RESULT(xrCreateInstance(&ici, &instance));
        PFN_xrCreateDebugUtilsMessengerEXT pxrCreateDebugUtilsMessengerEXT{ nullptr };
        PFN_xrDestroyDebugUtilsMessengerEXT pxrDestroyDebugUtilsMessengerEXT{ nullptr };
        PFN_xrGetOpenGLGraphicsRequirementsKHR pxrGetOpenGLGraphicsRequirementsKHR{ nullptr };

        xrGetInstanceProcAddr(instance, "xrCreateDebugUtilsMessengerEXT",
                              (PFN_xrVoidFunction*)&pxrCreateDebugUtilsMessengerEXT);
        xrGetInstanceProcAddr(instance, "xrDestroyDebugUtilsMessengerEXT",
                              (PFN_xrVoidFunction*)&pxrDestroyDebugUtilsMessengerEXT);
        xrGetInstanceProcAddr(instance, "xrGetOpenGLGraphicsRequirementsKHR",
                              (PFN_xrVoidFunction*)&pxrGetOpenGLGraphicsRequirementsKHR);

        // Turn on debug logging
        if (enableDebug) {
            messenger = xrs::DebugUtilsEXT::create(instance);
        }

        // Log the instance properties
        XrInstanceProperties instanceProperties{ XR_TYPE_INSTANCE_PROPERTIES, nullptr };

        CHECK_XR_RESULT(xrGetInstanceProperties(instance, &instanceProperties));
        LOG_INFO("OpenXR Runtime {} version {}.{}.{}",  //
                 (const char*)instanceProperties.runtimeName, XR_VERSION_MAJOR(instanceProperties.runtimeVersion),
                 XR_VERSION_MINOR(instanceProperties.runtimeVersion), XR_VERSION_PATCH(instanceProperties.runtimeVersion));
    }

    XrSystemId systemId;
    glm::uvec2 renderTargetSize;
    XrGraphicsRequirementsOpenGLKHR graphicsRequirements;
    void prepareXrSystem() {
        // We want to create an HMD example, so we ask for a runtime that supposts that form factor
        // and get a response in the form of a systemId
        XrSystemGetInfo getInfo{ XR_TYPE_SYSTEM_GET_INFO, nullptr, XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY };
        CHECK_XR_RESULT(xrGetSystem(instance, &getInfo, &systemId));

        // Log the system properties
        {
            XrSystemProperties systemProperties{ XR_TYPE_SYSTEM_PROPERTIES, nullptr };
            CHECK_XR_RESULT(xrGetSystemProperties(instance, systemId, &systemProperties));
            LOG_INFO("OpenXR System {} max layers {} max swapchain image size {}x{}",  //
                     (const char*)systemProperties.systemName, (uint32_t)systemProperties.graphicsProperties.maxLayerCount,
                     (uint32_t)systemProperties.graphicsProperties.maxSwapchainImageWidth,
                     (uint32_t)systemProperties.graphicsProperties.maxSwapchainImageHeight);
        }

        // Find out what view configurations we have available
        {
            uint32_t viewConfigCount{ 0 };
            CHECK_XR_RESULT(xrEnumerateViewConfigurations(instance, systemId, viewConfigCount, &viewConfigCount, nullptr));
            std::vector<XrViewConfigurationType> viewConfigTypes;
            viewConfigTypes.resize(viewConfigCount);
            CHECK_XR_RESULT(
                xrEnumerateViewConfigurations(instance, systemId, viewConfigCount, &viewConfigCount, viewConfigTypes.data()));
            auto viewConfigType = viewConfigTypes[0];
            if (viewConfigType != XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO) {
                throw std::runtime_error("Example only supports stereo-based HMD rendering");
            }
            //xr::ViewConfigurationProperties viewConfigProperties =
            //    instance.getViewConfigurationProperties(systemId, viewConfigType);
            //logging::log(logging::Level::Info, fmt::format(""));
        }

        uint32_t viewConfigCount = 0;
        CHECK_XR_RESULT(xrEnumerateViewConfigurationViews(instance, systemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
                                                          viewConfigCount, &viewConfigCount, nullptr));
        std::vector<XrViewConfigurationView> viewConfigViews;
        viewConfigViews.resize(viewConfigCount);
        CHECK_XR_RESULT(xrEnumerateViewConfigurationViews(instance, systemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
                                                          viewConfigCount, &viewConfigCount, nullptr));

        // Instead of createing a swapchain per-eye, we create a single swapchain of double width.
        // Even preferable would be to create a swapchain texture array with one layer per eye, so that we could use the
        // VK_KHR_multiview to render both eyes with a single set of draws, but sadly the Oculus runtime doesn't currently
        // support texture array swapchains
        if (viewConfigViews.size() != 2) {
            throw std::runtime_error("Unexpected number of view configurations");
        }

        if (viewConfigViews[0].recommendedImageRectHeight != viewConfigViews[1].recommendedImageRectHeight) {
            throw std::runtime_error("Per-eye images have different recommended heights");
        }

        renderTargetSize = { viewConfigViews[0].recommendedImageRectWidth * 2, viewConfigViews[0].recommendedImageRectHeight };

        CHECK_XR_RESULT(pxrGetOpenGLGraphicsRequirementsKHR(instance, systemId, &graphicsRequirements));
    }

    SDL_Window* window;
    SDL_GLContext context;
    glm::uvec2 windowSize;
    void prepareWindow() {
        assert(renderTargetSize.x != 0 && renderTargetSize.y != 0);
        windowSize = renderTargetSize;
        windowSize /= 4;

        SDL_Init(SDL_INIT_VIDEO);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
        SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, XR_VERSION_MAJOR(graphicsRequirements.maxApiVersionSupported));
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, XR_VERSION_MINOR(graphicsRequirements.maxApiVersionSupported));
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);

        window = SDL_CreateWindow("", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, windowSize.x, windowSize.y,
                                  SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
        context = SDL_GL_CreateContext(window);
        SDL_GL_MakeCurrent(window, context);
        SDL_GL_SetSwapInterval(0);
        gladLoadGL();
        glDebugMessageCallback(debugMessageCallback, NULL);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    }

    XrSession session;
    void prepareXrSession() {
        XrGraphicsBindingOpenGLWin32KHR graphicsBinding{ XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR, nullptr, wglGetCurrentDC(),
                                                         wglGetCurrentContext() };
        XrSessionCreateInfo sci{ XR_TYPE_SESSION_CREATE_INFO, &graphicsBinding, 0, systemId };
        CHECK_XR_RESULT(xrCreateSession(instance, &sci, &session));

        uint32_t referenceSpaceCount{ 0 };
        CHECK_XR_RESULT(xrEnumerateReferenceSpaces(session, referenceSpaceCount, &referenceSpaceCount, nullptr));
        std::vector<XrReferenceSpaceType> referenceSpaces;
        referenceSpaces.resize(referenceSpaceCount);
        CHECK_XR_RESULT(xrEnumerateReferenceSpaces(session, referenceSpaceCount, &referenceSpaceCount, referenceSpaces.data()));

        XrReferenceSpaceCreateInfo rsci{ XR_TYPE_REFERENCE_SPACE_CREATE_INFO, nullptr };
        CHECK_XR_RESULT(xrCreateReferenceSpace(session, &rsci, &space));

        uint32_t swapchainFormatCount{ 0 };
        CHECK_XR_RESULT(xrEnumerateSwapchainFormats(session, swapchainFormatCount, &swapchainFormatCount, nullptr));
        std::vector<int64_t> swapchainFormats;
        swapchainFormats.resize(swapchainFormatCount);
        CHECK_XR_RESULT(
            xrEnumerateSwapchainFormats(session, swapchainFormatCount, &swapchainFormatCount, swapchainFormats.data()));
        for (const auto& format : swapchainFormats) {
            LOG_INFO("\t{}", formatToString((GLenum)format));
        }
    }

    XrSwapchainCreateInfo swapchainCreateInfo;
    XrSwapchain swapchain;
    std::vector<XrSwapchainImageOpenGLKHR> swapchainImages;
    void prepareXrSwapchain() {
        swapchainCreateInfo = XrSwapchainCreateInfo{ XR_TYPE_SWAPCHAIN_CREATE_INFO, nullptr };
        swapchainCreateInfo.usageFlags = XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT;
        swapchainCreateInfo.format = (int64_t)GL_SRGB8_ALPHA8;
        swapchainCreateInfo.sampleCount = 1;
        swapchainCreateInfo.arraySize = 1;
        swapchainCreateInfo.faceCount = 1;
        swapchainCreateInfo.mipCount = 1;
        swapchainCreateInfo.width = renderTargetSize.x;
        swapchainCreateInfo.height = renderTargetSize.y;

        CHECK_XR_RESULT(xrCreateSwapchain(session, &swapchainCreateInfo, &swapchain));
        uint32_t swapchainImageCount{ 0 };
        CHECK_XR_RESULT(xrEnumerateSwapchainImages(swapchain, swapchainImageCount, &swapchainImageCount, nullptr));
        swapchainImages.resize(swapchainImageCount);
        CHECK_XR_RESULT(xrEnumerateSwapchainImages(swapchain, swapchainImageCount, &swapchainImageCount,
                                                   (XrSwapchainImageBaseHeader*)swapchainImages.data()));
    }

    std::array<XrCompositionLayerProjectionView, 2> projectionLayerViews;
    XrCompositionLayerProjection projectionLayer{ XR_TYPE_COMPOSITION_LAYER_PROJECTION, nullptr, 0, 0, 2,
                                                  projectionLayerViews.data() };
    XrSpace& space{ projectionLayer.space };
    std::vector<XrCompositionLayerBaseHeader*> layersPointers;
    void prepareXrCompositionLayers() {
        //session.getReferenceSpaceBoundsRect(xr::ReferenceSpaceType::Local, bounds);
        projectionLayer.viewCount = 2;
        projectionLayer.views = projectionLayerViews.data();
        layersPointers.push_back((XrCompositionLayerBaseHeader*)&projectionLayer);
        // Finish setting up the layer submission
        xrs::for_each_side_index([&](uint32_t eyeIndex) {
            auto& layerView = projectionLayerViews[eyeIndex];
            layerView.subImage.swapchain = swapchain;
            layerView.subImage.imageRect.extent = { (int32_t)renderTargetSize.x / 2, (int32_t)renderTargetSize.y };
            if (eyeIndex == 1) {
                layerView.subImage.imageRect.offset.x = layerView.subImage.imageRect.extent.width;
            }
        });
    }

    struct GLFBO {
        GLuint id{ 0 };
        GLuint depthBuffer{ 0 };
    } fbo;

    void prepareGlFramebuffer() {
        // Create a depth renderbuffer compatible with the Swapchain sample count and size
        glGenRenderbuffers(1, &fbo.depthBuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, fbo.depthBuffer);
        if (swapchainCreateInfo.sampleCount == 1) {
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, swapchainCreateInfo.width, swapchainCreateInfo.height);
        } else {
            glRenderbufferStorageMultisample(GL_RENDERBUFFER, swapchainCreateInfo.sampleCount, GL_DEPTH24_STENCIL8,
                                             swapchainCreateInfo.width, swapchainCreateInfo.height);
        }

        // Create a framebuffer and attach the depth buffer to it
        glGenFramebuffers(1, &fbo.id);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo.id);
        glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, fbo.depthBuffer);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    }

    //////////////////////////////////////
    // Per-frame work                   //
    //////////////////////////////////////
    void frame() {
        pollSdlEvents();
        pollXrEvents();
        if (quit) {
            return;
        }
        if (startXrFrame()) {
            updateXrViews();
            if (frameState.shouldRender) {
                render();
            }
            endXrFrame();
        }
    }

    void pollSdlEvents() {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_KEYUP:
                    if (event.key.keysym.sym == SDLK_ESCAPE) {
                        quit = true;
                    }
                    break;
            }
        }
    }

    void pollXrEvents() {
        while (true) {
            XrEventDataBuffer eventBuffer;
            auto pollResult = xrPollEvent(instance, &eventBuffer);
            if (pollResult == XR_EVENT_UNAVAILABLE) {
                break;
            }

            switch (eventBuffer.type) {
                case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED:
                    onSessionStateChanged(reinterpret_cast<XrEventDataSessionStateChanged&>(eventBuffer));
                    break;

                default:
                    break;
            }
        }
    }

    XrSessionState sessionState{ XR_SESSION_STATE_IDLE };
    void onSessionStateChanged(const XrEventDataSessionStateChanged& sessionStateChangedEvent) {
        sessionState = sessionStateChangedEvent.state;
        switch (sessionState) {
            case XR_SESSION_STATE_READY:
                if (!quit) {
                    XrSessionBeginInfo sbi{ XR_TYPE_SESSION_BEGIN_INFO, nullptr, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO };
                    CHECK_XR_RESULT(xrBeginSession(session, &sbi));
                }
                break;

            case XR_SESSION_STATE_STOPPING:
                xrEndSession(session);
                session = nullptr;
                quit = true;
                break;

            default:
                break;
        }
    }

    XrFrameState frameState;
    XrResult waitFrameResult;
    bool startXrFrame() {
        XrFrameWaitInfo frameWaitInfo{ XR_TYPE_FRAME_WAIT_INFO, nullptr };
        switch (sessionState) {
            case XR_SESSION_STATE_READY:
            case XR_SESSION_STATE_FOCUSED:
            case XR_SESSION_STATE_SYNCHRONIZED:
            case XR_SESSION_STATE_VISIBLE:
                return XR_UNQUALIFIED_SUCCESS(xrWaitFrame(session, &frameWaitInfo, &frameState));

            default:
                break;
        }

        return false;
    }

    void endXrFrame() {
        XrFrameEndInfo frameEndInfo{ XR_TYPE_FRAME_END_INFO, nullptr, frameState.predictedDisplayTime,
                                     XR_ENVIRONMENT_BLEND_MODE_OPAQUE };
        if (frameState.shouldRender) {
            xrs::for_each_side_index([&](uint32_t eyeIndex) {
                auto& layerView = projectionLayerViews[eyeIndex];
                const auto& eyeView = eyeViewStates[eyeIndex];
                layerView.fov = eyeView.fov;
                layerView.pose = eyeView.pose;
            });
            frameEndInfo.layerCount = (uint32_t)layersPointers.size();
            frameEndInfo.layers = layersPointers.data();
        }
        xrEndFrame(session, &frameEndInfo);
    }

    std::vector<XrView> eyeViewStates;
    std::array<glm::mat4, 2> eyeViews;
    std::array<glm::mat4, 2> eyeProjections;
    void updateXrViews() {
        XrViewState vs;
        XrViewLocateInfo vi{ XR_TYPE_VIEW_LOCATE_INFO, nullptr, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
                             frameState.predictedDisplayTime, space };
        uint32_t eyeViewStateCount{ 0 };
        CHECK_XR_RESULT(xrLocateViews(session, &vi, &vs, eyeViewStateCount, &eyeViewStateCount, nullptr));
        eyeViewStates.resize(eyeViewStateCount);
        CHECK_XR_RESULT(xrLocateViews(session, &vi, &vs, eyeViewStateCount, &eyeViewStateCount, eyeViewStates.data()));
        xrs::for_each_side_index([&](size_t eyeIndex) {
            const auto& viewState = eyeViewStates[eyeIndex];
            eyeProjections[eyeIndex] = xrs::toGlm(viewState.fov);
            eyeViews[eyeIndex] = glm::inverse(xrs::toGlm(viewState.pose));
        });
    }

    void render() {
        uint32_t swapchainIndex;

        XrSwapchainImageAcquireInfo ai{ XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO, nullptr };
        CHECK_XR_RESULT(xrAcquireSwapchainImage(swapchain, &ai, &swapchainIndex));

        XrSwapchainImageWaitInfo wi{ XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO, nullptr, XR_INFINITE_DURATION };
        CHECK_XR_RESULT(xrWaitSwapchainImage(swapchain, &wi));

        glBindFramebuffer(GL_FRAMEBUFFER, fbo.id);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, swapchainImages[swapchainIndex].image, 0);

        // "render" to the swapchain image
        glEnable(GL_SCISSOR_TEST);
        glScissor(0, 0, renderTargetSize.x / 2, renderTargetSize.y);
        glClearColor(0, 1, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glScissor(renderTargetSize.x / 2, 0, renderTargetSize.x / 2, renderTargetSize.y);
        glClearColor(0, 0, 1, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // fast blit from the fbo to the window surface
        glDisable(GL_SCISSOR_TEST);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(0, 0, renderTargetSize.x, renderTargetSize.y, 0, 0, windowSize.x, windowSize.y, GL_COLOR_BUFFER_BIT,
                          GL_NEAREST);

        glFramebufferTexture(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 0, 0);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

        XrSwapchainImageReleaseInfo ri{ XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO, nullptr };
        CHECK_XR_RESULT(xrReleaseSwapchainImage(swapchain, &ri));

        SDL_GL_SwapWindow(window);
    }

    //////////////////////////////////////
    // Shutdown                         //
    //////////////////////////////////////
    void destroy() {
        if (fbo.id != 0) {
            glDeleteFramebuffers(1, &fbo.id);
            fbo.id = 0;
        }

        if (fbo.depthBuffer != 0) {
            glDeleteRenderbuffers(1, &fbo.depthBuffer);
            fbo.depthBuffer = 0;
        }

        if (swapchain) {
            xrDestroySwapchain(swapchain);
            swapchain = nullptr;
        }
        if (session) {
            xrDestroySession(session);
            session = nullptr;
        }

        SDL_GL_DeleteContext(context);
        SDL_DestroyWindow(window);

        if (messenger) {
            pxrDestroyDebugUtilsMessengerEXT(messenger);
            messenger = nullptr;
        }
        if (instance) {
            xrDestroyInstance(instance);
            instance = nullptr;
        }

        SDL_Quit();
    }
};

int main(int argc, char* argv[]) {
    try {
        OpenXrExample().run();
    } catch (const std::exception& err) {
        logging::log(logging::Level::Error, err.what());
    }
    return 0;
}
