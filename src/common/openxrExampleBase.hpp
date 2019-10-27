#include "common.hpp"

// Make sure we have precisely ONE graphics API defined
// clang-format off
#if defined(XR_USE_GRAPHICS_API_OPENGL) 
    #if (defined(XR_USE_GRAPHICS_API_OPENGL_ES) || defined(XR_USE_GRAPHICS_API_VULKAN) || defined(XR_USE_GRAPHICS_API_D3D11) || defined(XR_USE_GRAPHICS_API_D3D12))
        #error "Only one graphics API may be used at a time"
    #endif
#elif defined(XR_USE_GRAPHICS_API_OPENGL_ES) 
    #if (defined(XR_USE_GRAPHICS_API_OPENGL) || defined(XR_USE_GRAPHICS_API_VULKAN) || defined(XR_USE_GRAPHICS_API_D3D11) || defined(XR_USE_GRAPHICS_API_D3D12))
        #error "Only one graphics API may be used at a time"
    #endif
#elif defined(XR_USE_GRAPHICS_API_VULKAN) 
    #if (defined(XR_USE_GRAPHICS_API_OPENGL) || defined(XR_USE_GRAPHICS_API_OPENGL_ES) || defined(XR_USE_GRAPHICS_API_D3D11) || defined(XR_USE_GRAPHICS_API_D3D12))
        #error "Only one graphics API may be used at a time"
    #endif
#elif defined(XR_USE_GRAPHICS_API_D3D11) 
    #if (defined(XR_USE_GRAPHICS_API_OPENGL) || defined(XR_USE_GRAPHICS_API_OPENGL_ES) || defined(XR_USE_GRAPHICS_API_VULKAN) || defined(XR_USE_GRAPHICS_API_D3D12))
        #error "Only one graphics API may be used at a time"
    #endif
#elif defined(XR_USE_GRAPHICS_API_D3D12) 
    #if (defined(XR_USE_GRAPHICS_API_OPENGL) || defined(XR_USE_GRAPHICS_API_OPENGL_ES) || defined(XR_USE_GRAPHICS_API_VULKAN) || defined(XR_USE_GRAPHICS_API_D3D11))
        #error "Only one graphics API may be used at a time"
    #endif
#else
    #error "You must specify a graphics API with the XR_USE_GRAPHICS_API_(VULKAN|OPENGL|OPENGL_ES|D3D11|D3D12) preprocessor define"
#endif
// clang-format on

#if defined(XR_USE_GRAPHICS_API_VULKAN)
#include <vulkan/vulkan.hpp>
#include <vks/context.hpp>
#endif

#include <xrs/context.hpp>
#include <xrs/swapchain.hpp>
#include <gl/framebuffer.hpp>
#include <gl/debug.hpp>
#include <interfaces.hpp>
#include <assets.hpp>
#include <glad.hpp>

namespace xr_examples {

template <typename WindowType, typename FramebufferType, typename SceneType>
class OpenXrExampleBase {
public:
#if defined(XR_USE_GRAPHICS_API_VULKAN)
    vks::Context& vkContext{ vks::Context::get() };
#endif

    uint32_t frameCounter{ 0 };
    xr::Extent2Di renderTargetSize;
    EyeStates eyeStates;
    float fpsTimer{ 0.0f };
    float lastFps{ 0.0f };
    float nearZ{ 0.01f };
    float farZ{ 1000.0f };

    OpenXrExampleBase() {
        // Static initialization for whatever our backed (Qt, Magnum, etc) needs
        WindowType::init();
    }

    virtual ~OpenXrExampleBase() {
#if !defined(DISABLE_XR)
        xrContext.destroy();
#endif

#if defined(XR_USE_GRAPHICS_API_VULKAN)
        vkContext.destroy();
#endif
    }

    virtual void prepare() {
        prepareXrInstance();
        prepareWindow();
        prepareXrSession();
        prepareXrSpaces();
        prepareXrActions();
        preapreXrLayers();
        prepareScene();
    }

    xrs::Context xrContext;
    void prepareXrInstance() {
        xrContext.requiredExtensions.insert(XR_KHR_COMPOSITION_LAYER_DEPTH_EXTENSION_NAME);
        // Startup the OpenXR instance and get a system ID and view configuration
        // All of this is independent of the interaction between Xr and the
        // eventual Graphics API used for rendering
        xrContext.create();

        const auto& viewConfigViews = xrContext.viewConfigViews;

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

        renderTargetSize = { (int32_t)viewConfigViews[0].recommendedImageRectWidth * 2,
                             (int32_t)viewConfigViews[0].recommendedImageRectHeight };
    }

