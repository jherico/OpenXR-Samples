#define XR_USE_GRAPHICS_API_OPENGL

#include <openxrExampleBase.hpp>
#include <qt/window.hpp>
#include <qt/scene.hpp>
#include <qt/framebuffer.hpp>
#include <qt/qml/offscreenSurface.hpp>
#include <qt/math.hpp>

using namespace xr_examples;

class OpenXrExample : public OpenXrExampleBase<qt::Window, qt::Framebuffer, qt::Scene> {
    using Parent = OpenXrExampleBase<qt::Window, qt::Framebuffer, qt::Scene>;
public:
    xr::CompositionLayerQuad uiLayer;
    qml::OffscreenSurface qmlSurface;
    xrs::gl::FramebufferSwapchain uiSwapchain;


    void prepare() override {
        Parent::prepare();
        prepareUiLayer();
    }

    void prepareUiLayer() {
        QUrl url = QUrl::fromLocalFile(assets::getAssetPathString("qml/dynamicscene.qml").c_str());
        uiSwapchain.create(xrSession, { 800, 600 });
        uiSwapchain.acquireImage();
        uiSwapchain.waitImage();
        uiSwapchain.bind();
        uiSwapchain.clear();
        uiSwapchain.bindDefault();
        uiSwapchain.releaseImage();

        qmlSurface.setSwapchain(uiSwapchain.swapchain);
        qmlSurface.resize({ 800, 600 });
        qmlSurface.load(url);

        uiLayer.subImage.swapchain = uiSwapchain.swapchain;
        uiLayer.subImage.imageRect.extent = { 1, 1 };
        uiLayer.size = { 0.4f, 0.3f };
        uiLayer.pose.position.z = -0.4f;
        uiLayer.pose.position.x = -0.4f;
        uiLayer.space = space;
		QTimer::singleShot(1000, [=] {
		    layersPointers.push_back(&uiLayer);
		});
        
    }
    
    //bool update(float delta) {
    //    auto result = Parent::update(delta);
    //    qmlSurface
    //    _shared->_lastRenderTime
    //    return result;
    //}

};

RUN_EXAMPLE(OpenXrExample)
