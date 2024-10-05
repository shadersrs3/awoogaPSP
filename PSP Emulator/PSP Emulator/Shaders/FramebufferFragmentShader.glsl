#version 460 core

out vec4 colorOutput;

in vec2 textureCoord;

uniform sampler2D _texels;

void main() {
	colorOutput = vec4(texture(_texels, textureCoord));
}