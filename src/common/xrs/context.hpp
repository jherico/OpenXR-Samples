#pragma once

#include "common.hpp"

#include <openxr/openxr.hpp>
#include <xrs/debug.hpp>

#if defined(XR_USE_GRAPHICS_API_VULKAN)
#include <vulkan/vulkan.hpp>
#endif


namespace xrs {

#if defined(XR_USE_GRAPHICS_API_VULKAN)
#define GRAPHICS_API_EXTENSION XR_KHR_VULKAN_ENABLE_EXTENSION_NAME
#elif defined(XR_USE_GRAPHICS_API_OPENGL)
#define GRAPHICS_API_EXTENSION XR_KHR_OPENGL_ENABLE_EXTENSION_NAME
#elif defined(XR_USE_GRAPHICS_API_OPENGL_ES)
#define GRAPHICS_API_EXTENSION XR_KHR_OPENGL_ES_ENABLE_EXTENSION_NAME
#endif

struct Context {
#if defined(XR_USE_GRAPHICS_API_VULKAN)
    template <typename T>
    std::vector<std::string> splitCharBuffer(const T& buffer) {
        std::vector<std::string> result;
        std::string curString;
        for (const auto c : buffer) {
            if (c == 0 || c == ' ') {
                if (!curString.empty()) {
                    result.push_back(curString);
                    curString.clear();
                }
                if (c == 0) {
                    break;
                }
                continue;
            }
            curString.push_back(c);
        }
        return result;
    }

    std::vector<std::string> getVulkanInstanceExtensions() {
        return splitCharBuffer<>(instance.getVulkanInstanceExtensionsKHR(systemId, dispatch));
    }
    std::vector<std::string> getVulkanDeviceExtensions() {
        return splitCharBuffer<>(instance.getVulkanDeviceExtensionsKHR(systemId, dispatch));
    }
#endif

    bool enableDebug{ true };

    xr::Instance instance;
    xr::SystemId systemId;

    // Interaction with non-core (KHR, EXT, etc) functions requires a dispatch instance
    xr::DispatchLoaderDynamic dispatch;
    std::unordered_map<std::string, xr::ExtensionProperties> discoveredExtensions;

    xr::FormFactor requiredFormFactor{ xr::FormFactor::HeadMountedDisplay };
    xr::ViewConfigurationType requiredViewConfiguration{ xr::ViewConfigurationType::PrimaryStereo };

    std::set<std::string> requiredExtensions;

    xr::Session session;
    xr::InstanceProperties instanceProperties;
    xr::SystemProperties systemProperties;
    bool stopped{ false };

    xr::SessionState state{ xr::SessionState::Idle };
    xr::FrameState frameState;
    xr::Result beginFrameResult{ xr::Result::FrameDiscarded };
    xr::ViewConfigurationProperties viewConfigProperties;
    std::array<xr::ViewConfigurationView, 2> viewConfigViews;
    std::array<xr::View, 2> eyeViewStates;

    DebugUtilsEXT::Messenger messenger;

