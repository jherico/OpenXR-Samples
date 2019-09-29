#version 450 core

layout(binding=0) uniform samplerCube cubeMap;

layout(location=0) in vec3  _normal;
layout(location=0) out vec4 _fragColor;

void main(void) {
    _fragColor = vec4(texture(cubeMap, normalize(_normal)).rgb, 1.0);
    //_fragColor = vec4(abs(_normal), 1.0);
}