    WindowType window;
    FramebufferType framebuffer;
    void prepareWindow() {
        assert(renderTargetSize.width != 0 && renderTargetSize.height != 0);
        window.create({ renderTargetSize.width / 4, renderTargetSize.height / 4 });

#if defined(XR_USE_GRAPHICS_API_OPENGL)
        window.makeCurrent();
        window.setSwapInterval(0);
        glad::init();
        gl::enableDebugLogging();
#endif

        framebuffer.create(renderTargetSize);
    }

    SceneType scene;
    virtual void prepareScene() {
        scene.create();
        scene.loadModel(assets::getAssetPathString("models/2CylinderEngine.glb"));
        scene.setCubemap(assets::getAssetPathString("yokohama.basis"));
    }

    xr::Session& xrSession{ xrContext.session };
    xr::Extent2Df bounds;
    void prepareXrSession() {
#if defined(XR_USE_GRAPHICS_API_VULKAN)
        auto requirements = xrContext.instance.getVulkanGraphicsRequirementsKHR(xrContext.systemId, xrContext.dispatch);

        // Set up interaction between OpenXR and Vulkan
        // This work MUST happen before you create a Vulkan instance, since
        // OpenXR may require specific Vulkan instance and device extensions
        vkContext.requireExtensions(xrContext.getVulkanInstanceExtensions());
        vkContext.requireDeviceExtensions(xrContext.getVulkanDeviceExtensions());

        // Our example Vulkan abstraction allows a client to select a specific vk::PhysicalDevice via
        // a `DevicePicker` callback
        // This is critical because the HMD will ultimately be dependent on the specific GPU to which it is
        // attached
        // Note that we don't and can't actually determine the target vk::PhysicalDevice *right now* because
        // vk::Instance::getVulkanGraphicsDeviceKHR depends on a passed VkInstance, which hasn't been created yet.
        vkContext.setDevicePicker([this](const std::vector<vk::PhysicalDevice>& availableDevices) -> vk::PhysicalDevice {
            vk::PhysicalDevice targetDevice;
            {
                VkPhysicalDevice targetDeviceRaw;
                xrContext.instance.getVulkanGraphicsDeviceKHR(xrContext.systemId, vkContext.instance, &targetDeviceRaw,
                                                              xrContext.dispatch);
                targetDevice = targetDeviceRaw;
            }
            for (const auto& availableDevice : availableDevices) {
                if (availableDevice == targetDevice) {
                    return availableDevice;
                }
            }
            throw std::runtime_error("Requested device not found");
        });

        vkContext.createInstance();
        vkContext.createDevice();
        xr::GraphicsBindingVulkanKHR binding{ vkContext.instance, vkContext.physicalDevice, vkContext.device,
                                              vkContext.queueIndices.graphics, 0 };
        xrContext.createSession(binding);
#endif

#if defined(XR_USE_GRAPHICS_API_OPENGL) || defined(XR_USE_GRAPHICS_API_OPENGL_ES)
        auto requirements = xrContext.instance.getOpenGLGraphicsRequirementsKHR(xrContext.systemId, xrContext.dispatch);
        xrContext.createSession(xr::GraphicsBindingOpenGLWin32KHR{ wglGetCurrentDC(), wglGetCurrentContext() });
#endif
        xrSession.getReferenceSpaceBoundsRect(xr::ReferenceSpaceType::Local, bounds);
    }

    xr::Space space;
    void prepareXrSpaces() {
        space = xrSession.createReferenceSpace(xr::ReferenceSpaceCreateInfo{ xr::ReferenceSpaceType::Local });
    }

    std::vector<xr::CompositionLayerBaseHeader*> layersPointers;

#define USE_DEPTH_INFO 0
    std::array<xr::CompositionLayerProjectionView, 2> projectionLayerViews;
    xr::CompositionLayerProjection projectionLayer{ {}, {}, 2, projectionLayerViews.data() };
    xr::Swapchain projectionColorSwapchain;

#if USE_DEPTH_INFO
    std::array<xr::CompositionLayerDepthInfoKHR, 2> projectionDeptInfos;
    xr::Swapchain projectionDepthSwapchain;
#endif

