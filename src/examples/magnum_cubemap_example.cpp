#define XR_USE_GRAPHICS_API_OPENGL

#include <openxrExampleBase.hpp>
#include <magnum/scene.hpp>
#include <magnum/framebuffer.hpp>
#include <magnum/window.hpp>
#include <xrs/swapchain.hpp>
#include <basis.hpp>
#include <glad/glad.h>

using namespace xr_examples;

class OpenXrExample : public OpenXrExampleBase<magnum::Window, magnum::Framebuffer, magnum::Scene> {
	using Parent = OpenXrExampleBase<magnum::Window, magnum::Framebuffer, magnum::Scene>;
	struct Cubemap {
		xr::CompositionLayerCubeKHR layer;
		xrs::Swapchain<> swapchain;

		void prepare(const xr::Session& xrSession) {
			auto cubemapData = assets::getAssetContentsBinary("yokohama.basis");
			BasisReader cubemapReader{ cubemapData.data(), cubemapData.size() };
			auto formats = xrSession.enumerateSwapchainFormats();
			xr::SwapchainCreateInfo ci;
			ci.createFlags = xr::SwapchainCreateFlagBits::StaticImage;
			ci.height = cubemapReader.imageInfo.m_orig_height;
			ci.width = cubemapReader.imageInfo.m_orig_width;
			ci.faceCount = 1;
			ci.sampleCount = 1;
			ci.mipCount = 1;
			ci.arraySize = 1;
			ci.usageFlags = xr::SwapchainUsageFlagBits::TransferDst;
			ci.format = GL_RGBA8;
			// Currently broken on Oculus: "ovrLayerType_Cube does not support ovrLayerFlag_TextureOriginAtBottomLeft. Disabling layer 0"
			swapchain.createSwapchain(xrSession, ci);
			{
				std::vector<uint8_t> imageData;
				imageData.resize(cubemapReader.getImageSize());
				cubemapReader.readImageToBuffer(imageData.data());
				auto swapchainImage = swapchain.acquireImage();
				swapchain.waitImage();
				glBindTexture(GL_TEXTURE_2D, swapchainImage.image);
				glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, ci.width, ci.height, GL_RGBA, GL_UNSIGNED_BYTE, imageData.data());
				glBindTexture(GL_TEXTURE_2D, 0);
				swapchain.releaseImage();
			}
			layer.swapchain = swapchain.swapchain;
		}
	} cubemap;


	void prepare() override {
		// Use of the Cylinder layer requires the extension
		xrContext.requiredExtensions.insert(XR_KHR_COMPOSITION_LAYER_CUBE_EXTENSION_NAME);
		Parent::prepare();
		{
			cubemap.prepare(xrSession);
			cubemap.layer.space = space;
			layersPointers.clear();
			layersPointers.insert(layersPointers.begin(), &cubemap.layer);
		}
	}

	// We override the prepareScene method to avoid loading the default cubemap
	void prepareScene() override {
        scene.create();
        scene.loadModel(assets::getAssetPathString("models/2CylinderEngine.glb"));
    }

};

RUN_EXAMPLE(OpenXrExample)
