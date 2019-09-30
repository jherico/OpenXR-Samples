#define XR_USE_GRAPHICS_API_OPENGL
//#define DISABLE_XR

#include <common.hpp>
#include <openxrExampleBase.hpp>
#include <assets.hpp>

#include <magnum/scene.hpp>
#include <magnum/framebuffer.hpp>
#include <magnum/window.hpp>

using namespace xr_examples;

using ExampleBase = OpenXrExampleBase<magnum::Window, magnum::Framebuffer, magnum::Scene>;

class OpenXrExample : public ExampleBase {
    using Parent = ExampleBase;
public:


};

RUN_EXAMPLE(OpenXrExample)
