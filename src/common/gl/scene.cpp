#include "scene.hpp"

#include <openxr/openxr.hpp>

#include <basis.hpp>
#include <assets.hpp>

#include <magnum/math.hpp>

using namespace xr_examples::gl;

struct Scene::Private {
    using EyeStates = xr_examples::EyeStates;
    using HandStates = xr_examples::HandStates;

    Private() {}

    ~Private() {}

    void loadCubemap(const std::string& filename) {
    }

    void loadScene(const std::string& filename) {
    }

    void render(Framebuffer& framebuffer) {
        xr::for_each_side_index([&](uint32_t eyeIndex) {
            framebuffer.setViewportSide(eyeIndex);
        });
    }

    HandStates handStates;
    EyeStates eyeStates;
};

Scene::~Scene() {
}

void Scene::destroy() {
    d.reset();
}

void Scene::create() {
    d = std::make_shared<Private>();
}

void Scene::setCubemap(const std::string& cubemapPrefix) {
    d->loadCubemap(cubemapPrefix);
}

void Scene::loadModel(const std::string& modelfile) {
    d->loadScene(modelfile);
}

void Scene::render(xr_examples::Framebuffer& stereoFramebuffer) {
    d->render(stereoFramebuffer);
}

void Scene::updateHands(const xr_examples::HandStates& handStates) {
    d->handStates = handStates;
}

void Scene::updateEyes(const xr_examples::EyeStates& eyeStates) {
    d->eyeStates = eyeStates;
}

#if 0 
struct GLState {
        uint32_t vao{ 0 };
        uint32_t skyboxPipeline{ 0 };
        uint32_t skyboxCubemap{ 0 };
        uint32_t blitPipeline{ 0 };
        uint32_t eyeViewsBuffer{ 0 };
        gl::EyeViews eyeViewsData;
        gl::MultiviewFramebuffer sceneFramebuffer;
    } glState;

#endif