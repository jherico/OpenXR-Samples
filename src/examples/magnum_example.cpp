#define XR_USE_GRAPHICS_API_OPENGL
//#define DISABLE_XR

#include <common.hpp>
#include <openxrExampleBase.hpp>

#if !defined(DISABLE_XR)
#include <xrs/glm.hpp>
#endif

#include <assets.hpp>

#include <magnum/scene.hpp>
#include <magnum/framebuffer.hpp>

class OpenXrExample : public xr_examples::OpenXRExampleBase {
    using Parent = xr_examples::OpenXRExampleBase;
	using Scene = xr_examples::magnum::Scene;
	using Framebuffer = xr_examples::magnum::Framebuffer;
public:
    Framebuffer framebuffer;
    Scene scene;

    std::array<xr::CompositionLayerProjectionView, 2> projectionLayerViews;
    xr::CompositionLayerProjection projectionLayer{ {}, {}, 2, projectionLayerViews.data() };
    xrs::gl::FramebufferSwapchain projectionSwapchain;

    OpenXrExample() {}
    ~OpenXrExample() {}


    void prepare() {
        renderTargetSize = { 1280, 720 };
        Parent::prepare();
        preapreXrLayers();
		framebuffer.create(renderTargetSize);
        prepareScene();
    }

    void preapreXrLayers() {
        auto referenceSpaces = xrContext.session.enumerateReferenceSpaces();
        projectionSwapchain.create(xrSession, renderTargetSize);
        projectionLayer.space = space;
        projectionLayer.viewCount = 2;
        projectionLayer.views = projectionLayerViews.data();
        // Finish setting up the layer submission
        xr::for_each_side_index([&](uint32_t eyeIndex) {
            auto& layerView = projectionLayerViews[eyeIndex];
            layerView.subImage.swapchain = projectionSwapchain.swapchain;
            layerView.subImage.imageRect.extent = { (int32_t)renderTargetSize.x / 2, (int32_t)renderTargetSize.y };
            if (eyeIndex == 1) {
                layerView.subImage.imageRect.offset.x = layerView.subImage.imageRect.extent.width;
            }
        });
        layersPointers.push_back(&projectionLayer);
    }

    void prepareScene() {
        scene.create();
        scene.loadModel(assets::getAssetPathString("models/2CylinderEngine.glb"));
        scene.setCubemap(assets::getAssetPathString("yokohama.basis"));
    }

    void update(float delta) {
        if (xrContext.stopped) {
            scene.destroy();
        }

		Parent::update(delta);

        xr::for_each_side_index([&](size_t eyeIndex) {
            const auto& viewState = xrContext.eyeViewStates[eyeIndex];
            // Copy the eye states to the projection layer.
            // Remember when doing asynchronous rendering to
            auto& projectionLayerView = projectionLayerViews[eyeIndex];
            projectionLayerView.fov = viewState.fov;
            projectionLayerView.pose = viewState.pose;
        });

        if (xrContext.shouldRender()) {
            scene.updateHands(handStates);
            scene.updateEyes(eyeStates);
        }
    }

    void render() {
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

        scene.render(framebuffer);

#if !defined(DISABLE_XR)
        // Blit to the swapchain
        {
            projectionSwapchain.acquireImage();
            projectionSwapchain.waitImage();
            framebuffer.blitTo(projectionSwapchain.object, renderTargetSize);
            projectionSwapchain.releaseImage();
        }

        // Submit the image layers
        {
            xr::FrameEndInfo frameEndInfo;
            frameEndInfo.displayTime = xrContext.frameState.predictedDisplayTime;
            frameEndInfo.environmentBlendMode = xr::EnvironmentBlendMode::Opaque;
            frameEndInfo.layerCount = static_cast<uint32_t>(layersPointers.size());
            frameEndInfo.layers = layersPointers.data();
            xrContext.session.endFrame(frameEndInfo);
        }

#endif

        // Blit to the window
        framebuffer.blitTo(0, window.getSize());
        window.swapBuffers();
    }

    std::string getWindowTitle() {
        static const std::string device = (const char*)glGetString(GL_VERSION);
        return "OpenXR SDK Example " + device + " - " + std::to_string((int)lastFps) + " fps";
    }
};

RUN_EXAMPLE(OpenXrExample)
