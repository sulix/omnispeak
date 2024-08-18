#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 outColour;
layout(location = 0) in vec2 texCoord;
layout(set = 3, binding = 0) uniform FragUniforms
{
	vec2 texOffs;
	vec4 pal[16];
} ubo;
layout(set = 2, binding = 0, r8ui) uniform readonly  uimage2D screen;

void main() {
	ivec2 fixedTexCoord = ivec2(texCoord + ubo.texOffs);
	uint palIndex = imageLoad(screen, fixedTexCoord).r;
	outColour = ubo.pal[palIndex];
}

