#version 450
#extension GL_ARB_separate_shader_objects : enable

out gl_PerVertex {
    vec4 gl_Position;
};

layout(location=0) out vec2 texCoord;

vec2 positions[4] = vec2[](
    vec2(-1.0, -1.0),
    vec2(1.0, -1.0),
    vec2(-1.0, 1.0),
    vec2(1.0, 1.0)
);

vec2 texCoords[4] = vec2[](
	vec2(0.0, 0.0),
	vec2(320.0, 0.0),
	vec2(0.0, 200.0),
	vec2(320.0, 200.0)
);

void main() {
	texCoord = vec2(texCoords[gl_VertexIndex]);
	gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
}
