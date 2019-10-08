#include "pipeline.hpp"
#include <glad/glad.h>

using namespace xr_examples::gl;

void Pipeline::addShaderSources(ShaderStage stage, const StringArrayProxy& newShaderSources) {
    auto& list = shaderSources[stage];
    for (const auto& shaderSource : newShaderSources) {
        list.push_back(shaderSource);
    }
}

void Pipeline::destroy() noexcept {
    if (program != 0) {
        glDeleteProgram(program);
        program = 0;
    }
    if (vao != 0) {
        glDeleteVertexArrays(1, &vao);
        vao = 0;
    }
}

void Pipeline::setAttributeFormat(uint32_t location, const AttributeFormat& format) {
    attributes[location] = format;
}

void Pipeline::setAttributeFormat(uint32_t location, uint32_t size, uint32_t scalarType, uint32_t offset) {
    setAttributeFormat(location, { AttributeFormat::Type::Float, size, scalarType, offset });
}
void Pipeline::setAttributeIFormat(uint32_t location, uint32_t size, uint32_t scalarType, uint32_t offset) {
    setAttributeFormat(location, { AttributeFormat::Type::Integer, size, scalarType, offset });
}
void Pipeline::setAttributeNFormat(uint32_t location, uint32_t size, uint32_t scalarType, uint32_t offset) {
    setAttributeFormat(location, { AttributeFormat::Type::NFloat, size, scalarType, offset });
}
void Pipeline::setAttributeLFormat(uint32_t location, uint32_t size, uint32_t scalarType, uint32_t offset) {
    setAttributeFormat(location, { AttributeFormat::Type::Long, size, scalarType, offset });
}

void Pipeline::setAttributeFormat(uint32_t location,
                                  AttributeFormat::Type formatType,
                                  uint32_t size,
                                  uint32_t scalarType,
                                  uint32_t offset) {
    setAttributeFormat(location, { formatType, size, scalarType, offset });
}

static GLenum toGl(ShaderStage stage) {
    switch (stage) {
        case ShaderStage::eVertex:
            return GL_VERTEX_SHADER;
        case ShaderStage::eFragment:
            return GL_FRAGMENT_SHADER;
        default:
            break;
    }
    throw std::runtime_error("Not implemented");
}

template <typename V>
std::vector<const GLchar*> getSources(const V& sources) {
    std::vector<const GLchar*> result;
    for (const auto& source : sources) {
        result.emplace_back(source.c_str());
    }
    return result;
}

static std::string getShaderInfoLog(GLuint glshader) {
    std::string result;
    GLint infoLength = 0;
    glGetShaderiv(glshader, GL_INFO_LOG_LENGTH, &infoLength);
    if (infoLength > 0) {
        char* temp = new char[infoLength];
        glGetShaderInfoLog(glshader, infoLength, NULL, temp);
        result = std::string(temp);
        delete[] temp;
    }
    return result;
}

static std::string getProgramInfoLog(GLuint glprogram) {
    std::string result;
    GLint infoLength = 0;
    glGetProgramiv(glprogram, GL_INFO_LOG_LENGTH, &infoLength);
    if (infoLength > 0) {
        char* temp = new char[infoLength];
        glGetProgramInfoLog(glprogram, infoLength, NULL, temp);
        result = std::string(temp);
        delete[] temp;
    }
    return result;
}

void Pipeline::create() {
    destroy();
    std::vector<uint32_t> glshaders;
    try {
        glCreateVertexArrays(1, &vao);
        for (const auto& entry : attributes) {
            const auto& location = entry.first;
            const auto& format = entry.second;
            glEnableVertexArrayAttrib(vao, location);
            switch (format.type) {
                case AttributeFormat::Type::Float:
                    glVertexArrayAttribFormat(vao, location, format.size, format.scalarType, GL_FALSE, format.offset);
                    break;
                case AttributeFormat::Type::NFloat:
                    glVertexArrayAttribFormat(vao, location, format.size, format.scalarType, GL_TRUE, format.offset);
                    break;
                case AttributeFormat::Type::Integer:
                    glVertexArrayAttribIFormat(vao, location, format.size, format.scalarType, format.offset);
                    break;
                case AttributeFormat::Type::Long:
                    glVertexArrayAttribLFormat(vao, location, format.size, format.scalarType, format.offset);
                    break;
                default:
                    break;
            }
        }

        program = glCreateProgram();
        for (const auto& entry : shaderSources) {
            const auto& shaderStage = entry.first;
            uint32_t glshader;
            {
                glshader = glCreateShader(toGl(shaderStage));
                if (!glshader) {
                    throw std::runtime_error("Shader creation failed");
                }
            }
            glshaders.push_back(glshader);
            {
                const auto& sources = entry.second;
                uint32_t sourceCount = (uint32_t)sources.size();
                auto csources = getSources(sources);
                glShaderSource(glshader, sourceCount, csources.data(), nullptr);
                glCompileShader(glshader);
            }
            {
                int32_t compileStatus;
                glGetShaderiv(glshader, GL_COMPILE_STATUS, &compileStatus);
                if (GL_TRUE != compileStatus) {
                    std::string error = getShaderInfoLog(glshader);
                    throw std::runtime_error("Failed to compile shader");
                }
            }
            glAttachShader(program, glshader);
        }
        glLinkProgram(program);
        {
            int32_t linkStatus;
            glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
            if (GL_TRUE != linkStatus) {
                std::string error = getProgramInfoLog(program);
                throw std::runtime_error("Failed to link program");
            }
        }
    } catch (const std::runtime_error&) {
        for (const auto& glshader : glshaders) {
            glDeleteShader(glshader);
        }
        destroy();
        throw;
    }
}

void Pipeline::bind() {
    glUseProgram(program);
    glBindVertexArray(vao);
}
