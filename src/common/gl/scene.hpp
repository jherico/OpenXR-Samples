#pragma once

#include <interfaces.hpp>
#include <memory>

namespace xr_examples { namespace gl {

class Scene : public xr_examples::Scene {
    struct Private;

public:
    virtual ~Scene();
    void render(xr_examples::Framebuffer& stereoFramebuffer) override;
    void create() override;
    void setCubemap(const std::string& cubemapPrefix) override;
    void loadModel(const std::string& modelfile) override;
    void destroy() override;
    void updateHands(const HandStates& handStates) override;
    void updateEyes(const EyeStates& eyeStates) override;

private:
    std::shared_ptr<Private> d;
};

}}  // namespace xr_examples::magnum
