#version 460 core

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec2 aTextureCoord;
layout (location = 2) in vec4 aColor;
layout (location = 3) in vec4 aNormal;

subroutine vec4 VertexProcessor();

subroutine uniform VertexProcessor vertexProcessor;

const mat4 throughModeMatrixTranslation = mat4(
     2.0/480.0,  0.0      ,  0.0        ,  0.0,
     0.0      , -2.0/272.0,  0.0        ,  0.0,
     0.0      ,  0.0      ,  2.0/65535.0,  0.0,
    -1.0      ,  1.0      , -1.0        ,  1.0
);

out vec2 textureCoord;
out vec4 color;
out vec4 normal;

uniform mat4 uProjection, uView, uWorld;
uniform float textureScaleX, textureScaleY;

subroutine(VertexProcessor) vec4 normalModeVertexProcessor() {
	textureCoord = aTextureCoord;
	return uProjection * uView * uWorld * vec4(aPosition, 1);
}

subroutine(VertexProcessor) vec4 throughModeVertexProcessor() {
	textureCoord = vec2(aTextureCoord.x / textureScaleX, aTextureCoord.y / textureScaleY);
	return throughModeMatrixTranslation * vec4(aPosition, 1.);
}

void main() {
	color = aColor;
	normal = aNormal;
	gl_Position = vertexProcessor();
}