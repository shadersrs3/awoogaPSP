#version 460 core

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec2 aTextureCoord;

out vec2 textureCoord;

void main() {
	textureCoord = aTextureCoord;
	gl_Position = vec4(aPosition, 1.);
}