#pragma once

#include <interfaces.hpp>

namespace xr_examples { namespace gl {

struct Framebuffer : public xr_examples::Framebuffer {
public:
    void create(const xr::Extent2Di& size) override;
    void destroy() override;

    void bind(Target target = Draw) override;
    void clear() override;

    void bindDefault(Target target = Draw) override;
    void setViewport(const xr::Rect2Di& vp) override;

    inline uint32_t id() override { return fbo; }

    uint32_t fbo{ 0 };
    uint32_t color{ 0 };
    uint32_t depthStencil{ 0 };
};

}}  // namespace xr_examples::gl
