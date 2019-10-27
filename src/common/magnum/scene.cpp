#include "scene.hpp"

#include <openxr/openxr.hpp>

#pragma warning(push)
#pragma warning(disable : 4251)
#pragma warning(disable : 4267)
#pragma warning(disable : 4244)

#include <Corrade/Containers/Array.h>
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/Reference.h>
#include <Corrade/PluginManager/Manager.h>
#include <Corrade/Utility/Resource.h>
#include <Magnum/ImageView.h>
#include <Magnum/Mesh.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/PixelStorage.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/BufferImage.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/GL/CubeMapTexture.h>
#include <Magnum/GL/Framebuffer.h>
#include <Magnum/GL/PixelFormat.h>
#include <Magnum/ResourceManager.h>
#include <Magnum/MeshTools/Compile.h>
#include <Magnum/Primitives/Line.h>
#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/SceneGraph/Drawable.h>
#include <Magnum/SceneGraph/MatrixTransformation3D.h>
#include <Magnum/SceneGraph/Scene.h>
#include <Magnum/Shaders/Phong.h>
#include <Magnum/Shaders/Flat.h>
#include <Magnum/Trade/AbstractImporter.h>
#include <Magnum/Trade/ImageData.h>
#include <Magnum/Trade/MeshData3D.h>
#include <Magnum/Trade/MeshObjectData3D.h>
#include <Magnum/Trade/PhongMaterialData.h>
#include <Magnum/Trade/SceneData.h>
#include <Magnum/Trade/TextureData.h>
#include <Magnum/Platform/Sdl2Application.h>

#include <Magnum/GL/Shader.h>
#include <Magnum/GL/Version.h>

#include <Magnum/Math/Functions.h>
#include <Magnum/MeshTools/FlipNormals.h>
#include <Magnum/MeshTools/Interleave.h>
#include <Magnum/MeshTools/CompressIndices.h>
#include <Magnum/Primitives/Cube.h>
#pragma warning(pop)

#include <basis.hpp>
#include <assets.hpp>

#include <magnum/math.hpp>

namespace xr_examples { namespace magnum { namespace impl {

using namespace Magnum;
using namespace Magnum::Math::Literals;

using Scene3D = SceneGraph::Scene<SceneGraph::MatrixTransformation3D>;
using Object3D = SceneGraph::Object<SceneGraph::MatrixTransformation3D>;

using SceneResourceManager = Magnum::ResourceManager<GL::Buffer,
                                                     GL::Mesh,
                                                     Trade::AbstractImporter,
                                                     GL::Texture2D,
                                                     GL::CubeMapTexture,
                                                     GL::AbstractShaderProgram>;

class FlatDrawable : public SceneGraph::Drawable3D {
public:
    explicit FlatDrawable(Object3D& object,
                             Shaders::Flat3D& shader,
                             GL::Mesh& mesh,
                             const Color4& color,
                             SceneGraph::DrawableGroup3D& group) :
        SceneGraph::Drawable3D{ object, &group },
        _shader(shader), _mesh(mesh), _color{ color } {}

    void draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) override {
        _shader.setColor(_color).setTransformationProjectionMatrix(camera.projectionMatrix() * transformationMatrix);
        _mesh.draw(_shader);
    }

    Shaders::Flat3D& _shader;
    GL::Mesh& _mesh;
    Color4 _color;
};

class ColoredDrawable : public SceneGraph::Drawable3D {
public:
    explicit ColoredDrawable(Object3D& object,
                             Shaders::Phong& shader,
                             GL::Mesh& mesh,
                             const Color4& color,
                             SceneGraph::DrawableGroup3D& group) :
        SceneGraph::Drawable3D{ object, &group },
        _shader(shader), _mesh(mesh), _color{ color } {}

    void draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) override {
        _shader.setDiffuseColor(_color)
            .setLightPosition(camera.cameraMatrix().transformPoint({ -3.0f, 10.0f, 10.0f }))
            .setTransformationMatrix(transformationMatrix)
            .setNormalMatrix(transformationMatrix.rotationScaling())
            .setProjectionMatrix(camera.projectionMatrix());
        _mesh.draw(_shader);
    }

