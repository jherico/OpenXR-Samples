#define XR_USE_GRAPHICS_API_OPENGL

#include <openxrExampleBase.hpp>
#include <qt/window.hpp>
#include <qt/scene.hpp>
#include <qt/framebuffer.hpp>
#include <qt/qml/offscreenSurface.hpp>
using namespace xr_examples;

class OpenXrExample : public OpenXrExampleBase<qt::Window, qt::Framebuffer, qt::Scene> {
	using Parent = OpenXrExampleBase<qt::Window, qt::Framebuffer, qt::Scene>;
public:
	xr::CompositionLayerQuad uiLayer;
	qml::OffscreenSurface qmlSurface;

	void prepare() override {
		Parent::prepare();
		prepareUiLayer();
	}

	void prepareUiLayer() {
		QUrl url = QUrl::fromLocalFile(assets::getAssetPathString("qml/dynamicscene.qml").c_str());
		qml::OffscreenSurface::setSharedSession(xrSession);
		qmlSurface.resize({ 800, 600 });
		qmlSurface.load(url);
		uiLayer.subImage.swapchain = qmlSurface.getSwapchain();
		uiLayer.subImage.imageRect.extent = { 1, 1 };
		uiLayer.size = { 0.4f, 0.3f };
		uiLayer.pose.position.z = -0.4f;
		uiLayer.pose.position.x = -0.4f;
		uiLayer.space = space;
		layersPointers.push_back(&uiLayer);
	}

};

RUN_EXAMPLE(OpenXrExample)
