#pragma once

#ifdef USE_MAGNUM

#include <interfaces.hpp>
#include <memory>

namespace xr_examples { namespace magnum {

class Framebuffer : public xr_examples::Framebuffer {
    struct Private;

public:
    ~Framebuffer();
    void create(const glm::uvec2& size) override;
    void bind(Target target = Draw) override;
    void clear() override;
    void bindDefault(Target target = Draw) override;
    void setViewport(const glm::uvec4& viewport) override;
	uint32_t id() override;
private:
    std::shared_ptr<Private> d;
};

}}  // namespace xr_examples::magnum

#endif