    Shaders::Phong& _shader;
    GL::Mesh& _mesh;
    Color4 _color;
};

class TexturedDrawable : public SceneGraph::Drawable3D {
public:
    explicit TexturedDrawable(Object3D& object,
                              Shaders::Phong& shader,
                              GL::Mesh& mesh,
                              GL::Texture2D& texture,
                              SceneGraph::DrawableGroup3D& group) :
        SceneGraph::Drawable3D{ object, &group },
        _shader(shader), _mesh(mesh), _texture(texture) {}

    void draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) override {
        _shader.setLightPosition(camera.cameraMatrix().transformPoint({ -3.0f, 10.0f, 10.0f }))
            .setTransformationMatrix(transformationMatrix)
            .setNormalMatrix(transformationMatrix.rotationScaling())
            .setProjectionMatrix(camera.projectionMatrix())
            .bindDiffuseTexture(_texture);

        _mesh.draw(_shader);
    }

    Shaders::Phong& _shader;
    GL::Mesh& _mesh;
    GL::Texture2D& _texture;
};

namespace Attributes {
using Position = GL::Attribute<0, Vector3>;
}

struct Shared {
    SceneResourceManager resourceManager;
    PluginManager::Manager<Trade::AbstractImporter> manager;

    Resource<GL::Mesh> buildCubePrimitive() {
        Resource<GL::Mesh> cube = resourceManager.get<GL::Mesh>("cube");
        if (!cube) {
            Trade::MeshData3D cubeData = Primitives::cubeSolid();

            GL::Buffer* buffer = new GL::Buffer;
            buffer->setData(MeshTools::interleave(cubeData.positions(0), cubeData.normals(0)), GL::BufferUsage::StaticDraw);

            Containers::Array<char> indexData;
            MeshIndexType indexType;
            UnsignedInt indexStart, indexEnd;
            std::tie(indexData, indexType, indexStart, indexEnd) = MeshTools::compressIndices(cubeData.indices());

            GL::Buffer* indexBuffer = new GL::Buffer;
            indexBuffer->setData(indexData, GL::BufferUsage::StaticDraw);

            GL::Mesh* mesh = new GL::Mesh;
            mesh->setPrimitive(cubeData.primitive())
                .setCount((Magnum::Int)cubeData.indices().size())
                .addVertexBuffer(*buffer, 0, Shaders::Phong::Position{}, Shaders::Phong::Normal{})
                .setIndexBuffer(*indexBuffer, 0, indexType, indexStart, indexEnd);

            resourceManager.set("cube-buffer", buffer, ResourceDataState::Final, ResourcePolicy::Resident)
                .set("cube-index-buffer", indexBuffer, ResourceDataState::Final, ResourcePolicy::Resident)
                .set(cube.key(), mesh, ResourceDataState::Final, ResourcePolicy::Resident);
        }
        return cube;
    }

    Resource<GL::Mesh> buildLinePrimitive() {
        Resource<GL::Mesh> line = resourceManager.get<GL::Mesh>("line");
        if (!line) {
            Trade::MeshData3D lineData = Primitives::line3D({ 0, 0, 0 }, { 0, 0, -10000 });

            GL::Buffer* buffer = new GL::Buffer;
            buffer->setData(lineData.positions(0), GL::BufferUsage::StaticDraw);

            GL::Mesh* mesh = new GL::Mesh;
            mesh->setPrimitive(lineData.primitive())
                .setCount((Magnum::Int)2)
                .addVertexBuffer(*buffer, 0, Shaders::Phong::Position{});

            resourceManager.set("line-buffer", buffer, ResourceDataState::Final, ResourcePolicy::Resident)
                .set(line.key(), mesh, ResourceDataState::Final, ResourcePolicy::Resident);
        }
        return line;
    }

    void shutdown() { resourceManager.clear(); }

    static Shared& get();
};

Shared& Shared::get() {
    static Shared shared;
    return shared;
}

