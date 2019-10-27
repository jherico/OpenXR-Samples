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
    xr::Posef grip;
    xr::Posef aim;
    float squeeze{ -1.0 };
    bool squeezeTouched{ false };
    float trigger{ -1.0 };
    bool triggerTouched{ false };
    xr::Vector2f thumb;
    bool thumbClicked{ false };
    bool thumbTouched{ false };
};

using HandStates = std::array<HandState, 2>;

using EyeState = xr::View;
using EyeStates = std::array<EyeState, 2>;

struct Context {
    using Pointer = std::shared_ptr<Context>;

    virtual void makeCurrent() const = 0;
    virtual void doneCurrent() const = 0;
    virtual void destroy() {}
};

struct Window : public Context {
    virtual void create(const xr::Extent2Di& size) = 0;
    virtual void setSwapInterval(uint32_t swapInterval) const = 0;
    virtual void setTitle(const ::std::string& title) const = 0;
    virtual void runWindowLoop(const ::std::function<void()>& handler) = 0;
    virtual void swapBuffers() const = 0;
    virtual void requestClose() = 0;
    virtual Context::Pointer createOffscreenContext() const { return {}; }
    virtual xr::Extent2Di getSize() const = 0;
};

struct Framebuffer {
    enum Target
    {
        Draw = 1,
        Read = 2,
    };

    enum BlitMask
    {
        Depth = 0x00000100,
        Stencil = 0x00000400,
        Color = 0x00004000
    };

    enum Filter
    {
        Nearest = 0x2600,
        Linear = 0x2601,
    };

    void blitTo(uint32_t dest, const xr::Extent2Di& destSize, uint32_t mask = Color, Filter filter = Nearest);

    static void blit(uint32_t source,
                     const xr::Extent2Di& sourceSize,
                     uint32_t dest,
                     const xr::Extent2Di& destSize,
                     uint32_t mask = Color,
                     Filter filter = Nearest);

    virtual void create(const xr::Extent2Di& size) = 0;
    virtual void destroy() = 0;
    virtual void bind(Target target = Draw) = 0;
    virtual void clear(const xr::Color4f& color = { 0, 0, 0, 1 }, float depth = 1.0f, int stencil = 0) = 0;
    virtual void bindDefault(Target target = Draw) = 0;
    virtual void setViewport(const xr::Rect2Di& viewport) = 0;
    virtual uint32_t id() = 0;

    virtual void setViewportSide(uint32_t side) final {
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
