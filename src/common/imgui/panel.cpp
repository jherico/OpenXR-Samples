#include "panel.hpp"

#include <gl/pipeline.hpp>
#include <assets.hpp>

#include <imgui.h>
#include <glad/glad.h>

using namespace xr_examples::imgui;

constexpr const char* VERTEX_SHADER_PATH{ "shaders/imgui.vert.glsl" };
constexpr const char* FRAGMENT_SHADER_PATH{ "shaders/imgui.frag.glsl" };
constexpr uint32_t TEX_0{ 0 };
constexpr uint32_t UBO_MATRICES{ 0 };
constexpr uint32_t UBO_SIZE{ sizeof(float) * 16 };
constexpr uint32_t ATTR_POS{ 0 };
constexpr uint32_t ATTR_UV{ 1 };
constexpr uint32_t ATTR_COLOR{ 2 };
constexpr uint32_t MAX_VERTEX_COUNT{ UINT16_MAX };
constexpr uint32_t MAX_VERTEX_SIZE{ MAX_VERTEX_COUNT * sizeof(ImDrawVert) };
constexpr uint32_t MAX_INDEX_COUNT{ UINT16_MAX };
constexpr uint32_t MAX_INDEX_SIZE{ UINT16_MAX * sizeof(ImDrawIdx) };

// All the internal details of rendering IMGUI to OpenGL
struct Renderer::Private {
    using Pipeline = xr_examples::gl::Pipeline;
    using ShaderStage = xr_examples::gl::ShaderStage;
    Pipeline pipeline;
    uint32_t vertexBuffer{ 0 };
    uint32_t indexBuffer{ 0 };
    uint32_t uniformBuffer{ 0 };
    uint32_t fontTexture{ 0 };

