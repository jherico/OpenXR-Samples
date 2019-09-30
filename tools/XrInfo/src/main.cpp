#define _CRT_SECURE_NO_WARNINGS

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonValue>

#include <QtCore/qlogging.h>
#include <QtCore/QDebug>
#include <QtCore/QTimer>

#include <QtGui/QGuiApplication>
#include <QtGui/QClipboard>
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLWindow>

#include <mutex>
#include <vector>
#include <unordered_set>

#include <fmt/format.h>

#if defined(Q_OS_WIN32)
#define XR_USE_PLATFORM_WIN32
#pragma comment(lib, "Opengl32.lib")
#elif defined(Q_OS_ANDROID)
#define XR_USE_PLATFORM_ANDROID
#else
#define XR_USE_PLATFORM_XLIB
#endif

#define XR_USE_GRAPHICS_API_OPENGL
#define XR_USE_GRAPHICS_API_OPENGL_ES

#define XR_USE_GRAPHICS_API_VULKAN
#include <vulkan/vulkan.h>

#define XR_USE_TIMESPEC

#ifdef WIN32
#define XR_USE_GRAPHICS_API_D3D11
#define XR_USE_GRAPHICS_API_D3D12
#include <d3d11.h>
#include <d3d12.h>
#endif

#include <openxr/openxr.hpp>

#include "OffscreenGLSurface.h"

QSurfaceFormat getDefaultFormat() {
    QSurfaceFormat format;
#if defined(USE_GLES)
    format.setRenderableType(QSurfaceFormat::OpenGLES);
    format.setRedBufferSize(8);
    format.setGreenBufferSize(8);
    format.setBlueBufferSize(8);
    format.setAlphaBufferSize(8);
    format.setMajorVersion(3);
    format.setMinorVersion(2);
#else
    format.setProfile(QSurfaceFormat::OpenGLContextProfile::CoreProfile);
    format.setOption(QSurfaceFormat::DebugContext);
    format.setMajorVersion(4);
    format.setMinorVersion(6);
#endif
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    return format;
}

QString toJson(const xr::Version& version) {
    return fmt::format("{}.{}.{}", version.major, version.minor, version.patch).c_str();
}

QJsonObject toJson(const xr::ExtensionProperties& v) {
    QJsonObject result;
    result["name"] = v.extensionName;
    result["version"] = QJsonValue((qint64)v.extensionVersion);
    return result;
}

QJsonArray extensionsForLayer(const QString& layerName) {
    QJsonArray result;
    for (const auto& extensionProperties : xr::enumerateInstanceExtensionProperties(layerName.toStdString().c_str())) {
        result.append(toJson(extensionProperties));
    }
    return result;
}

QJsonObject toJson(const xr::ApiLayerProperties& v) {
    QJsonObject result;
    result["name"] = QString(v.layerName);
    result["description"] = QString(v.description);
    result["version"] = (qint64)v.layerVersion;
    result["extensions"] = extensionsForLayer(v.layerName);
    return result;
}

QJsonArray scanLayers(const xr::DispatchLoaderDynamic& dispatch) {
    QJsonArray result;
    for (const auto& layerProperties : xr::enumerateApiLayerProperties(dispatch)) {
        result.append(toJson(layerProperties));
    }
    return result;
}

QJsonObject toJson(const xr::SystemGraphicsProperties& graphicsProperties) {
    QJsonObject result;
    result["maxLayerCount"] = (qint64)graphicsProperties.maxLayerCount;
    result["maxSwapchainImageHeight"] = (qint64)graphicsProperties.maxSwapchainImageHeight;
    result["maxSwapchainImageWidth"] = (qint64)graphicsProperties.maxSwapchainImageWidth;
    return result;
}

QJsonObject toJson(const xr::SystemTrackingProperties& trackingProperties) {
    QJsonObject result;
    result["position"] = trackingProperties.positionTracking != XR_FALSE;
    result["orientation"] = trackingProperties.orientationTracking != XR_FALSE;
    return result;
}

