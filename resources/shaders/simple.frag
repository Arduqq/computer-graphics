#version 150

in vec3 pass_Normal;
in vec4 vertexPosition;
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
  vec4 lightPosition = ViewMatrix * vec4(0.0, 0.0, 0.0, 1.0);
  vec4 worldPosition = (ViewMatrix * ModelMatrix) * vertexPosition;
  vec3 normal = normalize(pass_Normal);
  vec3 light = normalize(lightPosition.xyz - worldPosition.xyz);
  vec3 vertex = normalize(-worldPosition.xyz);

  float lambertian = max(dot(light, normal), 0.0);

  float specular = 0.0;
  if(lambertian > 0.0) {
    vec3 halfDir = normalize(light + vertex); // halfway vector
    float specularAngle = max(dot(halfDir, normal), 0.0);
    specular = pow(specularAngle, glance);
  }

  out_Color = vec4(planetColor + lambertian * diffuseColor + specular * specularColor, 1.0);
}
