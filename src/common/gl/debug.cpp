#include "debug.hpp"

#include <logging.hpp>

#include <glad/glad.h>
#include <fmt/format.h>

static logging::Level toLogLevel(GLenum severity) {
    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH:
            return logging::Level::Error;
        case GL_DEBUG_SEVERITY_MEDIUM:
            return logging::Level::Warning;
        case GL_DEBUG_SEVERITY_LOW:
            return logging::Level::Info;
        default:
            return logging::Level::Error;
    }
}

void GLAPIENTRY MessageCallback(GLenum source,
                                GLenum type,
                                GLuint id,
                                GLenum severity,
                                GLsizei length,
                                const GLchar* message,
                                const void* userParam) {
    if (type == GL_DEBUG_TYPE_OTHER) {
        return;
    }
    auto level = toLogLevel(severity);
    LOG_FORMATTED(level, fmt::format("GL CALLBACK: 0x{:x}, message = {}", type, message).c_str());
}

void xr_examples::gl::enableDebugLogging() {
    // During init, enable debug output
    //glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(MessageCallback, 0);
}
