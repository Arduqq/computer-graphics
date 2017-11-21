#version 150
#extension GL_ARB_explicit_attrib_location : require
// vertex attributes of VAO
layout(location = 0) in vec3 in_Position;
layout(location = 1) in vec3 in_Normal;

//Matrix Uniforms as specified with glUniformMatrix4fv
uniform mat4 ModelMatrix;
uniform mat4 ViewMatrix;
uniform mat4 ProjectionMatrix;
uniform mat4 NormalMatrix;
uniform vec3 ColorVector;

out vec3 planetColor;

const float offset = 0.3;

void main(void)
{
	vec4 tpos = vec4(in_Position + in_Normal * offset, 1.0);
	gl_Position = (ProjectionMatrix  * ViewMatrix * ModelMatrix) * tpos;
	planetColor = ColorVector;
}