    gl::SwapchainFramebuffer projectionFramebuffer;

    void preapreXrLayers() {
        xr::SwapchainCreateInfo ci;
        ci.usageFlags = xr::SwapchainUsageFlagBits::TransferDst;
        ci.format = xrs::DEFAULT_SWAPCHAIN_FORMAT;
        ci.width = (uint32_t)renderTargetSize.width;
        ci.height = (uint32_t)renderTargetSize.height;
        ci.arraySize = 1;
        ci.sampleCount = 1;
        ci.faceCount = 1;
        ci.mipCount = 1;

        projectionColorSwapchain = xrSession.createSwapchain(ci);
        projectionFramebuffer.setSwapchain(projectionColorSwapchain);

#if USE_DEPTH_INFO
        ci.format = xrs::DEFAULT_SWAPCHAIN_DEPTH_FORMAT;
        projectionDepthSwapchain = xrSession.createSwapchain(ci);
        projectionFramebuffer.setDepthSwapchain(projectionDepthSwapchain);
#endif

        projectionFramebuffer.create(renderTargetSize);
        projectionLayer.space = space;
        // Finish setting up the layer submission
        xr::for_each_side_index([&](uint32_t eyeIndex) {
            xr::Rect2Di imageRect;
            imageRect.extent = { (int32_t)renderTargetSize.width / 2, (int32_t)renderTargetSize.height };
            if (eyeIndex == 1) {
                imageRect.offset.x = imageRect.extent.width;
            }

            auto& layerView = projectionLayerViews[eyeIndex];
            layerView.subImage.swapchain = projectionColorSwapchain;
            layerView.subImage.imageRect = imageRect;

#if USE_DEPTH_INFO
            auto& depthInfo = projectionDeptInfos[eyeIndex];
            depthInfo.maxDepth = 1.0f;
            depthInfo.farZ = farZ;
            depthInfo.nearZ = nearZ;
            depthInfo.subImage.swapchain = projectionDepthSwapchain;
            depthInfo.subImage.imageRect = imageRect;
            layerView.next = &depthInfo;
#endif
        });
        layersPointers.push_back(&projectionLayer);
    }

    xr::ActionSet actionSet;
    xr::Action gripPoseAction, aimPoseAction;
    xr::Action squeezeAction, triggerAction, thumbstickAction, thumbClickAction;
    xr::Action vibrateAction, quitAction;
    xr::BilateralPaths handPaths;
    std::array<xr::Space, 2> gripHandSpace;
    std::array<xr::Space, 2> aimHandSpace;
    void prepareXrActions() {
        // Create subactions for left and right hands.
        handPaths = xrContext.makeHandSubpaths();
        actionSet = xrContext.instance.createActionSet(xr::ActionSetCreateInfo{ "gameplay", "Gameplay" });

        std::vector<xr::ActionSuggestedBinding> commonBindings;
        commonBindings.reserve(100);

        using AT = xr::ActionType;
        auto handsData = handPaths.data();
        // Create pose actions
        gripPoseAction = actionSet.createAction({ "grip_pose", AT::PoseInput, 2, handsData, "Grip Pose" });
        aimPoseAction = actionSet.createAction({ "aim_pose", AT::PoseInput, 2, handsData, "Aim Pose" });
        // Create input actions.
        squeezeAction = actionSet.createAction({ "grab_object", AT::FloatInput, 2, handsData, "Grab Object" });
        triggerAction = actionSet.createAction({ "point_object", AT::FloatInput, 2, handsData, "Point" });
        thumbstickAction = actionSet.createAction({ "thumbstick", AT::Vector2FInput, 2, handsData, "Thumbstick XY" });
        thumbClickAction = actionSet.createAction({ "thumbstick_click", AT::BooleanInput, 2, handsData, "Thumbstick Click" });
        // Create input actions for quitting the session using the left and right controller.
        quitAction = actionSet.createAction({ "quit_session", AT::BooleanInput, 2, handsData, "Quit Session" });
        // Create output haptic actions for vibrating the left and right controller.
        vibrateAction = actionSet.createAction({ "hand_vibrate", AT::VibrationOutput, 2, handsData, "Vibrate" });

        xrContext.addToBindings(commonBindings, squeezeAction, "/input/squeeze/value");
        xrContext.addToBindings(commonBindings, triggerAction, "/input/trigger/value");
        xrContext.addToBindings(commonBindings, gripPoseAction, "/input/grip/pose");
        xrContext.addToBindings(commonBindings, aimPoseAction, "/input/aim/pose");
        xrContext.addToBindings(commonBindings, thumbstickAction, "/input/thumbstick");
        xrContext.addToBindings(commonBindings, thumbClickAction, "/input/thumbstick/click");
        xrContext.addToBindings(commonBindings, vibrateAction, "/output/haptic");

        // Suggest bindings for KHR Simple.
        xrContext.suggestBindings("/interaction_profiles/khr/simple_controller", commonBindings);
        // Suggest bindings for the Oculus Touch.
        xrContext.suggestBindings("/interaction_profiles/oculus/touch_controller", commonBindings);
        // Suggest bindings for the Vive Controller.
        xrContext.suggestBindings("/interaction_profiles/htc/vive_controller", commonBindings);
        // Suggest bindings for the Microsoft Mixed Reality Motion Controller.
        xrContext.suggestBindings("/interaction_profiles/microsoft/motion_controller", commonBindings);
        xrSession.attachSessionActionSets(xr::SessionActionSetsAttachInfo{ 1, &actionSet });

        // Create pose spaces
        xr::for_each_side_index([&](uint32_t side) {
            gripHandSpace[side] = xrSession.createActionSpace({ gripPoseAction, handPaths[side], {} });
            aimHandSpace[side] = xrSession.createActionSpace({ aimPoseAction, handPaths[side], {} });
        });
    }

