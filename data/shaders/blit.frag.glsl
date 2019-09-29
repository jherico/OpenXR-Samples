#version 450 core

layout(binding=0) uniform sampler2DArray sceneTexture;

layout(location=0) in vec2 position;
layout(location=0) out vec4 _fragColor;

void main(void) {
    vec3 uv = vec3(position, 0.0);
    uv.y += 1.0;
    uv.y /= 2.0;
    if (position.x < 0.0) {
        uv.x = 1.0 - abs(position.x);
    } else {
        uv.z = 1.0;
    }
    _fragColor = vec4(texture(sceneTexture, uv).rgb, 1.0);
}
