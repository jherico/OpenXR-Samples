#include "scene.hpp"

#ifdef HAVE_QT

#include <QtGui/QOpenGLContext>
#include <Qt3DCore/QNode>
#include <Qt3DCore/QEntity>
#include <Qt3DCore/QTransform>
#include <Qt3DCore/QAspectEngine>

#include <Qt3DRender/QCamera>
#include <Qt3DRender/QCameraLens>
#include <Qt3DRender/QCameraSelector>
#include <Qt3DRender/QMesh>
#include <Qt3DRender/QPointLight>
#include <Qt3DRender/QSceneLoader>

#include <Qt3DExtras/QTorusMesh>
#include <Qt3DExtras/QConeMesh>
#include <Qt3DExtras/QCylinderMesh>
#include <Qt3DExtras/QCuboidMesh>
#include <Qt3DExtras/QPhongMaterial>
#include <Qt3DExtras/QPlaneMesh>
#include <Qt3DExtras/QSkyboxEntity>
#include <Qt3DExtras/QSphereMesh>

#include <Qt3DRender/QRenderAspect>
#include <Qt3DRender/QRenderSettings>
#include <Qt3DRender/QRenderSurfaceSelector>
#include <Qt3DRender/QViewport>
#include <Qt3DRender/private/qrenderaspect_p.h>
#include <Qt3DCore/private/qaspectengine_p.h>
#include <Qt3DCore/private/qaspectmanager_p.h>
#include <Qt3DCore/private/qservicelocator_p.h>
#include <Qt3DCore/private/qabstractframeadvanceservice_p.h>

#include <Qt3DLogic/QLogicAspect>

#include <qt/math.hpp>

namespace xr_examples { namespace qt {

extern QObject* g_sceneSurface;
extern QOpenGLContext* g_sceneContext;
struct FrameAdvance : public Qt3DCore::QAbstractFrameAdvanceService {
    int type() const { return Qt3DCore::QServiceLocator::FrameAdvanceService; }
    qint64 waitForNextFrame() { return 0; }
    virtual void start() {}
    virtual void stop() {}
};

struct StereoRenderer {
    Qt3DCore::QAspectEngine* aspectEngine;
    Qt3DCore::QAspectEnginePrivate* aspectEnginePrivate;
    Qt3DRender::QRenderAspect* renderAspect;
    Qt3DRender::QRenderAspectPrivate* renderAspectPrivate;
    Qt3DLogic::QLogicAspect* logicAspect;
    Qt3DCore::QNode* sceneRoot{ nullptr };

    StereoRenderer(const std::array<Qt3DRender::QCamera*, 2>& cameras) {
        aspectEngine = new Qt3DCore::QAspectEngine();
        renderAspect = new Qt3DRender::QRenderAspect(Qt3DRender::QRenderAspect::Synchronous);
        logicAspect = new Qt3DLogic::QLogicAspect();
        aspectEngine->registerAspect(renderAspect);
        aspectEngine->registerAspect(logicAspect);

        aspectEnginePrivate = Qt3DCore::QAspectEnginePrivate::get(aspectEngine);
        auto serviceLocator = aspectEnginePrivate->m_aspectManager->serviceLocator();
        serviceLocator->registerServiceProvider(Qt3DCore::QServiceLocator::FrameAdvanceService, new FrameAdvance());

        Qt3DCore::QEntityPtr root(new Qt3DCore::QEntity());
        auto renderSettings = new Qt3DRender::QRenderSettings(root.data());
        root->addComponent(renderSettings);
        auto renderSurfaceSelector = new Qt3DRender::QRenderSurfaceSelector(renderSettings);
        renderSurfaceSelector->setSurface(g_sceneSurface);
        for (uint32_t eyeIndex = 0; eyeIndex < 2; ++eyeIndex) {
            auto stereoViewport = new Qt3DRender::QViewport(renderSurfaceSelector);
            stereoViewport->setNormalizedRect(QRectF(eyeIndex == 0 ? 0.0 : 0.5, 0.0, 0.5, 1.0));
            auto cameraSelector = new Qt3DRender::QCameraSelector(stereoViewport);
            cameraSelector->setCamera(cameras[eyeIndex]);
        }
        renderSettings->setActiveFrameGraph(renderSurfaceSelector);
        auto abstractPrivate = Qt3DRender::QRenderAspectPrivate::get(renderAspect);
        renderAspectPrivate = static_cast<Qt3DRender::QRenderAspectPrivate*>(abstractPrivate);
        renderAspectPrivate->renderInitialize(g_sceneContext);
        aspectEngine->setRootEntity(root);
    }
    ~StereoRenderer() {
        // Clean up after ourselves.
        aspectEngine->setRootEntity(Qt3DCore::QEntityPtr());
        aspectEngine->unregisterAspect(logicAspect);
        aspectEngine->unregisterAspect(renderAspect);
        delete logicAspect;
        delete renderAspect;
        delete aspectEngine;
    }
};

struct Scene::Private {
    StereoRenderer* m_stereoRenderer{ nullptr };
    Qt3DCore::QEntity* m_sceneRootEntity{ nullptr };