    bool getHandActionBool(uint32_t hand, const xr::Action& action, bool defaultValue = false) {
        if (!action) {
            return defaultValue;
        }
        auto value = xrSession.getActionStateBoolean({ action, handPaths[hand] });
        if (XR_TRUE == value.isActive) {
            return value.currentState != XR_FALSE;
        }
        return defaultValue;
    }

    float getHandActionFloat(uint32_t hand, const xr::Action& action, float defaultValue = 0.0f) {
        if (!action) {
            return defaultValue;
        }
        auto value = xrSession.getActionStateFloat({ action, handPaths[hand] });
        if (XR_TRUE == value.isActive) {
            return value.currentState;
        }
        return defaultValue;
    }

    xr::Vector2f getHandActionVec2(uint32_t hand, const xr::Action& action, const xr::Vector2f& defaultValue = {}) {
        if (!action) {
            return defaultValue;
        }
        auto value = xrSession.getActionStateVector2f({ action, handPaths[hand] });
        if (XR_TRUE == value.isActive) {
            return value.currentState;
        }
        return defaultValue;
    }

    xr::Posef getHandPose(uint32_t hand, const xr::Action& action, xr::Space& handSpace) {
        if (!action) {
            return {};
        }
        auto value = xrSession.getActionStatePose({ action, handPaths[hand] });
        if (!value.isActive) {
            return {};
        }

        const xr::SpaceLocationFlags requiredFlags = xr::SpaceLocationFlags{ xr::SpaceLocationFlagBits::PositionValid } |
                                                     xr::SpaceLocationFlags{ xr::SpaceLocationFlagBits::OrientationValid };
        auto locationResult = handSpace.locateSpace(space, xrContext.frameState.predictedDisplayTime);
        if (locationResult.locationFlags & requiredFlags) {
            return locationResult.pose;
        }
        return {};
    }

    HandStates handStates;
    void updateHandStates() {
        xr::for_each_side_index([&](uint32_t hand) {
            auto& handState = handStates[hand];
            handState.aim = getHandPose(hand, aimPoseAction, aimHandSpace[hand]);
            handState.grip = getHandPose(hand, gripPoseAction, gripHandSpace[hand]);
            handState.squeeze = getHandActionFloat(hand, squeezeAction);
            handState.trigger = getHandActionFloat(hand, triggerAction);
            handState.thumbClicked = getHandActionBool(hand, thumbClickAction);
            handState.thumb = getHandActionVec2(hand, thumbstickAction);

            if (handState.squeeze > 0.7f) {
                xr::HapticVibration vibration;
                vibration.amplitude = 0.5;
                vibration.duration = xr::Duration::minHaptic();
                vibration.frequency = XR_FREQUENCY_UNSPECIFIED;
                xrSession.applyHapticFeedback(xr::HapticActionInfo{ vibrateAction, handPaths[hand] },
                                              (XrHapticBaseHeader*)&vibration);
            }
        });
    }

