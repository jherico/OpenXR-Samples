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

#if !defined(DISABLE_XR)
#include <xrs/context.hpp>
#include <xrs/swapchain.hpp>
#endif

#include <magnum/window.hpp>

namespace xr_examples {
class OpenXRExampleBase {
public:
    using Window = magnum::Window;

    std::vector<xr::CompositionLayerBaseHeader*> layersPointers;

#if defined(XR_USE_GRAPHICS_API_VULKAN)
    vks::Context& vkContext{ vks::Context::get() };
#endif

    uint32_t frameCounter{ 0 };
    glm::uvec2 renderTargetSize;
    EyeStates eyeStates;
    float fpsTimer{ 0.0f };
    float lastFps{ 0.0f };

    OpenXRExampleBase() {
        // Static initialization for whatever our backed (Qt, Magnum, etc) needs
        Window::init();
    }

    ~OpenXRExampleBase() {
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
    }

    xrs::Context xrContext;
    void prepareXrInstance() {
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

        renderTargetSize = { viewConfigViews[0].recommendedImageRectWidth * 2, viewConfigViews[0].recommendedImageRectHeight };
    }

    Window window;
    void prepareWindow() {
        assert(renderTargetSize.x != 0 && renderTargetSize.y != 0);
        window.create(renderTargetSize / 4u);

#if defined(XR_USE_GRAPHICS_API_OPENGL)
        window.makeCurrent();
        window.setSwapInterval(0);
#endif
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
        xrContext.addToBindings(commonBindings, thumbstickAction, "/input/thumbstick/click");
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

    glm::vec2 getHandActionVec2(uint32_t hand, const xr::Action& action, const glm::vec2& defaultValue = glm::vec2{ 0.0f }) {
        if (!action) {
            return defaultValue;
        }
        auto value = xrSession.getActionStateVector2f({ action, handPaths[hand] });
        if (XR_TRUE == value.isActive) {
            return { value.currentState.x, value.currentState.y };
        }
        return defaultValue;
    }

    xrs::pose getHandPose(uint32_t hand, const xr::Action& action, xr::Space& handSpace) {
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
            return xrs::toGlmPose(locationResult.pose);
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

    virtual void update(float delta) {
        if (xrContext.stopped) {
            window.requestClose();
            return;
        }
        xrContext.pollEvents();

        switch (xrContext.state) {
            case xr::SessionState::Focused:
            case xr::SessionState::Visible:
            case xr::SessionState::Synchronized: {
                const xr::ActiveActionSet activeActionSet{ actionSet, xr::Path{ XR_NULL_PATH } };
                xrSession.syncActions({ 1, &activeActionSet });
                updateHandStates();
            } break;

            default:
                break;
        }

        xrContext.onFrameStart();
        xrContext.updateEyeViews(space);

        xr::for_each_side_index([&](size_t eyeIndex) {
            const auto& viewState = xrContext.eyeViewStates[eyeIndex];
            auto& eyeState = eyeStates[eyeIndex];
            eyeState.projection = xrs::toGlmGL(viewState.fov);
            eyeState.pose.position = xrs::toGlm(viewState.pose.position);
            eyeState.pose.rotation = xrs::toGlm(viewState.pose.orientation);
        });
    }

    virtual void render() = 0;

    virtual std::string getWindowTitle() = 0;

    void run() {
        prepare();
        auto tStart = std::chrono::high_resolution_clock::now();
        static auto lastFrameCounter = frameCounter;
        window.runWindowLoop([&] {
            auto tEnd = std::chrono::high_resolution_clock::now();
            auto tDiff = std::chrono::duration<double, std::milli>(tEnd - tStart).count();
            update((float)tDiff / 1000.0f);
            render();
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
        });
    }
};
}  // namespace xr_examples