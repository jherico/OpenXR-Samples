#include "scene.hpp"

#ifdef HAVE_QT

#include <QtCore/QCoreApplication>
#include <QtCore/QTimer>
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
#include <Qt3DRender/QObjectPicker>

#include <Qt3DExtras/QTorusMesh>
#include <Qt3DExtras/QConeMesh>
#include <Qt3DExtras/QCylinderMesh>
#include <Qt3DExtras/QCuboidMesh>
#include <Qt3DExtras/QPhongMaterial>
#include <Qt3DExtras/QPlaneMesh>
#include <Qt3DExtras/QSkyboxEntity>
#include <Qt3DExtras/QSphereMesh>

#include <Qt3DRender/QPickEvent>
#include <Qt3DRender/QAttribute>
#include <Qt3DRender/QRenderAspect>
#include <Qt3DRender/QRenderSettings>
#include <Qt3DRender/QRenderSurfaceSelector>
#include <Qt3DRender/QViewport>
#include <Qt3DRender/QLayer>
#include <Qt3DRender/QLayerFilter>
#include <Qt3DRender/QRayCaster>
#include <Qt3DRender/private/qrenderaspect_p.h>

#include <Qt3DLogic/QLogicAspect>

#include <qt/math.hpp>

extern xr_examples::Window* g_window;

namespace xr_examples { namespace qt {

extern QObject* g_sceneSurface;
extern QOpenGLContext* g_sceneContext;

QMatrix4x4 scaling(const QVector3D& scaling) {
    QMatrix4x4 result;
    result.scale(scaling);
    return result;
}

QMatrix4x4 scaling(float scaling) {
    QMatrix4x4 result;
    result.scale(scaling);
    return result;
}

QMatrix4x4 translation(const QVector3D& v) {
    QMatrix4x4 result;
    result.translate(v);
    return result;
}

struct StereoRenderer {
    Qt3DCore::QAspectEngine* m_aspectEngine{ new Qt3DCore::QAspectEngine };
    //Qt3DCore::QAspectEnginePrivate* m_aspectEnginePrivate;
    Qt3DLogic::QLogicAspect* m_logicAspect{ new Qt3DLogic::QLogicAspect };
    Qt3DRender::QRenderAspect* m_renderAspect{ new Qt3DRender::QRenderAspect(Qt3DRender::QRenderAspect::Synchronous) };
    Qt3DRender::QRenderAspectPrivate* m_renderAspectPrivate{ nullptr };
    Qt3DRender::QRenderSurfaceSelector* m_renderSurfaceSelector{ nullptr };

    StereoRenderer(const std::array<Qt3DRender::QCamera*, 2>& cameras) {
        m_aspectEngine->registerAspect(m_renderAspect);
        m_aspectEngine->registerAspect(m_logicAspect);
        m_aspectEngine->setRunMode(Qt3DCore::QAspectEngine::Manual);

        Qt3DCore::QEntityPtr root(new Qt3DCore::QEntity());
        auto renderSettings = new Qt3DRender::QRenderSettings(root.data());
        root->addComponent(renderSettings);
        m_renderSurfaceSelector = new Qt3DRender::QRenderSurfaceSelector(renderSettings);
        m_renderSurfaceSelector->setSurface(g_sceneSurface);
        for (uint32_t eyeIndex = 0; eyeIndex < 2; ++eyeIndex) {
            auto stereoViewport = new Qt3DRender::QViewport(m_renderSurfaceSelector);
            QRectF viewport = QRectF(eyeIndex == 0 ? 0.0 : 0.5, 0.0, 0.5, 1.0);
            stereoViewport->setNormalizedRect(viewport);
            auto cameraSelector = new Qt3DRender::QCameraSelector(stereoViewport);
            cameraSelector->setCamera(cameras[eyeIndex]);
        }
        renderSettings->pickingSettings()->setPickMethod(Qt3DRender::QPickingSettings::BoundingVolumePicking);
        renderSettings->pickingSettings()->setPickResultMode(Qt3DRender::QPickingSettings::NearestPick);
        renderSettings->setActiveFrameGraph(m_renderSurfaceSelector);
        m_renderAspectPrivate =
            static_cast<Qt3DRender::QRenderAspectPrivate*>(Qt3DRender::QRenderAspectPrivate::get(m_renderAspect));
        m_renderAspectPrivate->renderInitialize(g_sceneContext);
        m_aspectEngine->setRootEntity(root);
    }