    Qt3DCore::QEntity* m_coneEntity{ nullptr };
    Qt3DCore::QEntity* m_cylinderEntity{ nullptr };
    Qt3DCore::QEntity* m_torusEntity{ nullptr };
    Qt3DCore::QEntity* m_cuboidEntity{ nullptr };
    Qt3DCore::QEntity* m_planeEntity{ nullptr };
    Qt3DCore::QEntity* m_sphereEntity{ nullptr };
    Qt3DCore::QEntity* m_lightEntity{ nullptr };
    Qt3DExtras::QSkyboxEntity* m_skyboxEntity{ nullptr };

    struct Player {
        Qt3DCore::QEntity* m_entity{ nullptr };
        Qt3DCore::QTransform* m_transform{ nullptr };

        struct Hand {
            Qt3DCore::QEntity* m_entity;
            Qt3DCore::QTransform* m_transform;
        };
        std::array<Hand, 2> m_hands;
    } m_player;

    std::array<Qt3DRender::QCamera*, 2> m_cameras;

    Private() {}
    ~Private() {
        delete m_stereoRenderer;
        m_stereoRenderer = nullptr;
    }

    void create() {
        // Root entity in the 3D scene.
        m_sceneRootEntity = new Qt3DCore::QEntity();

        // Set up the player root entity (from which the cameras and hands are parented
        {
            m_player.m_entity = new Qt3DCore::QEntity(m_sceneRootEntity);
            m_player.m_transform = new Qt3DCore::QTransform();
            m_player.m_transform->setTranslation(QVector3D(0, 0, 2));
            m_player.m_entity->addComponent(m_player.m_transform);
            xr::for_each_side_index([&](uint32_t eyeIndex) {
                auto& hand = m_player.m_hands[eyeIndex];
                hand.m_entity = new Qt3DCore::QEntity(m_player.m_entity);
                // CylinderMesh Transform
                hand.m_transform = new Qt3DCore::QTransform();
                hand.m_entity->addComponent(hand.m_transform);

                //pointer.entity = new Qt3DCore::QEntity(pointer.handEntity);
                Qt3DExtras::QCylinderMesh* mesh = new Qt3DExtras::QCylinderMesh();
                mesh->setRadius(0.005f);
                mesh->setLength(0.30f);
                mesh->setRings(4);
                mesh->setSlices(4);
                hand.m_entity->addComponent(mesh);

                Qt3DExtras::QPhongMaterial* mat = new Qt3DExtras::QPhongMaterial();
                mat->setDiffuse(QColor(QRgb(0xFF00FF)));
                hand.m_entity->addComponent(mat);
            });

			// Set up a camera to point at the shapes.
            xr::for_each_side_index([&](uint32_t eyeIndex) {
                auto& camera = m_cameras[eyeIndex];
                camera = new Qt3DRender::QCamera(m_player.m_entity);
                camera->lens()->setPerspectiveProjection(45.0f, 1.0f, 0.1f, 1000.0f);
                camera->setPosition(QVector3D(0, 0, 0));
                camera->setUpVector(QVector3D(0, 1, 0));
                camera->setViewCenter(QVector3D(0, 0, -1));
            });
        }

        // skybox
        //{
        //    m_skyboxEntity = new Qt3DExtras::QSkyboxEntity(m_sceneRootEntity);
        //    m_skyboxEntity->setBaseName(QUrl::fromLocalFile("C:\\Users\\bdavi\\git\\OpenXRSamples\\data\\yokohama").toString());
        //    m_skyboxEntity->setExtension(".jpg");
        //}

        // Set up a light to illuminate the shapes.
        {
            Qt3DRender::QPointLight* light = new Qt3DRender::QPointLight();
            light->setColor("white");
            light->setIntensity(1);

            Qt3DCore::QTransform* lightTransform = new Qt3DCore::QTransform();
            lightTransform->setTranslation(m_cameras[0]->position());

            Qt3DCore::QEntity* lightEntity = new Qt3DCore::QEntity(m_sceneRootEntity);
            lightEntity->addComponent(light);
            lightEntity->addComponent(lightTransform);
        }

        // Torus
        {
            // Torus shape data
            Qt3DExtras::QTorusMesh* torusMesh = new Qt3DExtras::QTorusMesh();
            torusMesh->setRadius(1.0f);
            torusMesh->setMinorRadius(0.4f);
            torusMesh->setRings(100);
            torusMesh->setSlices(20);

            // TorusMesh Transform
            Qt3DCore::QTransform* torusTransform = new Qt3DCore::QTransform();
            torusTransform->setScale(2.0f * 0.05f);
            torusTransform->setRotation(QQuaternion::fromAxisAndAngle(QVector3D(0.0f, 1.0f, 0.0f), 25.0f));
            torusTransform->setTranslation(QVector3D(5.0f, 4.0f, 0.0f) * 0.05f);

            Qt3DExtras::QPhongMaterial* torusMaterial = new Qt3DExtras::QPhongMaterial();
            torusMaterial->setDiffuse(QColor(QRgb(0xbeb32b)));

            m_torusEntity = new Qt3DCore::QEntity(m_sceneRootEntity);
            m_torusEntity->addComponent(torusMesh);
            m_torusEntity->addComponent(torusTransform);
            m_torusEntity->addComponent(torusMaterial);
        }

        // Cone shape data
        {
            Qt3DExtras::QConeMesh* cone = new Qt3DExtras::QConeMesh();
            cone->setTopRadius(0.5);
            cone->setBottomRadius(1);
            cone->setLength(3);
            cone->setRings(50);
            cone->setSlices(20);

            // ConeMesh Transform
            Qt3DCore::QTransform* coneTransform = new Qt3DCore::QTransform();
            coneTransform->setScale(1.5f * 0.05f);
            coneTransform->setRotation(QQuaternion::fromAxisAndAngle(QVector3D(1.0f, 0.0f, 0.0f), 45.0f));
            coneTransform->setTranslation(QVector3D(0.0f, 4.0f, -1.5) * 0.05f);

            Qt3DExtras::QPhongMaterial* coneMaterial = new Qt3DExtras::QPhongMaterial();
            coneMaterial->setDiffuse(QColor(QRgb(0x928327)));

            // Cone
            m_coneEntity = new Qt3DCore::QEntity(m_sceneRootEntity);
            m_coneEntity->addComponent(cone);
            m_coneEntity->addComponent(coneMaterial);
            m_coneEntity->addComponent(coneTransform);
        }

        // Cylinder
        {
            // Cylinder shape data
            Qt3DExtras::QCylinderMesh* cylinder = new Qt3DExtras::QCylinderMesh();
            cylinder->setRadius(1);
            cylinder->setLength(3);
            cylinder->setRings(100);
            cylinder->setSlices(20);

            // CylinderMesh Transform
            Qt3DCore::QTransform* cylinderTransform = new Qt3DCore::QTransform();
            cylinderTransform->setScale(1.5f * 0.05f);
            cylinderTransform->setRotation(QQuaternion::fromAxisAndAngle(QVector3D(1.0f, 0.0f, 0.0f), 45.0f));
            cylinderTransform->setTranslation(QVector3D(-5.0f, 4.0f, -1.5) * 0.05f);

            Qt3DExtras::QPhongMaterial* cylinderMaterial = new Qt3DExtras::QPhongMaterial();
            cylinderMaterial->setDiffuse(QColor(QRgb(0x928327)));

            m_cylinderEntity = new Qt3DCore::QEntity(m_sceneRootEntity);
            m_cylinderEntity->addComponent(cylinder);
            m_cylinderEntity->addComponent(cylinderMaterial);
            m_cylinderEntity->addComponent(cylinderTransform);
        }

        //Cuboid
        {
            // Cuboid shape data
            Qt3DExtras::QCuboidMesh* cuboid = new Qt3DExtras::QCuboidMesh();

            // CuboidMesh Transform
            Qt3DCore::QTransform* cuboidTransform = new Qt3DCore::QTransform();
            cuboidTransform->setScale(4.0f * 0.05f);
            cuboidTransform->setTranslation(QVector3D(5.0f, -4.0f, 0.0f) * 0.05f);

            Qt3DExtras::QPhongMaterial* cuboidMaterial = new Qt3DExtras::QPhongMaterial();
            cuboidMaterial->setDiffuse(QColor(QRgb(0x665423)));

            m_cuboidEntity = new Qt3DCore::QEntity(m_sceneRootEntity);
            m_cuboidEntity->addComponent(cuboid);
            m_cuboidEntity->addComponent(cuboidMaterial);
            m_cuboidEntity->addComponent(cuboidTransform);
        }

        // Plane
        {
            // Plane shape data
            Qt3DExtras::QPlaneMesh* planeMesh = new Qt3DExtras::QPlaneMesh();
            planeMesh->setWidth(2);
            planeMesh->setHeight(2);

            // Plane mesh transform
            Qt3DCore::QTransform* planeTransform = new Qt3DCore::QTransform();
            planeTransform->setScale(1.3f * 0.05f);
            planeTransform->setRotation(QQuaternion::fromAxisAndAngle(QVector3D(1.0f, 0.0f, 0.0f), 45.0f));
            planeTransform->setTranslation(QVector3D(0.0f, -4.0f, 0.0f) * 0.05f);

            Qt3DExtras::QPhongMaterial* planeMaterial = new Qt3DExtras::QPhongMaterial();
            planeMaterial->setDiffuse(QColor(QRgb(0xa69929)));

            m_planeEntity = new Qt3DCore::QEntity(m_sceneRootEntity);
            m_planeEntity->addComponent(planeMesh);
            m_planeEntity->addComponent(planeMaterial);
            m_planeEntity->addComponent(planeTransform);
        }

        // Sphere
        {
            // Sphere shape data
            Qt3DExtras::QSphereMesh* sphereMesh = new Qt3DExtras::QSphereMesh();
            sphereMesh->setRings(20);
            sphereMesh->setSlices(20);
            sphereMesh->setRadius(2);

            // Sphere mesh transform
            Qt3DCore::QTransform* sphereTransform = new Qt3DCore::QTransform();

            sphereTransform->setScale(1.3f * 0.05f);
            sphereTransform->setTranslation(QVector3D(-5.0f, -4.0f, 0.0f) * 0.05f);

            Qt3DExtras::QPhongMaterial* sphereMaterial = new Qt3DExtras::QPhongMaterial();
            sphereMaterial->setDiffuse(QColor(QRgb(0xa69929)));

            m_sphereEntity = new Qt3DCore::QEntity(m_sceneRootEntity);
            m_sphereEntity->addComponent(sphereMesh);
            m_sphereEntity->addComponent(sphereMaterial);
            m_sphereEntity->addComponent(sphereTransform);
        }

#if 0 
    auto buggyEntity = new Qt3DCore::QEntity(m_sceneRootEntity);
    Qt3DCore::QTransform* buggyTransform = new Qt3DCore::QTransform();
    buggyTransform->setScale(0.05f);
    buggyTransform->setTranslation(QVector3D(-5.0f, -4.0f, 0.0f));
    Qt3DRender::QSceneLoader* sceneLoader = new Qt3DRender::QSceneLoader(buggyEntity);
    sceneLoader->setSource(QUrl::fromLocalFile("C:\\Users\\bdavi\\git\\OpenXRSamples\\data\\models\\Buggy.gltf"));
    buggyEntity->addComponent(buggyTransform);
    buggyEntity->addComponent(sceneLoader);

#endif
        m_stereoRenderer = new StereoRenderer(m_cameras);
        m_sceneRootEntity->setParent(m_stereoRenderer->aspectEngine->rootEntity().data());
    }
};

Scene::Scene() : d{ std::make_shared<Private>() } {};

void Scene::create() {
    d->create();
}

void Scene::setCubemap(const std::string& cubemapPrefix) {
}

void Scene::loadModel(const std::string& modelfile) {
}

void Scene::updateHands(const HandStates& handStates) {
    xr::for_each_side_index([&](uint32_t index) {
        const auto& handState = handStates[index];
        auto& hand = d->m_player.m_hands[index];
        hand.m_transform->setMatrix(fromXr(handState.grip));
    });
}

void Scene::updateEyes(const EyeStates& views) {
    xr::for_each_side_index([&](uint32_t index) {
        const auto& eyeView = views[index];
        // Now convert to Qt3D's weird lookat version of a view matrix
        Qt3DRender::QCamera* eyeCamera = d->m_cameras[index];
        eyeCamera->transform()->setMatrix(fromXr(eyeView.pose));
        // And a projection matrix
        eyeCamera->lens()->setProjectionMatrix(fromXrGL(eyeView.fov));
    });
}

void Scene::render(Framebuffer& framebuffer) {
    framebuffer.bind();
    glClearColor(0.2f, 0.2f, 0.2f, 1);
    framebuffer.clear();
    d->m_stereoRenderer->renderAspectPrivate->renderSynchronous(false);
    framebuffer.bindDefault();
}

}}  // namespace xr_examples::qt

#endif