    Private() {
        pipeline.addShaderSources(ShaderStage::eVertex, assets::getAssetContents(VERTEX_SHADER_PATH));
        pipeline.addShaderSources(ShaderStage::eFragment, assets::getAssetContents(FRAGMENT_SHADER_PATH));
        // pipeline
        pipeline.setAttributeFormat(ATTR_POS, 2, GL_FLOAT, offsetof(ImDrawVert, pos));
        pipeline.setAttributeFormat(ATTR_UV, 2, GL_FLOAT, offsetof(ImDrawVert, uv));
        pipeline.setAttributeNFormat(ATTR_COLOR, 4, GL_UNSIGNED_BYTE, offsetof(ImDrawVert, col));
        pipeline.create();
        ImGuiIO& io{ ImGui::GetIO() };

        // Create buffers
        glCreateBuffers(3, &vertexBuffer);
        glNamedBufferStorage(uniformBuffer, UBO_SIZE, nullptr, GL_DYNAMIC_STORAGE_BIT);
        glNamedBufferStorage(vertexBuffer, MAX_VERTEX_SIZE, nullptr, GL_DYNAMIC_STORAGE_BIT);
        glNamedBufferStorage(indexBuffer, MAX_INDEX_SIZE, nullptr, GL_DYNAMIC_STORAGE_BIT);

        // Build texture atlas
        {
            unsigned char* pixels;
            int width, height;
            io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
            glCreateTextures(GL_TEXTURE_2D, 1, &fontTexture);
            glTextureStorage2D(fontTexture, 1, GL_RGBA8, width, height);
            glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
            glTextureParameteri(fontTexture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTextureParameteri(fontTexture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTextureSubImage2D(fontTexture, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
            io.Fonts->TexID = (ImTextureID)(intptr_t)fontTexture;
        }
    }

    void setupRenderState(ImDrawData* draw_data, int fb_width, int fb_height) {
        // Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled, polygon fill
        glEnable(GL_BLEND);
        glBlendEquation(GL_FUNC_ADD);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_SCISSOR_TEST);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        // Setup viewport, orthographic projection matrix
        // Our visible imgui space lies from draw_data->DisplayPos (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayPos is (0,0) for single viewport apps.
        glViewport(0, 0, (GLsizei)fb_width, (GLsizei)fb_height);
        float L = draw_data->DisplayPos.x;
        float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
        float T = draw_data->DisplayPos.y;
        float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
        const float ortho_projection[4][4] = {
            { 2.0f / (R - L), 0.0f, 0.0f, 0.0f },
            { 0.0f, 2.0f / (T - B), 0.0f, 0.0f },
            { 0.0f, 0.0f, -1.0f, 0.0f },
            { (R + L) / (L - R), (T + B) / (B - T), 0.0f, 1.0f },
        };

        pipeline.bind();
        glNamedBufferSubData(uniformBuffer, 0, UBO_SIZE, &ortho_projection[0][0]);
        glBindBufferBase(GL_UNIFORM_BUFFER, UBO_MATRICES, uniformBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);

        glBindVertexBuffer(ATTR_POS, vertexBuffer, 0, sizeof(ImDrawVert));
        glBindVertexBuffer(ATTR_UV, vertexBuffer, 0, sizeof(ImDrawVert));
        glBindVertexBuffer(ATTR_COLOR, vertexBuffer, 0, sizeof(ImDrawVert));
    }

    void render() {
        auto draw_data = ImGui::GetDrawData();
        int fb_width = (int)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
        int fb_height = (int)(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
        if (fb_width <= 0 || fb_height <= 0)
            return;

        setupRenderState(draw_data, fb_width, fb_height);

        // Will project scissor/clipping rectangles into framebuffer space
        const ImVec2& clip_off = draw_data->DisplayPos;          // (0,0) unless using multi-viewports
        const ImVec2& clip_scale = draw_data->FramebufferScale;  // (1,1) unless using retina display which are often (2,2)

        // Render command lists
        ImVec4 currentScissor;
        uint32_t currentTexture{ 0 };
        for (int n = 0; n < draw_data->CmdListsCount; n++) {
            const ImDrawList* cmd_list = draw_data->CmdLists[n];
            // Upload vertex/index buffers
            glNamedBufferSubData(vertexBuffer, 0, (GLsizeiptr)cmd_list->VtxBuffer.Size * sizeof(ImDrawVert),
                                 (const GLvoid*)cmd_list->VtxBuffer.Data);
            glNamedBufferSubData(indexBuffer, 0, (GLsizeiptr)cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx),
                                 (const GLvoid*)cmd_list->IdxBuffer.Data);

            for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++) {
                const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
                if (pcmd->UserCallback != NULL) {
                    // User callback, registered via ImDrawList::AddCallback()
                    // (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
                    if (pcmd->UserCallback == ImDrawCallback_ResetRenderState) {
                        setupRenderState(draw_data, fb_width, fb_height);
                    } else {
                        pcmd->UserCallback(cmd_list, pcmd);
                    }
                } else {
                    // Project scissor/clipping rectangles into framebuffer space
                    ImVec4 clip_rect;
                    clip_rect.x = (pcmd->ClipRect.x - clip_off.x) * clip_scale.x;
                    clip_rect.y = (pcmd->ClipRect.y - clip_off.y) * clip_scale.y;
                    clip_rect.z = (pcmd->ClipRect.z - clip_off.x) * clip_scale.x;
                    clip_rect.w = (pcmd->ClipRect.w - clip_off.y) * clip_scale.y;

                    if (clip_rect.x < fb_width && clip_rect.y < fb_height && clip_rect.z >= 0.0f && clip_rect.w >= 0.0f) {
                        // Apply scissor/clipping rectangle
                        //currentScissor = { clip_rect.x, fb_height - clip_rect.w, clip_rect.z - clip_rect.x, clip_rect.w - clip_rect.y };
                        glScissor((int)clip_rect.x, (int)(fb_height - clip_rect.w), (int)(clip_rect.z - clip_rect.x),
                                  (int)(clip_rect.w - clip_rect.y));
                        // Bind texture, Draw
                        glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->TextureId);
                        glDrawElementsBaseVertex(GL_TRIANGLES, (GLsizei)pcmd->ElemCount,
                                                 sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT,
                                                 (void*)(intptr_t)(pcmd->IdxOffset * sizeof(ImDrawIdx)),
                                                 (GLint)pcmd->VtxOffset);
                    }
                }
            }
        }
    }
};

void Renderer::init() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    //ImGuiIO& io = ImGui::GetIO();
    //ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
    //io.MouseDrawCursor = true;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    ImGui::StyleColorsDark();
    ImGuiIO& io{ ImGui::GetIO() };
    io.BackendRendererName = "OpenXR-Samples";
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
}

void Renderer::initContext() {
    d = std::make_shared<Private>();
}

void Renderer::render() {
    if (d && handler) {
        handler();
        d->render();
    }
}

void Renderer::setHandler(const Handler& handler) {
    this->handler = handler;
}