    ~StereoRenderer() {
        // Clean up after ourselves.
        m_aspectEngine->setRootEntity(Qt3DCore::QEntityPtr());
        m_aspectEngine->unregisterAspect(m_logicAspect);
        m_aspectEngine->unregisterAspect(m_renderAspect);
        delete m_logicAspect;
        delete m_renderAspect;
        delete m_aspectEngine;
    }
};

struct TransformEntity : public Qt3DCore::QEntity {
private:
    using Parent = Qt3DCore::QEntity;

public:
    using Transform = Qt3DCore::QTransform;
    TransformEntity(Qt3DCore::QNode* parent = nullptr) : Parent(parent) { addComponent(m_transform); }

    void setTransform(const QMatrix4x4& mat) { m_transform->setMatrix(mat); }

    Transform* m_transform{ new Transform{ this } };
};

struct PhongEntity : public TransformEntity {
private:
    using Parent = TransformEntity;

public:
    PhongEntity(Qt3DCore::QNode* parent = nullptr) : Parent(parent) { addComponent(m_mat); }

    Qt3DExtras::QPhongMaterial* m_mat{ new Qt3DExtras::QPhongMaterial };
};

struct CubeEntity : public PhongEntity {
private:
    using Parent = PhongEntity;

public:
    CubeEntity(const QVector3D& extent, Qt3DCore::QNode* parent = nullptr) : Parent(parent) {
        auto mesh = new Qt3DExtras::QCuboidMesh();
        mesh->setXExtent(extent.x());
        mesh->setYExtent(extent.y());
        mesh->setZExtent(extent.z());
        addComponent(mesh);
    };

    CubeEntity(float extent, Qt3DCore::QNode* parent = nullptr) : CubeEntity({ extent, extent, extent }, parent){};
};

struct BoxEntity : public TransformEntity {
private:
    using Parent = TransformEntity;

public:
    BoxEntity(Qt3DCore::QNode* parent = nullptr) : Parent(parent) {
        constexpr float S = 0.01f;
        constexpr float B = 1.0f;
        constexpr float M = -0.5f;
        constexpr float P = 0.5f;
        CubeEntity* ce;
        ce = new CubeEntity({ S, B, S }, this);
        ce->m_transform->setTranslation({ P, 0, P });
        ce = new CubeEntity({ S, B, S }, this);
        ce->m_transform->setTranslation({ P, 0, M });
        ce = new CubeEntity({ S, B, S }, this);
        ce->m_transform->setTranslation({ M, 0, M });
        ce = new CubeEntity({ S, B, S }, this);
        ce->m_transform->setTranslation({ M, 0, P });

        ce = new CubeEntity({ S, S, B }, this);
        ce->m_transform->setTranslation({ P, P, 0 });
        ce = new CubeEntity({ S, S, B }, this);
        ce->m_transform->setTranslation({ P, M, 0 });
        ce = new CubeEntity({ S, S, B }, this);
        ce->m_transform->setTranslation({ M, M, 0 });
        ce = new CubeEntity({ S, S, B }, this);
        ce->m_transform->setTranslation({ M, P, 0 });

        ce = new CubeEntity({ B, S, S }, this);
        ce->m_transform->setTranslation({ 0, P, P });
        ce = new CubeEntity({ B, S, S }, this);
        ce->m_transform->setTranslation({ 0, P, M });
        ce = new CubeEntity({ B, S, S }, this);
        ce->m_transform->setTranslation({ 0, M, M });
        ce = new CubeEntity({ B, S, S }, this);
        ce->m_transform->setTranslation({ 0, M, P });
    }
};

struct LineEntity : public TransformEntity {
private:
    using Parent = TransformEntity;

public:
    LineEntity(Qt3DCore::QNode* parent = nullptr) : Parent(parent) {
        CubeEntity* ce;
        ce = new CubeEntity({ 0.01f, 0.01f, 1.0 }, this);
        ce->m_transform->setTranslation({ 0, 0, -0.5f });
    }
};

struct Scene::Private {
    StereoRenderer* m_stereoRenderer{ nullptr };
    Qt3DCore::QEntity* m_sceneRootEntity{ nullptr };
    Qt3DCore::QEntity* m_diageticRootEntity{ nullptr };
    Qt3DRender::QLayer* m_diageticLayer{ nullptr };
    Qt3DCore::QEntity* m_coneEntity{ nullptr };
    Qt3DCore::QEntity* m_cylinderEntity{ nullptr };
    Qt3DCore::QEntity* m_torusEntity{ nullptr };
    Qt3DCore::QEntity* m_cuboidEntity{ nullptr };
    Qt3DCore::QEntity* m_planeEntity{ nullptr };
    Qt3DCore::QEntity* m_sphereEntity{ nullptr };
    Qt3DCore::QEntity* m_lightEntity{ nullptr };
    Qt3DExtras::QSkyboxEntity* m_skyboxEntity{ nullptr };
    BoxEntity* m_boundingBox{ nullptr };
    LineEntity* m_lineEntity{ nullptr };
    Qt3DRender::QRayCaster* m_rayCaster{ nullptr };

