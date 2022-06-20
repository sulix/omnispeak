#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 outColour;
layout(location = 0) in vec2 texCoord;
layout(binding = 0) uniform FragUniforms
{
	vec2 texOffs;
	vec4 pal[16];
} ubo;
layout(set = 1, binding = 1) uniform usampler2D screen;

void main() {
	ivec2 fixedTexCoord = ivec2(texCoord + ubo.texOffs);
	uint palIndex = texelFetch(screen, fixedTexCoord, 0).r;
	outColour = ubo.pal[palIndex];
}

