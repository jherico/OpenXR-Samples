//
//  Created by Bradley Austin Davis
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "interfaces.hpp"
#include <glad/glad.h>

using namespace xr_examples;

void Framebuffer::blitTo(uint32_t dest, const xr::Extent2Di& destSize, uint32_t mask, Filter filter) {
    blit(id(), getSize(), dest, destSize, mask, filter);
}

void Framebuffer::blit(uint32_t source,
                       const xr::Extent2Di& sourceSize,
                       uint32_t dest,
                       const xr::Extent2Di& destSize,
                       uint32_t mask,
                       Filter filter) {
    glBlitNamedFramebuffer(source, dest,                               //
                           0, 0, sourceSize.width, sourceSize.height,  //
                           0, 0, destSize.width, destSize.height,      //
                           mask, filter);
}
