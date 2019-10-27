#define XR_USE_GRAPHICS_API_OPENGL

#include <openxrExampleBase.hpp>
#include <magnum/scene.hpp>
#include <magnum/framebuffer.hpp>
#include <magnum/window.hpp>
#include <imgui/panel.hpp>
#include <imgui.h>

using namespace xr_examples;

using namespace xr_examples;
using WindowType = magnum::Window;
using FramebufferType = magnum::Framebuffer;
using SceneType = magnum::Scene;
using ExampleType = OpenXrExampleBase<WindowType, FramebufferType, SceneType>;

static const xr::Extent2Di UI_SIZE{ 720, 1280 };

class OpenXrExample : public ExampleType {
    using Parent = ExampleType;

    struct UILayer {
        imgui::Renderer renderer;
        xr::CompositionLayerQuad layer;
    } ui;

    void uiHandler() {
        ImGuiIO& io{ ImGui::GetIO() };
        io.DisplaySize = ImVec2((float)UI_SIZE.width, (float)UI_SIZE.height);
        ImGui::NewFrame();
        {
            static float f = 0.0f;
            static int counter = 0;
            static bool show_demo_window = true;
            static bool show_another_window = false;
            static ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

            ImGui::SetNextWindowSize(io.DisplaySize, ImGuiCond_Always);
            ImGui::SetNextWindowPos({ 0, 0 }, ImGuiCond_Always);
            ImGui::Begin("Hello, world!");  // Create a window called "Hello, world!" and append into it.

            ImGui::Text("This is some useful text.");           // Display some text (you can use a format strings too)
            ImGui::Checkbox("Demo Window", &show_demo_window);  // Edit bools storing our window open/close state
            ImGui::Checkbox("Another Window", &show_another_window);

            ImGui::InputFloat3("Pos", &handStates[0].aim.position.x, ImGuiInputTextFlags_ReadOnly);
            ImGui::InputFloat4("Rot", &handStates[0].aim.orientation.x, ImGuiInputTextFlags_ReadOnly);
            if (ImGui::Button("Button"))
                counter++;
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
                        ImGui::GetIO().Framerate);
            ImGui::End();
        }
        ImGui::EndFrame();
        ImGui::Render();
    }

    void prepareUi() {
        imgui::Renderer::init();
        ui.renderer.setHandler([this] { uiHandler(); });
        ui.renderer.create(UI_SIZE, xrSession, window);
        ui.layer.space = space;
        ui.layer.subImage.swapchain = ui.renderer.getSwapchain();
        ui.layer.subImage.imageRect = { { 0, 0 }, UI_SIZE };
        static float PPI = 3600.0f;
        ui.layer.size = { (float)UI_SIZE.width / PPI, (float)UI_SIZE.height / PPI };
        layersPointers.push_back(&ui.layer);
    }

    void prepare() {
        Parent::prepare();
        prepareUi();
    }

    void renderExtraLayers() override {
        ui.layer.pose.position = handStates[0].aim.position;
        ui.renderer.requestNewFrame();
    }
};

RUN_EXAMPLE(OpenXrExample)
