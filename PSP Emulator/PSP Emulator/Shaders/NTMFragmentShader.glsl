#version 460 core

out vec4 colorOutput;

in vec2 textureCoord;
in vec4 color;
in vec4 normal;

uniform sampler2D texels;

void main() {
	colorOutput = color * texture(texels, textureCoord);
}