QJsonObject toJson(const xr::GraphicsRequirementsOpenGLKHR& requirements) {
    QJsonObject result;
    result["maxApiVersionSupported"] = toJson(requirements.maxApiVersionSupported);
    result["minApiVersionSupported"] = toJson(requirements.minApiVersionSupported);
    return result;
}

QJsonObject toJson(const xr::GraphicsRequirementsOpenGLESKHR& requirements) {
    QJsonObject result;
    result["maxApiVersionSupported"] = toJson(requirements.maxApiVersionSupported);
    result["minApiVersionSupported"] = toJson(requirements.minApiVersionSupported);
    return result;
}

QJsonObject toJson(const xr::GraphicsRequirementsVulkanKHR& requirements) {
    QJsonObject result;
    result["maxApiVersionSupported"] = toJson(requirements.maxApiVersionSupported);
    result["minApiVersionSupported"] = toJson(requirements.minApiVersionSupported);
    return result;
}

#ifdef WIN32
QString d3dFeatureLevelToString(D3D_FEATURE_LEVEL level) {
    switch (level) {
        case D3D_FEATURE_LEVEL_1_0_CORE:
            return "D3D_FEATURE_LEVEL_1_0_CORE";
        case D3D_FEATURE_LEVEL_9_1:
            return "D3D_FEATURE_LEVEL_9_1";
        case D3D_FEATURE_LEVEL_9_2:
            return "D3D_FEATURE_LEVEL_9_2";
        case D3D_FEATURE_LEVEL_9_3:
            return "D3D_FEATURE_LEVEL_9_3";
        case D3D_FEATURE_LEVEL_10_0:
            return "D3D_FEATURE_LEVEL_10_0";
        case D3D_FEATURE_LEVEL_10_1:
            return "D3D_FEATURE_LEVEL_10_1";
        case D3D_FEATURE_LEVEL_11_0:
            return "D3D_FEATURE_LEVEL_11_0";
        case D3D_FEATURE_LEVEL_11_1:
            return "D3D_FEATURE_LEVEL_11_1";
        case D3D_FEATURE_LEVEL_12_0:
            return "D3D_FEATURE_LEVEL_12_0";
        case D3D_FEATURE_LEVEL_12_1:
            return "D3D_FEATURE_LEVEL_12_1";
    }
    return "Unknown";
}
QJsonObject toJson(const xr::GraphicsRequirementsD3D11KHR& requirements) {
    QJsonObject result;
    // Recording the LUID doesn't make sense since it's "locally unique"
    result["minFeatureLevel"] = d3dFeatureLevelToString(requirements.minFeatureLevel);
    return result;
}

QJsonObject toJson(const xr::GraphicsRequirementsD3D12KHR& requirements) {
    QJsonObject result;
    // Recording the LUID doesn't make sense since it's "locally unique"
    result["minFeatureLevel"] = d3dFeatureLevelToString(requirements.minFeatureLevel);
    return result;
}
#endif