class CubeMapShader : public GL::AbstractShaderProgram {
public:
    explicit CubeMapShader() {
        GL::Shader vert(GL::Version::GL330, GL::Shader::Type::Vertex);
        vert.addSource(assets::getAssetContents("shaders/CubeMapShader.vert"));
        GL::Shader frag(GL::Version::GL330, GL::Shader::Type::Fragment);
        frag.addSource(assets::getAssetContents("shaders/CubeMapShader.frag"));
        if (!GL::Shader::compile({ vert, frag })) {
            throw std::runtime_error("Failed to compile shader");
        }
        attachShaders({ vert, frag });
        if (!link()) {
            throw std::runtime_error("Failed to compile shader");
        }
        _transformationProjectionMatrixUniform = uniformLocation("transformationProjectionMatrix");
        setUniform(uniformLocation("textureData"), 0);
    }

    CubeMapShader& setTransformationProjectionMatrix(const Matrix4& matrix) {
        setUniform(_transformationProjectionMatrixUniform, matrix);
        return *this;
    }

    CubeMapShader& setTexture(GL::CubeMapTexture& texture) {
        texture.bind(0);
        return *this;
    }

private:
    Int _transformationProjectionMatrixUniform;
};

class CubeMap : public Object3D, SceneGraph::Drawable3D {
public:
    CubeMap(Object3D* parent, SceneGraph::DrawableGroup3D* group) : Object3D(parent), SceneGraph::Drawable3D(*this, group) {
        auto& shared = Shared::get();
        auto& resourceManager = shared.resourceManager;

        if (!(_texture = resourceManager.get<GL::CubeMapTexture>("texture"))) {
            GL::CubeMapTexture* cubeMap = new GL::CubeMapTexture;
            cubeMap->setWrapping(GL::SamplerWrapping::ClampToEdge)
                .setMagnificationFilter(GL::SamplerFilter::Linear)
                .setMinificationFilter(GL::SamplerFilter::Linear, GL::SamplerMipmap::Linear);
            resourceManager.set(_texture.key(), cubeMap, ResourceDataState::Final, ResourcePolicy::Manual);
        }

        if (!(_skybox = resourceManager.get<GL::Mesh>("skybox"))) {
            Trade::MeshData3D cubeData = Primitives::cubeSolid();
            MeshTools::flipFaceWinding(cubeData.indices());

            GL::Buffer* buffer = new GL::Buffer;
            buffer->setData(MeshTools::interleave(cubeData.positions(0)), GL::BufferUsage::StaticDraw);

            Containers::Array<char> indexData;
            MeshIndexType indexType;
            UnsignedInt indexStart, indexEnd;
            std::tie(indexData, indexType, indexStart, indexEnd) = MeshTools::compressIndices(cubeData.indices());

            GL::Buffer* indexBuffer = new GL::Buffer;
            indexBuffer->setData(indexData, GL::BufferUsage::StaticDraw);

            GL::Mesh* mesh = new GL::Mesh;
            mesh->setPrimitive(cubeData.primitive())
                .setCount((Magnum::Int)cubeData.indices().size())
                .addVertexBuffer(*buffer, 0, Attributes::Position{})
                .setIndexBuffer(*indexBuffer, 0, indexType, indexStart, indexEnd);

            resourceManager.set("skybox-buffer", buffer, ResourceDataState::Final, ResourcePolicy::Resident)
                .set("skybox-index-buffer", indexBuffer, ResourceDataState::Final, ResourcePolicy::Resident)
                .set(_skybox.key(), mesh, ResourceDataState::Final, ResourcePolicy::Resident);
        }

        if (!(_shader = resourceManager.get<GL::AbstractShaderProgram, CubeMapShader>("shader"))) {
            resourceManager.set<GL::AbstractShaderProgram>(_shader.key(), new CubeMapShader, ResourceDataState::Final,
                                                           ResourcePolicy::Manual);
        }
    }

