//
//  Created by Bradley Austin Davis
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#if !defined(__ANDROID__)
#include <glad/glad.h>
#endif

#include <mutex>
#include <string>

#include "logging.hpp"

#if defined(WIN32)

typedef PROC(APIENTRYP PFNWGLGETPROCADDRESS)(LPCSTR);
PFNWGLGETPROCADDRESS glad_wglGetProcAddress;
#define wglGetProcAddress glad_wglGetProcAddress

static inline void* getGlProcessAddress(const char* namez) {
    static HMODULE glModule = nullptr;
    if (!glModule) {
        glModule = LoadLibraryW(L"opengl32.dll");
        glad_wglGetProcAddress = (PFNWGLGETPROCADDRESS)GetProcAddress(glModule, "wglGetProcAddress");
    }

    auto result = wglGetProcAddress(namez);
    if (!result) {
        result = GetProcAddress(glModule, namez);
    }
    if (!result) {
        OutputDebugStringA(namez);
        OutputDebugStringA("\n");
    }
    return (void*)result;
}

#endif

namespace gl {

inline void init() {
#if !defined(__ANDROID__)
    static std::once_flag once;
    std::call_once(once, [] { gladLoadGL(); });
#endif
}
inline GLuint loadShader(const std::string& shaderSource, GLenum shaderType) {
    GLuint shader = glCreateShader(shaderType);
    int sizes = (int)shaderSource.size();
    const GLchar* strings = shaderSource.c_str();
    glShaderSource(shader, 1, &strings, &sizes);
    glCompileShader(shader);

    GLint isCompiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
    if (isCompiled == GL_FALSE) {
        GLint maxLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

        // The maxLength includes the NULL character
        std::vector<GLchar> errorLog(maxLength);
        glGetShaderInfoLog(shader, maxLength, &maxLength, &errorLog[0]);
        std::string strError;
        strError.insert(strError.end(), errorLog.begin(), errorLog.end());

        // Provide the infolog in whatever manor you deem best.
        // Exit with failure.
        glDeleteShader(shader);  // Don't leak the shader.
        throw std::runtime_error("Shader compiled failed");
    }
    return shader;
}

inline GLuint buildProgram(const std::string& vertexShaderSource, const std::string& fragmentShaderSource) {
    GLuint program = glCreateProgram();
    GLuint vs = loadShader(vertexShaderSource, GL_VERTEX_SHADER);
    GLuint fs = loadShader(fragmentShaderSource, GL_FRAGMENT_SHADER);
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    // fixme error checking
    glDeleteShader(vs);
    glDeleteShader(fs);
    return program;
}

inline void report() {
    LOG_INFO("GL Vendor: {}", (const char*)glGetString(GL_VENDOR));
    LOG_INFO("GL Renderer: {}", (const char*)glGetString(GL_RENDERER));
    LOG_INFO("GL Version: {}", (const char*)glGetString(GL_VERSION));
    //GLint n{ 0 };
    //glGetIntegerv(GL_NUM_EXTENSIONS, &n);
    //for (GLint i = 0; i < n; i++) {
    //    LOG_INFO("\tExtension: {}", (const char*)glGetStringi(GL_EXTENSIONS, i));
    //}
}

inline logging::Level severityToLevel(GLenum severity) {
    switch (severity) {
        case GL_DEBUG_SEVERITY_NOTIFICATION:
            return logging::Level::Debug;
        case GL_DEBUG_SEVERITY_HIGH:
            return logging::Level::Error;
        case GL_DEBUG_SEVERITY_MEDIUM:
            return logging::Level::Warning;
        case GL_DEBUG_SEVERITY_LOW:
            return logging::Level::Info;
    }
    return logging::Level::Error;
}

inline void debugMessageCallback(GLenum source,
                                 GLenum type,
                                 GLuint id,
                                 GLenum severity,
                                 GLsizei length,
                                 const GLchar* message,
                                 const void* userParam) {
    LOG_FORMATTED(severityToLevel(severity), message);
}

inline void setupDebugLogging() {
    glDebugMessageCallback(debugMessageCallback, NULL);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
}

}  // namespace gl
