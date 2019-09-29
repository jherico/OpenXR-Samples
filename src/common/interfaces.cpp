//
//  Created by Bradley Austin Davis
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "interfaces.hpp"

#include <openxr/openxr.hpp>
#include <xrs/glm.hpp>

#pragma warning(push)
#pragma warning(disable : 4251)
#pragma warning(disable : 4267)
#include <Magnum/GL/GL.h>
#include <Magnum/GL/Framebuffer.h>
#pragma warning(pop)

using namespace xr_examples;

EyeState& xr_examples::EyeState::operator=(const xr::View& eyeView) {
    projection = xrs::toGlmGL(eyeView.fov);
    pose.position = xrs::toGlm(eyeView.pose.position);
    pose.rotation = xrs::toGlm(eyeView.pose.orientation);
    return *this;
}

void Framebuffer::blitTo(uint32_t dest, const glm::uvec2& destSize) {
    blitTo(dest, destSize, GL_COLOR_BUFFER_BIT, GL_NEAREST);
}
void Framebuffer::blitTo(uint32_t dest, const glm::uvec2& destSize, uint32_t mask, uint32_t filter) {
    blit(id(), getSize(), dest, destSize, mask, filter);
}

void Framebuffer::blit(uint32_t source,
                       const glm::uvec2& sourceSize,
                       uint32_t dest,
                       const glm::uvec2& destSize,
                       uint32_t mask,
                       uint32_t filter) {
    glBlitNamedFramebuffer(source, dest,                      //
                           0, 0, sourceSize.x, sourceSize.y,  //
                           0, 0, destSize.x, destSize.y,      //
                           mask, filter);
}

void Framebuffer::blit(uint32_t source, const glm::uvec2& sourceSize, uint32_t dest, const glm::uvec2& destSize) {
    blit(source, sourceSize, dest, destSize, GL_COLOR_BUFFER_BIT, GL_NEAREST);
}
