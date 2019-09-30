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
#include <openxr/openxr.hpp>

namespace xr_examples {

struct HandState {
    bool thumbClicked{ false };
    xr::Posef grip;
    xr::Posef aim;
    float squeeze{ -1.0 };
    float trigger{ -1.0 };
    xr::Vector2f thumb;
};
using HandStates = std::array<HandState, 2>;

using EyeState = xr::View;
using EyeStates = std::array<EyeState, 2>;

struct Window {
    virtual void create(const xr::Extent2Di& size) = 0;
    virtual void makeCurrent() = 0;
    virtual void doneCurrent() = 0;
    virtual void setSwapInterval(uint32_t swapInterval) = 0;
    virtual void setTitle(const ::std::string& title) = 0;
    virtual void runWindowLoop(const ::std::function<void()>& handler) = 0;
    virtual void swapBuffers() = 0;
    virtual void requestClose() = 0;
    virtual xr::Extent2Di getSize() const = 0;
};

struct Framebuffer {
    enum Target
    {
        Draw = 1,
        Read = 2,
    };

    void blitTo(uint32_t dest, const xr::Extent2Di& destSize);
    void blitTo(uint32_t dest, const xr::Extent2Di& destSize, uint32_t mask, uint32_t filter);

    static void blit(uint32_t source, const xr::Extent2Di& sourceSize, uint32_t dest, const xr::Extent2Di& destSize);

    static void blit(uint32_t source,
                     const xr::Extent2Di& sourceSize,
                     uint32_t dest,
                     const xr::Extent2Di& destSize,
                     uint32_t mask,
                     uint32_t filter);

    virtual void create(const xr::Extent2Di& size) = 0;
	virtual void destroy() = 0;
    virtual void bind(Target target = Draw) = 0;
    virtual void clear() = 0;
    virtual void bindDefault(Target target = Draw) = 0;
    virtual void setViewport(const xr::Rect2Di& viewport) = 0;
    virtual uint32_t id() = 0;

    virtual void setViewport(uint32_t side) final {
        xr::Rect2Di result{ { 0, 0 }, eyeSize };
        if (side == 1) {
            result.offset.x += eyeSize.width;
        }
        setViewport(result);
    }

    virtual const xr::Extent2Di& getSize() const final { return size; }
    virtual const xr::Extent2Di& getEyeSize() const final { return eyeSize; }

protected:
    xr::Extent2Di size;
    xr::Extent2Di eyeSize;
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
