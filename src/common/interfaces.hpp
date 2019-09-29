//
//  Created by Bradley Austin Davis
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include <array>
#include <string>
#include <functional>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <xrs/glm.hpp>

// Forward declare the Xr structre used to populate the EyeState
namespace xr {
struct View;
}

namespace xr_examples {

struct HandState {
	bool thumbClicked{ false };
    xrs::pose grip;
    xrs::pose aim;
    float squeeze{ -1.0 };
    float trigger{ -1.0 };
    glm::vec2 thumb;
};

using HandStates = std::array<HandState, 2>;

struct EyeState {
    xrs::pose pose;
    glm::mat4 projection;

    EyeState& operator=(const xr::View& view);
};

using EyeStates = std::array<EyeState, 2>;

struct Window {
    virtual void create(const glm::uvec2& size) = 0;
    virtual void makeCurrent() = 0;
    virtual void doneCurrent() = 0;
    virtual void setSwapInterval(uint32_t swapInterval) = 0;
    virtual void setTitle(const ::std::string& title) = 0;
    virtual void runWindowLoop(const ::std::function<void()>& handler) = 0;
    virtual void swapBuffers() = 0;
    virtual void requestClose() = 0;
    virtual glm::uvec2 getSize() const = 0;
};

struct Framebuffer {
    enum Target
    {
        Draw = 1,
        Read = 2,
    };

    void blitTo(uint32_t dest, const glm::uvec2& destSize);
    void blitTo(uint32_t dest, const glm::uvec2& destSize, uint32_t mask, uint32_t filter);

    static void blit(uint32_t source, const glm::uvec2& sourceSize, uint32_t dest, const glm::uvec2& destSize);

    static void blit(uint32_t source,
                     const glm::uvec2& sourceSize,
                     uint32_t dest,
                     const glm::uvec2& destSize,
                     uint32_t mask,
                     uint32_t filter);

    virtual void create(const glm::uvec2& size) = 0;
    virtual void bind(Target target = Draw) = 0;
    virtual void clear() = 0;
    virtual void bindDefault(Target target = Draw) = 0;
    virtual void setViewport(const glm::uvec4& viewport) = 0;
    virtual uint32_t id() = 0;

    virtual void setViewport(uint32_t side) final {
        glm::uvec4 result{ 0, 0, eyeSize.x, eyeSize.y };
        if (side == 1) {
            result.x += eyeSize.x;
            result.z += eyeSize.x;
        }
        setViewport(result);
    }

    virtual const glm::uvec2& getSize() const final { return size; }
    virtual const glm::uvec2& getEyeSize() const final { return eyeSize; }

protected:
    glm::uvec2 size;
    glm::uvec2 eyeSize;
};

class Scene {
    virtual void render(Framebuffer& stereoFramebuffer) = 0;
    virtual void create() = 0;
	virtual void setCubemap(const std::string& cubemapPrefix) = 0;
	virtual void loadModel(const std::string& modelfile) = 0;
	virtual void destroy() {}
    virtual void updateHands(const HandStates& handStates) = 0;
    virtual void updateEyes(const EyeStates& eyeStates) = 0;
};

}  // namespace xr_examples