    void create() {
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

        requiredExtensions.insert(GRAPHICS_API_EXTENSION);

        std::vector<const char*> requestedExtensions;
        for (const auto& extension : requiredExtensions) {
            if (0 == discoveredExtensions.count(extension)) {
                throw std::runtime_error(FORMAT("Required API extension not available: {}", extension));
            }
            requestedExtensions.push_back(extension.c_str());
        }

        if (enableDebug) {
            requestedExtensions.push_back(XR_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        {
            xr::InstanceCreateInfo ici{ {},
                                        { "vr_openxr", 0, "vulkan_cpp_examples", 0, xr::Version{ XR_MAKE_VERSION(1, 0, 0) } },
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

            instance = xr::createInstance(ici);
            // Having created the isntance, the very first thing to do is populate the dynamic dispatch, loading
            // all the available functions from the runtime
            dispatch = xr::DispatchLoaderDynamic::createFullyPopulated(instance, &xrGetInstanceProcAddr);

            // Turn on debug logging
            if (enableDebug) {
                messenger = xrs::DebugUtilsEXT::create(instance);
            }
        }

        instanceProperties = instance.getInstanceProperties();

        // We want to create an HMD example
        systemId = instance.getSystem(xr::SystemGetInfo{ requiredFormFactor });
        systemProperties = instance.getSystemProperties(systemId);

        // Find out what view configurations we have available
        auto viewConfigTypes = instance.enumerateViewConfigurations(systemId);
        if (viewConfigTypes.end() == std::find(viewConfigTypes.begin(), viewConfigTypes.end(), requiredViewConfiguration)) {
            throw std::runtime_error("Example only supports stereo-based HMD rendering");
        }

        viewConfigProperties = instance.getViewConfigurationProperties(systemId, requiredViewConfiguration);
        {
            auto viewConfigViewsVec = instance.enumerateViewConfigurationViews(systemId, requiredViewConfiguration);
            if (viewConfigViewsVec.size() != 2) {
                throw std::runtime_error("Example only supports stereo-based HMD rendering");
            }
            xr::for_each_side_index([&](uint32_t side) { viewConfigViews[side] = viewConfigViewsVec[side]; });
        }
    }

    void updateEyeViews(xr::Space space) {
        xr::ViewState vs;
        xr::ViewLocateInfo vi{ xr::ViewConfigurationType::PrimaryStereo, frameState.predictedDisplayTime, space };
        auto eyeViewStatesVec = session.locateViews(vi, &(vs.operator XrViewState&()));
        if (eyeViewStatesVec.size() != 2) {
            throw std::runtime_error("Example only supports stereo-based HMD rendering");
        }
        xr::for_each_side_index([&](uint32_t side) { eyeViewStates[side] = eyeViewStatesVec[side]; });
    }

    void destroy() {
        destroySession();
        if (instance) {
            instance.destroy();
            instance = nullptr;
        }
    }

    template <typename T>
    std::vector<T> getSwapchainFormats() {
        std::vector<T> swapchainFormats;
        auto swapchainFormatsRaw = session.enumerateSwapchainFormats();
        swapchainFormats.reserve(swapchainFormatsRaw.size());
        for (const auto& format : swapchainFormatsRaw) {
            swapchainFormats.push_back((T)format);
        }
        return swapchainFormats;
    }

    void destroySession() {
        if (session) {
            session.destroy();
            session = nullptr;
        }
    }

    template <typename T>
    void createSession(const T& graphicsBinding) {
        // Create the session bound to the vulkan device and queue
        xr::SessionCreateInfo sci{ {}, systemId };
        sci.next = &graphicsBinding;
        session = instance.createSession(sci);
    }

    void pollEvents() {
        while (true) {
            xr::EventDataBuffer eventBuffer;
            auto pollResult = instance.pollEvent(eventBuffer);
            if (pollResult == xr::Result::EventUnavailable) {
                break;
            }

            switch (eventBuffer.type) {
                case xr::StructureType::EventDataSessionStateChanged: {
                    onSessionStateChanged(reinterpret_cast<xr::EventDataSessionStateChanged&>(eventBuffer));
                    break;
                }
                case xr::StructureType::EventDataInstanceLossPending: {
                    onInstanceLossPending(reinterpret_cast<xr::EventDataInstanceLossPending&>(eventBuffer));
                    break;
                }
                case xr::StructureType::EventDataInteractionProfileChanged: {
                    onInteractionprofileChanged(reinterpret_cast<xr::EventDataInteractionProfileChanged&>(eventBuffer));
                    break;
                }
                case xr::StructureType::EventDataReferenceSpaceChangePending: {
                    onReferenceSpaceChangePending(reinterpret_cast<xr::EventDataReferenceSpaceChangePending&>(eventBuffer));
                    break;
                }
            }
        }
    }

    void onSessionStateChanged(const xr::EventDataSessionStateChanged& sessionStateChangedEvent) {
        state = sessionStateChangedEvent.state;
        switch (state) {
            case xr::SessionState::Ready: {
                if (!stopped) {
                    session.beginSession(xr::SessionBeginInfo{ requiredViewConfiguration });
                }
            } break;

            case xr::SessionState::Stopping: {
                session.endSession();
                stopped = true;
            } break;

            case xr::SessionState::Exiting:
            case xr::SessionState::LossPending: {
                destroySession();
            } break;

            default:
                break;
        }
    }

    void onInstanceLossPending(const xr::EventDataInstanceLossPending&) {}

    void onEventsLost(const xr::EventDataEventsLost&) {}

    void onReferenceSpaceChangePending(const xr::EventDataReferenceSpaceChangePending&) {}

    void onInteractionprofileChanged(const xr::EventDataInteractionProfileChanged&) {}

    void onFrameStart() {
        beginFrameResult = xr::Result::FrameDiscarded;
        switch (state) {
            case xr::SessionState::Focused:
            case xr::SessionState::Synchronized:
            case xr::SessionState::Visible: {
                session.waitFrame(xr::FrameWaitInfo{}, frameState);
                beginFrameResult = session.beginFrame(xr::FrameBeginInfo{});
                switch (beginFrameResult) {
                    case xr::Result::FrameDiscarded:
                    case xr::Result::SessionLossPending:
                        return;

                    default:
                        break;
                }
            } break;

            default:
                beginFrameResult = xr::Result::FrameDiscarded;
        }
    }

    bool shouldRender() const { return beginFrameResult == xr::Result::Success && frameState.shouldRender; }

    xr::BilateralPaths makeHandSubpaths(const std::string& subpath = "") {
        xr::BilateralPaths result;
        // Create subactions for left and right hands.
        xr::for_each_side_index([&](uint32_t side) {
            std::string fullPath = xr::reserved_paths[side] + subpath;
            result[side] = instance.stringToPath(fullPath.c_str());
        });
        return result;
    }

    void addToBindings(std::vector<xr::ActionSuggestedBinding>& bindings,
                       const xr::Action& action,
                       const std::string& subpath) {
        auto valuePaths = makeHandSubpaths(subpath);
        xr::for_each_side_index([&](uint32_t side) { bindings.push_back({ action, valuePaths[side] }); });
    }

    void suggestBindings(const std::string& profilePath, const std::vector<xr::ActionSuggestedBinding>& bindings) {
        auto interactionProfilePath = instance.stringToPath(profilePath.c_str());
        instance.suggestInteractionProfileBindings({ interactionProfilePath, (uint32_t)bindings.size(), bindings.data() });
    }
};
}  // namespace xrs