    struct Hand : public Qt3DCore::QEntity {
    private:
        using Parent = Qt3DCore::QEntity;

    public:
        CubeEntity* grip{ new CubeEntity{ 0.01f, this } };
        CubeEntity* aim{ new CubeEntity{ 0.01f, this } };

        Hand(uint32_t side, Qt3DCore::QNode* parent) : Parent(parent) {
            aim->setObjectName(QString("Aim %d").arg(side));
            aim->m_mat->setDiffuse(QColor(QRgb(side == 0 ? 0xFF0000 : 0x00FF00)));
            grip->m_mat->setDiffuse(QColor(QRgb(side == 0 ? 0xFF0000 : 0x00FF00)));
            grip->setObjectName(QString("Grip %d").arg(side));
        }
    };

    struct Player : public TransformEntity {
    private:
        using Parent = TransformEntity;

    public:
        const QVector3D DEFAULT_POSITION{ 0.0f, 0.2f, 0.65f };
        std::array<Hand*, 2> m_hands;
        Player(Qt3DCore::QNode* parent) : Parent(parent) {
            m_transform->setTranslation(DEFAULT_POSITION);
            xr::for_each_side_index([&](uint32_t sideIndex) { m_hands[sideIndex] = new Hand(sideIndex, this); });
        };
    } * m_player{ nullptr };

    std::array<Qt3DRender::QCamera*, 2> m_cameras{ nullptr, nullptr };
    Qt3DCore::QTransform* m_cuboidTransform = nullptr;
    Private() {}
    ~Private() {
        delete m_stereoRenderer;
        m_stereoRenderer = nullptr;
    }

    void reportHits() {
        auto hits = m_rayCaster->hits();
        auto hitCount = hits.size();
        for (uint32_t i = 0; i < hitCount; ++i) {
            const auto& hit = hits.at(i);
            auto entity = hit.entity()->objectName();
            auto transform = hit.entity()->componentsOfType<Qt3DCore::QTransform>().at(0);
            if (transform) {
            m_boundingBox->m_transform->setTranslation(transform->translation());
            } else {
                m_boundingBox->m_transform->setTranslation(hit.worldIntersection());
            }
            m_boundingBox->m_transform->setScale(0.3f);
            qDebug() << "Hits" << hit.worldIntersection() << hit.entity()->objectName();
        }
    }

