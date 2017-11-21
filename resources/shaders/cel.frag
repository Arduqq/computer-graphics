#version 330
#extension GL_ARB_explicit_attrib_location : require

uniform vec3 ColorVector;

layout(location = 0) out vec4 FragColor;

void main(void){
   FragColor = vec4(ColorVector, 1.0);
}