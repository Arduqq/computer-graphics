#version 330
#extension GL_ARB_explicit_attrib_location : require

// vertex attributes 
layout(location = 0) in vec3 in_Position;
layout(location = 1) in vec3 in_Normal;

uniform mat4 ModelMatrix;
uniform mat4 ViewMatrix;
uniform mat4 ProjectionMatrix;

const float offset1 = 0.65;

void main(void){
   vec4 tPos  = vec4(in_Position + in_Normal * offset1, 1.0);
   gl_Position	= (ProjectionMatrix * ModelMatrix * ViewMatrix) * tPos;
}