    void loadImage(const std::string& filename) {
        auto& shared = Shared::get();
        auto& resourceManager = shared.resourceManager;

        auto buffer = assets::getAssetContentsBinary(filename);
        auto basisReader = std::make_shared<BasisReader>(buffer.data(), buffer.size());
        const auto& ii = basisReader->imageInfo;

        Vector2i size{ (int32_t)ii.m_orig_width, (int32_t)ii.m_orig_height };
        auto& cubemap = *_texture;
        cubemap.setStorage(Math::log2(size.min()) + 1, GL::TextureFormat::RGBA8, size);

        std::vector<uint8_t> imageBuffer;
        imageBuffer.resize(basisReader->getImageSize());
        Corrade::Containers::ArrayView<uint8_t> arrayView{ imageBuffer.data(), imageBuffer.size() };
        ImageView2D imageView{ GL::PixelFormat::RGBA, GL::PixelType::UnsignedByte, size, arrayView };
        basisReader->readImageToBuffer(imageBuffer.data(), 0, 0);
        cubemap.setSubImage(GL::CubeMapCoordinate::PositiveX, 0, {}, imageView);
        basisReader->readImageToBuffer(imageBuffer.data(), 0, 1);
        cubemap.setSubImage(GL::CubeMapCoordinate::NegativeX, 0, {}, imageView);
        basisReader->readImageToBuffer(imageBuffer.data(), 0, 2);
        cubemap.setSubImage(GL::CubeMapCoordinate::PositiveY, 0, {}, imageView);
        basisReader->readImageToBuffer(imageBuffer.data(), 0, 3);
        cubemap.setSubImage(GL::CubeMapCoordinate::NegativeY, 0, {}, imageView);
        basisReader->readImageToBuffer(imageBuffer.data(), 0, 4);
        cubemap.setSubImage(GL::CubeMapCoordinate::PositiveZ, 0, {}, imageView);
        basisReader->readImageToBuffer(imageBuffer.data(), 0, 5);
        cubemap.setSubImage(GL::CubeMapCoordinate::NegativeZ, 0, {}, imageView);
        cubemap.generateMipmap();
    }

    void draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) override {
        if (!_texture) {
            return;
        }

        _shader->setTransformationProjectionMatrix(camera.projectionMatrix() * Matrix4{ transformationMatrix.rotation() })
            .setTexture(*_texture);
        _skybox->draw(*_shader);
    }

    Resource<GL::Mesh> _skybox;
    Resource<GL::CubeMapTexture> _texture;
    Resource<GL::AbstractShaderProgram, CubeMapShader> _shader;
};

struct AABB {
    Vector3 scale;
    Vector3 corner;
    bool isInvalid() const { return corner.x() == std::numeric_limits<float>::infinity(); }

    Vector3 getSize() const { return scale - corner; };

    float scaleForFit(float targetSize) const {
        auto size = getSize();
        float longestDimension = std::max(std::max(size.x(), size.y()), size.z());
        return targetSize / longestDimension;
    }

    inline AABB& operator+=(const Vector3& point) {
        bool valid = !isInvalid();
        auto maximum = Math::max(corner + scale, point);
        corner = Math::min(corner, point);
        if (valid) {
            scale = maximum - corner;
        }
        return (*this);
    }

    inline Vector3 calcTopFarLeft() const { return corner + scale; }

    inline AABB& operator+=(const AABB& box) {
        if (!box.isInvalid()) {
            (*this) += box.corner;
            (*this) += box.calcTopFarLeft();
        }
        return (*this);
    }
};

}}};  // namespace xr_examples::magnum::impl

using namespace xr_examples::magnum;
using namespace xr_examples::magnum::impl;

struct Scene::Private {
    Scene3D scene;
    SceneGraph::DrawableGroup3D drawables;
    Object3D* playerRoot{ new Object3D(&scene) };
    Object3D* modelsRoot{ new Object3D(&scene) };
    CubeMap* cubemap{ nullptr };
    struct EyeData {
        // Holds the camera transform
        Object3D* cameraObject{ new Object3D() };
        // Holds the camera projection and executes the rendering
        SceneGraph::Camera3D* camera{ new SceneGraph::Camera3D{ *cameraObject } };
    };
    std::array<EyeData, 2> eyesData;

    struct HandData {
        Object3D* gripRoot{ new Object3D };
        ColoredDrawable* gripDrawable{ nullptr };

        Object3D* aimRoot{ new Object3D };
        ColoredDrawable* aimDrawable{ nullptr };
        FlatDrawable* lineDrawable{ nullptr };
    };
    std::array<HandData, 2> handsData;

    Shaders::Phong coloredShader, texturedShader{ Shaders::Phong::Flag::DiffuseTexture };
    Shaders::Flat3D flatShader;

