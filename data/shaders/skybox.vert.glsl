#version 450 core

#extension GL_OVR_multiview : enable
layout(num_views = 2) in;

layout(location=0) out vec3 _normal;

struct EyeView {
	mat4 ProjectionMatrix;
	mat4 InverseProjectionMatrix;
	mat4 ViewMatrix;
	mat4 InverseViewMatrix;
};

layout (binding=0, std140) uniform EyeViews{
	EyeView eyeView[2];
} eyeViews;


void main(void) {
    const float INV_ROOT_3 = 0.5773502691896258;
    const float depth = 1.0;
    const vec4 UNIT_QUAD[4] = vec4[4](
        vec4(-1.0, -1.0, depth, 1.0),
        vec4(1.0, -1.0, depth, 1.0),
        vec4(-1.0, 1.0, depth, 1.0),
        vec4(1.0, 1.0, depth, 1.0)
    );

	mat4 inverseProjection = eyeViews.eyeView[gl_ViewID_OVR].InverseProjectionMatrix;
	mat4 inverseView = eyeViews.eyeView[gl_ViewID_OVR].InverseViewMatrix;

    vec4 inPosition = UNIT_QUAD[gl_VertexID];
    vec3 clipDir = vec3(inPosition.xy, 0.0);
    vec3 eyeDir = vec3(inverseProjection * vec4(clipDir.xyz, 1.0));
    _normal =  vec3(inverseView * vec4(eyeDir.xyz, 0.0));;
    gl_Position = inPosition;
}
