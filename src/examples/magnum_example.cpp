#define XR_USE_GRAPHICS_API_OPENGL

#include <openxrExampleBase.hpp>
#include <magnum/scene.hpp>
#include <magnum/framebuffer.hpp>
#include <magnum/window.hpp>

using namespace xr_examples;

class OpenXrExample : public OpenXrExampleBase<magnum::Window, magnum::Framebuffer, magnum::Scene> {};

RUN_EXAMPLE(OpenXrExample)
