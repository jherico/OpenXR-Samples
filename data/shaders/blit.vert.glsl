#version 450 core

layout (binding=0, std140) uniform UBO {
	 vec2 resolution;
} ubo;

layout(location = 0) out vec2 position;

void main(void) {
    const vec4 UNIT_QUAD[4] = vec4[4](
        vec4(-1.0, -1.0, 0.0, 1.0),
        vec4(1.0, -1.0, 0.0, 1.0),
        vec4(-1.0, 1.0, 0.0, 1.0),
        vec4(1.0, 1.0, 0.0, 1.0)
    );
    vec4 inPosition = UNIT_QUAD[gl_VertexID];
    position = inPosition.xy;
    gl_Position = inPosition;
}
