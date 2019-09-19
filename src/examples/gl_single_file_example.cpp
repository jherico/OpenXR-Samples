//
//  Created by Bradley Austin Davis on 2019/09/18
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#define XR_USE_GRAPHICS_API_OPENGL
#define SUPPRESS_DEBUG_UTILS
#define _CRT_SECURE_NO_WARNINGS

#if defined(WIN32)
#define XR_USE_PLATFORM_WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#elif defined(__ANDROID__)
#define XR_USE_PLATFORM_ANDROID
#elif
#define XR_USE_PLATFORM_XLIB
#endif

#include <cstdint>

#include <unordered_map>
#include <functional>
#include <sstream>
#include <iostream>
#include <chrono>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>

#include <openxr/openxr.hpp>
#include <openxr/openxr_platform.h>

#include <gl.hpp>
#include <glfw.hpp>
#include <logging.hpp>

namespace xrs {

namespace DebugUtilsEXT {

using MessageSeverityFlagBits = xr::DebugUtilsMessageSeverityFlagBitsEXT;
using MessageTypeFlagBits = xr::DebugUtilsMessageTypeFlagBitsEXT;
using MessageSeverityFlags = xr::DebugUtilsMessageSeverityFlagsEXT;
using MessageTypeFlags = xr::DebugUtilsMessageTypeFlagsEXT;
using CallbackData = xr::DebugUtilsMessengerCallbackDataEXT;
using Messenger = xr::DebugUtilsMessengerEXT;

// Raw C callback
static XrBool32 debugCallback(XrDebugUtilsMessageSeverityFlagsEXT sev_,
                              XrDebugUtilsMessageTypeFlagsEXT type_,
                              const XrDebugUtilsMessengerCallbackDataEXT* data_,
                              void* userData) {
    LOG_FORMATTED((logging::Level)sev_, "{}: message", data_->functionName, data_->message);
    return XR_TRUE;
}

Messenger create(const xr::Instance& instance,
                 const MessageSeverityFlags& severityFlags = MessageSeverityFlagBits::AllBits,
                 const MessageTypeFlags& typeFlags = MessageTypeFlagBits::AllBits,
                 void* userData = nullptr) {
    return instance.createDebugUtilsMessengerEXT({ severityFlags, typeFlags, debugCallback, userData },
                                                 xr::DispatchLoaderDynamic{ instance });
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

struct FrameCounter {
    using time_point = std::chrono::time_point<std::chrono::steady_clock>;
    int64_t counter{ 0 };
    int64_t lastFpsCounter{ 0 };
    int64_t elapsed{ 0 };
    int64_t lastFpsElapsed{ 0 };
    time_point startTime{ std::chrono::high_resolution_clock::now() };

    float lastFps{ 0.0f };
    static constexpr size_t NS_TO_MS{ 1000000ULL };
    static constexpr size_t MIN_CALC_NS{ NS_TO_MS * 10 };
    static constexpr size_t MIN_CALC_FRAMES{ 3 };

    void reset() {
        counter = 0;
        elapsed = 0;
        lastFpsCounter = 0;
        lastFpsElapsed = 0;
        startTime = { std::chrono::high_resolution_clock::now() };
    }

    bool updateFps() {
        auto fpsElapsedNS = elapsed - lastFpsElapsed;
        auto fpsCount = counter - lastFpsCounter;
        if ((fpsElapsedNS > MIN_CALC_NS) && (fpsCount > MIN_CALC_FRAMES)) {
            float fpsElapsed = (float)(fpsElapsedNS / NS_TO_MS) / 1000.0f;
            lastFpsCounter = counter;
            lastFpsElapsed = elapsed;
            lastFps = ((float)fpsCount / fpsElapsed);
            return true;
        }
        return false;
    }

    int64_t nextFrame() {
        auto endTime = std::chrono::high_resolution_clock::now();
        ++counter;
        auto interval = std::chrono::duration<int64_t, std::nano>(endTime - startTime).count();
        elapsed += interval;
        startTime = endTime;
        return interval;
    }
};

struct OpenXrExample {
    FrameCounter frameCounter;

    // Application main function
    void run() {
        // Startup work
        prepare();

        // Loop
        window.runWindowLoop([&] { frame(); });

        // Teardown work
        destroy();
    }

    //////////////////////////////////////
    // One-time setup work              //
    //////////////////////////////////////
    void prepare() {
        prepareXrInstance();
        prepareWindow();
        prepareXrSession();
        prepareXrSwapchain();
        prepareXrCompositionLayers();
        prepareGlFramebuffer();
    }

    bool enableDebug{ true };
    xr::Instance instance;
    xr::SystemId systemId;
    xr::DispatchLoaderDynamic dispatch;
    glm::uvec2 renderTargetSize;
	xrs::DebugUtilsEXT::Messenger messenger;
    xr::GraphicsRequirementsOpenGLKHR graphicsRequirements;
    void prepareXrInstance() {
        std::unordered_map<std::string, xr::ExtensionProperties> discoveredExtensions;
        for (const auto& extensionProperties : xr::enumerateInstanceExtensionProperties(nullptr)) {
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

        {
            xr::InstanceCreateInfo ici{ {},
                                        { "gl_single_file_example", 0, "openXrSamples", 0, xr::Version::current() },
                                        0,
                                        nullptr,
                                        (uint32_t)requestedExtensions.size(),
                                        requestedExtensions.data() };

            xr::DebugUtilsMessengerCreateInfoEXT dumci;
            if (enableDebug) {
                dumci.messageSeverities = xr::DebugUtilsMessageSeverityFlagBitsEXT::AllBits;
                dumci.messageTypes = xr::DebugUtilsMessageTypeFlagBitsEXT::AllBits;
                dumci.userData = this;
                dumci.userCallback = &xrs::DebugUtilsEXT::debugCallback;
                ici.next = &dumci;
            }

            // Create the actual instance
            instance = xr::createInstance(ici);

            // Turn on debug logging
            if (enableDebug) {
                messenger = xrs::DebugUtilsEXT::create(instance);
            }
        }

        // Having created the isntance, the very first thing to do is populate the dynamic dispatch, loading
        // all the available functions from the runtime
        dispatch = xr::DispatchLoaderDynamic::createFullyPopulated(instance, &xrGetInstanceProcAddr);
        // Log the instance properties
        {
            // Now we want to fetch the instance properties
            xr::InstanceProperties instanceProperties = instance.getInstanceProperties();
            LOG_INFO("OpenXR Runtime {} version {}.{}.{}",  //
                     (const char*)instanceProperties.runtimeName, (uint32_t)instanceProperties.runtimeVersion.major,
                     (uint32_t)instanceProperties.runtimeVersion.minor, (uint32_t)instanceProperties.runtimeVersion.patch);
        }

        // We want to create an HMD example, so we ask for a runtime that supposts that form factor
        // and get a response in the form of a systemId
        systemId = instance.getSystem(xr::SystemGetInfo{ xr::FormFactor::HeadMountedDisplay });

        // Log the system properties
        {
            xr::SystemProperties systemProperties = instance.getSystemProperties(systemId);
            LOG_INFO("OpenXR System {} max layers {} max swapchain image size {}x{}",  //
                     (const char*)systemProperties.systemName, (uint32_t)systemProperties.graphicsProperties.maxLayerCount,
                     (uint32_t)systemProperties.graphicsProperties.maxSwapchainImageWidth,
                     (uint32_t)systemProperties.graphicsProperties.maxSwapchainImageHeight);
        }

        // Find out what view configurations we have available
        {
            auto viewConfigTypes = instance.enumerateViewConfigurations(systemId);
            auto viewConfigType = viewConfigTypes[0];
            if (viewConfigType != xr::ViewConfigurationType::PrimaryStereo) {
                throw std::runtime_error("Example only supports stereo-based HMD rendering");
            }
            //xr::ViewConfigurationProperties viewConfigProperties =
            //    instance.getViewConfigurationProperties(systemId, viewConfigType);
            //logging::log(logging::Level::Info, fmt::format(""));
        }

        std::vector<xr::ViewConfigurationView> viewConfigViews =
            instance.enumerateViewConfigurationViews(systemId, xr::ViewConfigurationType::PrimaryStereo);

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

        graphicsRequirements = instance.getOpenGLGraphicsRequirementsKHR(systemId, dispatch);
    }

    glfw::Window window;
    glm::uvec2 windowSize;
    void prepareWindow() {
        assert(renderTargetSize.x != 0 && renderTargetSize.y != 0);
        windowSize = renderTargetSize;
        windowSize /= 4;
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, graphicsRequirements.maxApiVersionSupported.major);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, graphicsRequirements.maxApiVersionSupported.minor);
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
        window.createWindow(windowSize);

        window.makeCurrent();
        window.setSwapInterval(0);

        // Initialize GLAD
        gl::init();
        gl::report();
        // Make sure we get GL errors reported
        gl::setupDebugLogging();
    }

    xr::Session session;
    void prepareXrSession() {
        xr::GraphicsBindingOpenGLWin32KHR graphicsBinding{ GetDC(window.getNativeWindowHandle()),
                                                           window.getNativeContextHandle() };
        xr::SessionCreateInfo sci{ {}, systemId };
        sci.next = &graphicsBinding;
        session = instance.createSession(sci);

        auto referenceSpaces = session.enumerateReferenceSpaces();
        space = session.createReferenceSpace(xr::ReferenceSpaceCreateInfo{ xr::ReferenceSpaceType::Local });
    }

    xr::SwapchainCreateInfo swapchainCreateInfo;
    xr::Swapchain swapchain;
    std::vector<xr::SwapchainImageOpenGLKHR> swapchainImages;
    void prepareXrSwapchain() {
        swapchainCreateInfo.usageFlags = xr::SwapchainUsageFlagBits::TransferDst;
        swapchainCreateInfo.format = (int64_t)GL_SRGB8_ALPHA8;
        swapchainCreateInfo.sampleCount = 1;
        swapchainCreateInfo.arraySize = 1;
        swapchainCreateInfo.faceCount = 1;
        swapchainCreateInfo.mipCount = 1;
        swapchainCreateInfo.width = renderTargetSize.x;
        swapchainCreateInfo.height = renderTargetSize.y;

        swapchain = session.createSwapchain(swapchainCreateInfo);

        swapchainImages = swapchain.enumerateSwapchainImages<xr::SwapchainImageOpenGLKHR>();
    }

    std::array<xr::CompositionLayerProjectionView, 2> projectionLayerViews;
    xr::CompositionLayerProjection projectionLayer{ {}, {}, 2, projectionLayerViews.data() };
    xr::Space& space{ projectionLayer.space };
    std::vector<xr::CompositionLayerBaseHeader*> layersPointers;
    void prepareXrCompositionLayers() {
        //session.getReferenceSpaceBoundsRect(xr::ReferenceSpaceType::Local, bounds);
        projectionLayer.viewCount = 2;
        projectionLayer.views = projectionLayerViews.data();
        layersPointers.push_back(&projectionLayer);
        // Finish setting up the layer submission
        xr::for_each_side_index([&](uint32_t eyeIndex) {
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
        // Use the frame counter to get the time since the last frame
        auto intervalNS = frameCounter.nextFrame();

        pollXrEvents();
        if (startXrFrame()) {
            updateXrViews();
            if (frameState.shouldRender) {
                render();
            }
            endXrFrame();
            if (frameCounter.updateFps()) {
                static const std::string device = (const char*)glGetString(GL_VERSION);
                window.setTitle(fmt::format("OpenXR SDK Example {} - {} fps", device, frameCounter.lastFps));
            }
        }
    }

    void pollXrEvents() {
        while (true) {
            xr::EventDataBuffer eventBuffer;
            auto pollResult = instance.pollEvent(eventBuffer);
            if (pollResult == xr::Result::EventUnavailable) {
                break;
            }

            switch (eventBuffer.type) {
                case xr::StructureType::EventDataSessionStateChanged:
                    onSessionStateChanged(reinterpret_cast<xr::EventDataSessionStateChanged&>(eventBuffer));
                    break;

                default:
                    break;
            }
        }
    }

    xr::SessionState sessionState{ xr::SessionState::Idle };
    void onSessionStateChanged(const xr::EventDataSessionStateChanged& sessionStateChangedEvent) {
        sessionState = sessionStateChangedEvent.state;
        switch (sessionState) {
            case xr::SessionState::Ready:
                if (!window.shouldClose()) {
                    session.beginSession(xr::SessionBeginInfo{ xr::ViewConfigurationType::PrimaryStereo });
                }
                break;

            case xr::SessionState::Stopping:
                session.endSession();
                window.requestClose();
                break;

            default:
                break;
        }
    }

    xr::FrameState frameState;
    bool startXrFrame() {
        switch (sessionState) {
            case xr::SessionState::Focused:
            case xr::SessionState::Synchronized:
            case xr::SessionState::Visible:
                session.waitFrame(xr::FrameWaitInfo{}, frameState);
                return xr::Result::Success == session.beginFrame(xr::FrameBeginInfo{});

            default:
                break;
        }

        return false;
    }

    void endXrFrame() {
        xr::FrameEndInfo frameEndInfo{ frameState.predictedDisplayTime, xr::EnvironmentBlendMode::Opaque };
        if (frameState.shouldRender) {
            xr::for_each_side_index([&](uint32_t eyeIndex) {
                auto& layerView = projectionLayerViews[eyeIndex];
                const auto& eyeView = eyeViewStates[eyeIndex];
                layerView.fov = eyeView.fov;
                layerView.pose = eyeView.pose;
            });
            frameEndInfo.layerCount = (uint32_t)layersPointers.size();
            frameEndInfo.layers = layersPointers.data();
        }
        session.endFrame(frameEndInfo);
    }

    std::vector<xr::View> eyeViewStates;
    std::array<glm::mat4, 2> eyeViews;
    std::array<glm::mat4, 2> eyeProjections;
    void updateXrViews() {
        xr::ViewState vs;
        xr::ViewLocateInfo vi{ xr::ViewConfigurationType::PrimaryStereo, frameState.predictedDisplayTime, space };
        eyeViewStates = session.locateViews(vi, &(vs.operator XrViewState&()));
        xr::for_each_side_index([&](size_t eyeIndex) {
            const auto& viewState = eyeViewStates[eyeIndex];
            eyeProjections[eyeIndex] = xrs::toGlm(viewState.fov);
            eyeViews[eyeIndex] = glm::inverse(xrs::toGlm(viewState.pose));
        });
    }

    void render() {
        uint32_t swapchainIndex;

        swapchain.acquireSwapchainImage(xr::SwapchainImageAcquireInfo{}, &swapchainIndex);
        swapchain.waitSwapchainImage(xr::SwapchainImageWaitInfo{ xr::Duration::infinite() });

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

        swapchain.releaseSwapchainImage(xr::SwapchainImageReleaseInfo{});

        window.swapBuffers();
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
            swapchain.destroy();
            swapchain = nullptr;
        }
        if (session) {
            session.destroy();
            session = nullptr;
        }
		if (messenger) {
			messenger.destroy(dispatch);
		}
        if (instance) {
            instance.destroy();
            instance = nullptr;
        }
    }
};

int main(const int argc, const char* argv[]) {
    try {
        OpenXrExample().run();
    } catch (const std::exception& err) {
        logging::log(logging::Level::Error, err.what());
    }
    return 0;
}