    virtual bool update(float delta) {
        if (xrContext.stopped) {
            scene.destroy();
            window.requestClose();
            return false;
        }
        xrContext.pollEvents();

        bool sessionSynced = false;
        switch (xrContext.state) {
            case xr::SessionState::Focused:
            case xr::SessionState::Visible:
            case xr::SessionState::Synchronized:
                sessionSynced = true;
                break;

            // If we're not in one of the above session states, we're not going to do anything else this frame
            default:
                //xrContext.beginFrameResult = xr::Result::FrameDiscarded;
                //return false;
                break;
        }

        if (sessionSynced) {
            const xr::ActiveActionSet activeActionSet{ actionSet, xr::Path{ XR_NULL_PATH } };
            xrSession.syncActions({ 1, &activeActionSet });
            xrContext.onFrameStart();
            xrContext.updateEyeViews(space);
            updateHandStates();

            xr::for_each_side_index([&](size_t eyeIndex) {
                const auto& viewState = xrContext.eyeViewStates[eyeIndex];
                eyeStates[eyeIndex] = viewState;

                // Copy the eye states to the projection layer.
                // Remember when doing asynchronous rendering to carry the eye states to the rendering layer
                {
                    auto& projectionLayerView = projectionLayerViews[eyeIndex];
                    projectionLayerView.fov = viewState.fov;
                    projectionLayerView.pose = viewState.pose;
                }
            });
            scene.updateEyes(eyeStates);
            scene.updateHands(handStates);

        }

        return true;
    }

    virtual void renderSceneLayer() final { 
        framebuffer.bind();
        framebuffer.clear();
        scene.render(framebuffer); 
        framebuffer.bindDefault();
    }

    virtual void blitToProjection() {
        // Blit to the swapchain
        projectionFramebuffer.bind();
        framebuffer.blitTo(projectionFramebuffer.fbo, renderTargetSize);
        projectionFramebuffer.bindDefault();
        projectionFramebuffer.advance();
    }

    virtual void renderExtraLayers() {}

    virtual void blitToWindow() {
        framebuffer.blitTo(0, window.getSize());
        window.swapBuffers();
    }

    virtual void submitFrame() {
        xr::FrameEndInfo frameEndInfo;
        frameEndInfo.displayTime = xrContext.frameState.predictedDisplayTime;
        frameEndInfo.environmentBlendMode = xr::EnvironmentBlendMode::Opaque;
        frameEndInfo.layerCount = static_cast<uint32_t>(layersPointers.size());
        frameEndInfo.layers = layersPointers.data();
        xrContext.session.endFrame(frameEndInfo);
    }

    virtual void render() {
        if (xrContext.stopped) {
            return;
        }

        if (!xrContext.shouldRender()) {
            if (xrContext.beginFrameResult == xr::Result::Success) {
                xrContext.session.endFrame(
                    xr::FrameEndInfo{ xrContext.frameState.predictedDisplayTime, xr::EnvironmentBlendMode::Opaque });
            }
            window.swapBuffers();
            return;
        }

        renderSceneLayer();
        blitToProjection();

        renderExtraLayers();

        // Submit the image layers
        submitFrame();

        // Blit to the window
        blitToWindow();
    }

    virtual std::string getWindowTitle() {
        //static const std::string device = (const char*)glGetString(GL_VERSION);
        static std::string device;
        return "OpenXR SDK Example " + device + " - " + std::to_string((int)lastFps) + " fps";
    }

    void run() {
        prepare();
        auto tStart = std::chrono::high_resolution_clock::now();
        static auto lastFrameCounter = frameCounter;
        window.runWindowLoop([&] {
            auto tEnd = std::chrono::high_resolution_clock::now();
            auto tDiff = std::chrono::duration<double, std::milli>(tEnd - tStart).count();
            fpsTimer += (float)tDiff;
            if (fpsTimer > 1000.0f) {
                window.setTitle(getWindowTitle());
                lastFps = (float)(frameCounter - lastFrameCounter);
                lastFps *= 1000.0f;
                lastFps /= (float)fpsTimer;
                fpsTimer = 0.0f;
                lastFrameCounter = frameCounter;
            }
            tStart = tEnd;
            ++frameCounter;
            if (!update((float)tDiff / 1000.0f)) {
                Sleep(100);
                return;
            }
            render();
        });
    }
};
}  // namespace xr_examples