    std::vector<AABB> meshExtents;
    Containers::Array<Containers::Optional<GL::Mesh>> meshes;
    Containers::Array<Containers::Optional<GL::Texture2D>> textures;
    Containers::Array<Containers::Optional<Trade::PhongMaterialData>> materials;

    Private() {
        setupImporters();
        setupRendering();
        setupBaseScene();
        setupHands();
    }

    ~Private() {
        scene.children().clear();
        Shared::get().shutdown();
    }

    void setupImporters() {
        auto& manager = Shared::get().manager;
        auto& resourceManager = Shared::get().resourceManager;
        // Setup importers
        resourceManager.set<Trade::AbstractImporter>("jpeg-importer", manager.loadAndInstantiate("JpegImporter").release(),
                                                     ResourceDataState::Final, ResourcePolicy::Manual);
        resourceManager.set<Trade::AbstractImporter>("png-importer", manager.loadAndInstantiate("PngImporter").release(),
                                                     ResourceDataState::Final, ResourcePolicy::Manual);
        resourceManager.set<Trade::AbstractImporter>("scene-importer", manager.loadAndInstantiate("TinyGltfImporter").release(),
                                                     ResourceDataState::Final, ResourcePolicy::Manual);
    }

    void setupRendering() {
        GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
        GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);
        coloredShader.setAmbientColor(0x111111_rgbf).setSpecularColor(0xffffff_rgbf).setShininess(80.0f);
        texturedShader.setAmbientColor(0x111111_rgbf).setSpecularColor(0x111111_rgbf).setShininess(80.0f);
    }

    void setupBaseScene() {
        playerRoot->translate({ 0.0f, 0.2f, 0.65f });
        xr::for_each_side_index([&](uint32_t eyeIndex) {
            auto& cameraObject = *eyesData[eyeIndex].cameraObject;
            cameraObject.setParent(playerRoot);
            auto& camera = *eyesData[eyeIndex].camera;
            camera.setAspectRatioPolicy(SceneGraph::AspectRatioPolicy::NotPreserved);
            camera.setProjectionMatrix(Matrix4::perspectiveProjection(35.0_degf, 1.0f, 0.01f, 1000.0f));
        });
    }

    void setupCubemap(const std::string cubemapPrefix) {
        cubemap = new CubeMap(&scene, &drawables);
        cubemap->scale(Vector3(20.0f));
        cubemap->loadImage(cubemapPrefix);
    }

    void setupHands() {
        auto& resourceManager = Shared::get().resourceManager;
        Resource<GL::Mesh> cubeMesh = Shared::get().buildCubePrimitive();
        Resource<GL::Mesh> lineMesh = Shared::get().buildLinePrimitive();
        xr::for_each_side_index([&](uint32_t eyeIndex) {
            auto color = eyeIndex == 0 ? 0xff0000_rgbf : 0x00ff00_rgbf;
            auto& handData = handsData[eyeIndex];

            handData.gripRoot = new Object3D(playerRoot);
            handData.gripRoot->setParent(playerRoot);
            handData.gripDrawable = new ColoredDrawable(*handData.gripRoot, coloredShader, *cubeMesh, color, drawables);

            handData.aimRoot = new Object3D(playerRoot);
            handData.aimRoot->setParent(playerRoot);
            handData.aimDrawable = new ColoredDrawable(*handData.aimRoot, coloredShader, *cubeMesh, color, drawables);
            handData.lineDrawable = new FlatDrawable(*handData.aimRoot, flatShader, *lineMesh, color, drawables);
        });
    }

    void loadScene(const std::string& filename) {
        auto& resourceManager = Shared::get().resourceManager;
        Resource<Trade::AbstractImporter> importer = resourceManager.get<Trade::AbstractImporter>("scene-importer");
        if (!importer->openFile(filename)) {
            throw std::runtime_error("Unable to open scene file");
        }

        textures = Containers::Array<Containers::Optional<GL::Texture2D>>{ importer->textureCount() };
        for (UnsignedInt i = 0; i != importer->textureCount(); ++i) {
            Debug{} << "Importing texture" << i << importer->textureName(i).c_str();

            Containers::Optional<Trade::TextureData> textureData = importer->texture(i);
            if (!textureData || textureData->type() != Trade::TextureData::Type::Texture2D) {
                Warning{} << "Cannot load texture properties, skipping";
                continue;
            }

            Debug{} << "Importing image" << textureData->image() << importer->image2DName(textureData->image()).c_str();

            Containers::Optional<Trade::ImageData2D> imageData = importer->image2D(textureData->image());
            GL::TextureFormat format;
            if (imageData && imageData->format() == PixelFormat::RGB8Unorm)
                format = GL::TextureFormat::RGB8;
            else if (imageData && imageData->format() == PixelFormat::RGBA8Unorm)
                format = GL::TextureFormat::RGBA8;
            else {
                Warning{} << "Cannot load texture image, skipping";
                continue;
            }

            /* Configure the texture */
            GL::Texture2D texture = GL::Texture2D();
            texture.setMagnificationFilter(textureData->magnificationFilter())
                .setMinificationFilter(textureData->minificationFilter(), textureData->mipmapFilter())
                .setWrapping(textureData->wrapping().xy())
                .setStorage(Math::log2(imageData->size().max()) + 1, format, imageData->size())
                .setSubImage(0, {}, *imageData)
                .generateMipmap();

            textures[i] = std::move(texture);
        }

        materials = Containers::Array<Containers::Optional<Trade::PhongMaterialData>>{ importer->materialCount() };
        for (UnsignedInt i = 0; i != importer->materialCount(); ++i) {
            Debug{} << "Importing material" << i << importer->materialName(i).c_str();

            Containers::Pointer<Trade::AbstractMaterialData> materialData = importer->material(i);
            if (!materialData || materialData->type() != Trade::MaterialType::Phong) {
                Warning{} << "Cannot load material, skipping";
                continue;
            }

            materials[i] = std::move(static_cast<Trade::PhongMaterialData&>(*materialData));
        }

        meshes = Containers::Array<Containers::Optional<GL::Mesh>>{ importer->mesh3DCount() };
        meshExtents.resize(importer->mesh3DCount());
        for (UnsignedInt i = 0; i != importer->mesh3DCount(); ++i) {
            Debug{} << "Importing mesh" << i << importer->mesh3DName(i).c_str();

            Containers::Optional<Trade::MeshData3D> meshData = importer->mesh3D(i);
            if (!meshData || !meshData->hasNormals() || meshData->primitive() != MeshPrimitive::Triangles) {
                Warning{} << "Cannot load the mesh, skipping";
                continue;
            }

            auto& aabb = meshExtents[i];
            for (uint32_t pi = 0; pi < meshData->positionArrayCount(); ++pi) {
                for (const auto& v : meshData->positions(pi)) {
                    aabb += v;
                }
            }
            /* Compile the mesh */
            meshes[i] = MeshTools::compile(*meshData);
        }

        auto* object = new Object3D{ modelsRoot };
        AABB modelExtent;
        if (importer->defaultScene() != -1) {
            Debug{} << "Adding default scene" << importer->sceneName(importer->defaultScene()).c_str();
            Containers::Optional<Trade::SceneData> sceneData = importer->scene(importer->defaultScene());
            if (!sceneData) {
                Error{} << "Cannot load scene, exiting";
                return;
            }
            for (UnsignedInt objectId : sceneData->children3D()) {
                modelExtent = addObject(*importer, object, objectId);
            }
        } else if (!meshes.empty() && meshes[0]) {
            new ColoredDrawable{ *object, coloredShader, *meshes[0], 0xffffff_rgbf, drawables };
            modelExtent = meshExtents[0];
        }

        // Scale down the root object to a reasonable size
        float modelScale = modelExtent.scaleForFit(0.5f);
        object->setTransformation(Matrix4::scaling({ modelScale, modelScale, modelScale }));
    }
    AABB addObject(Trade::AbstractImporter& importer, Object3D* parent, UnsignedInt i) {
        AABB extent;
        Containers::Pointer<Trade::ObjectData3D> objectData = importer.object3D(i);
        if (!objectData) {
            return extent;
        }

        auto* object = new Object3D{ parent };
        object->setTransformation(objectData->transformation());
        if (objectData->instanceType() == Trade::ObjectInstanceType3D::Mesh && objectData->instance() != -1 &&
            meshes[objectData->instance()]) {
            auto& mesh = meshes[objectData->instance()];
            auto& meshExtent = meshExtents[objectData->instance()];
            extent += meshExtent;
            const Int materialId = static_cast<Trade::MeshObjectData3D*>(objectData.get())->material();
            if (materialId == -1 || !materials[materialId]) {
                new ColoredDrawable{ *object, coloredShader, *meshes[objectData->instance()], 0xffffff_rgbf, drawables };
            } else if (materials[materialId]->flags() & Trade::PhongMaterialData::Flag::DiffuseTexture) {
                Containers::Optional<GL::Texture2D>& texture = textures[materials[materialId]->diffuseTexture()];
                if (texture) {
                    new TexturedDrawable{ *object, texturedShader, *meshes[objectData->instance()], *texture, drawables };
                } else {
                    new ColoredDrawable{ *object, coloredShader, *meshes[objectData->instance()], 0xffffff_rgbf, drawables };
                }
            } else {
                new ColoredDrawable{ *object, coloredShader, *meshes[objectData->instance()],
                                     materials[materialId]->diffuseColor(), drawables };
            }
        }

        for (std::size_t id : objectData->children()) {
            extent += addObject(importer, object, (Magnum::UnsignedInt)id);
        }
        return extent;
    }

    void render(Framebuffer& framebuffer) {
        xr::for_each_side_index([&](uint32_t eyeIndex) {
            framebuffer.setViewportSide(eyeIndex);
            auto& camera = *eyesData[eyeIndex].camera;
            camera.setViewport(fromXr(framebuffer.getEyeSize()));
            camera.draw(drawables);
        });
    }
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
    d->setupCubemap(cubemapPrefix);
}

