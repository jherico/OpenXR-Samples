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
	struct Cylinder {
		xr::CompositionLayerCylinderKHR layer;
		xrs::Swapchain<> swapchain;

		void prepare(const xr::Session& xrSession) {
			// FIXME texture loading doesn't work
			//auto cubemapData = assets::getAssetContentsBinary("yokohama.basis");
			//BasisReader cubemapReader{ cubemapData.data(), cubemapData.size() };

			xr::SwapchainCreateInfo ci;
			ci.createFlags = xr::SwapchainCreateFlagBits::StaticImage;
			//ci.height = cubemapReader.imageInfo.m_orig_height;
			//ci.width = cubemapReader.imageInfo.m_orig_width;
			ci.height = 2;
			ci.width = 2;
			ci.faceCount = 1;
			ci.sampleCount = 1;
			ci.mipCount = 1;
			ci.arraySize = 1;
			ci.usageFlags = xr::SwapchainUsageFlagBits::TransferDst;
			ci.format = xrs::DEFAULT_SWAPCHAIN_FORMAT;
			swapchain.createSwapchain(xrSession, ci);
			{
				//std::vector<uint8_t> imageData;
				//imageData.resize(cubemapReader.getImageSize());
				//cubemapReader.readImageToBuffer(imageData.data());
				auto swapchainImage = swapchain.acquireImage();
				swapchain.waitImage();
				uint32_t COLORS[4] = { 0xff0000ff, 0x00ff00ff, 0x0000ffff, 0xff00ffff };
				glTextureSubImage2D(swapchainImage.image, 0, 0, 0, ci.width, ci.height, GL_RGBA, GL_UNSIGNED_BYTE, COLORS);
				glTextureParameteri(swapchainImage.image, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				glTextureParameteri(swapchainImage.image, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				swapchain.releaseImage();
			}
			layer.radius = 0.25f;
			layer.aspectRatio = 16.0f / 9.0f;
			layer.centralAngle = 3.14159f/2.0f;
			layer.pose.position.z = -0.5f;
			layer.subImage.imageRect = { { 0, 0 }, { 1, 1 } };
			layer.subImage.swapchain = swapchain.swapchain;
		}
	} cylinder;


	void prepare() override {
		// Use of the Cylinder layer requires the extension
		xrContext.requiredExtensions.insert(XR_KHR_COMPOSITION_LAYER_CYLINDER_EXTENSION_NAME);
		Parent::prepare();
		{
			cylinder.prepare(xrSession);
			cylinder.layer.space = space;
			layersPointers.clear();
			layersPointers.insert(layersPointers.begin(), &cylinder.layer);
		}
	}

	// We override the prepareScene method to avoid loading the default cubemap
	void prepareScene() override {
        scene.create();
        scene.loadModel(assets::getAssetPathString("models/2CylinderEngine.glb"));
    }

};

RUN_EXAMPLE(OpenXrExample)
