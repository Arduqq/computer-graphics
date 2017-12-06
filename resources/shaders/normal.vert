#version 150
#extension GL_ARB_explicit_attrib_location : require
// vertex attributes of VAO
layout(location = 0) in vec3 in_Position;
layout(location = 1) in vec3 in_Normal;
layout(location = 2) in vec2 in_TexCoord;
layout(location = 2) in vec3 in_Tangent;

//Matrix Uniforms as specified with glUniformMatrix4fv
uniform mat4 ModelMatrix;
uniform mat4 ViewMatrix;
uniform mat4 ProjectionMatrix;
uniform mat4 NormalMatrix;
uniform vec3 ColorVector;

out vec3 pass_Normal;
out vec4 vertexPosition;
out vec2 pass_TexCoord;
out vec3 pass_Tangent;

void main(void)
{
	gl_Position = (ProjectionMatrix  * ViewMatrix * ModelMatrix) * vec4(in_Position, 1.0);
	vec4 vertex_Position = (ViewMatrix * ModelMatrix) * vec4(in_Position, 1.0);
	pass_Normal = (NormalMatrix * vec4(in_Normal, 0.0)).xyz;
	pass_Tangent = normalize((NormalMatrix * vec4(in_Tangent, 0.0)).xyz);
	pass_TexCoord = in_TexCoord;
}