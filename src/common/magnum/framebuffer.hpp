#pragma once

#ifdef USE_MAGNUM

#include <interfaces.hpp>
#include <memory>

namespace xr_examples { namespace magnum {

class Framebuffer : public xr_examples::Framebuffer {
    struct Private;

public:
    ~Framebuffer();
    void create(const xr::Extent2Di& size) override;
    void bind(Target target = Draw) override;
    void clear(const xr::Color4f& color, float depth, int stencil) override;
    void destroy() override { d.reset(); }
    void bindDefault(Target target = Draw) override;
    void setViewport(const xr::Rect2Di& viewport) override;
    uint32_t id() override;
private:
    std::shared_ptr<Private> d;
};

}}  // namespace xr_examples::magnum

#endif