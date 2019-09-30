#define XR_USE_GRAPHICS_API_OPENGL

#include <openxrExampleBase.hpp>
#include <qt/window.hpp>
#include <qt/scene.hpp>
#include <qt/framebuffer.hpp>
#include <gl/framebuffer.hpp>

//#include <qt/gl/offscreenSurface.hpp>
//#include <assets.hpp>
//#include <gl/framebuffer.hpp>
//#include <qt/scene.hpp>

using namespace xr_examples;

class OpenXrExample : public OpenXrExampleBase<qt::Window, qt::Framebuffer, qt::Scene> {
};

RUN_EXAMPLE(OpenXrExample)
