#pragma once

#ifdef HAVE_QT

#include <interfaces.hpp>

namespace xr_examples { namespace qt {

class Scene : public ::xr_examples::Scene {
    struct Private;

public:
    Scene();
    void render(Framebuffer& stereoFramebuffer) override;
    void create() override;
    void setCubemap(const std::string& cubemapPrefix) override;
    void loadModel(const std::string& modelfile) override;
    void destroy() {}
    void updateHands(const HandStates& handStates) override;
    void updateEyes(const EyeStates& eyeStates) override;

private:
    std::shared_ptr<Private> d;
};

}}  // namespace qt::_3d

#endif
