#pragma once

#ifdef HAVE_QT

#include <interfaces.hpp>
#include <memory>

namespace xr_examples { namespace qt {

class Framebuffer : public ::xr_examples::Framebuffer {
    struct Private;

public:
    ~Framebuffer() { d.reset(); }
    void create(const xr::Extent2Di& size) override;
    void destroy() override { d.reset(); }
    void bind(Target target = Draw) override;
    void clear(const xr::Color4f& color = { 0, 0, 0, 1 }, float depth = 1.0f, int stencil = 0) override;
    void bindDefault(Target target = Draw) override;
    void setViewport(const xr::Rect2Di& viewport) override;
    uint32_t id() override;

private:
    std::shared_ptr<Private> d;
};

}}  // namespace xr_examples::qt

#endif