    void create() {
        // Root entity in the 3D scene.
        m_sceneRootEntity = new Qt3DCore::QEntity();

        m_diageticRootEntity = new Qt3DCore::QEntity(m_sceneRootEntity);
        m_diageticLayer = new Qt3DRender::QLayer(m_diageticRootEntity);
        m_diageticLayer->setRecursive(true);
        m_diageticRootEntity->addComponent(m_diageticLayer);
        m_boundingBox = new BoxEntity(m_diageticLayer);
        m_lineEntity = new LineEntity(m_diageticLayer);
        m_rayCaster = new Qt3DRender::QRayCaster{ m_diageticLayer };
        m_rayCaster->setRunMode(Qt3DRender::QRayCaster::SingleShot);
        m_rayCaster->setFilterMode(Qt3DRender::QRayCaster::DiscardAnyMatchingLayers);
        m_diageticRootEntity->addComponent(m_rayCaster);
        qRegisterMetaType<Qt3DRender::QRayCaster::Hits>();
        QObject::connect(m_rayCaster, &Qt3DRender::QRayCaster::hitsChanged, [this] { reportHits(); });
        m_rayCaster->addLayer(m_diageticLayer);

        // Set up the player root entity (from which the cameras and hands are parented
        {
            m_player = new Player(m_diageticRootEntity);
            m_player->m_transform->setTranslation(QVector3D(0, 0, 2));
            // Set up a camera to point at the shapes.
            xr::for_each_side_index([&](uint32_t eyeIndex) {
                auto& camera = m_cameras[eyeIndex];
                camera = new Qt3DRender::QCamera(m_player);
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
            m_torusEntity->setObjectName("Torus");
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
            m_coneEntity->setObjectName("Cone");
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
            m_cylinderEntity->setObjectName("Cylinder");
        }

        //Cuboid
        {
            // Cuboid shape data
            Qt3DExtras::QCuboidMesh* cuboid = new Qt3DExtras::QCuboidMesh();
            cuboid->setXExtent(0.2f);
            cuboid->setYExtent(0.2f);
            cuboid->setZExtent(0.2f);
            // CuboidMesh Transform
            m_cuboidTransform = new Qt3DCore::QTransform();
            m_cuboidTransform->setTranslation(QVector3D(5.0f, -4.0f, 0.0f) * 0.05f);

            Qt3DExtras::QPhongMaterial* cuboidMaterial = new Qt3DExtras::QPhongMaterial();
            cuboidMaterial->setDiffuse(QColor(QRgb(0x665423)));

            m_cuboidEntity = new Qt3DCore::QEntity(m_sceneRootEntity);
            m_cuboidEntity->addComponent(cuboid);
            m_cuboidEntity->addComponent(cuboidMaterial);
            m_cuboidEntity->addComponent(m_cuboidTransform);
            m_cuboidEntity->setObjectName("Cube");
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
            m_planeEntity->setObjectName("Plane");
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
            m_sphereEntity->setObjectName("Sphere");
        }

        m_stereoRenderer = new StereoRenderer(m_cameras);
        m_sceneRootEntity->setParent(m_stereoRenderer->m_aspectEngine->rootEntity().data());
    }

    void render(const xr::Extent2Di& size) {
        m_stereoRenderer->m_renderSurfaceSelector->setExternalRenderTargetSize({size.width, size.height});
        m_stereoRenderer->m_aspectEngine->processFrame();
        m_stereoRenderer->m_renderAspectPrivate->renderSynchronous(false);
    }

    static QVector3D minVec(const QVector3D& a, const QVector3D& b) {
        return { std::min(a.x(), b.x()), std::min(a.y(), b.y()), std::min(a.z(), b.z()) };
    }
    static QVector3D maxVec(const QVector3D& a, const QVector3D& b) {
        return { std::max(a.x(), b.x()), std::max(a.y(), b.y()), std::max(a.z(), b.z()) };
    }
    void scanTree(Qt3DCore::QEntity* node, QVector3D& minExtent, QVector3D& maxExtent) {
        if (!node) {
            return;
        }
        for (const auto& renderer : node->componentsOfType<Qt3DRender::QGeometryRenderer>()) {
            auto geometry = renderer->geometry();
            minExtent = minVec(geometry->minExtent(), minExtent);
            maxExtent = maxVec(geometry->maxExtent(), maxExtent);
        }

        for (const auto& geometry : node->componentsOfType<Qt3DRender::QGeometry>()) {
            minExtent = minVec(geometry->minExtent(), minExtent);
            maxExtent = maxVec(geometry->maxExtent(), maxExtent);
        }
        for (const auto& child : node->childNodes()) {
            scanTree(dynamic_cast<Qt3DCore::QEntity*>(child), minExtent, maxExtent);
        }
    }

    void sceneLoaded(TransformEntity* modelRoot, Qt3DRender::QSceneLoader* loader) {
        QTimer::singleShot(1000, [=]() {
            auto root = loader->entity("");
            auto geometry = root->componentsOfType<Qt3DRender::QGeometryRenderer>()[0]->geometry();
            QVector3D min = geometry->minExtent(), max = geometry->maxExtent();
            //scanTree(modelRoot, min, max);
            auto extent = max - min;
            auto longestDimension = std::max(std::max(extent.x(), extent.y()), extent.z());
            float scale = 0.5f / longestDimension;
            modelRoot->m_transform->setScale(scale);
        });
    }
};

Scene::Scene() : d{ std::make_shared<Private>() } {};

void Scene::create() {
    d->create();
}

void Scene::setCubemap(const std::string& cubemapPrefix) {
}

void Scene::loadModel(const std::string& modelfile) {
    auto modelEntity = new TransformEntity(d->m_sceneRootEntity);

    modelEntity->m_transform->setScale(0.05f);
    modelEntity->m_transform->setTranslation(QVector3D(-5.0f, -4.0f, 0.0f));
    Qt3DRender::QSceneLoader* sceneLoader = new Qt3DRender::QSceneLoader(modelEntity);
    sceneLoader->setSource(QUrl::fromLocalFile(modelfile.c_str()));
    modelEntity->addComponent(sceneLoader);
    if (sceneLoader->status() == Qt3DRender::QSceneLoader::Ready) {
        d->sceneLoaded(modelEntity, sceneLoader);
    } else {
        QObject::connect(sceneLoader, &Qt3DRender::QSceneLoader::statusChanged, [=] {
            if (sceneLoader->status() == Qt3DRender::QSceneLoader::Ready) {
                d->sceneLoaded(modelEntity, sceneLoader);
            }
        });
    }
}

void Scene::updateHands(const HandStates& handStates) {
    xr::for_each_side_index([&](uint32_t index) {
        const auto& handState = handStates[index];
        auto& hand = d->m_player->m_hands[index];

        hand->grip->m_transform->setMatrix(fromXr(handState.grip) * scaling(1.0f + (4.0f * handState.squeeze)));
        hand->aim->m_transform->setMatrix(fromXr(handState.aim) * translation({ 0.0f, 0.0f, handState.trigger * -0.10f }));

/*
        if (index == 0 && handState.trigger > 0.5f) {
            auto aimTransform = hand->aim->m_transform->worldMatrix();
            d->m_lineEntity->m_transform->setMatrix(aimTransform);
            auto origin = aimTransform * QVector3D{ 0, 0, 0 };
            auto direction = (aimTransform * QVector3D{ 0, 0, -1 }) - origin;
            d->m_rayCaster->trigger(origin, direction, 100.0f);
            //d->m_cuboidTransform->setMatrix(translation(origin) * QMatrix4x4(rotation.toRotationMatrix()));
        } else {
            //            d->m_cuboidTransform->setMatrix({});
            //            d->m_cuboidTransform->setScale(4.0f * 0.05f);
            //            d->m_cuboidTransform->setTranslation(QVector3D(5.0f, -4.0f, 0.0f) * 0.05f);
        }
        if (index == 0) {
            QVector3D playerTranslate{ handState.thumb.x, 0, -handState.thumb.y };
            auto curMatrix = d->m_player->m_transform->matrix();
            curMatrix.translate(playerTranslate * 0.01f);
            d->m_player->m_transform->setMatrix(curMatrix);

            if (handState.thumbClicked) {
                QMatrix4x4 resetTransform;
                resetTransform.translate({ 0.0f, 0.2f, 0.65f });
                d->m_player->m_transform->setMatrix(resetTransform);
            }
        }
*/
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
    //QCoreApplication::processEvents();
    d->render(framebuffer.getSize());
}
}}  // namespace xr_examples::qt

#endif
