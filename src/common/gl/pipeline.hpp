#pragma once

#include <interfaces.hpp>
#include <vulkan/vulkan.hpp>

namespace xr_examples { namespace gl {

using ShaderStage = vk::ShaderStageFlagBits;
using StringArrayProxy = vk::ArrayProxy<const std::string>;

struct AttributeFormat {
    enum class Type
    {
        Float,
        NFloat,
        Integer,
        Long,
    };
    AttributeFormat(Type type = Type::Float, uint32_t size = 4, uint32_t scalarType = 0x1406, uint32_t offset = 0) : type(type), size(size), scalarType(scalarType), offset(offset) {}
    Type type;
    uint32_t size;
    uint32_t scalarType;
    uint32_t offset;
};

struct Pipeline {
    void addShaderSources(ShaderStage stage, const StringArrayProxy& newShaderSources);
    void setAttributeFormat(uint32_t location, const AttributeFormat& formatType);
    void setAttributeFormat(uint32_t location, AttributeFormat::Type formatType, uint32_t size, uint32_t scalarType, uint32_t offset = 0);
    void setAttributeFormat(uint32_t location, uint32_t size, uint32_t scalarType, uint32_t offset = 0);
    void setAttributeNFormat(uint32_t location, uint32_t size, uint32_t scalarType, uint32_t offset = 0);
    void setAttributeLFormat(uint32_t location, uint32_t size, uint32_t scalarType, uint32_t offset = 0);
    void setAttributeIFormat(uint32_t location, uint32_t size, uint32_t scalarType, uint32_t offset = 0);
    void destroy() noexcept;
    void create();
    void bind();

    std::unordered_map<ShaderStage, std::list<std::string>> shaderSources;
    std::unordered_map<uint32_t, AttributeFormat> attributes;
    uint32_t vao{ 0 };
    uint32_t program{ 0 };
};

}}  // namespace xr_examples::gl
