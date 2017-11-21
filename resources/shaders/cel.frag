#version 150

in vec3 planetColor;

out vec4 out_Color;

uniform mat4 ModelMatrix;
uniform mat4 ViewMatrix;
uniform mat4 ProjectionMatrix;
uniform vec3 ColorVector;

const vec3 specularColor = vec3(0.2, 0.2, 0.2);
const vec3 ambientColor = vec3(0.5, 0.5, 0.5);
const vec3 diffuseColor = vec3(0.3, 0.3, 0.3); 
const float glance = 16.0;

uniform int mode;

void main() {

  out_Color = vec4(planetColor, 1.0);
}
