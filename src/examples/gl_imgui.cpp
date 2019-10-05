#define XR_USE_GRAPHICS_API_OPENGL

#include <common.hpp>
#include <openxrExampleBase.hpp>

#include <GLFW/glfw3.h>
#include <glfw/window.hpp>
#include <openxr/openxr.hpp>
#include <assets.hpp>
#include <basis.hpp>
#include <glad.hpp>

#include <gl/framebuffer.hpp>
#include <gl/scene.hpp>
#include <gl/pipeline.hpp>

#include <imgui/panel.hpp>
#include <imgui.h>

#include <unordered_map>
#include <gl/threadedSwapchainRenderer.hpp>

#if 0 

xr::Posef flipHand(const xr::Posef& pose) {
    xr::Posef result = pose;
    result.orientation.x = -result.orientation.x;
    result.orientation.w = -result.orientation.w;
    result.position.x = -result.position.x;
    return result;
}

namespace gl {

struct MultiviewFramebuffer {
    uint32_t id{ 0 };
    uint32_t color{ 0 };
    uint32_t depth{ 0 };
    uvec2 size;
    static constexpr uint32_t colorFormat{ GL_SRGB8_ALPHA8 };
    static constexpr uint32_t depthFormat{ GL_DEPTH24_STENCIL8 };
    static constexpr uint32_t arraySize{ 2 };

    void build() {
        glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &depth);
        glTextureStorage3D(depth, 1, depthFormat, size.x, size.y, arraySize);

        glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &color);
        glTextureParameteri(color, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(color, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureStorage3D(color, 1, colorFormat, size.x, size.y, arraySize);

        glCreateFramebuffers(1, &id);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, id);
        glFramebufferTextureMultiviewOVR(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, color, 0, 0, 2);
        glFramebufferTextureMultiviewOVR(GL_DRAW_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, depth, 0, 0, 2);

        /* Check FBO is OK. */
        GLenum result = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
        if (result != GL_FRAMEBUFFER_COMPLETE) {
            LOG_ERROR("Framebuffer incomplete at %s:%i\n", __FILE__, __LINE__);
            /* Unbind framebuffer. */
        }
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    }
};

struct EyeView {
    mat4 projectionMatrix{ 1.0 };
    mat4 inverseProjectionMatrix{ 1.0 };
    mat4 viewMatrix{ 1.0 };
    mat4 inverseViewMatrix{ 1.0 };

    void update(const xr::View& eyeView) {
        projectionMatrix = xrs::toGlmGL(eyeView.fov);
        inverseProjectionMatrix = glm::inverse(projectionMatrix);
        inverseViewMatrix = xrs::toGlm(eyeView.pose);
        viewMatrix = glm::inverse(inverseViewMatrix);
    }
};

using EyeViews = std::array<EyeView, 2>;

}  // namespace gl

    void preapreXrLayers() override {
        auto referenceSpaces = xrContext.session.enumerateReferenceSpaces();

        projectionSwapchain.create(xrSession, renderTargetSize);
        xrSession.getReferenceSpaceBoundsRect(xr::ReferenceSpaceType::Local, bounds);

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

    void prepareRendering() {
        glCreateVertexArrays(1, &glState.vao);
        glState.skyboxCubemap = gl::loadBasisTexture(assets::getAssetPath("yokohama.basis").string());
        glState.skyboxPipeline = gl::buildProgram(assets::loadStringAsset("shaders/skybox.vert.glsl"),
                                                  assets::loadStringAsset("shaders/skybox.frag.glsl"));
        glState.blitPipeline = gl::buildProgram(assets::loadStringAsset("shaders/blit.vert.glsl"),
                                                assets::loadStringAsset("shaders/blit.frag.glsl"));
        glState.sceneFramebuffer.size = { renderTargetSize.x / 2, renderTargetSize.y };
        glState.sceneFramebuffer.build();
        glCreateBuffers(1, &glState.eyeViewsBuffer);
        glNamedBufferStorage(glState.eyeViewsBuffer, sizeof(gl::EyeViews), nullptr, GL_DYNAMIC_STORAGE_BIT);
    }

#endif



#if 0 
void setup() {
    auto& ui = *this;
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    //ImGuiIO& io = ImGui::GetIO();
    //ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
    //io.MouseDrawCursor = true;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    ImGui::StyleColorsDark();

#if THREADED_UI_RENDERING
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    context = glfwCreateWindow(100, 100, "", nullptr, parent.window.window);
    glfwMakeContextCurrent(ui.context);
#endif
    swapchain.create(parent.xrSession, size);
    swapchain.acquireImage();
    swapchain.waitImage();
    swapchain.withBoundFramebuffer([&] {
        glClearColor(1, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);
    });
    swapchain.releaseImage();

    panel.init();
    layer.subImage.swapchain = ui.swapchain.swapchain;
    layer.subImage.imageRect = { { 0, 0 }, { (int32_t)ui.size.x, (int32_t)ui.size.y } };
    static float PPI = 3600.0f;
    layer.size = { (float)ui.size.x / PPI, (float)ui.size.y / PPI };

#if THREADED_UI_RENDERING
    glfwMakeContextCurrent(nullptr);
    thread = std::make_unique<std::thread>([this] { run(); });
#endif
}

void render() {
    ImGuiIO& io{ ImGui::GetIO() };
    io.DisplaySize = ImVec2((float)size.x, (float)size.y);

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

        ImGui::InputFloat3("Pos", &parent.handPoses[0].position.x, ImGuiInputTextFlags_ReadOnly);
        ImGui::InputFloat3("Rot", &parent.handPoses[0].orientation.x, ImGuiInputTextFlags_ReadOnly);
        if (ImGui::Button("Button"))
            counter++;
        ImGui::SameLine();
        ImGui::Text("counter = %d", counter);
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
                    ImGui::GetIO().Framerate);
        ImGui::End();
    }

    // Rendering
    ImGui::Render();
    panel.size = size;
    swapchain.acquireImage();
    swapchain.waitImage();
    swapchain.withBoundFramebuffer([&] {
        glClearColor(1, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        panel.render();
    });
    swapchain.releaseImage();
}
}
ui{ *this };
#endif

using namespace xr_examples;
using WindowType = glfw::Window;
using FramebufferType = gl::Framebuffer;
using SceneType = gl::Scene;
using ExampleType = OpenXrExampleBase<WindowType, FramebufferType, SceneType>;

static const xr::Extent2Di UI_SIZE{ 640, 360 };

class OpenXrExample : public ExampleType {
    using Parent = ExampleType;

    struct UILayer {
        imgui::Renderer renderer;
        xr::CompositionLayerQuad layer;
    } ui;

public:
    ~OpenXrExample() {}

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

    std::string getWindowTitle() {
        static const std::string device = (const char*)glGetString(GL_VERSION);
        return "OpenXR SDK Example " + device + " - " + std::to_string((int)lastFps) + " fps";
    }
};

RUN_EXAMPLE(OpenXrExample)