void Scene::loadModel(const std::string& modelfile) {
    d->loadScene(modelfile);
}

void Scene::render(xr_examples::Framebuffer& stereoFramebuffer) {
    d->render(stereoFramebuffer);
}

void Scene::updateHands(const xr_examples::HandStates& handStates) {
    xr::for_each_side_index([&](uint32_t eyeIndex) {
        const auto& handState = handStates[eyeIndex];

        Matrix4 rot;
        if (handState.thumb.x != 0.0f || handState.thumb.y != 0.0f) {
            Vector3 thumb{ handState.thumb.x, 0, -handState.thumb.y };
            Vector3 normalizedThumb = thumb.normalized();
            Vector3 UP{ 0, 1, 0 };
            Vector3 axis{ Math::cross(UP, normalizedThumb) };
            rot = Matrix4{ Quaternion::rotation(Rad(3.14159f / 3.0f * thumb.length()), axis).toMatrix() };
        }

        auto& handData = d->handsData[eyeIndex];
        float scale = 0.01f + (0.05f * handState.squeeze);
        handData.gripRoot->setTransformation(fromXr(handState.grip) * rot * Matrix4::scaling({ scale, scale, scale }));

        if (eyeIndex == 0) {
            Vector3 playerTranslate{ handState.thumb.x, 0, -handState.thumb.y };
            d->playerRoot->translate(playerTranslate * 0.01f);
            if (handState.thumbClicked) {
                d->playerRoot->setTransformation(Matrix4::translation({ 0.0f, 0.2f, 0.65f }));
            }
        }

        scale = 0.01f;
        float translate = handState.trigger * 0.1f;
        handData.aimRoot->setTransformation(fromXr(handState.aim) * Matrix4::translation({ 0.0f, 0.0f, -translate }) *
                                            Matrix4::scaling({ scale, scale, scale }));
        handData.gripDrawable->_color = Color4{ fabs(handState.thumb.x), fabs(handState.thumb.y), 0.0 };
    });
}

void Scene::updateEyes(const xr_examples::EyeStates& eyeStates) {
    xr::for_each_side_index([&](uint32_t eyeIndex) {
        const auto& eyeState = eyeStates[eyeIndex];
        auto& eyeData = d->eyesData[eyeIndex];
        eyeData.camera->setProjectionMatrix(fromXrGL(eyeState.fov));
        eyeData.cameraObject->setTransformation(fromXr(eyeState.pose));
    });
}