QJsonArray splitString(const std::string& buffer) {
    QJsonArray result;
    QString curString;
    for (const auto c : buffer) {
        if (c == 0 || c == ' ') {
            if (!curString.isEmpty()) {
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

QJsonObject toJson(const xr::InstanceProperties& v) {
    QJsonObject result;
    result["runtimeName"] = v.runtimeName;
    result["runtimeVersion"] = toJson(v.runtimeVersion);
    return result;
}

struct XrContext {
    const xr::DispatchLoaderDynamic dispatch;
    const xr::Instance instance;
    xr::SystemId systemId;
    xr::Session session;
    xr::SessionState sessionState = xr::SessionState::Unknown;
    xr::Space space;
    xr::SwapchainCreateInfo swapchainCreateInfo;
    xr::Swapchain swapchain;
    std::vector<xr::SwapchainImageOpenGLKHR> swapchainImages;
	bool finished{ false };

    std::unordered_set<std::string> discoveredExtensions;
    std::vector<const char*> desiredExtensions{ {
        "XR_KHR_convert_timespec_time", XR_KHR_VISIBILITY_MASK_EXTENSION_NAME, XR_KHR_COMPOSITION_LAYER_CUBE_EXTENSION_NAME,
        XR_KHR_COMPOSITION_LAYER_CYLINDER_EXTENSION_NAME, XR_KHR_COMPOSITION_LAYER_DEPTH_EXTENSION_NAME,
        XR_KHR_COMPOSITION_LAYER_EQUIRECT_EXTENSION_NAME, XR_KHR_OPENGL_ENABLE_EXTENSION_NAME,
        XR_KHR_OPENGL_ES_ENABLE_EXTENSION_NAME, XR_KHR_VULKAN_ENABLE_EXTENSION_NAME,  //
#ifdef WIN32
        XR_KHR_D3D11_ENABLE_EXTENSION_NAME, XR_KHR_D3D12_ENABLE_EXTENSION_NAME, "XR_KHR_win32_convert_performance_counter_time",
#endif
        //XR_EXT_DEBUG_UTILS_EXTENSION_NAME,
        //"XR_OCULUS_ovrsession_handle",
    } };

    XrContext() {
        for (const auto& extensionProperties : xr::enumerateInstanceExtensionProperties("")) {
            discoveredExtensions.insert(std::string(extensionProperties.extensionName));
        }

        xr::InstanceCreateInfo ci;
        strncpy(ci.applicationInfo.applicationName, "XrInfo", XR_MAX_APPLICATION_NAME_SIZE);
        ci.applicationInfo.applicationVersion = 1;
        strncpy(ci.applicationInfo.engineName, "XrInfo", XR_MAX_ENGINE_NAME_SIZE);
        ci.applicationInfo.engineVersion = 1;
        ci.applicationInfo.apiVersion = xr::Version::current();
        std::vector<const char*> enabledExtensions;
        enabledExtensions.reserve(desiredExtensions.size());
        for (const auto& extension : desiredExtensions) {
            if (0 != discoveredExtensions.count(std::string(extension))) {
                enabledExtensions.push_back(extension);
            }
        }
        ci.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());
        ci.enabledExtensionNames = enabledExtensions.data();

        const_cast<xr::Instance&>(instance) = xr::createInstance(ci);
        const_cast<xr::DispatchLoaderDynamic&>(dispatch) =
            xr::DispatchLoaderDynamic::createFullyPopulated(instance, &xrGetInstanceProcAddr);

        systemId = instance.getSystem(xr::FormFactor::HeadMountedDisplay);
    }

    void shutdown() {}
    void createSession() {
        auto context = QOpenGLContext::currentContext();
#if defined(WIN32)
        instance.getOpenGLGraphicsRequirementsKHR(systemId, dispatch);
        using GraphicsBinding = xr::GraphicsBindingOpenGLWin32KHR;
        xr::GraphicsBindingOpenGLWin32KHR binding{ wglGetCurrentDC(), wglGetCurrentContext() };
        const char* glVersionStr = (const char*)glGetString(GL_VERSION);
        qDebug() << glVersionStr;
#elif defined(__ANDROID__)
        using GraphicsBinding = xr::GraphicsBindingOpenGLESAndroidKHR;
#else
        using GraphicsBinding = xr::GraphicsBindingOpenGLXlibKHR;
#endif

        xr::SessionCreateInfo sci{ {}, systemId };
        sci.next = &binding;
        session = instance.createSession(sci);
        space = session.createReferenceSpace(xr::ReferenceSpaceType::Local);

        swapchainCreateInfo.usageFlags = xr::SwapchainUsageFlagBits::TransferDst;
        swapchainCreateInfo.format = (int64_t)GL_SRGB8_ALPHA8;
        swapchainCreateInfo.sampleCount = 1;
        swapchainCreateInfo.arraySize = 1;
        swapchainCreateInfo.faceCount = 1;
        swapchainCreateInfo.mipCount = 1;
        swapchainCreateInfo.width = 100;
        swapchainCreateInfo.height = 100;

        swapchain = session.createSwapchain(swapchainCreateInfo);
        swapchainImages = swapchain.enumerateSwapchainImages<xr::SwapchainImageOpenGLKHR>();
    }

    void onSessionStateChanged(const xr::EventDataSessionStateChanged& stateChangeEvent) {
        sessionState = stateChangeEvent.state;
        qDebug() << "Session state changed to " << xr::to_string_literal(sessionState);
        if (sessionState == xr::SessionState::Ready) {
            session.beginSession({ xr::ViewConfigurationType::PrimaryStereo });
            //result["paths"] = scanInput(session);
        } else if (sessionState == xr::SessionState::Synchronized) {
            QJsonDocument runtimeDesc;
            runtimeDesc.setObject(scanOpenXr());
            QGuiApplication::clipboard()->setText(runtimeDesc.toJson(QJsonDocument::Indented));
            session.requestExitSession();
        } else if (sessionState == xr::SessionState::Stopping) {
            session.endSession();
            QGuiApplication::quit();
        } else if (sessionState == xr::SessionState::Idle) {
			session.destroy();
			session = nullptr;
		}
    }

    void onTimer() {
        if (!finished && !session) {
            createSession();
        }

        while (session) {
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

    QJsonObject scanInput() {
        QJsonObject result;
        static const std::vector<const char*> BASE_PATHS{ {
            "/user/hand/left", "/user/hand/right",
            //"/user/head",
            //"/user/gamepad",
            //"/user/treadmill",
        } };

        static const std::vector<const char*> INPUT_PATHS{ {
            "select", "menu", "grip", "thumbstick", "trigger", "a", "b", "x", "y",
            // "aim",        "trackpad",
            // "joystick",     "throttle",  "trackball",  "pedal",
            // "dpad_up",      "dpad_down",     "dpad_left", "dpad_right",
            // "diamond_up",  "diamond_down", "diamond_left", "diamond_right",
            // "start",        "home",          "end",       "volume_up",  "volume_down", "mute_mic",
            // "play_pause",   "view",          "thumbrest", "shoulder",   "squeeze",     "wheel",
            // "system",
        } };

        static const std::vector<const char*> INPUT_COMPONENTS{ { "click", "touch", "force", "value", "twist", "x", "y",
                                                                  "pose" } };

        static std::vector<const char*> OUTPUT_PATHS{ { "haptic" } };
        std::unordered_set<XrPath> seenPaths;
        for (const auto& basePath : BASE_PATHS) {
            for (const auto& inputPath : INPUT_PATHS) {
                std::string fullInputPathStr = fmt::format("{}/{}", basePath, inputPath);
                auto fullInputPath = instance.stringToPath(fullInputPathStr.c_str());
                if (fullInputPath == XR_NULL_PATH) {
                    continue;
                }
                if (0 != seenPaths.count(fullInputPath.get())) {
                    continue;
                }

                seenPaths.insert(fullInputPath.get());
                std::string name =
                    session.getInputSourceLocalizedName({ fullInputPath, xr::InputSourceLocalizedNameFlagBits::AllBits });
                if (name.empty() || name[0] == 0) {
                    continue;
                }

                qDebug() << name.c_str();
                result[fullInputPathStr.c_str()] = name.c_str();

                //for (const auto& inputComponent : INPUT_COMPONENTS) {
                //    std::string fullComponentPathStr = fmt::format("{}/{}", fullInputPathStr, inputComponent);
                //    auto fullComponentPath = xrContext.instance.stringToPath(fullComponentPathStr.c_str());
                //    if (fullComponentPath == XR_NULL_PATH) {
                //        continue;
                //    }
                //    if (0 != seenPaths.count(fullComponentPath.get())) {
                //        continue;
                //    }
                //    seenPaths.insert(fullComponentPath.get());
                //    std::string name = session.getInputSourceLocalizedName(
                //        { fullComponentPath, xr::InputSourceLocalizedNameFlagBits::AllBits });
                //    if (name.empty()) {
                //        continue;
                //    }
                //    result[fullInputPathStr.c_str()] = name.c_str();
                //}
            }
        }
        return result;
    }

    QJsonObject scanRenderingApis() {
        QJsonObject result;

        if (discoveredExtensions.count(XR_KHR_OPENGL_ENABLE_EXTENSION_NAME)) {
            result["OpenGL"] = toJson(instance.getOpenGLGraphicsRequirementsKHR(systemId, dispatch));
        }
        if (discoveredExtensions.count(XR_KHR_OPENGL_ES_ENABLE_EXTENSION_NAME)) {
            result["OpenGLES"] = toJson(instance.getOpenGLESGraphicsRequirementsKHR(systemId, dispatch));
        }
        if (discoveredExtensions.count(XR_KHR_VULKAN_ENABLE_EXTENSION_NAME)) {
            QJsonObject vulkanRequirements = toJson(instance.getVulkanGraphicsRequirementsKHR(systemId, dispatch));
            vulkanRequirements["instanceExtensions"] = splitString(instance.getVulkanInstanceExtensionsKHR(systemId, dispatch));
            vulkanRequirements["deviceExtensions"] = splitString(instance.getVulkanDeviceExtensionsKHR(systemId, dispatch));
            result["Vulkan"] = vulkanRequirements;
        }

#if defined(WIN32)
        if (discoveredExtensions.count(XR_KHR_D3D11_ENABLE_EXTENSION_NAME)) {
            result["D3D11"] = toJson(instance.getD3D11GraphicsRequirementsKHR(systemId, dispatch));
        }
        if (discoveredExtensions.count(XR_KHR_D3D12_ENABLE_EXTENSION_NAME)) {
            result["D3D12"] = toJson(instance.getD3D12GraphicsRequirementsKHR(systemId, dispatch));
        }
#endif
        return result;
    }

    QJsonObject scanSystem() {
        QJsonObject result;
        auto systemProperties = instance.getSystemProperties(systemId);
        result["name"] = systemProperties.systemName;
        result["vendorId"] = (qint64)systemProperties.vendorId;
        result["trackingProperties"] = toJson(systemProperties.trackingProperties);
        result["graphicsProperties"] = toJson(systemProperties.graphicsProperties);
        result["graphicsRequirements"] = scanRenderingApis();
        //result["paths"] = scanInput();
        return result;
    }

    QJsonObject scanOpenXr() {
        QJsonObject result;
        result["properties"] = toJson(instance.getInstanceProperties());
        result["extensions"] = extensionsForLayer("");
        // result["layers"] = scanLayers();
        result["system"] = scanSystem();
        return result;
    }
};

int main(int argc, char** argv) {
    XrContext xrContext;

    QGuiApplication app(argc, argv);

    OffscreenGLSurface* surface = new OffscreenGLSurface();
    surface->setFormat(getDefaultFormat());
    surface->create();
    surface->makeCurrent();

    QTimer* appTimer = new QTimer();
    QObject::connect(appTimer, &QTimer::timeout, [&] { xrContext.onTimer(); });
    appTimer->setInterval(100);
    appTimer->setSingleShot(false);
    appTimer->start();

    QObject::connect(&app, &QGuiApplication::aboutToQuit, [&] {
        appTimer->stop();
        appTimer->deleteLater();
        surface->deleteLater();
        xrContext.shutdown();
    });

    return app.exec();
}

//QTimer::singleShot(100, [=] {
//    QJsonDocument runtimeDesc;
//    runtimeDesc.setObject(xrContext.scanOpenXr());
//    QGuiApplication::clipboard()->setText(runtimeDesc.toJson(QJsonDocument::Indented));
//    ->close();
//});
